#pragma once

#include <ddraw.h>
#include <dsound.h>

#include "TinyCodec/tiny_codec.h"

class MoviePlayer
{
protected:

	HWND m_hWnd;

	bool m_bInitialized;
	bool m_bMovieLoaded;
	bool m_bPaused;

	char m_szFilename[MAX_PATH];

	IDirectDraw* m_pDD;
	IDirectDraw4* m_pDD4;
	IDirectDrawSurface4* m_pDDSFront;
	IDirectDrawSurface4* m_pDDSBack;
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

public:

	MoviePlayer(HWND hWnd, HINSTANCE hInstance);
	virtual ~MoviePlayer();

	void OpenFile(const char* szFileName);
	void Update();
	void Play();
	void Pause();
	void Stop();
};