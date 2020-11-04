#pragma once

#include <ddraw.h>
#include <dsound.h>

#include "TinyCodec/tiny_codec.h"

struct DDDevice
{
	DDDevice* pNext;

	GUID m_Guid;
	char m_szName[256];
	bool m_IsHardware;
};

class MoviePlayer
{
protected:

	HWND m_hAppWnd;
	HWND m_hViewWnd;

	bool m_bInitialized;
	bool m_bMovieLoaded;
	bool m_bPaused;
	bool m_IsVideoFinished;
	bool m_IsAudioFinished;

	char m_szFilename[MAX_PATH];

	DDDevice m_Devices[5];
	int m_NumDevices;

	IDirectDraw* m_pDD;
	IDirectDraw4* m_pDD4;
	IDirectDrawSurface4* m_pDDSFront;
	IDirectDrawSurface4* m_pDDSBack;
	IDirectDrawSurface4* m_pDDSStretch;
	IDirectDrawClipper* m_pDDClipper;

	IDirectSound* m_pDS;
	IDirectSoundBuffer* m_pDSBPrimary;
	IDirectSoundBuffer* m_pDSBSecondary;
	DWORD m_RunningSampleIndex;

	tiny_codec_t m_Codec;
	ULONGLONG m_PerformanceFrequency;
	ULONGLONG m_LastFrameTime;
	ULONGLONG m_DeltaTime;
	RECT m_MovieRect;
	BYTE* m_AudioReadPosition;

	bool Initialize();
	bool LoadMovie();
	void Shutdown();
	void UpdateVideo();
	void UpdateAudio();
	void StreamAudio(void* region, DWORD regionSize);
	LONGLONG GetPerformanceCounter();
	LONGLONG GetPerformanceFrequency();

	static BOOL WINAPI DDEnumCallbackEx(GUID FAR* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm);

public:

	MoviePlayer(HWND hAppWnd, HWND hViewWnd, HINSTANCE hInstance);
	virtual ~MoviePlayer();

	void OpenFile(const char* szFileName);
	void Update();
	void Play();
	void Pause();
	void Stop();
};