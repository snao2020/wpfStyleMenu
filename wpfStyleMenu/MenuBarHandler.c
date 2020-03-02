#include <windows.h>
#include <windowsx.h>

#include "MenuBarHandler.h"
#include "ContextMenuHandler.h"
#include "MenuHelper.h"


static UINT Flags;

static BOOL HookWmMenuSelect(MSG* pMsg);
static BOOL HookWmKeyDown(MSG* pMsg);
static BOOL HookWmRegPostKeyDown(MSG* pMsg);
static BOOL HookWmChar(MSG* pMsg);
static BOOL HookWmMouseMove(MSG* pMsg);
static BOOL HookWmMouseButtonDown(MSG* pMsg); 


void MenuBarHandler_Setup()
{
	Flags = 0;
}


void MenuBarHandler_Cleanup()
{
}


BOOL MenuBarHandler_MsgFilterProc(MSG* pMsg)
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
			case WM_CHAR:			cancel = HookWmChar(pMsg); break;
			case WM_MOUSEMOVE:		cancel = HookWmMouseMove(pMsg); break;
			case WM_LBUTTONDOWN:	
			case WM_RBUTTONDOWN:	cancel = HookWmMouseButtonDown(pMsg); break;
			}
		}
	}
	return cancel;
}


BOOL IsDropDown;

