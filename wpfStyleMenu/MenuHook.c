#include <windows.h>
#include <windowsx.h>

#include "MenuHook.h"

#include "ContextMenuHandler.h"
#include "MenuBarHandler.h"
#include "SysMenuHandler.h"
#include "MenuHelper.h"


//---------------------------------------------------------
//
// MenuMode declaration
//

enum MenuModeEnum
{
	MENUMODE_NONE,
	MENUMODE_CANCEL,		// Exit SysCommand
	MENUMODE_CONTEXTMENU,	// WM_CONTEXTMENU(includes DefWindowProc)
	MENUMODE_MENUBAR,		// Click Alt or LButtonDown on MenuBarItem
	MENUMODE_SYSMENU,		// Alt+Space or Space in MENUMODE_MENUBAR or LButtonDown on App Icon
};

typedef enum MenuModeEnum MenuMode;


static MenuMode MenuMode_GetValue();
static void MenuMode_SetValue(HWND hwnd, MenuMode menuMode);


//-------------------------------------------------------
//
// MenuHook publics
//

static HMODULE HModule;
static DWORD ThreadId;
static HHOOK HHookCallWndProc;

static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);


void MenuHook_Hook(HMODULE hModule, DWORD dwThreadId)
{
	if (!HHookCallWndProc)
	{
		if (RegPostKeyDown())
		{
			HModule = hModule;
			ThreadId = dwThreadId;

			HHookCallWndProc = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hModule, dwThreadId);
		}
	}
}


void MenuHook_Unhook()
{
	if (HHookCallWndProc)
	{
		UnhookWindowsHookEx(HHookCallWndProc);
		HHookCallWndProc = 0;
	}
}


//---------------------------------------------------------
//
// MenuHook static definitions	
//

static HHOOK HHookMsgFilter;


static LRESULT CALLBACK MsgFilterProc(int nCode, WPARAM wParam, LPARAM lParam);
static BOOL IsValidSysMenu(HWND hwnd);

static void WmSysCommand(HWND hwnd, UINT cmd, int x, int y);
static BOOL WmCreate(HWND hwnd, CREATESTRUCT* pCreateStruct);
static void WmDestroy(HWND hwnd);
static void WmEnterMenuLoop(HWND hwnd, BOOL isTrackPopupMenu);
static void WmExitMenuLoop(HWND hwnd, BOOL isTrackPopupMenu);


static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wp, LPARAM lp)
{
	if (lp != 0)
	{
		CWPSTRUCT* pCWP = (CWPSTRUCT*)lp;
		WPARAM wParam = pCWP->wParam;
		LPARAM lParam = pCWP->lParam;
		switch (pCWP->message)
		{
			HANDLE_MSGBREAK(pCWP->hwnd, WM_SYSCOMMAND, WmSysCommand);
			HANDLE_MSGBREAK(pCWP->hwnd, WM_CREATE, WmCreate);
			HANDLE_MSGBREAK(pCWP->hwnd, WM_DESTROY, WmDestroy);
			HANDLE_MSGBREAK(pCWP->hwnd, WM_ENTERMENULOOP, WmEnterMenuLoop);
			HANDLE_MSGBREAK(pCWP->hwnd, WM_EXITMENULOOP, WmExitMenuLoop);
		}
	}
	return CallNextHookEx(HHookCallWndProc, nCode, wp, lp);
}


static void WmSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
	HMENU hMenu = GetMenu(hwnd);
	POINT pt;
	int pos;

	switch (cmd & 0xfff0)
	{
	case SC_KEYMENU:
		if (x == ' ')	// Alt+Space
		{
			if(IsValidSysMenu(hwnd))
				MenuMode_SetValue(hwnd, MENUMODE_SYSMENU);
			else
				MenuMode_SetValue(hwnd, MENUMODE_CANCEL);
		}
		else if (GetFirstEnabled(hMenu) >= 0)
			MenuMode_SetValue(hwnd, MENUMODE_MENUBAR);
		else	// hMenu is 0 or all items in menubar are disabled
			MenuMode_SetValue(hwnd, MENUMODE_CANCEL);
		break;

	case SC_MOUSEMENU:
		pt.x = x;
		pt.y = y;
		pos = MenuItemFromPoint(hwnd, hMenu, pt);
		if (pos >= 0)
		{
			UINT state = GetMenuState(hMenu, pos, MF_BYPOSITION);
			if (state != UINT_MAX && state & (MF_GRAYED | MF_DISABLED))
				MenuMode_SetValue(hwnd, MENUMODE_CANCEL);
			else
				MenuMode_SetValue(hwnd, MENUMODE_MENUBAR);
		}
		else
		{
			if (IsValidSysMenu(hwnd))
				MenuMode_SetValue(hwnd, MENUMODE_SYSMENU);
			else
				MenuMode_SetValue(hwnd, MENUMODE_CANCEL);
		}
		break;

	default:
		MenuMode_SetValue(hwnd, MENUMODE_NONE);
		break;
	}
}


static BOOL WmCreate(HWND hwnd, CREATESTRUCT* pCreateStruct)
{
	if (pCreateStruct->lpszClass == MAKEINTRESOURCE(0x8000))
	{
		PopupMenuList_Push(hwnd);
	}
	return 0;
}


static void WmDestroy(HWND hwnd)
{
	PopupMenuList_Pop(hwnd);
}


