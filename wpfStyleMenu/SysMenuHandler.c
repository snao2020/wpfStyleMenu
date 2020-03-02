#include <windows.h>
#include <windowsx.h>

#include "SysMenuHandler.h"

#include "ContextMenuHandler.h"
#include "MenuHelper.h"


static BOOL HookWmMenuSelect(MSG* pMsg);
static BOOL HookWmKeyDown(MSG* pMsg);
static BOOL HookWmRegPostKeyDown(MSG* pMsg);
static BOOL HookWmMouseMove(MSG* pMsg);
static BOOL HookWmMouseButtonDown(MSG* pMsg);


void SysMenuHandler_Setup()
{
}


void SysMenuHandler_Cleanup()
{
}


BOOL SysMenuHandler_MsgFilterProc(MSG* pMsg)
{
	BOOL cancel = FALSE;

	if (pMsg)
	{
		if (pMsg->message == RegPostKeyDown())
			cancel = HookWmRegPostKeyDown(pMsg);
		else
		{
			switch (pMsg->message)
			{
			case WM_MENUSELECT:		cancel = HookWmMenuSelect(pMsg); break;
			case WM_KEYDOWN:		cancel = HookWmKeyDown(pMsg); break;
			case WM_MOUSEMOVE:		cancel = HookWmMouseMove(pMsg); break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:	cancel = HookWmMouseButtonDown(pMsg); break;
			}
		}
	}
	return cancel;
}


static BOOL HookWmMenuSelect(MSG* pMsg)
{
	BOOL cancel = FALSE;

	//HWND hwnd = pMsg->hwnd;
	//HMENU hmenu = (HMENU)pMsg->lParam;
	//int item = HIWORD(pMsg->wParam) & MF_POPUP ? 0L : (int)LOWORD(pMsg->wParam);
	//HMENU hmenuPopup = HIWORD(pMsg->wParam) & MF_POPUP ? GetSubMenu((HMENU)pMsg->lParam, LOWORD(pMsg->wParam)) : 0L;
	//UINT flags = (UINT)((short)HIWORD(pMsg->wParam) == -1 ? 0xFFFFFFFF : HIWORD(pMsg->wParam));

	return cancel || ContextMenuHandler_MsgFilterProc(pMsg);
}



static BOOL HookWmKeyDown(MSG* pMsg)
{
	BOOL cancel = FALSE;

	//HWND hwnd = pMsg->hwnd;
	UINT vk = (UINT)pMsg->wParam;
	//int cRepeat = (int)(short)LOWORD(pMsg->lParam);
	//UINT flags = (UINT)HIWORD(pMsg->lParam);

	PopupMenuItem* pPMI;
	switch(vk)
	{
	case VK_ESCAPE:	// exit menuloop
		if (PopupMenuList_GetCount() == 1)
		{
			cancel = TRUE;
			EndMenu();
		}
		break;

	case VK_LEFT:	// do not move to menubar
		if (PopupMenuList_GetCount() == 1)
			cancel = TRUE;
		break;

	case VK_RIGHT:	// do not move to menubar
		pPMI = PopupMenuList_GetTop();
		if(pPMI)
		{
			HWND hwndPopup = PopupMenuItem_GetHWnd(pPMI);
			HMENU hmenuPopup = (HMENU)SendMessage(hwndPopup, MN_GETHMENU, 0, 0);
			if (hmenuPopup)
			{
				int pos = PopupMenuItem_GetKeyboardSelection(pPMI);
				if (pos >= 0)
				{
					UINT state = GetMenuState(hmenuPopup, pos, MF_BYPOSITION);
					if (state != UINT_MAX)
					{
						if (!(state & MF_POPUP))
							cancel = TRUE;
						else if (state & (MF_GRAYED | MF_DISABLED))
							cancel = TRUE;
					}
				}
			}
		}
		break;
	}
	return cancel || ContextMenuHandler_MsgFilterProc(pMsg);
}


static BOOL HookWmRegPostKeyDown(MSG* pMsg)
{
	return ContextMenuHandler_MsgFilterProc(pMsg);
}


static BOOL HookWmMouseMove(MSG* pMsg)
{
	BOOL cancel = FALSE;

	//HWND hwnd = pMsg->hwnd;
	//int x = GET_X_LPARAM(pMsg->lParam);
	//int y = GET_Y_LPARAM(pMsg->lParam);
	//UINT keyFlags = (UINT)pMsg->wParam;

	return cancel || ContextMenuHandler_MsgFilterProc(pMsg);
}


static BOOL HookWmMouseButtonDown(MSG* pMsg)
{
	BOOL cancel = FALSE;

	//HWND hwnd = pMsg->hwnd;
	//int x = GET_X_LPARAM(pMsg->lParam);
	//int y = GET_Y_LPARAM(pMsg->lParam);
	//UINT keyFlags = (UINT)pMsg->wParam;

	POINT ptScreen;
	ptScreen.x = GET_X_LPARAM(pMsg->lParam);
	ptScreen.y = GET_Y_LPARAM(pMsg->lParam);

	if (!PopupMenuList_HitTest(&ptScreen)) // do not move to menubar
	{
		cancel = TRUE;
		EndMenu();
	}

	return cancel || ContextMenuHandler_MsgFilterProc(pMsg);
}


