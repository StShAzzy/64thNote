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

#include <windows.h>
#include <stdio.h>
#include "types.h"

#define GeneralPurpose			1
#define ControlProcessor0		2
#define HiddenRegisters		    3
#define Vector1					4
#define Vector2					5

#define IDC_TAB_CONTROL			1000

/*** RSP Registers ***/
MIPSUWORD   RSP_GPR[32], RSP_Flags[4];
MIPSUDWORD  RSP_ACCUM[8];
VECTOR  RSP_Vect[32];

char * GPR_Strings[32] = {
	"R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
	"T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
	"S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
	"T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};

void InitilizeRSPRegisters (void) {
	memset(RSP_GPR,0,sizeof(RSP_GPR));
	memset(RSP_Vect,0,sizeof(RSP_Vect));
}
