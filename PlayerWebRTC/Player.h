#pragma once

#include"framework.h"

class Player : public IMFAsyncCallback
{
public:
	Player(HWND hWnd);
	~Player();

	HRESULT Play(LPCTSTR szFilename);
	void Dispose();
	void UpdateAndPresent();

	// Inherited via IMFAsyncCallback
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject) override;
	STDMETHODIMP_(ULONG) AddRef(void) override;
	STDMETHODIMP_(ULONG) Release(void) override;

	HRESULT STDMETHODCALLTYPE GetParameters(__RPC__out DWORD * pdwFlags, __RPC__out DWORD * pdwQueue) override;
	HRESULT STDMETHODCALLTYPE Invoke(__RPC__in_opt IMFAsyncResult * pAsyncResult) override;

private:
	volatile long m_refCount;
	bool m_bInitialized;

	ID3D11Device* m_pD3DDevice;
	ID3D11DeviceContext* m_pD3DDeviceContext;
	IDXGISwapChain* m_pDXGISwapChain;
	IDXGIOutput* m_pDXGIOutput;
	IMFDXGIDeviceManager* m_pDXGIDeviceManager;
	UINT m_DXGIDeviceManagerID;

	std::atomic_bool m_bPresentNow;
	PTP_WORK m_pWork;
	IMFMediaSession* m_pMediaSession;
	IMFMediaSource* m_pMediaSource;
	IMFMediaSink* m_pMediaSink;
	IMFAttributes* m_pMediaSinkAttributes;
	HANDLE m_evReadyOrFailed;
	HRESULT m_hrStatus;

	HWND m_hwnd;

	void Update();
	void Draw();

	HRESULT CreateMediaSession(IMFMediaSession** ppMediaSession);
	HRESULT CreateMediaSourceFromFile(LPCTSTR szFileName, IMFMediaSource** ppMediaSource);
	HRESULT CreateTopology(IMFTopology** ppTopology);
	HRESULT AddTopologyNode(IMFTopology* pTopology, IMFPresentationDescriptor* pPresentationDesc, DWORD index);
	HRESULT CreateSourceNode(IMFPresentationDescriptor* pPresentationDesc, IMFStreamDescriptor* pStreamDesc, IMFTopologyNode** ppSourceNode);
	HRESULT CreateOutputNode(IMFStreamDescriptor* pStreamDesc, IMFTopologyNode** ppOutputNode, GUID* pMajorType);

	void OnTopologyReady(IMFMediaEvent* pMediaEvent);
	void OnSessionStarted(IMFMediaEvent* pMediaEvent);
	void OnSessionPaused(IMFMediaEvent* pMediaEvent);
	void OnSessionStopped(IMFMediaEvent* pMediaEvent);
	void OnSessionClosed(IMFMediaEvent* pMediaEvent);
	void OnPresentaionEnded(IMFMediaEvent* pMediaEvent);
	void OnEndRegistrarTopologyWorkQueueWithMMCSS(IMFAsyncResult* pAsyncResult);

	static void CALLBACK PresentationSwapChainWork(PTP_CALLBACK_INSTANCE pInstace, LPVOID pvParam, PTP_WORK pWork);
	IUnknown* ID_RegistarTopologyWorkQueueWithMMCSS;

};

