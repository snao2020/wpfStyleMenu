#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>

#include "MenuHelper.h"

//-----------------------------------------------------
//
// PopupMenuItem 
//

struct PopupMenuItemStruct
{
	HWND HWnd;
	int KeyboardSelection;
	int MouseSelection;
	UINT Flags;
};


HWND PopupMenuItem_GetHWnd(PopupMenuItem* pPMI)
{
	return pPMI ? pPMI->HWnd : 0;
}


int PopupMenuItem_GetKeyboardSelection(PopupMenuItem* pPMI)
{
	return pPMI ? pPMI->KeyboardSelection : -1;
}


void PopupMenuItem_SetKeyboardSelection(PopupMenuItem* pPMI, int keyboardSelection)
{
	if (pPMI)
		pPMI->KeyboardSelection = keyboardSelection;
}


int PopupMenuItem_GetMouseSelection(PopupMenuItem* pPMI)
{
	return pPMI ? pPMI->MouseSelection : -1;
}


void PopupMenuItem_SetMouseSelection(PopupMenuItem* pPMI, int mouseSelection)
{
	if (pPMI)
		pPMI->MouseSelection = mouseSelection;
}


BOOL PopupMenuItem_IsFlagSet(PopupMenuItem* pPMI, UINT flag)
{
	return pPMI && pPMI->Flags & flag;
}


void PopupMenuItem_SetFlag(PopupMenuItem* pPMI, UINT flag)
{
	if (pPMI)
		pPMI->Flags |= flag;
}


void PopupMenuItem_ClearFlag(PopupMenuItem* pPMI, UINT flag)
{
	if (pPMI)
		pPMI->Flags &= ~flag;
}


static void PopupMenuItem_Initialize(PopupMenuItem* pPMI, HWND hwnd)
{
	if (pPMI)
	{
		pPMI->HWnd = hwnd;
		pPMI->KeyboardSelection = -1;
		pPMI->MouseSelection = -1;
		pPMI->Flags = 0;
	}
}


//-----------------------------------------------
//
// PopupMenuList
//

struct PopupMenuListStruct
{
	PopupMenuItem PMI;
	struct PopupMenuListStruct* Next;
};

typedef struct PopupMenuListStruct PopupMenuList;


static PopupMenuList* PopupMenuList_Root = 0;

static PopupMenuList* PopupMenuList_DoEnum(PopupMenuList_EnumFunc* func, void* param);


void PopupMenuList_Clear()
{
	PopupMenuList* p = PopupMenuList_Root;
	while (p)
	{
		PopupMenuList* pTemp = p;
		p = p->Next;
		free(pTemp);
	}
	PopupMenuList_Root = 0;
}


void PopupMenuList_Push(HWND hwnd)
{
	if (hwnd)
	{
		PopupMenuList* p = malloc(sizeof(PopupMenuList));
		PopupMenuItem_Initialize(&p->PMI, hwnd);
		p->Next = PopupMenuList_Root;
		PopupMenuList_Root = p;
	}
}


static BOOL IsNotHWnd(PopupMenuItem* pPMI, void* hwnd)
{
	return pPMI->HWnd != (HWND)hwnd;
}


void PopupMenuList_Pop(HWND hwnd)
{
	PopupMenuList* p = PopupMenuList_DoEnum(IsNotHWnd, hwnd);
	if (p)
	{
		PopupMenuList* pDel = PopupMenuList_Root;
		PopupMenuList_Root = p->Next;
		p->Next = 0;

		while (pDel)
		{
			PopupMenuList* pTemp = pDel;
			pDel = pTemp->Next;
			free(pTemp);
		}
	}
}


PopupMenuItem* PopupMenuList_GetTop()
{
	return PopupMenuList_Root ? &PopupMenuList_Root->PMI : 0;
}


PopupMenuItem* PopupMenuList_Enum(PopupMenuList_EnumFunc* func, void* param)
{
	PopupMenuList* p = PopupMenuList_DoEnum(func, param);
	return p ? &p->PMI : 0;
}


static BOOL IncCount(PopupMenuItem* pPMI, void* pCounter)
{
	(void)pPMI;
	int* p = (int*)pCounter;
	(*p)++;
	return TRUE;
}


int PopupMenuList_GetCount()
{
	int result = 0;
	PopupMenuList_DoEnum(IncCount, &result);
	return result;
}


