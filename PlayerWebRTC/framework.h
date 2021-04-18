// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <winsock2.h>
#include <ws2tcpip.h>

// Windows Header Files
#include <windows.h>

#include<initguid.h>

// Media Founation Headers
#include<mfapi.h>
#include<mfidl.h>
#include<mfreadwrite.h>
#include<Mferror.h>
#include<d3d11.h>

#include<Shlwapi.h>

#include<atomic>

#include<commdlg.h>

template<class T>
inline void SafeRelease(T*& pT) {
	if (pT != nullptr) {
		pT->Release();
		pT = nullptr;
	}
}

#ifdef _DEBUG
#	define  _OutputDebugString(str, ...)\
	{\
		TCHAR buf[256]; \
		_stprintf_s(buf, 256,  __VA_ARGS__); \
		OutputDebugString(buf); \
	}
#else 
#	define _OutputDebugString(str, ...)
#endif // _DEBUG

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

#pragma comment(lib, "Ws2_32.lib")

//// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