static BOOL HookWmMenuSelect(MSG* pMsg)
{
	BOOL cancel = FALSE;

	HWND hwnd = pMsg->hwnd;
	HMENU hmenu = (HMENU)pMsg->lParam;
	//int item = HIWORD(pMsg->wParam) & MF_POPUP ? 0L : (int)LOWORD(pMsg->wParam);
	//HMENU hmenuPopup = HIWORD(pMsg->wParam) & MF_POPUP ? GetSubMenu((HMENU)pMsg->lParam, LOWORD(pMsg->wParam)) : 0L;
	UINT flags = (UINT)((short)HIWORD(pMsg->wParam) == -1 ? 0xFFFFFFFF : HIWORD(pMsg->wParam));
	
	if (hmenu == GetMenu(hwnd))
	{
		if (!(Flags & PMI_FIRSTMSGDONE))
		{
			Flags |= PMI_FIRSTMSGDONE;

			if (flags != UINT_MAX && flags & (MF_GRAYED | MF_DISABLED))
			{
				if (GetFirstEnabled(hmenu) < 0)
					cancel = TRUE;
				else
					PostMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
			}
		}
	}

	else if (flags != UINT_MAX && flags & MF_SYSMENU)
	{
		PopupMenuItem* pPMI = PopupMenuList_FindItem(hmenu);
		if (pPMI)
		{
			HWND h = PopupMenuItem_GetHWnd(pPMI);
			if (h)
				SetWindowPos(h, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
		}
	}

	else
		cancel = ContextMenuHandler_MsgFilterProc(pMsg);

	return cancel;
}


static BOOL HookWmKeyDown(MSG* pMsg)
{
	BOOL cancel = FALSE;

	HWND hwnd = pMsg->hwnd;
	UINT vk = (UINT)pMsg->wParam;
	//int cRepeat = (int)(short)LOWORD(pMsg->lParam);
	//UINT flags = (UINT)HIWORD(pMsg->lParam);

	Flags |= PMI_FIRSTMSGDONE;

	DWORD guiThreadFlags;
	switch (vk)
	{
	case VK_TAB:	// do not exit menuloop
		cancel = TRUE;
		break;
	case VK_LEFT:
	case VK_RIGHT:
		PostMessage(hwnd, RegPostKeyDown(), vk, 0);
		break;
	default:
		guiThreadFlags = GetGuiThreadFlags();
		if (guiThreadFlags != MAXDWORD && guiThreadFlags & GUI_SYSTEMMENUMODE)
			cancel = TRUE;
		else
			cancel = ContextMenuHandler_MsgFilterProc(pMsg);
	}

	return cancel;
}


static BOOL HookWmRegPostKeyDown(MSG* pMsg)
{
	HWND hwnd = pMsg->hwnd;
	UINT vk = pMsg->wParam;

	if (vk == VK_LEFT || vk == VK_RIGHT)
	{
		DWORD guiThreadFlags = GetGuiThreadFlags();
		if (guiThreadFlags != MAXDWORD && guiThreadFlags & GUI_SYSTEMMENUMODE)
		{
			PostMessage(pMsg->hwnd, WM_KEYDOWN, vk, 0);
		}
		else if (PopupMenuList_GetTop() == 0)
		{
			HMENU hmenu = GetMenu(hwnd);
			int pos = GetHilitePos(hmenu);
			if (pos >= 0)
			{
				UINT state = GetMenuState(hmenu, pos, MF_BYPOSITION);
				if (state != UINT_MAX && state & (MF_GRAYED | MF_DISABLED))
				{
					PostMessage(hwnd, WM_KEYDOWN, vk, 0);
				}
			}
		}
	}
	else
		ContextMenuHandler_MsgFilterProc(pMsg);
	
	return TRUE;
}


static BOOL HookWmChar(MSG* pMsg)
{
	BOOL cancel = FALSE;

	HMENU hmenu = GetMenu(pMsg->hwnd);
	int pos = GetMnemonicItem(hmenu, (TCHAR)pMsg->wParam);
	if (pos >= 0)
	{
		UINT state = GetMenuState(hmenu, pos, MF_BYPOSITION);
		if (state != UINT_MAX && state & (MF_GRAYED | MF_DISABLED))
			cancel = TRUE;
	}
	return cancel;
}


static BOOL HookWmMouseMove(MSG* pMsg)
{
	BOOL cancel = FALSE;

	HWND hwnd = pMsg->hwnd;
	//int x = GET_X_LPARAM(pMsg->lParam);
	//int y = GET_Y_LPARAM(pMsg->lParam);
	//UINT keyFlags = (UINT)pMsg->wParam;

	Flags |= PMI_FIRSTMSGDONE;

	HMENU hmenu = GetMenu(hwnd);
	if (hmenu)
	{
		POINT ptScreen;
		ptScreen.x = GET_X_LPARAM(pMsg->lParam);
		ptScreen.y = GET_Y_LPARAM(pMsg->lParam);

		int pos = MenuItemFromPoint(hwnd, hmenu, ptScreen);
		if (pos >= 0)
		{
			UINT state = GetMenuState(hmenu, pos, MF_BYPOSITION);
			if (state != UINT_MAX && state & (MF_GRAYED | MF_DISABLED))
				cancel = TRUE;
		}
		else if (PopupMenuList_HitTest(&ptScreen))
			cancel = ContextMenuHandler_MsgFilterProc(pMsg);
		else
			cancel = TRUE;
	}
	return cancel;
}


static BOOL HookWmMouseButtonDown(MSG* pMsg)
{
	BOOL cancel = FALSE;

	HWND hwnd = pMsg->hwnd;
	//int x = GET_X_LPARAM(pMsg->lParam);
	//int y = GET_Y_LPARAM(pMsg->lParam);
	//UINT keyFlags = (UINT)pMsg->wParam;

	Flags |= PMI_FIRSTMSGDONE;
	
	HMENU hmenu = GetMenu(hwnd);
	if (hmenu)
	{
		POINT ptScreen;
		ptScreen.x = GET_X_LPARAM(pMsg->lParam);
		ptScreen.y = GET_Y_LPARAM(pMsg->lParam);

		int pos = MenuItemFromPoint(hwnd, hmenu, ptScreen);
		if (pos >= 0)
		{
			UINT state = GetMenuState(hmenu, pos, MF_BYPOSITION);
			if (state != UINT_MAX && state & (MF_GRAYED | MF_DISABLED))
				cancel = TRUE;
		}
	}
	return cancel;
}


