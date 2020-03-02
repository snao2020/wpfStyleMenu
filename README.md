## wpfStyleMenu 

# wpf-style menu navigation for win32 applications

## Features

* #### MF_GRAYED or MF_DISABLED MenuItem

    When mouse cursor is on grayed or disabled menuitem, hover effect is not appeared.

    When mouse cursor is on grayed or disabled menuitem, clicking enter key does not close PopupMenu.  

    KeyboardNavigation skips grayed or disabled menuitem.


* #### SystemMenu

    If SystemMenu is opened by Alt-Space key action or Mouse Leftbutton clicked on the window icon,
left-arrow key or right-arrow key does not close systemmenu nor move to menubar mode.


* #### MenuBar

    If no menubar is in a window,
alt key click does nothing(not enter to menubar mode).

    In menubar mode, only the space key can open systemmenu.

    In MenuBar mode and no drop-down menu, mouse cursor selects toplevel item.


* #### Windows bug is fixed

    If systemMenu is opened by mouse rightbutton click on titlebar, MF_DEFAULT is set to SC_MAXIMIZE or SC_RESTORE item.


## Usage

    MenuHook_Hook(0, GetCurrentThreadId());
    ShowWindow(hwnd, nCmdShow);
    while(GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    result = (int)msg.wParam;
    MenuHook_Unhook();


## Files
    MenuHook_Hook(),MenuHook_Unhook() Functions
        MenuHook.h / MenuHook.c
        ContextMenuHandler.h / ContextMenuHandler.c
        SystemMenuHandler.h / SystemMenuHandler.c
        MenuBarHandler.h / MenuBarHandler.c
        MenuHelper.h / MenuHelper.c

    Demo
        MainWindow.c
        Resource.h / Resource.rc
        TpmNotifyHook.h / TpmNotifyHook.c (TrackPopupMenu(Ex) Hook for EditControl)
