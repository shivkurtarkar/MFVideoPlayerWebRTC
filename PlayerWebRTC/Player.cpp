#include "Player.h"

#include"RTPSampleGrabberCallback.h"

Player::Player(HWND hWnd) :
	m_refCount(0),
	m_bInitialized(false),
	m_hrStatus(S_OK),
	m_bPresentNow(false),
	m_hwnd(hWnd)
{
	HRESULT hr = S_OK;
	m_pWork = ::CreateThreadpoolWork(Player::PresentationSwapChainWork, this, NULL);
	m_evReadyOrFailed = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	::MFStartup(MF_VERSION);

	D3D_FEATURE_LEVEL featureLevels[]{ D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;
	RECT clientRect;
	::GetClientRect(hWnd, &clientRect);
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	::ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.Width = clientRect.right - clientRect.left;
	swapChainDesc.BufferDesc.Height = clientRect.bottom - clientRect.top;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = true;
	hr = ::D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		featureLevels,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_pDXGISwapChain,
		&m_pD3DDevice,
		&featureLevel,
		&m_pD3DDeviceContext);
	if (FAILED(hr))
		return;

	ID3D10Multithread* pD3D10Multithread;
	do {
		if (FAILED(hr = m_pD3DDevice->QueryInterface(IID_ID3D10Multithread, (void**)&pD3D10Multithread)))
			break;

		pD3D10Multithread->SetMultithreadProtected(TRUE);
	} while (false);
	SafeRelease(pD3D10Multithread);
	if (FAILED(hr))
		return;

	// Setting up DXGIDeviceManager
	if (FAILED(hr = ::MFCreateDXGIDeviceManager(&m_DXGIDeviceManagerID, &m_pDXGIDeviceManager)))
		return;
	m_pDXGIDeviceManager->ResetDevice(m_pD3DDevice, m_DXGIDeviceManagerID);

	// IDXGIOutput
	IDXGIDevice* pDXGIDevice;
	IDXGIAdapter* pDXGIAdapter;
	do {
		if (FAILED(hr = m_pD3DDevice->QueryInterface(IID_IDXGIDevice, (void**)&pDXGIDevice)))
			break;
		if (FAILED(hr = pDXGIDevice->GetAdapter(&pDXGIAdapter)))
			break;
		if (FAILED(hr = pDXGIAdapter->EnumOutputs(0, &m_pDXGIOutput)))
			break;
	} while (false);
	SafeRelease(pDXGIAdapter);
	SafeRelease(pDXGIDevice);
	if (FAILED(hr))
		return;

	// Create media type
	IMFMediaType* pID = NULL;
	::MFCreateMediaType(&pID);
	this->ID_RegistarTopologyWorkQueueWithMMCSS = (IUnknown*)pID;

	m_bInitialized = true;
}

Player::~Player()
{
}

HRESULT Player::Play(LPCTSTR szFilename)
{
	HRESULT hr = S_OK;
	::ResetEvent(m_evReadyOrFailed);

	// Media Session
	if (FAILED(hr = this->CreateMediaSession(&m_pMediaSession)))
		return hr;
	// Media Source
	if (FAILED(hr = this->CreateMediaSourceFromFile(szFilename, &m_pMediaSource)))
		return hr;

	IMFTopology* pTopology = NULL;
	do {
		if (FAILED(hr = this->CreateTopology(&pTopology)))
			break;
		if (FAILED(hr = m_pMediaSession->SetTopology(0, pTopology)))
			break;
	} while (false);

	SafeRelease(pTopology);
	if (FAILED(hr))
		return hr;

	::WaitForSingleObject(m_evReadyOrFailed, 5000);
	if (FAILED(m_hrStatus))
		return m_hrStatus;

	// MediaSession
	PROPVARIANT prop;
	::PropVariantInit(&prop);
	m_pMediaSession->Start(NULL, &prop);

	return S_OK;
}

void Player::Dispose()
{
	m_bInitialized = false;

	if (NULL != m_pMediaSession)
		m_pMediaSession->Stop();

	::WaitForThreadpoolWorkCallbacks(m_pWork, true);
	::CloseThreadpoolWork(m_pWork);

	// MediaFoundation 
	SafeRelease(ID_RegistarTopologyWorkQueueWithMMCSS);
	SafeRelease(m_pMediaSinkAttributes);
	SafeRelease(m_pMediaSource);
	SafeRelease(m_pMediaSink);
	SafeRelease(m_pMediaSession);

	// DXGI, D3D11
	SafeRelease(m_pDXGIDeviceManager);
	SafeRelease(m_pDXGIOutput);
	SafeRelease(m_pDXGISwapChain);
	SafeRelease(m_pD3DDevice);
	SafeRelease(m_pD3DDeviceContext);

	// MediaFoundation
	::MFShutdown();
}

