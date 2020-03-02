#include <windows.h>
#include <windowsx.h>

#include "TpmNotifyHook.h"


#define OPCODESIZE 6

struct ApiHookInfoStruct
{
	TCHAR* ModuleName;
	char* FuncName;
	void* HookFunc;
	void* TargetAddress;
	BYTE OrgOpCode[OPCODESIZE];
};

typedef struct ApiHookInfoStruct ApiHookInfo;

static void ApiHookInfo_Initialize(ApiHookInfo* pInfo);
static void ApiHookInfo_Uninitialize(ApiHookInfo* pInfo);
static BOOL ApiHookInfo_Hook(ApiHookInfo* pInfo);
static BOOL ApiHookInfo_Unhook(ApiHookInfo* pInfo);






static BOOL CALLBACK HookTrackPopupMenu(HMENU hMenu, UINT fuFlags,
	int x, int y, int nReserved,
	HWND hwnd, CONST RECT* prcRect);
static BOOL CALLBACK HookTrackPopupMenuEx(HMENU hMenu, UINT fuFlags,
	int x, int y, HWND hwnd, LPTPMPARAMS lptpm);


static ApiHookInfo TrackPopupMenuInfo = { TEXT("User32"), "TrackPopupMenu", (void*)HookTrackPopupMenu };
static ApiHookInfo TrackPopupMenuExInfo = { TEXT("User32"), "TrackPopupMenuEx", (void*)HookTrackPopupMenuEx };


static BOOL CALLBACK HookTrackPopupMenu(HMENU hMenu, UINT fuFlags,
	int x, int y, int nReserved,
	HWND hwnd, CONST RECT* prcRect)
{
	BOOL result = FALSE;
	if (ApiHookInfo_Unhook(&TrackPopupMenuInfo))
	{
		result = TrackPopupMenu(hMenu, fuFlags & ~TPM_NONOTIFY,
								x, y, nReserved, hwnd, prcRect);
		ApiHookInfo_Hook(&TrackPopupMenuInfo);
	}
	return result;
}


static BOOL CALLBACK HookTrackPopupMenuEx(HMENU hMenu, UINT fuFlags,
	int x, int y, HWND hwnd, LPTPMPARAMS lptpm)
{
	BOOL result = FALSE;
	if (ApiHookInfo_Unhook(&TrackPopupMenuExInfo))
	{
		result = TrackPopupMenuEx(hMenu, fuFlags & ~TPM_NONOTIFY,
									x, y, hwnd, lptpm);
		ApiHookInfo_Hook(&TrackPopupMenuExInfo);
	}
	return result;
}



void TpmNotifyHook_Initialize()
{
	ApiHookInfo_Initialize(&TrackPopupMenuInfo);
	ApiHookInfo_Hook(&TrackPopupMenuInfo);
	ApiHookInfo_Initialize(&TrackPopupMenuExInfo);
	ApiHookInfo_Hook(&TrackPopupMenuExInfo);
}


void TpmNotifyHook_Uninitialize()
{
	ApiHookInfo_Unhook(&TrackPopupMenuInfo);
	ApiHookInfo_Uninitialize(&TrackPopupMenuInfo);
	ApiHookInfo_Unhook(&TrackPopupMenuExInfo);
	ApiHookInfo_Uninitialize(&TrackPopupMenuExInfo);
}





static void ApiHookInfo_Initialize(ApiHookInfo* pInfo)
{
	if (pInfo && !pInfo->TargetAddress)
	{
		HMODULE hModule = GetModuleHandle(pInfo->ModuleName);
		if (hModule)
		{
			void* pAddress = (void*)GetProcAddress(hModule, pInfo->FuncName);
			if (pAddress)
			{
				DWORD dw;
				HANDLE hProcess = GetCurrentProcess();
				if (ReadProcessMemory(hProcess, pAddress, 
										pInfo->OrgOpCode, OPCODESIZE, &dw)
					&& dw == OPCODESIZE)
				{
					pInfo->TargetAddress = pAddress;
				}
			}
		}
	}
}


static void ApiHookInfo_Uninitialize(ApiHookInfo* pInfo)
{
	if (pInfo)
	{
		pInfo->TargetAddress = 0;
	}
}


static BOOL ApiHookInfo_Hook(ApiHookInfo* pInfo)
{
	BOOL result = FALSE;
	if (pInfo && pInfo->TargetAddress)
	{
		DWORD dw;
		HANDLE hProcess = GetCurrentProcess();

		// opHook‚Í jmp dword ptr [MyFunc]
		BYTE opHook[OPCODESIZE];
		opHook[0] = 0xff;
		opHook[1] = 0x25;
		*(DWORD*)(opHook + 2) = (DWORD)&(pInfo->HookFunc);

		if (WriteProcessMemory(hProcess, pInfo->TargetAddress,
								opHook, OPCODESIZE, &dw)
			&& dw == OPCODESIZE)
		{
			result = TRUE;
			FlushInstructionCache(hProcess, pInfo->TargetAddress, OPCODESIZE);
		}
	}
	return result;
}


static BOOL ApiHookInfo_Unhook(ApiHookInfo* pInfo)
{
	BOOL result = FALSE;
	if (pInfo && pInfo->TargetAddress)
	{
		DWORD dw;
		HANDLE hProcess = GetCurrentProcess();
		if (WriteProcessMemory(hProcess, pInfo->TargetAddress,
			pInfo->OrgOpCode, OPCODESIZE, &dw))
		{
			result = TRUE;
			FlushInstructionCache(hProcess, pInfo->TargetAddress, OPCODESIZE);
		}
	}
	return result;
}


