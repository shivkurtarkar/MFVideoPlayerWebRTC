/*

* To view the RTP feed produced by this sample the steps are:
* 1. Download ffplay from http://ffmpeg.zeranoe.com/builds/ (the static build has
*    a ready to go ffplay executable),
*
* 2. Create a file called test.sdp with contents as below:
* v=0
* o=-0 0 IN IP4 127.0.0.1
* s=No Name
* t=0 0
* c=IN IP4 127.0.0.1
* m=video 1234 RTP/AVP 96
* a=rtpmap:96 H264/90000
* a=fmtp:96 packetization-mode=1
*
* 3. Start ffplay BEFORE running this sample:
* ffplay -i test.sdp -x 640 -y 480 -profile:v baseline -protocol_whitelist "file,rtp,udp"
*

*/

#pragma once

#include"framework.h"

//

#define OUTPUT_FRAME_WIDTH		640		// Adjust based on input
#define OUTPUT_FRAME_HEIGHT		480		// Adjust based on input
#define OUTPUT_FRAME_RATE		30		// Adjust based on input
#define RTP_MAX_PAYLOAD			1400	// Maximum size of an RTP packet, needs to be under the Ethernet MTU.
#define RTP_HEADER_LENGTH		12
#define RTP_VERSION				2
#define RTP_PAYLOAD_ID			96		// Needs to match the attribute set in the SDP(a=rtpmap:96 H264/90000).
#define H264_RTP_HEADER_LENGTH	2
#define FFPLAY_RTP_PORT			1234	// The port this sample will send to


/**
* Minimal 12 byte RTP heaser structure.
*/
class RtpHeader
{
public:
	uint8_t Version = RTP_VERSION;		// 2 bits
	uint8_t PaddingFlag = 0;			// 1 bit
	uint8_t HeaderExtensionFlag = 0;	// 1 bit
	uint8_t CSRCCount = 0;				// 4 bit
	uint8_t MarkerBit = 0;				// 1 bit
	uint8_t PayloadType = 0;			// 7 bit
	uint16_t SeqNum = 0;				// 16 bits
	uint32_t Timestamp = 0;				// 32 bits
	uint32_t SyncSource = 0;			// 32 bits

	void Serialize(uint8_t** buf) {
		*buf = (uint8_t*)calloc(RTP_HEADER_LENGTH, 1);
		*(*buf) = (Version << 6 & 0xC0) | (PaddingFlag << 5 & 0x20) | (HeaderExtensionFlag << 4 & 0x10) | (CSRCCount & 0x0f);
		*(*buf + 1) = (MarkerBit << 7 & 0x80) | (PayloadType & 0x7f);
		*(*buf + 2) = SeqNum >> 8 & 0xff;
		*(*buf + 3) = SeqNum & 0xff;
		*(*buf + 4) = Timestamp >> 24 & 0xff;
		*(*buf + 5) = Timestamp >> 16 & 0xff;
		*(*buf + 6) = Timestamp >> 8 & 0xff;
		*(*buf + 7) = Timestamp & 0xff;
		*(*buf + 8) = SyncSource >> 24 & 0xff;
		*(*buf + 9) = SyncSource >> 16 & 0xff;
		*(*buf + 10) = SyncSource >> 8 & 0xff;
		*(*buf + 11) = SyncSource & 0xff;
	}
};

class RTPSampleGrabberCallback :public IMFSampleGrabberSinkCallback
{
public:

	RTPSampleGrabberCallback();
	~RTPSampleGrabberCallback();

	// Inherited via IMFSampleGrabberSinkCallback
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;

	virtual HRESULT __stdcall OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset) override;
	virtual HRESULT __stdcall OnClockStop(MFTIME hnsSystemTime) override;
	virtual HRESULT __stdcall OnClockPause(MFTIME hnsSystemTime) override;
	virtual HRESULT __stdcall OnClockRestart(MFTIME hnsSystemTime) override;
	virtual HRESULT __stdcall OnClockSetRate(MFTIME hnsSystemTime, float flRate) override;

	virtual HRESULT __stdcall OnSetPresentationClock(IMFPresentationClock * pPresentationClock) override;
	virtual HRESULT __stdcall OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize) override;
	virtual HRESULT __stdcall OnShutdown(void) override;

	// Inherited via IMFSampleGrabberSinkCallback2
	virtual HRESULT __stdcall OnProcessSampleEx(REFGUID guidMajorMediaType, DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize, IMFAttributes * pAttributes);

private:

	volatile long m_refCount;

	//RTP
	uint16_t m_rtpSsrc; // Supposed to be pseudo-random.
	uint16_t m_rtpSeqNum;
	uint16_t m_rtpTimestamp;
	SOCKET m_rtpSocket;
	sockaddr_in m_service, m_dest;	   

	HRESULT InitializeRTP();
	HRESULT DeInitializeRTP();
};