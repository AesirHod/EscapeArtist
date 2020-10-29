#include "MoviePlayer.h"
#include <time.h>

//#define USE_FULLSCREEN
//#define FILL_WITH_WHITE
//#define DEBUG_ENUMERATE_DEVICES

int SamplesPerSound = 44100; // 48000
int SoundBufferLength = 44100 / 10;

MoviePlayer::MoviePlayer(HWND hWnd, HINSTANCE hInstance)
{
	m_hWnd = hWnd;

	m_bInitialized = false;
	m_bMovieLoaded = false;
	m_bPaused = false;

	ZeroMemory(m_szFilename, sizeof(m_szFilename));

	ZeroMemory(m_Devices, sizeof(m_Devices));
	m_NumDevices = 0;

	m_pDD = NULL;
	m_pDD4 = NULL;
	m_pDDSFront=NULL;
	m_pDDSBack=NULL;
	m_pDDSStretch = NULL;
	m_pDDClipper = NULL;

	m_pDS = NULL;
	m_pDSBPrimary = NULL;
	m_pDSBSecondary = NULL;
	m_RunningSampleIndex = 0;

	codec_init(&m_Codec, nullptr);

	m_PerformanceFrequency = 0;
	m_LastFrameTime = 0;
	m_DeltaTime = 0;
	ZeroMemory(&m_MovieRect, sizeof(m_MovieRect));
	m_AudioReadPosition = nullptr;
}

MoviePlayer::~MoviePlayer()
{
	Shutdown();
}

bool MoviePlayer::Initialize()
{
	HRESULT hr = NOERROR;
	DDSURFACEDESC2 ddsd, ddsd2;

	CoInitialize(NULL);

	GUID* pGuid = NULL;
#if defined DEBUG_ENUMERATE_DEVICES
	DirectDrawEnumerateEx(DDEnumCallbackEx, this, DDENUM_ATTACHEDSECONDARYDEVICES | DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_NONDISPLAYDEVICES);
	pGuid = &m_Devices[1].m_Guid;
#elif defined USE_FULLSCREEN
	pGuid = reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY);
#endif

	hr = DirectDrawCreate(pGuid, &m_pDD, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the IDirectDraw object.\n");
		return false;
	}

	hr = (m_pDD->QueryInterface(IID_IDirectDraw4, (LPVOID*)&m_pDD4));
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't get the IDirectDraw4 object.\n");
		return false;
	}

#ifdef USE_FULLSCREEN
	hr = m_pDD4->SetCooperativeLevel(GetDesktopWindow(), DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't set the cooperative level.\n");
		return false;
	}

	hr = m_pDD4->SetDisplayMode(1024, 768, 32, 0, 0);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't set the correct display mode.\n");
		return false;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	hr = m_pDD4->CreateSurface(&ddsd, &m_pDDSFront, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the front surface.\n");
		return false;
	}

	DDSCAPS2 ddscaps;
	ZeroMemory(&ddscaps, sizeof(ddscaps));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	hr = m_pDDSFront->GetAttachedSurface(&ddscaps, &m_pDDSBack);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't get the back surface.\n");
		return false;
	}

	hr = m_pDDSFront->GetSurfaceDesc(&ddsd);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't get the surface description.\n");
		return false;
	}

	ZeroMemory(&ddsd2, sizeof(ddsd2));
	ddsd2.dwSize = sizeof(ddsd2);
	ddsd2.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd2.dwHeight = ddsd.dwHeight;
	ddsd2.dwWidth = ddsd.dwWidth;
	ddsd2.ddpfPixelFormat = ddsd.ddpfPixelFormat;

	hr = m_pDD4->CreateSurface(&ddsd2, &m_pDDSStretch, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the stretch surface.\n");
		return false;
	}
#else
	hr = m_pDD4->SetCooperativeLevel(GetDesktopWindow(), DDSCL_NORMAL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't set the cooperative level.\n");
		return false;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hr = m_pDD4->CreateSurface(&ddsd, &m_pDDSFront, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the front surface.\n");
		return false;
	}

	hr = m_pDDSFront->GetSurfaceDesc(&ddsd);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't get the surface description.\n");
		return false;
	}

	ZeroMemory(&ddsd2, sizeof(ddsd2));
	ddsd2.dwSize = sizeof(ddsd2);
	ddsd2.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd2.dwHeight = ddsd.dwHeight;
	ddsd2.dwWidth = ddsd.dwWidth;
	ddsd2.ddpfPixelFormat = ddsd.ddpfPixelFormat;

	hr = m_pDD4->CreateSurface(&ddsd2, &m_pDDSBack, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the back surface.\n");
		return false;
	}

	hr = m_pDD4->CreateClipper(0, &m_pDDClipper, NULL);
	if(FAILED(hr))
	{
		OutputDebugString("Couldn't create the clipper.\n");
		return false;
	}

	hr = m_pDDSFront->SetClipper(m_pDDClipper);
	if(FAILED(hr))
	{
		OutputDebugString("Couldn't set the clipper.\n");
		return false;
	}

	hr = m_pDDClipper->SetHWnd(0, m_hWnd);
	if(FAILED(hr))
	{
		OutputDebugString("Couldn't set the hWnd.\n");
		return false;
	}
