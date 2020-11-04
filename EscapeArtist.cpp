#include <windows.h>
#include "MoviePlayer.h"
#include "resource.h"

enum ControlID
{
	CID_DDView = 1,
	CID_FileOpen,
	CID_FileExit,
	CID_PlayButton,
	CID_PauseButton,
	CID_StopButton,
};

HWND g_hMainWnd;
HINSTANCE g_hInst;
MoviePlayer* g_MovieView = NULL;

HWND InitWindow(int iCmdShow);
LRESULT CALLBACK WndProcMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void ProcessIdle();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	g_hInst = hInstance;
	g_hMainWnd = InitWindow(nCmdShow);

	if (!g_hMainWnd)
	{
		return -1;
	}

	while (true)
	{
		MSG msg;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			ProcessIdle();
		}
	}

	if (g_MovieView != NULL)
	{
		delete g_MovieView;
		g_MovieView = NULL;
	}

	return 0;
}

HWND InitWindow(int iCmdShow)
{
	WNDCLASS wcMainClass;
	wcMainClass.style = CS_HREDRAW | CS_VREDRAW;
	wcMainClass.lpfnWndProc = WndProcMain;
	wcMainClass.cbClsExtra = 0;
	wcMainClass.cbWndExtra = 0;
	wcMainClass.hInstance = g_hInst;
	wcMainClass.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ESCAPE_ICON));
	wcMainClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcMainClass.hbrBackground = GetSysColorBrush(COLOR_APPWORKSPACE);
	wcMainClass.lpszMenuName = TEXT("");
	wcMainClass.lpszClassName = TEXT("EscapeArtist");
	RegisterClass(&wcMainClass);

	WNDCLASS wxViewClass;
	wxViewClass.style = CS_HREDRAW | CS_VREDRAW;
	wxViewClass.lpfnWndProc = WndProcView;
	wxViewClass.cbClsExtra = 0;
	wxViewClass.cbWndExtra = 0;
	wxViewClass.hInstance = g_hInst;
	wxViewClass.hIcon = NULL;
	wxViewClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wxViewClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wxViewClass.lpszMenuName = TEXT("");
	wxViewClass.lpszClassName = TEXT("DDView");
	RegisterClass(&wxViewClass);

	int width = 640;
	int height = 520;

	HWND hMainWnd = CreateWindowEx(
		0,
		TEXT("EscapeArtist"),
		TEXT("EscapeArtist"),
		WS_OVERLAPPEDWINDOW, // WS_POPUP,
		(GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2),
		0, //(GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2),
		640, //GetSystemMetrics(SM_CXSCREEN) / 3,
		520, //GetSystemMetrics(SM_CYSCREEN) / 3,
		NULL,
		NULL,
		g_hInst,
		NULL
	);

	ShowWindow(hMainWnd, TRUE);
	UpdateWindow(hMainWnd);
	SetFocus(hMainWnd);

	return hMainWnd;
}