void Player::UpdateAndPresent()
{
	return;
	if (!m_bPresentNow) //m_bPresentNow, atomic bool
	{
		this->Draw();
		m_bPresentNow = true;
		::SubmitThreadpoolWork(m_pWork);
	}
	else {
		this->Update();
	}
}

HRESULT Player::QueryInterface(REFIID riid, __RPC__deref_out _Result_nullonfailure_ void ** ppvObject)
{
	if (NULL == ppvObject)
		return E_POINTER;
	if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
	}
	else if (riid == __uuidof(IMFAsyncCallback)) {
		*ppvObject = static_cast<IMFAsyncCallback*>(this);
	}
	else {
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	this->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG __stdcall) Player::AddRef(void)
{
	return InterlockedIncrement(&this->m_refCount);
}

STDMETHODIMP_(ULONG __stdcall) Player::Release(void)
{
	ULONG uCount = InterlockedDecrement(&this->m_refCount);
	if (uCount == 0)
		delete this;
	return uCount;
}

STDMETHODIMP Player::GetParameters(DWORD * pdwFlags, DWORD * pdwQueue)
{
	return E_NOTIMPL;
}

STDMETHODIMP Player::Invoke(__RPC__in_opt IMFAsyncResult *pAsyncResult)
{
	if (!m_bInitialized)
		return MF_E_SHUTDOWN;

	HRESULT hr = S_OK;
	IUnknown* pUnk;
	if (SUCCEEDED(hr = pAsyncResult->GetState(&pUnk))) {
		if (this->ID_RegistarTopologyWorkQueueWithMMCSS == pUnk) {
			this->OnEndRegistrarTopologyWorkQueueWithMMCSS(pAsyncResult);
			return S_OK;
		}
		else {
			return E_INVALIDARG;
		}
	}
	else {
		IMFMediaEvent* pMediaEvent = NULL;
		MediaEventType eventType;
		do {
			if (FAILED(hr = m_pMediaSession->EndGetEvent(pAsyncResult, &pMediaEvent)))
				break;
			if (FAILED(hr = pMediaEvent->GetType(&eventType)))
				break;
			if (FAILED(hr = pMediaEvent->GetStatus(&m_hrStatus)))
				break;

			if (FAILED(m_hrStatus)) {
				::SetEvent(m_evReadyOrFailed);
				return m_hrStatus;
			}

			switch (eventType)
			{
			case MESessionTopologyStatus:
				UINT32 topoStatus;
				if (FAILED(hr = pMediaEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &topoStatus)))
					break;
				switch (topoStatus)
				{
				case MF_TOPOSTATUS_READY:
					this->OnTopologyReady(pMediaEvent);
					break;
				}
				break;
			case MESessionStarted:
				this->OnSessionStarted(pMediaEvent);
				break;
			case MESessionPaused:
				this->OnSessionPaused(pMediaEvent);
				break;
			case MESessionStopped:
				this->OnSessionStopped(pMediaEvent);
				break;
			case MESessionClosed:
				this->OnSessionClosed(pMediaEvent);
				break;
			case MEEndOfPresentation:
				this->OnPresentaionEnded(pMediaEvent);
				break;
			}
		} while (false);
		SafeRelease(pMediaEvent);

		if (eventType != MESessionClosed) {
			hr = m_pMediaSession->BeginGetEvent(this, NULL);
		}
		return hr;
	}
}

void Player::Update()
{
}

void Player::Draw()
{
	HRESULT hr = S_OK;
	IMFSample* pSample = NULL;
	IMFMediaBuffer* pMediaBuffer = NULL;
	IMFDXGIBuffer* pDXGIBuffer = NULL;
	ID3D11Texture2D* pTexture2D = NULL;
	ID3D11Texture2D* pBackBufferTexture2D = NULL;

	do {
		//if(FAILED(hr = m_pMediaSinkAttributes->GetUnknown(TMS_Sample, IID_IMFSample, (void**)&pSample)))
	} while (false);
}