#endif

	hr = DirectSoundCreate(NULL, &m_pDS, NULL);
	if(FAILED(hr))
	{
		OutputDebugString("Couldn't create the IDirectSound object.\n");
		return false;
	}

	m_pDS->SetCooperativeLevel(m_hWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't set the cooperative level.\n");
		return false;
	}

	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	hr = m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBPrimary, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the primary sound buffer.\n");
		return false;
	}

	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = SamplesPerSound;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;

	hr = m_pDSBPrimary->SetFormat(&wfx);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't set the wave format.\n");
		return false;
	}

	m_bInitialized = true;

	return true;
}

void MoviePlayer::Shutdown()
{
	HRESULT hr;

	m_bInitialized = false;
	
	if (m_pDD4 != NULL)
	{
		hr = m_pDD4->RestoreDisplayMode();
		hr = m_pDD4->SetCooperativeLevel(GetDesktopWindow(), DDSCL_NORMAL);
	}

	if (m_pDS != NULL)
	{
		hr = m_pDS->SetCooperativeLevel(m_hWnd, DSSCL_NORMAL);
	}

	CoUninitialize();

	codec_clear(&m_Codec);

	if (m_pDSBSecondary != NULL)
	{
		m_pDSBSecondary->Release();
		m_pDSBSecondary = NULL;
	}

	if (m_pDSBPrimary != NULL)
	{
		m_pDSBPrimary->Release();
		m_pDSBPrimary = NULL;
	}

	if (m_pDS != NULL)
	{
		m_pDS->Release();
		m_pDS = NULL;
	}

	if (m_pDDSStretch != NULL)
	{
		m_pDDSStretch->Release();
		m_pDDSStretch = NULL;
	}

	if (m_pDDSBack != NULL)
	{
		m_pDDSBack->Release();
		m_pDDSBack = NULL;
	}

	if (m_pDDSFront != NULL)
	{
		m_pDDSFront->Release();
		m_pDDSFront = NULL;
	}

	if (m_pDDClipper != NULL)
	{
		m_pDDClipper->Release();
		m_pDDClipper = NULL;
	}

	if (m_pDD4 != NULL)
	{
		m_pDD4->Release();
		m_pDD4 = NULL;
	}

	if (m_pDD != NULL)
	{
		m_pDD->Release();
		m_pDD = NULL;
	}
}

bool MoviePlayer::LoadMovie()
{
	HRESULT hr = NOERROR;

	codec_init(&m_Codec, nullptr);

	codec_init(&m_Codec, RWFromFile(m_szFilename, "rb"));
	if (m_Codec.input == nullptr)
	{
		return false;
	}

	if (codec_open_rpl(&m_Codec) != 0)
	{
		return false;
	}

	m_MovieRect.top = 0;
	m_MovieRect.left = 0;
	m_MovieRect.bottom = m_Codec.video.height;
	m_MovieRect.right = m_Codec.video.width;

	m_RunningSampleIndex = 0;

	codec_decode_video(&m_Codec);

	// Apparently, this doesn't need to be the same as the primary buffer.
	// See https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb318673(v=vs.85)
	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = m_Codec.audio.channels;
	wfx.nSamplesPerSec = m_Codec.audio.sample_rate;
	wfx.wBitsPerSample = m_Codec.audio.bits_per_sample;
	wfx.nBlockAlign = (m_Codec.audio.channels * m_Codec.audio.bits_per_sample) / 8; // Eight bits in a byte.
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * m_Codec.audio.sample_rate;
	wfx.cbSize = 0;

	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
#ifdef USE_FULLSCREEN
	dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
#endif
	dsbd.dwBufferBytes = wfx.nBlockAlign * SoundBufferLength;
	dsbd.lpwfxFormat = &wfx;

	hr = m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBSecondary, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Couldn't create the secondary sound buffer.\n");
		return false;
	}

	hr = m_pDSBSecondary->SetCurrentPosition(0);
	if (FAILED(hr))
	{
		OutputDebugString("Could't set the sound buffer position.\n");
		return false;
	}

	UpdateAudio();

	m_PerformanceFrequency = GetPerformanceFrequency();
	m_LastFrameTime = GetPerformanceCounter();
	m_DeltaTime = 0;

	m_pDSBSecondary->Play(0, 0, DSBPLAY_LOOPING);

	return true;
}

