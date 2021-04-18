#include "RTPSampleGrabberCallback.h"

#include"Utility.h"

HRESULT SendH264RtpSample(SOCKET socket, sockaddr_in & dst, const BYTE* frameData, DWORD frameLength, uint32_t ssrc, uint32_t timestamp, uint16_t* seqNum);

RTPSampleGrabberCallback::RTPSampleGrabberCallback() :
	m_refCount(1)
{
}

RTPSampleGrabberCallback::~RTPSampleGrabberCallback()
{
	DeInitializeRTP();
}

HRESULT __stdcall RTPSampleGrabberCallback::QueryInterface(REFIID riid, void ** ppvObject)
{
	static const QITAB qit[] =
	{
		QITABENT(RTPSampleGrabberCallback, IMFSampleGrabberSinkCallback),
		//QITABENT(CSampleGrabber, IMFSampleGrabberSinkCallback2),
		QITABENT(RTPSampleGrabberCallback, IMFClockStateSink),
		{0}
	};
	return QISearch(this, qit, riid, ppvObject);
}

ULONG __stdcall RTPSampleGrabberCallback::AddRef(void)
{
	return InterlockedIncrement(&m_refCount);
}

ULONG __stdcall RTPSampleGrabberCallback::Release(void)
{
	ULONG uCount = InterlockedDecrement(&m_refCount);
	if (uCount == 0) {
		delete this;
	}
	return uCount;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnClockStop(MFTIME hnsSystemTime)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnClockPause(MFTIME hnsSystemTime)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnClockRestart(MFTIME hnsSystemTime)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnSetPresentationClock(IMFPresentationClock * pPresentationClock)
{
	HRESULT hr = S_OK;
	hr = this->InitializeRTP();
	return hr;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize)
{
	HRESULT hr = S_OK;
	IMFSample* pSample = NULL;
	do {		
		#ifdef _DEBUG
		OutputDebugString(_T("Processing sample .....\n"));
		#endif

		SendH264RtpSample(m_rtpSocket, m_dest, pSampleBuffer, dwSampleSize, m_rtpSsrc, (uint32_t)(llSampleTime / 10000), &m_rtpSeqNum);
	
	} while (false);
	return hr;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnShutdown(void)
{
	return S_OK;
}

HRESULT __stdcall RTPSampleGrabberCallback::OnProcessSampleEx(REFGUID guidMajorMediaType, DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize, IMFAttributes * pAttributes)
{
	return S_OK;
}

HRESULT RTPSampleGrabberCallback::InitializeRTP()
{
	WSADATA wsaData;
	m_rtpSsrc = 3334; // Supposed to be pseudo-random.
	m_rtpSeqNum = 0;
	m_rtpTimestamp = 0;

	HRESULT hr = S_OK;
	do {
		// Initialize Winsock
		if ((0 != WSAStartup(MAKEWORD(2, 2), &wsaData)))
			break;
		m_rtpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_rtpSocket == INVALID_SOCKET) {
#ifdef _DEBUG
			OutputDebugString(_T("WSAStartup Failed \n"));
#endif
			hr = E_UNEXPECTED;
			break;
		}

		// The sockaddr_in structure specifies the address family,
		// IP address, and port for the socket that is being bound.
		m_service.sin_family = AF_INET;
		m_service.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		m_service.sin_port = 0; // htons(27015);

		
		// bind the socket
		if (SOCKET_ERROR == bind(m_rtpSocket, (SOCKADDR*)&m_service, sizeof(m_service))) {
#ifdef _DEBUG
			OutputDebugString(_T("bind failed \n"));
#endif
			closesocket(m_rtpSocket);			
			hr = E_UNEXPECTED;
			break;
		}
		else {
#ifdef _DEBUG
			OutputDebugString(_T("bind successful \n"));
#endif
		}

		m_dest.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &m_dest.sin_addr.s_addr);
		m_dest.sin_port = htons(FFPLAY_RTP_PORT);
		

	} while (false);

	if (FAILED(hr)) {
		DeInitializeRTP();
	}
	return hr;
}

HRESULT RTPSampleGrabberCallback::DeInitializeRTP()
{
	closesocket(m_rtpSocket);
	WSACleanup();
	return S_OK;
}




HRESULT SendH264RtpSample(SOCKET socket, sockaddr_in & dst, const BYTE* frameData, DWORD frameLength, uint32_t ssrc, uint32_t timestamp, uint16_t* seqNum) {
	static uint16_t h264HeaderStart = 0x1c89;	// Start RTP packet in frame 0x1c 0x89
	static uint16_t h264HeaderMiddle = 0x1c09;	// Middle RTP packet in frame 0x1c 0x09
	static uint16_t h264HeaderEnd = 0x1c49;	// Last RTP packet in frame 0x1c 0x49

	HRESULT hr = S_OK;

	uint16_t pktSeqNum = *seqNum;
	do {

		for (UINT offset(0); offset < frameLength;) {
			bool isLast = ((offset + RTP_MAX_PAYLOAD) >= frameLength); // Note: Can be first or last packet at the same time if small frame.
			UINT payloadLength = !isLast ? RTP_MAX_PAYLOAD : frameLength - offset;

			RtpHeader rtpHeader;
			rtpHeader.SyncSource = ssrc;
			rtpHeader.SeqNum = pktSeqNum++;
			rtpHeader.Timestamp = timestamp;
			rtpHeader.MarkerBit = 0; //Set on first and last packet in frame.
			rtpHeader.PayloadType = RTP_PAYLOAD_ID;

			uint16_t h264Header = h264HeaderMiddle;
			if (isLast) {
				// This is the First AND Last RTP packet in the frame.
				h264Header = h264HeaderEnd;
				rtpHeader.MarkerBit = 1;
			}
			else if (offset == 0) {
				h264Header = h264HeaderStart;
				rtpHeader.MarkerBit = 1;
			}

			uint8_t* hdrSerialised = NULL;
			rtpHeader.Serialize(&hdrSerialised);

			int rtpPacketSize = RTP_HEADER_LENGTH + H264_RTP_HEADER_LENGTH + payloadLength;
			uint8_t* rtpPacket = (uint8_t*)malloc(rtpPacketSize);
			memcpy_s(rtpPacket, rtpPacketSize, hdrSerialised, RTP_HEADER_LENGTH);
			rtpPacket[RTP_HEADER_LENGTH] = (byte)(h264Header >> 8 & 0xff);
			rtpPacket[RTP_HEADER_LENGTH + 1] = (byte)(h264Header & 0xff);
			memcpy_s(&rtpPacket[RTP_HEADER_LENGTH + H264_RTP_HEADER_LENGTH], payloadLength, &frameData[offset], payloadLength);

			printf("Sending RTP packet, length %d.\n", rtpPacketSize);

			if (SOCKET_ERROR == sendto(socket, (const char*)rtpPacket, rtpPacketSize, 0, (sockaddr*)&dst, sizeof(dst))) {
#ifdef _DEBUG
				OutputDebugString(_T("failed to send frame. \n"));
#endif
			}			

			offset += payloadLength;
			free(hdrSerialised);
			free(rtpPacket);
		}

	} while (false);

	(*seqNum) = pktSeqNum;
	return hr;
}