static void WmEnterMenuLoop(HWND hwnd, BOOL isTrackPopupMenu)
{
	(void)isTrackPopupMenu;

	if (MenuMode_GetValue() == MENUMODE_CANCEL)
	{
		MenuMode_SetValue(hwnd, MENUMODE_NONE);
		SendMessage(hwnd, WM_CANCELMODE, 0, 0);
	}
	else if(!HHookMsgFilter)
	{
		if (MenuMode_GetValue() == MENUMODE_NONE)
		{
			DWORD guiThreadFlags = GetGuiThreadFlags();
			if (guiThreadFlags != MAXDWORD && guiThreadFlags & GUI_POPUPMENUMODE)
				MenuMode_SetValue(hwnd, MENUMODE_CONTEXTMENU);
		}
		if (MenuMode_GetValue() != MENUMODE_NONE)
		{
			HHookMsgFilter = SetWindowsHookEx(WH_MSGFILTER, MsgFilterProc, HModule, ThreadId);

			ContextMenuHandler_Setup();
			MenuBarHandler_Setup();
			SysMenuHandler_Setup();
		}
	}
}


static void WmExitMenuLoop(HWND hwnd, BOOL isTrackPopupMenu)
{
	(void)isTrackPopupMenu;

	if (HHookMsgFilter)
	{
		MenuMode_SetValue(hwnd, MENUMODE_NONE);

		SysMenuHandler_Cleanup();
		MenuBarHandler_Cleanup();
		ContextMenuHandler_Cleanup();

		PopupMenuList_Clear();

		UnhookWindowsHookEx(HHookMsgFilter);
		HHookMsgFilter = 0;
	}
}


static BOOL IsValidSysMenu(HWND hwnd)
{
	BOOL result = FALSE;
	HMENU hmenuParent = GetSysMenuParent(hwnd);
	if (hmenuParent)
	{
		HMENU hmenuPopup = GetSubMenu(hmenuParent, 0);
		if (hmenuPopup)
		{
			if (GetMenuItemCount(hmenuPopup) > 0)
			{
				result = TRUE;
			}
		}
	}
	return result;
}

 //--------------------------------------------
//
// MsgFilter definitions
//

static void UpdateMenuMode(MSG* pMsg);


static LRESULT CALLBACK MsgFilterProc(int code, WPARAM wParam, LPARAM lParam)
{
	BOOL cancel = FALSE;
	if (code == MSGF_MENU && lParam)
	{
		MSG* pMsg = (MSG*)lParam;
		UpdateMenuMode(pMsg);

		switch (MenuMode_GetValue())
		{
		case MENUMODE_CONTEXTMENU:	
			cancel = ContextMenuHandler_MsgFilterProc(pMsg);
			break;
		case MENUMODE_SYSMENU:		
			cancel = SysMenuHandler_MsgFilterProc(pMsg);
			break;
		case MENUMODE_MENUBAR:
			cancel = MenuBarHandler_MsgFilterProc(pMsg);
			break;
		}		
	}
	return cancel ? 1 : CallNextHookEx(HHookMsgFilter, code, wParam, lParam);
}


static void UpdateMenuMode(MSG* pMsg)
{
	if (MenuMode_GetValue() == MENUMODE_MENUBAR)
	{
		switch (pMsg->message)
		{
		case WM_KEYDOWN:
			if (pMsg->wParam == VK_SPACE && PopupMenuList_GetTop() == 0)
				MenuMode_SetValue(pMsg->hwnd, MENUMODE_SYSMENU);
			break;

		case WM_LBUTTONDOWN:
			if (SendMessage(pMsg->hwnd, WM_NCHITTEST, 0, pMsg->lParam) == HTSYSMENU)
				MenuMode_SetValue(pMsg->hwnd, MENUMODE_SYSMENU);
			break;
		}
	}
}


//----------------------------------------------------
//
// MenuMode definitions
//

static MenuMode MenuMode_Value = MENUMODE_NONE;
//static BOOL MenuMode_IsDefaultSysMenu;

//static void MenuMode_RestoreSysMenu(HWND hwnd);
//static void MenuMode_DisableSysMenu(HWND hwnd);


static MenuMode MenuMode_GetValue()
{
	return MenuMode_Value;
}


static void MenuMode_SetValue(HWND hwnd, MenuMode menuMode)
{
	(void)hwnd;
	if (menuMode != MenuMode_Value)
	{
		/*
		if (MenuMode_Value == MENUMODE_MENUBAR)
			MenuMode_RestoreSysMenu(hwnd);
		else if (menuMode == MENUMODE_MENUBAR)
			MenuMode_DisableSysMenu(hwnd);
		*/
		MenuMode_Value = menuMode;
	}
}

/*
static void MenuMode_DisableSysMenu(HWND hwnd)
{
	HMENU hmenuParent = GetSysMenuParent(hwnd);
	if (hmenuParent)
	{
		UINT ret = SetClearMenuItemState(hmenuParent, 0, MF_GRAYED | MF_DISABLED, 0);
		if (ret == UINT_MAX)	// hmenuParent is default system menu
		{
			MenuMode_IsDefaultSysMenu = TRUE;
			GetSystemMenu(hwnd, FALSE);
			ret = SetClearMenuItemState(GetSysMenuParent(hwnd), 0, MF_GRAYED | MF_DISABLED, 0);
		}
	}
}


static void MenuMode_RestoreSysMenu(HWND hwnd)
{
	HMENU hmenuParent = GetSysMenuParent(hwnd);
	if (hmenuParent)
	{
		UINT ret = SetClearMenuItemState(hmenuParent, 0, 0, MF_GRAYED | MF_DISABLED);

		// code below may lead to blue screen (windows7)
		//
		//if (MenuMode_IsDefaultSysMenu)
		//	GetSystemMenu(hwnd, TRUE);
	}
}
*/