void MoviePlayer::Update()
{
	HRESULT hr;

	if (!m_bInitialized || !m_bMovieLoaded)
	{
		return;
	}

	if (!m_bPaused)
	{
		ULONGLONG currentTime = GetPerformanceCounter();
		m_DeltaTime = currentTime - m_LastFrameTime;
		m_DeltaTime *= 1000000000;
		m_DeltaTime /= m_PerformanceFrequency;
		m_LastFrameTime = currentTime;

		if (m_Codec.input)
		{
			UpdateVideo();
			UpdateAudio();
		}
	}

#ifdef USE_FULLSCREEN
	hr = m_pDDSBack->Blt(NULL, m_pDDSStretch, &m_MovieRect, DDBLT_WAIT, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Blt failed.\n");
	}

	hr = m_pDDSFront->Flip(NULL, DDFLIP_WAIT);
	if (FAILED(hr))
	{
		OutputDebugString("Flip failed.\n");
	}
#else
	POINT point;
	RECT viewRect;
	GetClientRect(m_hWnd, &viewRect);
	point.x = viewRect.top;
	point.y = viewRect.left;
	ClientToScreen(m_hWnd, &point);
	viewRect.left = point.x;
	viewRect.top = point.y;
	point.x = viewRect.right;
	point.y = viewRect.bottom;
	ClientToScreen(m_hWnd, &point);
	viewRect.right = point.x;
	viewRect.bottom = point.y;

	hr = m_pDDSFront->Blt(&viewRect, m_pDDSBack, &m_MovieRect, DDBLT_WAIT, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Blt failed.\n");
	}
#endif
}

void MoviePlayer::UpdateVideo()
{
	HRESULT hr;

	bool isVideoPlaying = false;
	ULONGLONG frame = m_Codec.frame;
	codec_inc_time(&m_Codec, m_DeltaTime);
	while (frame < m_Codec.frame)
	{
		frame++;
		isVideoPlaying = (codec_decode_video(&m_Codec)) ? true : false;
	}

	if (m_Codec.video.rgba)
	{
#ifdef USE_FULLSCREEN
		IDirectDrawSurface4* pDestSurface = m_pDDSStretch;
#else
		IDirectDrawSurface4* pDestSurface = m_pDDSBack;
#endif

		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		hr = pDestSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, 0);
		if (FAILED(hr))
		{
			OutputDebugString("Could't lock the surface buffer.\n");
			return;
		}

#ifdef FILL_WITH_WHITE
		BYTE* pDest = (BYTE*)ddsd.lpSurface;
		for (int h = 0; h < ddsd.dwHeight; h++)
		{
			for (int w = 0; w < ddsd.dwWidth; w++)
			{
				*(DWORD*)pDest = 0xFFFFFFFF;
				pDest += 4;
			}
		}
#else
		int width = m_Codec.video.width;
		int height = m_Codec.video.height;
		const BYTE* pPixelData = m_Codec.video.rgba;
		BYTE* pTop = (BYTE*)ddsd.lpSurface;
		BYTE* pTopLeft = pTop;
		for (int h = 0; h < height; h++)
		{
			//memcpy(pTopLeft, data + (4 * width * h), 4 * width);
			//pTopLeft += (4 * ddsd.dwWidth);
			BYTE* pDest = pTopLeft;
			const BYTE* pSource = pPixelData + (4 * width * h);
			for (int w = 0; w < width; w++)
			{
				pDest[0] = pSource[2];
				pDest[1] = pSource[1];
				pDest[2] = pSource[0];
				pDest[3] = pSource[3];
				pSource += 4;
				pDest += 4;
			}
			pTopLeft += (4 * ddsd.dwWidth);
		}
#endif

		hr = pDestSurface->Unlock(0);
		if (FAILED(hr))
		{
			OutputDebugString("Could't unlock the surface buffer.\n");
			return;
		}
	}
}

