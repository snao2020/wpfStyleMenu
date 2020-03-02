#ifndef menuhelper_h
#define menuhelper_h


#define mn_selectitem 0x01E5 // wParam:position, lParam:n/a

#define HANDLE_MSGBREAK(hwnd, message, fn) \
   case (message): HANDLE_##message((hwnd), (wParam), (lParam), (fn)); break;


/* void Cls_OnEnterMenuLoop(HWND hwnd, BOOL fTrackPopupMenu) */
#define HANDLE_WM_ENTERMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_ENTERMENULOOP(hwnd, fTrackPopupMenu, fn) \
    (void)(fn)((hwnd), WM_ENTERMENULOOP, (WPARAM)(BOOL)(fTrackPopupMenu), 0L)


/* void Cls_OnExitMenuLoop(HWND hwnd, BOOL fShortcutMenu) */
#define HANDLE_WM_EXITMENULOOP(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_EXITMENULOOP(hwnd, fShortcutMenu, fn) \
    (void)(fn)((hwnd), WM_EXITMENULOOP, (WPARAM)(BOOL)(fShortcutMenu), 0L)



// PopupMenuItem_IsFlagSet/SetFlag/ClearFlag
#define PMI_FIRSTMSGDONE 0x0001


struct PopupMenuItemStruct;
typedef struct PopupMenuItemStruct PopupMenuItem;

HWND PopupMenuItem_GetHWnd(PopupMenuItem* pPMI);
int PopupMenuItem_GetKeyboardSelection(PopupMenuItem* pPMI);
void PopupMenuItem_SetKeyboardSelection(PopupMenuItem* pPMI, int keyboardSelection);
int PopupMenuItem_GetMouseSelection(PopupMenuItem* pPMI);
void PopupMenuItem_SetMouseSelection(PopupMenuItem* pPMI, int mouseSelection);
BOOL PopupMenuItem_IsFlagSet(PopupMenuItem* pPMI, UINT flag);
void PopupMenuItem_SetFlag(PopupMenuItem* pPMI, UINT flag);
void PopupMenuItem_ClearFlag(PopupMenuItem* pPMI, UINT flag);


void PopupMenuList_Clear(void);
void PopupMenuList_Push(HWND hwnd);
void PopupMenuList_Pop(HWND hwnd);
PopupMenuItem* PopupMenuList_GetTop(void);

typedef BOOL PopupMenuList_EnumFunc(PopupMenuItem* pPMI, void* param);
PopupMenuItem* PopupMenuList_Enum(PopupMenuList_EnumFunc* func, void* param);
int PopupMenuList_GetCount(void);
PopupMenuItem* PopupMenuList_FindItem(HMENU hmenu);
PopupMenuItem* PopupMenuList_HitTest(POINT* pPtScreen);


UINT RegPostKeyDown(); // wParam:VKey, lParam:hmenu, return 0
DWORD GetGuiThreadFlags(void); // if failed, return MAXDWORD
HMENU GetSysMenuParent(HWND hwnd);
int GetFirstEnabled(HMENU hmenu);
int GetHilitePos(HMENU hmenu);
UINT SetClearMenuItemState(HMENU hmenu, int pos, UINT set, UINT clear); // return old state (if failed, return UINT_MAX)
int GetMnemonicItem(HMENU hmenu, TCHAR c);

#endif // menuhelper_h
