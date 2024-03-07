/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
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

#include <Windows.h>
#include <stdio.h>
#include "RSP CPU.h"
#include "RSP Recompiler CPU.h"
#include "Rsp Registers.h"
#include "RSP memory.h"
#include "RSP main.h"
#include "main.h"

BOOL ConditionalMove;
BOOL ShowErrors;
DWORD CPUCore = RecompilerCPU;

HANDLE hMutex = NULL;

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/
void CloseRSP (void) {
	RSPFreeMemory();
}

/******************************************************************
  Function: InitiateRSP
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 RSP
			interface needs
  input:    Rsp_Info is passed to this function which is defined
            above.
			CycleCount is the number of cycles between switching
			control between teh RSP and r4300i core.
  output:   none
*******************************************************************/

RSP_COMPILER Compiler;
int _DetectCpuSpecs(void);

void DetectCpuSpecs(void) {
	DWORD Intel_Features = 0;
	DWORD AMD_Features = 0;

	__try {
			// Intel features
			Intel_Features=_DetectCpuSpecs();
			AMD_Features = Intel_Features;

    } __except ( EXCEPTION_EXECUTE_HANDLER) {
		AMD_Features = Intel_Features = 0;
    }

	if (Intel_Features & 0x02000000) {
		Compiler.mmx2 = TRUE;
		Compiler.sse = TRUE;
	}
	if (Intel_Features & 0x00800000) {
		Compiler.mmx = TRUE;
	}
	if (AMD_Features & 0x40000000) {
		Compiler.mmx2 = TRUE;
	}
	if (Intel_Features & 0x00008000) {
		ConditionalMove = TRUE;
	} else {
		ConditionalMove = FALSE;
	}
}

void InitiateRSP ( void ) {
	memset(&Compiler, 0, sizeof(Compiler));

	Compiler.bAlignGPR = TRUE;
	Compiler.bAlignVector = TRUE;
	Compiler.bFlags = TRUE;
	Compiler.bReOrdering = TRUE;
	Compiler.bSections = (RSPSECTIONS?TRUE:FALSE);
	Compiler.bDest = TRUE;
	Compiler.bAccum = TRUE;
	Compiler.bGPRConstants = TRUE;

	DetectCpuSpecs();
	hMutex = CreateMutex(NULL, FALSE, NULL);

	RSPAllocateMemory();
	InitilizeRSPRegisters();
	Build_RSP();
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/
void RSPRomClosed (void) {
	void ClearAllx86Code(void);

	InitilizeRSPRegisters();
	ClearAllx86Code();

}
