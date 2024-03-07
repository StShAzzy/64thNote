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
#ifndef __main_h
#define __main_h

#include "lang.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <objbase.h>
#include "Types.h"

/********* General Defaults **********/

#define AppName  "64th Note"
#define AppVer   " v1.2 beta 3 "__DATE__
#define BinaryName "in_usf.dll"
#define InternalININame "in_usf.ini"
#define ExternalININame "plugin.ini"

#define IniName						"Project64.rdb"
#define NotesIniName				"Project64.rdn"
#define ExtIniName					"Project64.rdx"
#define CheatIniName				"Project64.cht"
#define LangFileName				"Project64.lng"
#define CacheFileName				"Project64.cache"
#define Default_AdvancedBlockLink	TRUE
#define Default_AutoStart			TRUE
#define Default_AutoSleep			TRUE
#define Default_DisableRegCaching	FALSE
#define Default_RdramSize			0x400000
#define Default_UseIni				TRUE
#define Default_AutoZip				TRUE
#define Default_LimitFPS			TRUE
#define Default_AlwaysOnTop			FALSE
#define Default_BasicMode			TRUE
#define Default_RememberCheats		FALSE
#define Default_RomsToRemember		4
#define Default_RomsDirsToRemember	4
#define LinkBlocks
#define TLB_HACK

#define AudioHLEVersion				1	// used to match version in in_usf.ini
										// 0: never used
										// 1: as of beta 15, HLE from Mupen 0.5

/************** Core *****************/
#define CPU_Default					-1
#define CPU_Interpreter				0
#define CPU_Recompiler				1
#define Default_CPU					CPU_Recompiler

/******* Self modifying code *********/
#define ModCode_Default				-1
#define ModCode_None				0
#define ModCode_Cache				1
#define ModCode_ProtectedMemory		2
#define ModCode_ChangeMemory		4
#define ModCode_CheckMemoryCache	6
#define ModCode_CheckMemory2		7 // *** Add in Build 53

/********** Counter Factor ***********/
#define Default_CountPerOp			2

/************ Debugging **************/
#define Default_AutoMap				TRUE
#define Default_ShowUnhandledMemory	FALSE
#define Default_ShowTLBMisses		FALSE
#define Default_ShowDlistCount		FALSE
#define Default_ShowCompileMemory	TRUE
#define Default_ShowPifRamErrors	FALSE
#define Default_SelfModCheck		ModCode_CheckMemory2
//#define Interpreter_StackTest

/********* Global Variables **********/
extern BOOL DisableRegCaching, UseTlb, UseLinking, ShowPifRamErrors, SystemABL, DelaySI, AudioSignal,
	ShowUnhandledMemory, ShowTLBMisses;
extern DWORD CPU_Type, SystemCPU_Type, SelfModCheck, SystemSelfModCheck;
extern HWND hMainWindow;
extern char CurrentSave[256];
extern HMENU hMainMenu;
extern HINSTANCE hInst;

extern int UseRecompiler, AudioHLE, AutoAudioHLE, AutoAudioHLEtriggered, RSPSECTIONS, round_frequency, silencelength,
	softsaturate, DetectSilence, FastSeek, paused, IgnoreTrackLength, title_default_to_file_name,
	DefaultLength, SetLength, CPUPriority, DisplayErrors, EnableBackwards,FadeType,UseGameName;
extern long deflen,deffade;
extern char titlefmt[256], *pristr[];
extern double defrelvol;
extern int priarray[];

/******** Function Prototype *********/
void  __cdecl DisplayError       ( char * Message, ... );
void  LoadSettings        ( void );
int InitalizeApplication ( HINSTANCE hInstance );
void ShutdownApplication ( void );
long LengthFromString(const char * timestring);

#ifdef __cplusplus
}
#endif
#endif