void MoviePlayer::UpdateAudio()
{
	HRESULT hr;

	DWORD playCursor;
	DWORD writeCursor;
	hr = m_pDSBSecondary->GetCurrentPosition(&playCursor, &writeCursor);
	if (FAILED(hr))
	{
		OutputDebugString("Could't get the sound buffer positions\n");
		return;
	}

	int blockAlign = (m_Codec.audio.channels * m_Codec.audio.bits_per_sample) / 8; // Eight bits in a byte.
	int soundBufferSize = SoundBufferLength * blockAlign;
	DWORD bytesToLock = (m_RunningSampleIndex * blockAlign) % soundBufferSize;
	DWORD bytesToWrite;
	if (bytesToLock > playCursor)
	{
		bytesToWrite = soundBufferSize - bytesToLock;
		bytesToWrite += playCursor;
	}
	else
	{
		bytesToWrite = playCursor - bytesToLock;
	}

	if (bytesToWrite == 0)
	{
		return;
	}

	void* region1;
	DWORD region1Size;
	void* region2;
	DWORD region2Size;

	hr = m_pDSBSecondary->Lock(bytesToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0);
	if (FAILED(hr))
	{
		OutputDebugString("Could't lock the sound buffer.\n");
		return;
	}

	if (region1Size > 0)
	{
		StreamAudio(region1, region1Size);
	}

	if (region2Size > 0)
	{
		StreamAudio(region2, region2Size);
	}

	m_RunningSampleIndex += bytesToWrite / blockAlign;

	hr = m_pDSBSecondary->Unlock(region1, region1Size, region2, region2Size);
	if (FAILED(hr))
	{
		OutputDebugString("Could't unlock the sound buffer.\n");
	}
}

void MoviePlayer::StreamAudio(void* region, DWORD regionSize)
{
	BYTE* pDest = (BYTE*)region;
	for (DWORD i = 0; i < regionSize; i++)
	{
		if (m_AudioReadPosition == nullptr || m_AudioReadPosition >= (m_Codec.audio.buff + m_Codec.audio.buff_size))
		{
			codec_decode_audio(&m_Codec);
			m_AudioReadPosition = m_Codec.audio.buff;
		}

		*pDest = *m_AudioReadPosition;
		pDest++;
		m_AudioReadPosition++;
	}
}

void MoviePlayer::OpenFile(const char* szFileName)
{
	strncpy(m_szFilename, szFileName, MAX_PATH);

	if (m_bInitialized)
	{
		Shutdown();
	}

	if (!Initialize())
	{
		Shutdown();
		return;
	}

	if (LoadMovie())
	{
		m_bMovieLoaded = true;
		m_bPaused = false;
	}
	else
	{
		Shutdown();
		m_bMovieLoaded = false;
		m_bPaused = false;
	}
}

LONGLONG MoviePlayer::GetPerformanceCounter()
{
	LARGE_INTEGER counter;

	if (!QueryPerformanceCounter(&counter))
	{
		return clock();
	}

	return counter.QuadPart;
}

LONGLONG MoviePlayer::GetPerformanceFrequency()
{
	LARGE_INTEGER frequency;

	if (!QueryPerformanceFrequency(&frequency))
	{
		return 1000;
	}

	return frequency.QuadPart;
}

void MoviePlayer::Play()
{
	if (m_bInitialized && m_bMovieLoaded && m_bPaused)
	{
		m_bPaused = false;
		m_LastFrameTime = GetPerformanceCounter();
		Update();
		m_pDSBSecondary->Play(0, 0, DSBPLAY_LOOPING);
	}
}

void MoviePlayer::Pause()
{
	if (m_bInitialized && m_bMovieLoaded && !m_bPaused)
	{
		m_pDSBSecondary->Stop();
		Update();
		m_bPaused = true;
	}
}

void MoviePlayer::Stop()
{
	if (m_bInitialized && m_bMovieLoaded)
	{
		m_bPaused = false;
		Update();
		m_bPaused = false;
	}
}

BOOL WINAPI MoviePlayer::DDEnumCallbackEx(GUID FAR* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
	MoviePlayer* pThis = (MoviePlayer*)lpContext;
	DDDevice& device = pThis->m_Devices[pThis->m_NumDevices];

	if (lpGUID != NULL)
	{
		memcpy(&device.m_Guid, lpGUID, sizeof(device.m_Guid));
	}

	strncpy(device.m_szName, lpDriverDescription, sizeof(device.m_szName));

	device.m_IsHardware = false;
	pThis->m_NumDevices++;

	return pThis->m_NumDevices < 5 ? TRUE : FALSE;
}