LRESULT CALLBACK WndProcMain(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			HWND hViewWnd = CreateWindowEx(
				0,
				"DDView",
				NULL,
				WS_CHILD,
				10,
				10,
				600,
				400,
				hWnd,
				(HMENU)CID_DDView,
				g_hInst,
				NULL
			);

			ShowWindow(hViewWnd, TRUE);
			UpdateWindow(hViewWnd);

			HWND hPlayBtn = CreateWindowEx(
				0,
				"Button",
				"Play",
				BS_PUSHBUTTON | BS_ICON | WS_CHILD | WS_VISIBLE,
				20,
				420,
				100,
				20,
				hWnd,
				(HMENU)CID_PlayButton,
				g_hInst,
				0
			);

			HICON hPlayIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PLAY_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			SendMessage(hPlayBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hPlayIcon);

			ShowWindow(hPlayBtn, TRUE);
			UpdateWindow(hPlayBtn);

			HWND hPauseBtn = CreateWindowEx(
				0,
				"Button",
				"Pause",
				BS_PUSHBUTTON | BS_ICON | WS_CHILD | WS_VISIBLE,
				120,
				420,
				100,
				20,
				hWnd,
				(HMENU)CID_PauseButton,
				g_hInst,
				0
			);

			HICON hPauseIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_PAUSE_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			SendMessage(hPauseBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hPauseIcon);

			ShowWindow(hPauseBtn, TRUE);
			UpdateWindow(hPauseBtn);

			HWND hStopBtn = CreateWindowEx(
				0,
				"Button",
				"Stop",
				BS_PUSHBUTTON | BS_ICON | WS_CHILD | WS_VISIBLE,
				220,
				420,
				100,
				20,
				hWnd,
				(HMENU)CID_StopButton,
				g_hInst,
				0
			);

			HICON hStopIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_STOP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			SendMessage(hStopBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hStopIcon);

			ShowWindow(hStopBtn, TRUE);
			UpdateWindow(hStopBtn);

			HMENU hMenu = CreateMenu();

			HMENU hSubMenu = CreatePopupMenu();

			AppendMenu(hSubMenu, MF_STRING, CID_FileOpen, "&Open");
			AppendMenu(hSubMenu, MF_STRING, CID_FileExit, "&Exit");
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "&File");

			SetMenu(hWnd, hMenu);

			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				if (g_MovieView != NULL)
				{
					g_MovieView->Pause();
				}
			}
			else
			{
				if (g_MovieView != NULL)
				{
					g_MovieView->Play();
				}
			}

			break;
		}
		case WM_ENTERSIZEMOVE:
		{
			if (g_MovieView != NULL)
			{
				g_MovieView->Pause();
			}

			break;
		}
		case WM_EXITSIZEMOVE:
		{
			if (g_MovieView != NULL)
			{
				g_MovieView->Play();
			}

			break;
		}
		case WM_COMMAND:
		{
			switch ((HIWORD(wParam)))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
						case CID_FileOpen:
						{
							char szFileName[MAX_PATH];

							OPENFILENAME ofn;
							ZeroMemory(&szFileName, sizeof(szFileName));
							ZeroMemory(&ofn, sizeof(ofn));
							ofn.lStructSize = sizeof(ofn);
							ofn.hwndOwner = hWnd;
							ofn.lpstrFilter = "RPL Movie Files (*.rpl)\0*.rpl\0All Files (*.*)\0*.*\0";
							ofn.lpstrFile = szFileName;
							ofn.nMaxFile = MAX_PATH;
							ofn.lpstrTitle = "Open";
							ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

							if (GetOpenFileName(&ofn))
							{
								g_MovieView->OpenFile(ofn.lpstrFile);
							}

							break;
						}
						case CID_FileExit:
						{
							PostQuitMessage(0);
							return 0;
						}
						case CID_PlayButton:
						{
							if (g_MovieView != NULL)
							{
								g_MovieView->Play();
							}
							break;
						}
						case CID_PauseButton:
						{
							if (g_MovieView != NULL)
							{
								g_MovieView->Pause();
							}
							break;
						}
						case CID_StopButton:
						{
							if (g_MovieView != NULL)
							{
								g_MovieView->Stop();
							}
							break;
						}
						default:
						{
							break;
						}
					}
				}
				default:
				{
					break;
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		}
		default:
		{
			break;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK WndProcView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			//g_MovieView = new MoviePlayer(GetDesktopWindow(), hWnd, g_hInst);
			g_MovieView = new MoviePlayer(GetParent(hWnd), hWnd, g_hInst);
			break;
		}
		case WM_DESTROY:
		{
			delete g_MovieView;
			g_MovieView = NULL;
			break;
		}
		default:
		{
			break;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void ProcessIdle()
{
	if (g_MovieView != NULL)
	{
		g_MovieView->Update();
	}
}