#include <windows.h>
#include <windowsx.h>

#include "TpmNotifyHook.h"	
#include "MenuHook.h"
#include "MenuHelper.h"

#include "Resource.h"


HINSTANCE HInstance;
TCHAR MainWindowClassName[] = TEXT("MainWindow");

static BOOL RegisterWindowClass(void);
static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL OpenContextMenu(HWND hwnd, LPARAM lParam);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpstrCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpstrCmdLine;

	int result = 0;
	HInstance = hInstance;
	
	if (RegisterWindowClass())
	{
		HWND hwnd = CreateWindow(MainWindowClassName, TEXT("MainWindow"),
			WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);

		if (hwnd)
		{
			TpmNotifyHook_Initialize();
			MenuHook_Hook(0, GetCurrentThreadId());

			ShowWindow(hwnd, nCmdShow);

			MSG msg;
			while (GetMessage(&msg, 0, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			result = (int)msg.wParam;

			MenuHook_Unhook();
			TpmNotifyHook_Uninitialize();
		}
	}
	return result;
}


static BOOL RegisterWindowClass()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = HInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAINWINDOW);
	wc.lpszClassName = MainWindowClassName;

	return RegisterClass(&wc) != 0;
}


static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	PAINTSTRUCT ps;
	HMENU hPopup;

	switch (message)
	{
	case WM_CONTEXTMENU:
		if (!OpenContextMenu(hwnd, lParam))
			result = DefWindowProc(hwnd, message, wParam, lParam);
		break;	
	case WM_SIZE:
		MoveWindow(GetDlgItem(hwnd, IDC_EDIT), 0, 0, GET_X_LPARAM(lParam) / 2, GET_Y_LPARAM(lParam), TRUE);
		break;
	case WM_CREATE:
		CreateWindow(TEXT("EDIT"), 0, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT, HInstance, 0);
		
		hPopup = CreatePopupMenu();
		AppendMenu(hPopup, 0, 100, TEXT("childItem"));
		AppendMenu(hPopup, 0, 101, TEXT("childItem"));
		AppendMenu(hPopup, 0, 102, TEXT("childItem"));
		AppendMenu(hPopup, 0, 103, TEXT("childItem"));
		AppendMenu(hPopup, 0, 104, TEXT("childItem"));
		InsertMenu(GetSystemMenu(hwnd, FALSE), 0, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hPopup, TEXT("SubMenu"));
		break;	
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		if (BeginPaint(hwnd, &ps))
			EndPaint(hwnd, &ps);
		break;
	default:
		result = DefWindowProc(hwnd, message, wParam, lParam);
		break;
	}
	return result;
}


static BOOL OpenContextMenu(HWND hwnd, LPARAM lParam)
{
	BOOL result = TRUE;

	RECT rc;
	GetClientRect(hwnd, &rc);

	POINT pt;
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	if (pt.x == -1 && pt.y == -1)
	{
		pt.x = (rc.left + rc.right) / 2;
		pt.y = (rc.top + rc.bottom) / 2;
		ClientToScreen(hwnd, &pt);
	}
	else
	{
		POINT ptClient = pt;
		ScreenToClient(hwnd, &ptClient);
		result = PtInRect(&rc, ptClient);
	}
	if (result)
	{
		HMENU hMenu = LoadMenu(HInstance, MAKEINTRESOURCE(IDM_CONTEXTMENU));
		if (hMenu)
		{
			HMENU hMenuPopup = GetSubMenu(hMenu, 0);
			TrackPopupMenu(hMenuPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, 0);

			DestroyMenu(hMenu);
		}
	}
	return result;
}