static BOOL IsNotHMenu(PopupMenuItem* pPMI, void* hmenu)
{
	return (HMENU)SendMessage(pPMI->HWnd, MN_GETHMENU, 0, 0) != (HMENU)hmenu;
}


PopupMenuItem* PopupMenuList_FindItem(HMENU hmenu)
{
	PopupMenuList* p = PopupMenuList_DoEnum(IsNotHMenu, hmenu);
	return p ? &p->PMI : 0;
}


static BOOL IsNotHit(PopupMenuItem* pPMI, void* pPtScreen)
{
	RECT rc;
	return !GetWindowRect(pPMI->HWnd, &rc) || !PtInRect(&rc, *(POINT*)pPtScreen);
}


PopupMenuItem* PopupMenuList_HitTest(POINT* pPtScreen)
{
	PopupMenuList* p = PopupMenuList_DoEnum(IsNotHit, pPtScreen);
	return p ? &p->PMI : 0;
}


static PopupMenuList* PopupMenuList_DoEnum(PopupMenuList_EnumFunc* func, void* param)
{
	PopupMenuList* result = 0;
	for (PopupMenuList* p = PopupMenuList_Root; !result && p != 0; p = p->Next)
	{
		if (!func(&p->PMI, param))
			result = p;
	}
	return result;
}


//--------------------------------------------------
//
// Utils
// 

UINT RegPostKeyDown()
{
	static UINT message;
	if (message == 0)
		message = RegisterWindowMessage(TEXT("MenuHookPostKeyDownMessage"));
	return message;
}


DWORD GetGuiThreadFlags()
{
	GUITHREADINFO gti;
	gti.cbSize = sizeof(gti);
	if (GetGUIThreadInfo(GetCurrentThreadId(), &gti))
		return gti.flags;
	else
		return MAXDWORD;
}


HMENU GetSysMenuParent(HWND hwnd)
{
	MENUBARINFO mbi;
	mbi.cbSize = sizeof(mbi);
	if (GetMenuBarInfo(hwnd, OBJID_SYSMENU, 0, &mbi))
		return mbi.hMenu;
	else
		return 0;
}


int GetFirstEnabled(HMENU hmenu)
{
	int result = -1;
	if (hmenu)
	{
		int count = GetMenuItemCount(hmenu);
		for (int i = 0; result < 0 && i < count; i++)
		{
			UINT state = GetMenuState(hmenu, i, MF_BYPOSITION);
			if (state != UINT_MAX && !(state & (MF_GRAYED | MF_DISABLED)))
				result = i;
		}
	}
	return result;
}


int GetHilitePos(HMENU hmenu)
{
	int result = -1;
	if (hmenu)
	{
		int count = GetMenuItemCount(hmenu);
		for (int i = 0; result < 0 && i < count; i++)
		{
			UINT state = GetMenuState(hmenu, i, MF_BYPOSITION);
			if (state != UINT_MAX && state & MF_HILITE)
				result = i;
		}
	}
	return result;
}


UINT SetClearMenuItemState(HMENU hMenu, int pos, UINT set, UINT clear)
{
	UINT result;
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	if (GetMenuItemInfo(hMenu, pos, TRUE, &mii))
	{
		result = mii.fState;
		if (set || clear)
		{
			mii.fState = (mii.fState | set) & ~clear;
			if (!SetMenuItemInfo(hMenu, pos, TRUE, &mii))
				result = UINT_MAX;
		}
	}
	else
		result = UINT_MAX;

	return result;
}


int GetMnemonicItem(HMENU hmenu, TCHAR c)
{
	int result = -1;
	if (hmenu && c)
	{
		int count = GetMenuItemCount(hmenu);
		for (int i = 0; result < 0 && i < count; i++)
		{
			int len = GetMenuString(hmenu, i, 0, 0, MF_BYPOSITION);
			TCHAR* p = malloc((len + 1) * sizeof(TCHAR));
			if (GetMenuString(hmenu, i, p, len + 1, MF_BYPOSITION))
			{
				TCHAR* p2 = StrChr(p, TEXT('&'));
				if(p2 && ChrCmpI(*(p2 + 1), c) == 0)
					result = i;
			}
			free(p);
		}
	}
	return result;
}
