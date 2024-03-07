/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */


#include <windows.h>
#include <stdio.h>

#include "main.h"
#include "cpu.h"
#include "plugin.h"

BOOL DisableRegCaching, UseTlb, UseLinking, ShowPifRamErrors, SystemABL, DelaySI, AudioSignal,
	ShowUnhandledMemory, ShowTLBMisses;
DWORD CPU_Type, SystemCPU_Type, SelfModCheck, SystemSelfModCheck;
HWND hMainWindow;
char CurrentSave[256];
HINSTANCE hInst;

// config options (defaults specified here)
char titlefmt[256] = "%game% - %title%";
int UseRecompiler = TRUE;
int AutoAudioHLE = FALSE;
int AutoAudioHLEtriggered = FALSE;
int AudioHLE = FALSE;
int RSPSECTIONS = FALSE;
int round_frequency = FALSE;
int silencelength = 6;
int softsaturate = FALSE;
int DetectSilence = FALSE;
int FastSeek = FALSE;
int paused = FALSE;
int title_default_to_file_name = TRUE;
int DefaultLength = TRUE;
int IgnoreTrackLength = FALSE;
int SetLength = FALSE;
int CPUPriority = 3;
long deflen=180, deffade=10;
double defrelvol=1.0;
int DisplayErrors = FALSE;
int EnableBackwards = TRUE;
int FadeType = 0;
int UseGameName = TRUE;

char *pristr[] = {"Idle","Lowest","Below Normal","Normal","Above Normal","Highest (not recommended)","Time Critical (not recommended)"};
int priarray[] = {THREAD_PRIORITY_IDLE,THREAD_PRIORITY_LOWEST,THREAD_PRIORITY_BELOW_NORMAL,THREAD_PRIORITY_NORMAL,THREAD_PRIORITY_ABOVE_NORMAL,THREAD_PRIORITY_HIGHEST,THREAD_PRIORITY_TIME_CRITICAL};

void __cdecl DisplayError (char * Message, ...) {
	char Msg[1000];
	va_list ap;

	if (!DisplayErrors) { return; }

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	MessageBox(NULL,Msg,"Error",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	SetActiveWindow(hMainWindow);
}

// Will work on any properly formatted string, will fail miserably on anything else
long LengthFromString(const char * timestring) {
	int c=0,decimalused=0,multiplier=1;
	DWORD total=0;
	if (strlen(timestring) == 0) return 0;
	for (c=strlen(timestring)-1; c >= 0; c--) {
		if (timestring[c]=='.' || timestring[c]==',') {
			decimalused=1;
			total*=1000/multiplier;
			multiplier=1000;
		} else if (timestring[c]==':') multiplier=multiplier*6/10;
		else {
			total+=(timestring[c]-'0')*multiplier;
			multiplier*=10;
		}
	}
	if (!decimalused) total*=1000;
	return total;
}

void LoadSettings (void) {
	CPU_Type = SystemCPU_Type = (UseRecompiler?CPU_Recompiler:CPU_Interpreter);
	SelfModCheck = SystemSelfModCheck = ModCode_None;
	RdramSize = SystemRdramSize = 0x400000;
	SystemABL = TRUE;
	DisableRegCaching = FALSE;
	ShowUnhandledMemory = FALSE; //TRUE;
	ShowTLBMisses = TRUE;

 	UseTlb         = TRUE;
 	DelaySI        = FALSE;
 	AudioSignal    = FALSE;
 	SPHack         = FALSE;
 	UseLinking     = TRUE; //-1;

	CountPerOp = 2;
}

int InitalizeApplication ( HINSTANCE hInstance ) {
	HKEY hKeyResults = 0;

	hInst = hInstance;

	if (!Allocate_Memory()) {
		DisplayError("Allocate_Memory failed");
		//DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		return FALSE;
	}

	hPauseMutex = CreateMutex(NULL,FALSE,NULL);

	InitiliazeCPUFlags();
	LoadSettings();
	SetupRegisters(&Registers);
	//QueryPerformanceFrequency(&Frequency);
	InitilizeInitialCompilerVariable();

	SetupPlugins();
	return TRUE;
}

void ShutdownApplication ( void ) {
	CloseCpu();
	ShutdownPlugins();
	Release_Memory();
	CloseHandle(hPauseMutex);
}