HRESULT Player::CreateMediaSession(IMFMediaSession ** ppMediaSession)
{
	HRESULT hr = S_OK;
	IMFMediaSession* pMediaSession = nullptr;
	do {
		// Media Session
		if (FAILED(hr = MFCreateMediaSession(NULL, &pMediaSession)))
			break;
		if (FAILED(hr = pMediaSession->BeginGetEvent(this, nullptr)))
			break;
		(*ppMediaSession) = pMediaSession;
		(*ppMediaSession)->AddRef();
		hr = S_OK;
	} while (false);
	SafeRelease(pMediaSession);
	return hr;
}

HRESULT Player::CreateMediaSourceFromFile(LPCTSTR szFileName, IMFMediaSource ** ppMediaSource)
{
	HRESULT hr = S_OK;
	IMFSourceResolver* pResolver = nullptr;
	IMFMediaSource* pMediaSource = nullptr;
	do {
		if (FAILED(hr = ::MFCreateSourceResolver(&pResolver)))
			break;
		MF_OBJECT_TYPE type;
		if (FAILED(hr = pResolver->CreateObjectFromURL(szFileName, MF_RESOLUTION_MEDIASOURCE, NULL, &type, (IUnknown**)&pMediaSource)))
			break;

		(*ppMediaSource) = pMediaSource;
		(*ppMediaSource)->AddRef();
		hr = S_OK;
	} while (false);
	SafeRelease(pMediaSource);
	SafeRelease(pResolver);
	return hr;
}

HRESULT Player::CreateTopology(IMFTopology ** ppTopology)
{
	HRESULT hr = S_OK;

	IMFTopology* pTopology = NULL;
	IMFPresentationDescriptor* pPresentationDesc = NULL;
	do {
		if (FAILED(hr = ::MFCreateTopology(&pTopology)))
			break;

		if (FAILED(hr = m_pMediaSource->CreatePresentationDescriptor(&pPresentationDesc)))
			break;
		DWORD dwDescCount;
		if (FAILED(hr = pPresentationDesc->GetStreamDescriptorCount(&dwDescCount)))
			break;

		for (DWORD i = 0; i < dwDescCount; i++) {
			if (FAILED(hr = this->AddTopologyNode(pTopology, pPresentationDesc, i)))
				break;
		}
		if (FAILED(hr))
			break;

		(*ppTopology) = pTopology;
		(*ppTopology)->AddRef();
		hr = S_OK;
	} while (false);
	SafeRelease(pTopology);
	SafeRelease(pPresentationDesc);
	return hr;
}

HRESULT Player::AddTopologyNode(IMFTopology * pTopology, IMFPresentationDescriptor * pPresentationDesc, DWORD index)
{
	HRESULT hr = S_OK;
	BOOL bSelected;
	IMFStreamDescriptor* pStreamDesc = NULL;
	IMFTopologyNode* pSourceNode = NULL;
	IMFTopologyNode* pOutputNode = NULL;
	do {
		if (FAILED(hr = pPresentationDesc->GetStreamDescriptorByIndex(index, &bSelected, &pStreamDesc)))
			break;
		if (bSelected) {
			// Add stream to topology
			if (FAILED(hr = this->CreateSourceNode(pPresentationDesc, pStreamDesc, &pSourceNode)))
				break;
			GUID majorType;
			if (FAILED(hr = this->CreateOutputNode(pStreamDesc, &pOutputNode, &majorType)))
				break;

			if (majorType == MFMediaType_Audio) {
				pSourceNode->SetString(MF_TOPONODE_WORKQUEUE_MMCSS_CLASS, _T("Audio"));
				pSourceNode->SetUINT32(MF_TOPONODE_WORKQUEUE_ID, 1);
			}
			if (majorType == MFMediaType_Video) {
				pSourceNode->SetString(MF_TOPONODE_WORKQUEUE_MMCSS_CLASS, _T("Playback"));
				pSourceNode->SetUINT32(MF_TOPONODE_WORKQUEUE_ID, 2);
			}

			if (NULL != pSourceNode && NULL != pOutputNode) {
				pTopology->AddNode(pSourceNode);
				pTopology->AddNode(pOutputNode);

				pSourceNode->ConnectOutput(0, pOutputNode, 0);
			}
		}
		else {
			// skip stream 
		}
	} while (false);
	SafeRelease(pOutputNode);
	SafeRelease(pSourceNode);
	SafeRelease(pStreamDesc);

	return hr;
}

