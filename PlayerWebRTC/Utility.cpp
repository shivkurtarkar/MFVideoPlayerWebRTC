#include"Utility.h"

HRESULT CreateBuffer(const BYTE * pSrcByteBuffer, DWORD dwSampleSize, IMFMediaBuffer ** destBuf) {
	HRESULT hr = S_OK;
	do {
		if (!pSrcByteBuffer) {
			hr = E_POINTER;
			break;
		}
		if (!destBuf) {
			hr = E_POINTER;
			break;
		}
		hr = MFCreateMemoryBuffer(dwSampleSize, destBuf);
		if (FAILED(hr)) break;
		BYTE* destByteBuffer = NULL;
		hr = (*destBuf)->Lock(&destByteBuffer, NULL, NULL);
		if (FAILED(hr)) break;
		memcpy(destByteBuffer, pSrcByteBuffer, dwSampleSize);
		hr = (*destBuf)->Unlock();
		if (FAILED(hr)) break;
		hr = (*destBuf)->SetCurrentLength(dwSampleSize);

	} while (false);
	return hr;
}


HRESULT CreateSample(DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize, IMFSample** ppOutSample)
{
	HRESULT hr = S_OK;
	do {
		if (!ppOutSample) {
			hr = E_POINTER; break;
		}

		hr = MFCreateSample(ppOutSample);
		if (FAILED(hr))break;
		hr = (*ppOutSample)->SetSampleFlags(dwSampleFlags);
		if (FAILED(hr))break;
		hr = (*ppOutSample)->SetSampleTime(llSampleTime);
		if (FAILED(hr))break;
		hr = (*ppOutSample)->SetSampleDuration(llSampleDuration);
		if (FAILED(hr))break;

		DWORD bufferCount = 1;
		for (DWORD index = 0; index < bufferCount; ++index)
		{
			IMFMediaBuffer* reConstructedBuffer = NULL;
			hr = CreateBuffer(pSampleBuffer, dwSampleSize, &reConstructedBuffer);
			if (FAILED(hr))break;

			hr = (*ppOutSample)->AddBuffer(reConstructedBuffer);
			if (FAILED(hr))break;

		}
	} while (false);
	return hr;
}