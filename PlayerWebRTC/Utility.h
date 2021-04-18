#pragma once

#include"framework.h"

HRESULT CreateBuffer(const BYTE * pSrcByteBuffer, DWORD dwSampleSize, IMFMediaBuffer** destBuf);

HRESULT CreateSample(DWORD dwSampleFlags, LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer, DWORD dwSampleSize, IMFSample** ppOutSample);