HRESULT Player::CreateSourceNode(IMFPresentationDescriptor * pPresentationDesc, IMFStreamDescriptor * pStreamDesc, IMFTopologyNode ** ppSourceNode)
{
	HRESULT hr = S_OK;
	IMFTopologyNode* pSourceNode = NULL;
	do {
		if (FAILED(hr = ::MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode)))
			break;
		if (FAILED(hr = pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_pMediaSource)))
			break;
		if (FAILED(hr = pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPresentationDesc)))
			break;
		if (FAILED(hr = pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pStreamDesc)))
			break;
		(*ppSourceNode) = pSourceNode;
		(*ppSourceNode)->AddRef();
		hr = S_OK;

	} while (false);
	SafeRelease(pSourceNode);
	return hr;
}

HRESULT Player::CreateOutputNode(IMFStreamDescriptor * pStreamDesc, IMFTopologyNode ** ppOutputNode, GUID * pMajorType)
{
	HRESULT hr = S_OK;

	IMFTopologyNode* pOutputNode = NULL;
	IMFMediaTypeHandler* pMediaTypeHandler = NULL;
	do {
		if (FAILED(hr = ::MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode)))
			break;
		if (FAILED(hr = pStreamDesc->GetMediaTypeHandler(&pMediaTypeHandler)))
			break;
		GUID majorType;
		if (FAILED(hr = pMediaTypeHandler->GetMajorType(&majorType)))
			break;

		if (majorType == MFMediaType_Video) {
			IMFActivate* pActivate = NULL;
			do {
				bool bCreateEVR = false;
				if (bCreateEVR) {
					if (FAILED(hr = ::MFCreateVideoRendererActivate(m_hwnd, &pActivate)))
						break;
					if (FAILED(hr = pOutputNode->SetObject(pActivate)))
						break;
				}
				else {
					RTPSampleGrabberCallback* pCallback = new RTPSampleGrabberCallback();
					if (!pCallback) {
						hr = E_OUTOFMEMORY;
						break;
					}

					IMFMediaType* pMediaType = NULL;
					if (FAILED(hr = MFCreateMediaType(&pMediaType)))
						break;
					if(FAILED(hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, majorType)))
						break;
										
					if (FAILED(hr = MFCreateSampleGrabberSinkActivate(pMediaType, pCallback, &pActivate)))
						break;
					//if (FAILED(hr = pActivate->SetUINT32(MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, TRUE)))
					//	break;	
					if (FAILED(hr = pOutputNode->SetObject(pActivate)))
						break;
					SafeRelease(pMediaType);
				}

			} while (false);
			SafeRelease(pActivate);
		}
		else if (majorType == MFMediaType_Audio) {
			IMFActivate* pActivate = NULL;
			do {
				if (FAILED(hr = ::MFCreateAudioRendererActivate(&pActivate)))
					break;
				if (FAILED(hr = pOutputNode->SetObject(pActivate)))
					break;
			} while (false);
			SafeRelease(pActivate);
		}

		(*ppOutputNode) = pOutputNode;
		(*ppOutputNode)->AddRef();
		hr = S_OK;
	} while (false);
	SafeRelease(pOutputNode);
	SafeRelease(pMediaTypeHandler);
	return hr;
}

void Player::OnTopologyReady(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnSessionStarted(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnSessionPaused(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnSessionStopped(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnSessionClosed(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnPresentaionEnded(IMFMediaEvent * pMediaEvent)
{
}

void Player::OnEndRegistrarTopologyWorkQueueWithMMCSS(IMFAsyncResult * pAsyncResult)
{
	HRESULT hr;

	IMFGetService* pGetService = NULL;
	IMFWorkQueueServices* pWorkQueueServices = NULL;
	do {
		if (FAILED(hr = this->m_pMediaSession->QueryInterface(IID_IMFGetService, (void**)&pGetService)))
			break;
		if (FAILED(hr = pGetService->GetService(MF_WORKQUEUE_SERVICES, IID_IMFWorkQueueServices, (void**)&pWorkQueueServices)))
			break;
		if (FAILED(hr = pWorkQueueServices->EndRegisterTopologyWorkQueuesWithMMCSS(pAsyncResult)))
			break;
	} while (false);
	SafeRelease(pWorkQueueServices);
	SafeRelease(pGetService);

	if (FAILED(hr))
		this->m_hrStatus = hr;
	else
		this->m_hrStatus = S_OK;
	::SetEvent(this->m_evReadyOrFailed);
}

void Player::PresentationSwapChainWork(PTP_CALLBACK_INSTANCE pInstace, LPVOID pvParam, PTP_WORK pWork)
{
	Player* pPlayer = (Player*)pvParam;
	pPlayer->m_pDXGIOutput->WaitForVBlank();
	pPlayer->m_pDXGISwapChain->Present(1, 0);

	pPlayer->m_bPresentNow = false;
}
