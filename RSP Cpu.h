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

#include "RSP opcode.h"

#define NORMAL					0
#define DO_DELAY_SLOT			1
#define DO_END_DELAY_SLOT		2
#define DELAY_SLOT				3
#define END_DELAY_SLOT			4
#define LIKELY_DELAY_SLOT		5
#define JUMP	 				6
#define DELAY_SLOT_DONE			7
#define LIKELY_DELAY_SLOT_DONE	8
#define END_BLOCK 				9
#define FINISH_BLOCK			10 // from RSP Recompiler CPU
#define FINISH_SUB_BLOCK		11 // from RSP Recompiler CPU

extern UDWORD EleSpec[32], Indx[32];

extern void * RSP_Opcode[64];
extern void * RSP_RegImm[32];
extern void * RSP_Special[64];
extern void * RSP_Cop0[32];
extern void * RSP_Cop2[32];
extern void * RSP_Vector[64];
extern void * RSP_Lc2[32];
extern void * RSP_Sc2[32];
extern DWORD * PrgCount, RSP_Running;
extern OPCODE RSPOpC;

void SetCPU(DWORD core);
void Build_RSP (void);
