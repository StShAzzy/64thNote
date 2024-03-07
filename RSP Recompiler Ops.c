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
#include "RSP.h"
#include "RSP CPU.h"
#include "RSP Recompiler CPU.h"
#include "RSP Interpreter Ops.h"
#include "RSP Registers.h"
#include "RSP memory.h"
#include "RSP dma.h"
#include "RSP x86.h"
#include "RSP main.h"
#include "recompiler cpu.h"
#include "memory.h"
#include "registers.h"
#include "main.h"
#include "cpu log.h"

MIPSUWORD Recp, RecpResult, SQroot, SQrootResult;
DWORD ESP_RegSave = 0, EBP_RegSave = 0;
DWORD RSPBranchCompare = 0;

/* align option affects: sw, lh, sh */
/* align option affects: lrv, ssv, lsv */

#define RSPCompile_Immediates	/* ADDI, ADDIU, ANDI, ORI, XORI, LUI */
#define RSPCompile_GPRLoads	/* LB, LH, LW, LBU, LHU */
#define RSPCompile_GPRStores	/* SB, SH, SW */
#define RSPCompile_Special		/* SLL, SRL, SRA, SRLV */
							/* XOR, OR, AND, SUB, SUBU, ADDU, ADD, SLT */
#define RSPCompile_Cop0
#define RSPCompile_Cop2

#define RSP_VectorMuls
#define RSP_VectorLoads
#define RSP_VectorMisc

#ifdef RSP_VectorMuls
#	define RSPCompileVmulf	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmacf	/* Rewritten & Verified 12/15/2000 - Jabo */
#	define RSPCompileVmudm	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmudh	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmudn	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmudl	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmadl	
#	define RSPCompileVmadm	/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVmadh	/* Verified 12/15/2000 - Jabo */
#	define RSPCompileVmadn	/* Verified 12/17/2000 - Jabo */
#endif
#ifdef RSP_VectorMisc
#	define RSPCompileVrsqh
#	define RSPCompileVrcph
#	define RSPCompileVsaw		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVabs		/* Verified 12/15/2000 - Jabo */
#	define RSPCompileVmov		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVxor		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVor		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVand		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileVsub		/* Verified 12/17/2000 - Jabo (watch flags) */
#	define RSPCompileVadd		/* Verified 12/17/2000 - Jabo (watch flags) */
#	define RSPCompileVaddc
#	define RSPCompileVsubc
#	define RSPCompileVmrg
#endif
#ifdef RSP_VectorLoads
#	define RSPCompileSqv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileSdv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileSsv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileLrv		/* Rewritten & Verified 12/17/2000 - Jabo */
#	define RSPCompileLqv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileLdv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileLsv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileLlv		/* Verified 12/17/2000 - Jabo */
#	define RSPCompileSlv
#endif

void RSPBranch_AddRef(DWORD Target, DWORD * X86Loc) {
	if (RSPCurrentBlock.ResolveCount >= 150) {
		//CompilerWarning("Out of branch reference space",1);
	} else {
		BYTE * KnownCode = *(RSPJumpTable + (Target >> 2));

		if (KnownCode == NULL) {
			DWORD i = RSPCurrentBlock.ResolveCount;
			RSPCurrentBlock.BranchesToResolve[i].TargetPC = Target;
			RSPCurrentBlock.BranchesToResolve[i].X86JumpLoc = X86Loc;
			RSPCurrentBlock.ResolveCount += 1;
		} else {
			CPU_Message("      (static jump to %X)", KnownCode);
			RSPx86_SetBranch32b((DWORD*)X86Loc, (DWORD*)KnownCode);
		}
	}
}

void RSPCheat_r4300iOpcode ( void * FunctAddress, char * FunctName) {
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPMoveConstToVariable(RSPOpC.Hex, &RSPOpC.Hex, "RSPOpC.Hex" );
	RSPCall_Direct(FunctAddress, FunctName);
}

void RSPCheat_r4300iOpcodeNoMessage( void * FunctAddress, char * FunctName) {
	RSPMoveConstToVariable(RSPOpC.Hex, &RSPOpC.Hex, "RSPOpC.Hex" );
	RSPCall_Direct(FunctAddress, FunctName);
}

void RSPx86_SetBranch8b(void * JumpByte, void * Destination) {
	/* calculate 32-bit relative offset */
	signed int n = (BYTE*)Destination - ((BYTE*)JumpByte + 1);

	/* check limits, no pun intended */
	if (n > 0x80 || n < -0x7F) {
		//CompilerWarning("FATAL: Jump out of 8b range %i (PC = %04X)", n, RSPCompilePC);
	} else
		*(BYTE*)(JumpByte) = (BYTE)n;
}

void RSPx86_SetBranch32b(void * JumpByte, void * Destination) {
	*(DWORD*)(JumpByte) = (DWORD)((BYTE*)Destination - (BYTE*)((DWORD*)JumpByte + 1));
}

/************************* OpCode functions *************************/
void RSPCompile_SPECIAL ( void ) {
	((void (*)()) RSP_Special[ RSPOpC.funct ])();
}

void RSPCompile_REGIMM ( void ) {
	((void (*)()) RSP_RegImm[ RSPOpC.rt ])();
}

void RSPCompile_J ( void ) {
	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		RSPNextInstruction = DO_DELAY_SLOT;
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		RSPJmpLabel32 ( "BranchToJump", 0 );
		RSPBranch_AddRef((RSPOpC.target << 2) & 0xFFC, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_JAL ( void ) {
	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		RSPMoveConstToVariable(RSPCompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		RSPNextInstruction = DO_DELAY_SLOT;
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		RSPJmpLabel32 ( "BranchToJump", 0 );
		RSPBranch_AddRef((RSPOpC.target << 2) & 0xFFC, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_BEQ ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		if (RSPOpC.rt == 0) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		} else if (RSPOpC.rs == 0) {			
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
		} else {
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
			RSPCompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		}
		RSPSetzVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			RSPJmpLabel32 ( "BranchToJump", 0 );
			RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			if (RSPOpC.rt == 0) {
				RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			} else if (RSPOpC.rs == 0) {			
				RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
			} else {
				RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
				RSPCompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			}
			RSPJeLabel32("BranchEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchEqual", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BEQ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_BNE ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}

		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		if (RSPOpC.rt == 0) {			
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		} else if (RSPOpC.rs == 0) {			
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
		} else {
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
			RSPCompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		}
		RSPSetnzVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {			
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}

		if (FALSE == bDelayAffect) {
			if (RSPOpC.rt == 0) {			
				RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			} else if (RSPOpC.rs == 0) {			
				RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
			} else {
				RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
				RSPCompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			}
			RSPJneLabel32("BranchNotEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchNotEqual", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BNE error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_BLEZ ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetleVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			RSPJmpLabel32 ( "BranchToJump", 0 );
			RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			RSPJleLabel32("BranchLessEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchLessEqual", 0);
		}

		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BLEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_BGTZ ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetgVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			RSPJgLabel32("BranchGreater", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchGreater", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BGTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_ADDI ( void ) {
	int Immediate = (short)RSPOpC.immediate;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_ADDI,"RSP_Opcode_ADDI"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPAddConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		RSPMoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			RSPAddConstToX86Reg(x86_EAX, Immediate);
		}
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_ADDIU ( void ) {
	int Immediate = (short)RSPOpC.immediate;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_ADDIU,"RSP_Opcode_ADDIU"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPAddConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		RSPMoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddConstToX86Reg(x86_EAX, Immediate);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_SLTI ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SLTI,"RSP_Opcode_SLTI");
}

void RSPCompile_SLTIU ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SLTIU,"RSP_Opcode_SLTIU");
}

void RSPCompile_ANDI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_ANDI,"RSP_Opcode_ANDI"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPAndConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAndConstToX86Reg(x86_EAX, Immediate);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_ORI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_ORI,"RSP_Opcode_ORI"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPOrConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		RSPMoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			RSPOrConstToX86Reg(Immediate, x86_EAX);
		}
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_XORI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_XORI,"RSP_Opcode_XORI"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPXorConstToVariable(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), Immediate);
	} else if (RSPOpC.rs == 0) {
		RSPMoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			RSPXorConstToX86Reg(x86_EAX, Immediate);
		}
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_LUI ( void ) {
	int n = (short)RSPOpC.offset << 16;

	#ifndef RSPCompile_Immediates
	RSPCheat_r4300iOpcode(RSP_Opcode_LUI,"RSP_Opcode_LUI"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rt == 0) return;
	RSPMoveConstToVariable(n, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
}

void RSPCompile_COP0 (void) {
	((void (*)()) RSP_Cop0[ RSPOpC.rs ])();
}

void RSPCompile_COP2 (void) {
	((void (*)()) RSP_Cop2[ RSPOpC.rs ])();
}

void RSPCompile_LB ( void ) {
	int Offset = (short)RSPOpC.offset;

	#ifndef RSPCompile_GPRLoads
	RSPCheat_r4300iOpcode(RSP_Opcode_LB,"RSP_Opcode_LB"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);
	RSPXorConstToX86Reg(x86_EBX, 3);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);

	RSPMoveSxN64MemToX86regByte(x86_EAX, x86_EBX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void RSPCompile_LH ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef RSPCompile_GPRLoads
	RSPCheat_r4300iOpcode(RSP_Opcode_LH,"RSP_Opcode_LH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Addr & 1) != 0) {
			CompilerWarning("Unaligned LH at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LH,"RSP_Opcode_LH");
			return;
		} else {
			char Address[32];			
			sprintf(Address, "Dmem + %Xh", Addr);
			RSPMoveSxVariableToX86regHalf(DMEM + Addr, Address, x86_EAX);
			RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			return;
		}
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);

	if (Compiler.bAlignGPR == FALSE) {
		RSPTestConstToX86Reg(1, x86_EBX);
		RSPJneLabel32("Unaligned", 0);
		Jump[0] = RSPRecompPos - 4;

		CompilerToggleBuffer();

		CPU_Message("   Unaligned:");
		RSPx86_SetBranch32b(Jump[0], RSPRecompPos);

		RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LH,"RSP_Opcode_LH");
		RSPJmpLabel32("Done", 0);
		Jump[1] = RSPRecompPos - 4;

		CompilerToggleBuffer();
	}

	RSPXorConstToX86Reg(x86_EBX, 2);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);

	RSPMoveSxN64MemToX86regHalf(x86_EAX, x86_EBX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	if (Compiler.bAlignGPR == FALSE) {
		CPU_Message("   Done:");
		RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
	}
}

void RSPCompile_LW ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef RSPCompile_GPRLoads
	RSPCheat_r4300iOpcode(RSP_Opcode_LW,"RSP_Opcode_LW"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned LW at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LW,"RSP_Opcode_LW");
			return;
		} else {
			char Address[32];			
			sprintf(Address, "Dmem + %Xh", Addr);
			RSPMoveVariableToX86reg(DMEM + Addr, Address, x86_EAX);
			RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			return;
		}
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);
	
	RSPTestConstToX86Reg(3, x86_EBX);
	RSPJneLabel32("UnAligned", 0);
	Jump[0] = RSPRecompPos - 4;

	CompilerToggleBuffer();

	RSPx86_SetBranch32b(Jump[0], RSPRecompPos);
	CPU_Message("   Unaligned:");

	RSPLeaSourceAndOffset(x86_ECX , x86_EBX, 2);
	RSPLeaSourceAndOffset(x86_EDX , x86_EBX, 3);
	RSPMoveX86RegToX86Reg(x86_EBX, x86_EAX);
	RSPAddConstToX86Reg(x86_EBX, 1);
	
	RSPXorConstToX86Reg(x86_EAX, 3);
	RSPXorConstToX86Reg(x86_EBX, 3);
	RSPXorConstToX86Reg(x86_ECX, 3);
	RSPXorConstToX86Reg(x86_EDX, 3);
	RSPMoveN64MemToX86regByte(x86_EAX, x86_EAX);
	RSPMoveN64MemToX86regByte(x86_EBX, x86_EBX);
	RSPMoveN64MemToX86regByte(x86_ECX, x86_ECX);
	RSPMoveN64MemToX86regByte(x86_EDX, x86_EDX);
	RSPMoveX86regByteToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UB[3], GPR_Name(RSPOpC.rt));
	RSPMoveX86regByteToVariable(x86_EBX, &RSP_GPR[RSPOpC.rt].UB[2], GPR_Name(RSPOpC.rt));
	RSPMoveX86regByteToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UB[1], GPR_Name(RSPOpC.rt));
	RSPMoveX86regByteToVariable(x86_EDX, &RSP_GPR[RSPOpC.rt].UB[0], GPR_Name(RSPOpC.rt));

	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPMoveN64MemToX86reg(x86_EAX, x86_EBX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	CPU_Message("   Done:");
	RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
}

void RSPCompile_LBU ( void ) {
	int Offset = (short)RSPOpC.offset;

	#ifndef RSPCompile_GPRLoads
	RSPCheat_r4300iOpcode(RSP_Opcode_LBU,"RSP_Opcode_LBU"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	RSPXorX86RegToX86Reg(x86_EAX, x86_EAX);

	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);
	RSPXorConstToX86Reg(x86_EBX, 3);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);

	RSPMoveN64MemToX86regByte(x86_EAX, x86_EBX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void RSPCompile_LHU ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef RSPCompile_GPRLoads
	RSPCheat_r4300iOpcode(RSP_Opcode_LHU,"RSP_Opcode_LHU"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Addr & 1) != 0) {
			CompilerWarning("Unaligned LHU at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LHU,"RSP_Opcode_LHU");
			return;
		} else {
			char Address[32];			
			sprintf(Address, "Dmem + %Xh", Addr);
			RSPMoveZxVariableToX86regHalf(DMEM + Addr, Address, x86_ECX);
			RSPMoveX86regToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			return;
		}
	}

	/*
	 * should really just do it by bytes but whatever for now
	 */
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) {
		RSPAddConstToX86Reg(x86_EBX, Offset);
	}
	RSPTestConstToX86Reg(1, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	RSPx86_SetBranch32b(Jump[0], RSPRecompPos);
	RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LHU,"RSP_Opcode_LHU");
	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	RSPXorConstToX86Reg(x86_EBX, 2);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPMoveZxN64MemToX86regHalf(x86_EAX, x86_EBX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	CPU_Message("   Done:");
	RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
}

void RSPCompile_SB ( void ) {
	int Offset = (short)RSPOpC.offset;

	#ifndef RSPCompile_GPRStores
	RSPCheat_r4300iOpcode(RSP_Opcode_SB,"RSP_Opcode_SB"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	RSPMoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);

	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);
	RSPXorConstToX86Reg(x86_EBX, 3);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);

	RSPMoveX86regByteToN64Mem(x86_EAX, x86_EBX);
}

void RSPCompile_SH ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef RSPCompile_GPRStores
	RSPCheat_r4300iOpcode(RSP_Opcode_SH,"RSP_Opcode_SH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Offset & 1) != 0) {
			CompilerWarning("Unaligned SH at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SH,"RSP_Opcode_SH");
			return;
		} else {
			char Address[32];			
			sprintf(Address, "Dmem + %Xh", Addr);
			RSPMoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			RSPMoveX86regHalfToVariable(x86_EAX, DMEM + Addr, Address);
			return;
		}
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);

	if (Compiler.bAlignGPR == FALSE) {
		RSPTestConstToX86Reg(1, x86_EBX);
		RSPJneLabel32("Unaligned", 0);
		Jump[0] = RSPRecompPos - 4;

		CompilerToggleBuffer();

		CPU_Message("   Unaligned:");
		RSPx86_SetBranch32b(Jump[0], RSPRecompPos);

		RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SH,"RSP_Opcode_SH");
		RSPJmpLabel32("Done", 0);
		Jump[1] = RSPRecompPos - 4;

		CompilerToggleBuffer();
	}

	RSPXorConstToX86Reg(x86_EBX, 2);
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);

	RSPMoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
	RSPMoveX86regHalfToN64Mem(x86_EAX, x86_EBX);

	if (Compiler.bAlignGPR == FALSE) {
		CPU_Message("   Done:");
		RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
	}
}

void RSPCompile_SW ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef RSPCompile_GPRStores
	RSPCheat_r4300iOpcode(RSP_Opcode_SW,"RSP_Opcode_SW"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned SW at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SW,"RSP_Opcode_SW");
			return;
		} else {
			char Address[32];			
			sprintf(Address, "Dmem + %Xh", Addr);
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			RSPMoveX86regToVariable(x86_EAX, DMEM + Addr, Address);			
			return;
		}
	} 

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) RSPAddConstToX86Reg(x86_EBX, Offset);
	
	if (Compiler.bAlignGPR == FALSE) {
		RSPTestConstToX86Reg(3, x86_EBX);
		RSPJneLabel32("Unaligned", 0);
		Jump[0] = RSPRecompPos - 4;

		CompilerToggleBuffer();

		CPU_Message("   Unaligned:");
		RSPx86_SetBranch32b(Jump[0], RSPRecompPos);

		RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SW,"RSP_Opcode_SW");
		RSPJmpLabel32("Done", 0);
		Jump[1] = RSPRecompPos - 4;

		CompilerToggleBuffer();
	}

	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
	RSPMoveX86regToN64Mem(x86_EAX, x86_EBX);

	if (Compiler.bAlignGPR == FALSE) {
		CPU_Message("   Done:");
		RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
	}
}

void RSPCompile_LC2 (void) {
	((void (*)()) RSP_Lc2 [ RSPOpC.rd ])();
}

void RSPCompile_SC2 (void) {
	((void (*)()) RSP_Sc2 [ RSPOpC.rd ])();
}
/********************** R4300i OpCodes: Special **********************/

void RSPCompile_Special_SLL ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SLL,"RSP_Special_SLL"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		RSPShiftLeftSignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPShiftLeftSignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SRL ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SRL,"RSP_Special_SRL"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		RSPShiftRightUnsignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPShiftRightUnsignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SRA ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SRA,"RSP_Special_SRA"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		RSPShiftRightSignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPShiftRightSignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SLLV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Special_SLLV,"RSP_Special_SLLV");
}

void RSPCompile_Special_SRLV ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SRLV,"RSP_Special_SRLV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (RSPOpC.rd == 0) return;

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_ECX);
	RSPAndConstToX86Reg(x86_ECX, 0x1F);
	RSPShiftRightUnsign(x86_EAX);
	RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));	
}

void RSPCompile_Special_SRAV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Special_SRAV,"RSP_Special_SRAV");
}

void RSPCompile_Special_JR (void) {
	BYTE * Jump;

	if ( RSPNextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		/* transfer destination to location pointed to by PrgCount */
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs),x86_EAX);
		RSPAndConstToX86Reg(x86_EAX,0xFFC);
		RSPMoveX86regToVariable(x86_EAX,PrgCount,"RSP PC");
		RSPNextInstruction = DO_DELAY_SLOT;
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		RSPMoveVariableToX86reg(PrgCount,"RSP PC", x86_EAX);
		RSPAddVariableToX86reg(x86_EAX, &RSPJumpTable, "RSPJumpTable");
		RSPMoveX86regPointerToX86reg(x86_EAX, x86_EAX);

		RSPTestX86RegToX86Reg(x86_EAX, x86_EAX);
		RSPJeLabel8("Null", 0);
		Jump = RSPRecompPos - 1;
		RSPJumpX86Reg(x86_EAX);

		RSPx86_SetBranch8b(Jump, RSPRecompPos);
		CPU_Message(" Null:");
		RSPRet();
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("WTF\n\nJR\nNextInstruction = %X", NextInstruction);
	}
}

void RSPCompile_Special_JALR ( void ) {
	BYTE * Jump;
	DWORD Const = (RSPCompilePC + 8) & 0xFFC;

	if (RSPNextInstruction == NORMAL) {		
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		RSPMoveConstToVariable(Const, &RSP_GPR[RSPOpC.rd].W,GPR_Name(RSPOpC.rd));
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs),x86_EAX);
		RSPAndConstToX86Reg(x86_EAX,0xFFC);
		RSPMoveX86regToVariable(x86_EAX,PrgCount,"RSP PC");
		RSPNextInstruction = DO_DELAY_SLOT;
	} else if (RSPNextInstruction == DELAY_SLOT_DONE) {
		RSPMoveVariableToX86reg(PrgCount,"RSP PC", x86_EAX);
		RSPAddVariableToX86reg(x86_EAX, &RSPJumpTable, "RSPJumpTable");
		RSPMoveX86regPointerToX86reg(x86_EAX, x86_EAX);

		RSPTestX86RegToX86Reg(x86_EAX, x86_EAX);
		RSPJeLabel8("Null", 0);
		Jump = RSPRecompPos - 1;
		RSPJumpX86Reg(x86_EAX);

		RSPx86_SetBranch8b(Jump, RSPRecompPos);
		CPU_Message(" Null:");
		RSPRet();
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("WTF\n\nJALR\nNextInstruction = %X", NextInstruction);
	}
}

void RSPCompile_Special_BREAK ( void ) {
	RSPCheat_r4300iOpcode(RSP_Special_BREAK,"RSP_Special_BREAK");
	if (RSPNextInstruction != NORMAL) {
		DisplayError("Compile_Special_BREAK: problem");
	}
	RSPMoveConstToVariable(RSPCompilePC + 4,PrgCount,"RSP PC");
	RSPRet();
	RSPNextInstruction = FINISH_BLOCK;
}

void RSPCompile_Special_ADD ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_ADD,"RSP_Special_ADD"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPAddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rd == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddX86RegToX86Reg(x86_EAX, x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddVariableToX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_ADDU ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_ADDU,"RSP_Special_ADDU"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPAddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rd == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddX86RegToX86Reg(x86_EAX, x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAddVariableToX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SUB ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SUB,"RSP_Special_SUB"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPSubX86regFromVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, "RSP_GPR[RSPOpC.rd].W");
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPSubVariableFromX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SUBU ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SUBU,"RSP_Special_SUBU"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPSubX86regFromVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPSubVariableFromX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_AND ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_AND,"RSP_Special_AND"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPAndX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAndX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPAndVariableToX86Reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_OR ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_OR,"RSP_Special_OR"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPOrX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPOrX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPOrVariableToX86Reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_XOR ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_XOR,"RSP_Special_XOR"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPXorX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPXorX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == RSPOpC.rt) {
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		RSPXorVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_NOR ( void ) {
	RSPCheat_r4300iOpcode(RSP_Special_NOR,"RSP_Special_NOR");
}

void RSPCompile_Special_SLT ( void ) {
	#ifndef RSPCompile_Special
	RSPCheat_r4300iOpcode(RSP_Special_SLT,"RSP_Special_SLT"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (RSPOpC.rt == 0) { return; }

	if (RSPOpC.rt == RSPOpC.rs) {
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
	} else {
		RSPXorX86RegToX86Reg(x86_EBX, x86_EBX);
		if (RSPOpC.rs == 0) {
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			RSPCompConstToX86reg(x86_EAX, 0);
			RSPSetg(x86_EBX);
		} else if (RSPOpC.rt == 0) {
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
			RSPCompConstToX86reg(x86_EAX, 0);
			RSPSetl(x86_EBX);
		} else {
			RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
			RSPCompX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			RSPSetl(x86_EBX);
		}
		RSPMoveX86regToVariable(x86_EBX, &RSP_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
	}
}

void RSPCompile_Special_SLTU ( void ) {
	RSPCheat_r4300iOpcode(RSP_Special_SLTU,"RSP_Special_SLTU");
}

/********************** R4300i OpCodes: RegImm **********************/
void RSPCompile_RegImm_BLTZ ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetlVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			RSPJlLabel32("BranchLess", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchLess", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		//CompilerWarning("BLTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nPC = %X\nEmulation will now stop", NextInstruction, RSPCompilePC);
	}
}

void RSPCompile_RegImm_BGEZ ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetgeVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			RSPJmpLabel32 ( "BranchToJump", 0 );
			RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			RSPJgeLabel32("BranchGreaterEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchGreaterEqual", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BGEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_RegImm_BLTZAL ( void ) {
	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		RSPMoveConstToVariable(RSPCompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;			
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetlVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}

		/* take a look at the branch compare variable */
		RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
		RSPJeLabel32("BranchLessEqual", 0);
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BLTZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

void RSPCompile_RegImm_BGEZAL ( void ) {
	static BOOL bDelayAffect;

	if ( RSPNextInstruction == NORMAL ) {
		CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
		RSPMoveConstToVariable(RSPCompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		if (RSPOpC.rs == 0) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(RSPCompilePC);
		if (FALSE == bDelayAffect) {
			RSPNextInstruction = DO_DELAY_SLOT;
			return;
		}
		RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		RSPSetgeVariable(&RSPBranchCompare, "RSPBranchCompare");
		RSPNextInstruction = DO_DELAY_SLOT;	
	} else if ( RSPNextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (RSPCompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			RSPJmpLabel32 ( "BranchToJump", 0 );
			RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
			RSPNextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			RSPCompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			RSPJgeLabel32("BranchGreaterEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			RSPCompConstToVariable(TRUE, &RSPBranchCompare, "RSPBranchCompare");
			RSPJeLabel32("BranchGreaterEqual", 0);
		}
		RSPBranch_AddRef(Target, (DWORD*)(RSPRecompPos - 4));
		RSPNextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("BGEZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
	}
}

/************************** Cop0 functions *************************/

void RSPCompile_Cop0_MF ( void ) {
	#ifndef RSPCompile_Cop0
	RSPCheat_r4300iOpcode(RSP_Cop0_MF,"RSP_Cop0_MF"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	switch (RSPOpC.rd) {
	case 4: 
		RSPMoveVariableToX86reg(&SP_STATUS_REG, "SP_STATUS_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 5: 
		RSPMoveVariableToX86reg(&SP_DMA_FULL_REG, "SP_DMA_FULL_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 6: 
		RSPMoveVariableToX86reg(&SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 7: 
		RSPMoveConstToVariable(0, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		//RSPCheat_r4300iOpcode(RSP_Cop0_MF,"RSP_Cop0_MF");
		break;
	case 8:
		RSPMoveVariableToX86reg(&DPC_START_REG, "DPC_START_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 9:
		RSPMoveVariableToX86reg(&DPC_END_REG, "DPC_END_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 10:
		RSPMoveVariableToX86reg(&DPC_CURRENT_REG, "DPC_CURRENT_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 11: 
		RSPMoveVariableToX86reg(&DPC_STATUS_REG, "DPC_STATUS_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 12: 
		RSPMoveVariableToX86reg(&DPC_CLOCK_REG, "DPC_CLOCK_REG", x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;

	//default:
		//CompilerWarning("have not implemented RSP MF CP0 reg %s (%d)",COP0_Name(RSPOpC.rd),RSPOpC.rd);
	}
}

void RSPCompile_Cop0_MT ( void ) {
#ifndef RSPCompile_Cop0
	RSPCheat_r4300iOpcode(RSP_Cop0_MT,"RSP_Cop0_MT");
#else
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	switch (RSPOpC.rd) {
	case 0:
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG");
		break;
	case 1:
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG");
		break;
	case 2: 
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &SP_RD_LEN_REG,"SP_RD_LEN_REG");
		RSPCall_Direct(RSP_SP_DMA_READ, "RSP_SP_DMA_READ");
		break;
	case 3: 
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &SP_WR_LEN_REG,"SP_WR_LEN_REG");
		RSPCall_Direct(RSP_SP_DMA_WRITE, "RSP_SP_DMA_WRITE");
		break;
	case 7: 
		RSPMoveConstToVariable(0, &SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
		break;
	case 8: 
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &DPC_START_REG,"DPC_START_REG");
		RSPMoveX86regToVariable(x86_EAX, &DPC_CURRENT_REG,"DPC_CURRENT_REG");
		break;
	case 9: 
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &DPC_END_REG,"DPC_END_REG");

		break;
	case 10:
		RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		RSPMoveX86regToVariable(x86_EAX, &DPC_CURRENT_REG,"DPC_CURRENT_REG");
		break;

	default:
		RSPCheat_r4300iOpcode(RSP_Cop0_MT,"RSP_Cop0_MT");
		break;
	}
#endif
	if (RSPOpC.rd == 2) {
		BYTE * Jump;

		RSPTestConstToVariable(0x1000, &SP_MEM_ADDR_REG, "&SP_MEM_ADDR_REG");
		RSPJeLabel8("DontExit", 0);
		Jump = RSPRecompPos - 1;

		RSPMoveConstToVariable(RSPCompilePC + 4,PrgCount,"RSP PC");
		RSPRet();

		CPU_Message("DontExit:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	}
}
/************************** Cop2 functions *************************/

void RSPCompile_Cop2_MF ( void ) {
	char Reg[256];
	int element = (RSPOpC.sa >> 1);

	int element1 = 15 - element;
	int element2 = 15 - ((element + 1) % 16);
	
	#ifndef RSPCompile_Cop2
	RSPCheat_r4300iOpcode(RSP_Cop2_MF,"RSP_Cop2_MF"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (element2 != (element1 - 1)) {
		RSPXorX86RegToX86Reg(x86_EAX, x86_EAX);
		RSPXorX86RegToX86Reg(x86_EBX, x86_EBX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element1);
		RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rd].B[element1], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element2);
		RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rd].B[element2], Reg, x86_EBX);

		RSPShiftLeftSignImmed(x86_EAX, 8);
		RSPOrX86RegToX86Reg(x86_EAX, x86_EBX);
		RSPCwde();

		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
	} else {
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element2);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].B[element2], Reg, x86_EAX);

		RSPMoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
	}
}

void RSPCompile_Cop2_CF ( void ) {
	RSPCheat_r4300iOpcode(RSP_Cop2_CF,"RSP_Cop2_CF");
}

void RSPCompile_Cop2_MT ( void ) {
	char Reg[256];
	int element = 15 - (RSPOpC.sa >> 1);

	#ifndef RSPCompile_Cop2
	RSPCheat_r4300iOpcode(RSP_Cop2_MT,"RSP_Cop2_MT"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	if (element == 0) {
		sprintf(Reg, "RSP_GPR[%i].B[1]", RSPOpC.rt);
		RSPMoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].B[1], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element);
		RSPMoveX86regByteToVariable(x86_EAX, &RSP_Vect[RSPOpC.rd].B[element], Reg);
	} else {
		sprintf(Reg, "RSP_GPR[%i].B[0]", RSPOpC.rt);
		RSPMoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].B[0], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element - 1);
		RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.rd].B[element - 1], Reg);
	}
}

void RSPCompile_Cop2_CT ( void ) {
	RSPCheat_r4300iOpcode(RSP_Cop2_CT,"RSP_Cop2_CT");
}

void RSPCompile_COP2_VECTOR (void) {
	((void (*)()) RSP_Vector[ RSPOpC.funct ])();
}

/************************** Vect functions **************************/

UDWORD MMX_Scratch;

void RSP_Element2Mmx(int MmxReg) {
	char Reg[256];

	DWORD Rs = RSPOpC.rs & 0x0f;
	DWORD el;

	switch (Rs) {
	case 0: case 1:
	case 2: case 3:
	case 4:	case 5:
	case 6:	case 7:
		//CompilerWarning("Unimplemented RSP_Element2Mmx");
		break;

	default:
		/*
		 * Noticed the exclusive-or of seven to take into account
		 * the pseudo-swapping we have in the vector registers
		 */

		el = (RSPOpC.rs & 0x07) ^ 7;

		if (IsMmx2Enabled == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, el);
			RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[el], Reg, x86_ECX);
			RSPMoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
			RSPMoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[1], "MMX_Scratch.HW[1]");
			RSPMoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[2], "MMX_Scratch.HW[2]");
			RSPMoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[3], "MMX_Scratch.HW[3]");
			RSPMmxMoveQwordVariableToReg(MmxReg, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
		} else {
			unsigned long Qword;
			
			Qword = (el >> 2) & 0x1;
			el &= 0x3;

			sprintf(Reg, "RSP_Vect[%i].DW[%i]", RSPOpC.rt, Qword);
			RSPMmxShuffleMemoryToReg(MmxReg, 
				&RSP_Vect[RSPOpC.rt].DW[Qword], Reg, _MMX_SHUFFLE(el, el, el, el));
		}
		break;
	}
}

void RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2) {
	char Reg[256];
	DWORD Rs = RSPOpC.rs & 0x0f;

	/*
	 * Ok, this is tricky, hopefully this clears it up:
	 *
	 * $vd[0] = $vd[0] + $vt[2] 
	 * because of swapped registers becomes:
	 * $vd[7] = $vd[7] + $vt[5]
	 *
	 * we must perform this swap correctly, this involves the 3-bit
	 * xclusive or, 2-bits of which are done within a dword boundary, 
	 * the last bit, is ignored because we are loading the source linearly,
	 * so the xclusive or has transparently happened on that side
	 *
	 */

	switch (Rs) {
	case 0:
	case 1:
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
		break;
	case 2:
		/* [0q]    | 0 | 0 | 2 | 2 | 4 | 4 | 6 | 6 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xF5);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xF5);
		break;
	case 3:
		/* [1q]    | 1 | 1 | 3 | 3 | 5 | 5 | 7 | 7 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xA0);
		//RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x0A);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xA0);
		//RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x0A);
		break;
	case 4:
		/* [0h]    | 0 | 0 | 0 | 0 | 4 | 4 | 4 | 4 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xFF);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xFF);
		break;
	case 5:
		/* [1h]    | 1 | 1 | 1 | 1 | 5 | 5 | 5 | 5 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xAA);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xAA);
		break;
	case 6:
		/* [2h]    | 2 | 2 | 2 | 2 | 6 | 6 | 6 | 6 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x55);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x55);
		break;
	case 7:
		/* [3h]    | 3 | 3 | 3 | 3 | 7 | 7 | 7 | 7 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x00);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		RSPMmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x00);
		break;

	default:
		//CompilerWarning("Unimplemented RSP_MultiElement2Mmx [?]");
		break;
	}
}

BOOL RSPCompile_Vector_VMULF_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	/* NOTE: Problem here is the lack of +/- 0x8000 rounding */
	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		RSPMmxPmulhwRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		RSPMmxPmulhwRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPmulhwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM1, x86_MM3);
	}
	RSPMmxPsllwImmed(x86_MM0, 1);
	RSPMmxPsllwImmed(x86_MM1, 1);

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VMULF ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);

	#ifndef RSPCompileVmulf
	RSPCheat_r4300iOpcode(RSP_Vector_VMULF,"RSP_Vector_VMULF"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VMULF_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		RSPMoveConstToX86reg(0x7fff0000, x86_ESI);
	}
	if (bWriteToAccum == TRUE) {
		RSPXorX86RegToX86Reg(x86_EDI, x86_EDI);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);

		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (RSPOpC.rt == RSPOpC.rd && !bOptimize) {
			RSPimulX86reg(x86_EAX);
		} else {
			if (bOptimize == FALSE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			}
			RSPimulX86reg(x86_EBX);
		}

		RSPShiftLeftSignImmed(x86_EAX, 1);
		RSPAddConstToX86Reg(x86_EAX, 0x8000);

		if (bWriteToAccum == TRUE) {
			RSPMoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
			/* calculate sign extension into edx */
			RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
			RSPShiftRightSignImmed(x86_EDX, 31);
		}

		RSPCompConstToX86reg(x86_EAX, 0x80008000);

		if (bWriteToAccum == TRUE) {
			RSPCondMoveEqual(x86_EDX, x86_EDI);
			RSPMoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[3], "RSP_ACCUM[el].HW[3]");
		}
		if (bWriteToDest == TRUE) {
			RSPCondMoveEqual(x86_EAX, x86_ESI);
			RSPShiftRightUnsignImmed(x86_EAX, 16);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], "RSP_Vect[RSPOpC.sa].HW[el]");
		}
	}
}

void RSPCompile_Vector_VMULU ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VMULU,"RSP_Vector_VMULU");
}

BOOL RSPCompile_Vector_VMUDL_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if (IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
		
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM3);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	
	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VMUDL ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmudl
	RSPCheat_r4300iOpcode(RSP_Vector_VMUDL,"RSP_Vector_VMUDL"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VMUDL_MMX())
			return;
	}
	
	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToAccum == TRUE)
		RSPXorX86RegToX86Reg(x86_EDI, x86_EDI);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPimulX86reg(x86_EBX);

		if (bWriteToAccum == TRUE) {
			sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
			RSPMoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
			sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
			RSPMoveX86regToVariable(x86_EDI, &RSP_ACCUM[el].UW[1], Reg);
		}

		if (bWriteToDest == TRUE) {
			RSPShiftRightUnsignImmed(x86_EAX, 16);
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
}

BOOL RSPCompile_Vector_VMUDM_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if (IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM4, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM5, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);

		/* Copy the signed portion */
		RSPMmxMoveRegToReg(x86_MM2, x86_MM0);
		RSPMmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM5);

		/* low((a >> 15) * b) */
		RSPMmxPsrawImmed(x86_MM2, 15);
		RSPMmxPsrawImmed(x86_MM3, 15);
		RSPMmxPmullwRegToReg(x86_MM2, x86_MM4);
		RSPMmxPmullwRegToReg(x86_MM3, x86_MM5);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM4);

		/* Copy the signed portion */
		RSPMmxMoveRegToReg(x86_MM2, x86_MM0);
		RSPMmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM4);

		/* low((a >> 15) * b) */
		RSPMmxPsrawImmed(x86_MM2, 15);
		RSPMmxPsrawImmed(x86_MM3, 15);
		RSPMmxPmullwRegToReg(x86_MM2, x86_MM4);
		RSPMmxPmullwRegToReg(x86_MM3, x86_MM4);
	} else {
		RSP_MultiElement2Mmx(x86_MM4, x86_MM5);

		/* Copy the signed portion */
		RSPMmxMoveRegToReg(x86_MM2, x86_MM0);
		RSPMmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		RSPMmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		RSPMmxPmulhuwRegToReg(x86_MM1, x86_MM5);

		/* low((a >> 15) * b) */
		RSPMmxPsrawImmed(x86_MM2, 15);
		RSPMmxPsrawImmed(x86_MM3, 15);
		RSPMmxPmullwRegToReg(x86_MM2, x86_MM4);
		RSPMmxPmullwRegToReg(x86_MM3, x86_MM5);
	}

	/* Add them up */
	RSPMmxPaddwRegToReg(x86_MM0, x86_MM2);
	RSPMmxPaddwRegToReg(x86_MM1, x86_MM3);

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VMUDM ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmudm
	RSPCheat_r4300iOpcode(RSP_Vector_VMUDM,"RSP_Vector_VMUDM"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VMUDM_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	RSPPush(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	if (bWriteToDest) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
	} else if (!bOptimize) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
		
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

		if (bOptimize == FALSE) {
			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			} else {
				RSPMoveZxX86RegPtrDispToX86RegHalf(x86_ECX, del * 2, x86_EBX);
			}
		}

		RSPimulX86reg(x86_EBX);

		if (bWriteToAccum == FALSE && bWriteToDest == TRUE) {
			RSPShiftRightUnsignImmed(x86_EAX, 16);
			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
			RSPMoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, el * 2);
		} else {
			RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
			RSPShiftRightSignImmed(x86_EDX, 16);
			RSPShiftLeftSignImmed(x86_EAX, 16);

			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
				RSPMoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
				sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
				RSPMoveX86regToVariable(x86_EDX ,&RSP_ACCUM[el].UW[1], Reg);
			}
			if (bWriteToDest == TRUE) {
				/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				RSPMoveX86regHalfToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
				RSPMoveX86regHalfToX86regPointerDisp(x86_EDX, x86_ECX, el * 2);
			}
		}
	}

	RSPPop(x86_EBP);
}

BOOL RSPCompile_Vector_VMUDN_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		RSPMmxPmullwVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		RSPMmxPmullwVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPmullwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VMUDN ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmudn
	RSPCheat_r4300iOpcode(RSP_Vector_VMUDN,"RSP_Vector_VMUDN"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VMUDN_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	RSPPush(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		/*sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);*/
		RSPMoveZxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPimulX86reg(x86_EBX);

		if (bWriteToDest == TRUE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}

		if (bWriteToAccum == TRUE) {
			RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
			RSPShiftRightSignImmed(x86_EDX, 16);
			RSPShiftLeftSignImmed(x86_EAX, 16);
			sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
			RSPMoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
			sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
			RSPMoveX86regToVariable(x86_EDX, &RSP_ACCUM[el].UW[1], Reg);
		}
	}
	RSPPop(x86_EBP);
}

BOOL RSPCompile_Vector_VMUDH_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].HW[4], Reg);

	/* Registers 4 & 5 are high */
	RSPMmxMoveRegToReg(x86_MM4, x86_MM0);
	RSPMmxMoveRegToReg(x86_MM5, x86_MM1);

	if ((RSPOpC.rs & 0x0f) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);

		RSPMmxPmullwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM4, x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM1, x86_MM3);
		RSPMmxPmulhwRegToReg(x86_MM5, x86_MM3);
	} else if ((RSPOpC.rs & 0x0f) >= 8) {
		RSP_Element2Mmx(x86_MM2);

		RSPMmxPmullwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM4, x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM1, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM5, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);

		RSPMmxPmullwRegToReg(x86_MM0, x86_MM2);
		RSPMmxPmulhwRegToReg(x86_MM4, x86_MM2);
		RSPMmxPmullwRegToReg(x86_MM1, x86_MM3);
		RSPMmxPmulhwRegToReg(x86_MM5, x86_MM3);
	}

	/* 0 & 1 are low, 4 & 5 are high */
	RSPMmxMoveRegToReg(x86_MM6, x86_MM0);
	RSPMmxMoveRegToReg(x86_MM7, x86_MM1);

	RSPMmxUnpackLowWord(x86_MM0, x86_MM4);
	RSPMmxUnpackHighWord(x86_MM6, x86_MM4);
	RSPMmxUnpackLowWord(x86_MM1, x86_MM5);		
	RSPMmxUnpackHighWord(x86_MM7, x86_MM5);

	/* Integrate copies */
	RSPMmxPackSignedDwords(x86_MM0, x86_MM6);
	RSPMmxPackSignedDwords(x86_MM1, x86_MM7);
	
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].HW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VMUDH ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmudh
	RSPCheat_r4300iOpcode(RSP_Vector_VMUDH,"RSP_Vector_VMUDH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VMUDH_MMX())
			return;
	}

	if (bWriteToDest == FALSE && bOptimize == TRUE) {
		RSPPush(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);

		/* Load source */
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

		/* 
		 * Pipe lined segment 0
		 */
		
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

		RSPImulX86RegToX86Reg(x86_EAX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ECX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_EDI, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ESI, x86_EBX);
		RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);

		RSPMoveOffsetToX86reg((DWORD)&RSP_ACCUM[0].W[0], "RSP_ACCUM[0].W[0]", x86_EBP);

		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 0);
		RSPMoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 4);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 8);
		RSPMoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 12);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 16);
		RSPMoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 20);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 24);
		RSPMoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 28);

		/* 
		 * Pipe lined segment 1
		 */

		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP,  8, x86_EAX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

		RSPImulX86RegToX86Reg(x86_EAX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ECX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_EDI, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ESI, x86_EBX);
		RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);

		RSPMoveOffsetToX86reg((DWORD)&RSP_ACCUM[0].W[0], "RSP_ACCUM[0].W[0]", x86_EBP);

		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 32);
		RSPMoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 36);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 40);
		RSPMoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 44);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 48);
		RSPMoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 52);
		RSPMoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 56);
		RSPMoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 60);

		RSPPop(x86_EBP);
	} else {
		if (bOptimize == TRUE) {
			del = (RSPOpC.rs & 0x07) ^ 7;
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
		if (bWriteToDest == TRUE) {
			/*
			 * Prepare for conditional moves
			 */
			RSPMoveConstToX86reg(0x00007fff, x86_ESI);
			RSPMoveConstToX86reg(0xFFFF8000, x86_EDI);
		}

		for (count = 0; count < 8; count++) {
			CPU_Message("     Iteration: %i", count);
			el = Indx[RSPOpC.rs].B[count];
			del = EleSpec[RSPOpC.rs].B[el];
		
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

			if (bOptimize == FALSE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			}
			RSPimulX86reg(x86_EBX);
			
			if (bWriteToAccum == TRUE) {
				RSPMoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");
				RSPMoveConstToVariable(0, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
			}
			
			if (bWriteToDest == TRUE) {
				RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
				RSPCondMoveGreater(x86_EAX, x86_ESI);
				RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
				RSPCondMoveLess(x86_EAX, x86_EDI);

				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
		}
	}
}

void RSPCompile_Vector_VMACF ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmacf
	RSPCheat_r4300iOpcode(RSP_Vector_VMACF,"RSP_Vector_VMACF"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007fff, x86_ESI);
		RSPMoveConstToX86reg(0xFFFF8000, x86_EDI);
	}
	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPimulX86reg(x86_EBX);

		RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
		RSPShiftRightSignImmed(x86_EDX, 15);
		RSPShiftLeftSignImmed(x86_EAX, 17);

		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
		RSPAdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");

		if (bWriteToDest == TRUE) {
			RSPMoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

			RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
			RSPCondMoveGreater(x86_EAX, x86_ESI);
			RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
			RSPCondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
}

void RSPCompile_Vector_VMACU ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VMACU,"RSP_Vector_VMACU");
}

void RSPCompile_Vector_VMACQ ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VMACQ,"RSP_Vector_VMACQ");
}

void RSPCompile_Vector_VMADL ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);

	#ifndef RSPCompileVmadl
	RSPCheat_r4300iOpcode(RSP_Vector_VMADL,"RSP_Vector_VMADL"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007FFF, x86_ESI);
		RSPMoveConstToX86reg(0xFFFF8000, x86_EDI);

		RSPPush(x86_EBP);
		RSPMoveConstToX86reg(0x0000FFFF, x86_EBP);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPimulX86reg(x86_EBX);
		sprintf(Reg, "RSP_ACCUM[%i].W[0]", el);
		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
		RSPAdcConstToVariable(&RSP_ACCUM[el].W[1], Reg, 0);

		if (bWriteToDest != FALSE) {
			RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);
			RSPMoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);
			RSPMoveZxVariableToX86regHalf(&RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].hW[1]", x86_ECX);

			RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
			RSPCondMoveGreater(x86_ECX, x86_EBP);
			RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
			RSPCondMoveLess(x86_ECX, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}

	if (bWriteToDest == TRUE) {
		RSPPop(x86_EBP);
	}
}

void RSPCompile_Vector_VMADM ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmadm
	RSPCheat_r4300iOpcode(RSP_Vector_VMADM,"RSP_Vector_VMADM"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007fff, x86_ESI);
		RSPMoveConstToX86reg(0xFFFF8000, x86_EDI);
	}

	RSPPush(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	if (bWriteToDest) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
	} else if (!bOptimize) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
		
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

		if (bOptimize == FALSE) {
			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], "RSP_Vect[RSPOpC.rt].HW[del]", x86_EBX);
			} else {
				RSPMoveZxX86RegPtrDispToX86RegHalf(x86_ECX, del * 2, x86_EBX);
			}
		}

		RSPimulX86reg(x86_EBX);

		RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
		RSPShiftRightSignImmed(x86_EDX, 16);
		RSPShiftLeftSignImmed(x86_EAX, 16);
		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
		RSPAdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");

		if (bWriteToDest == TRUE) {
			/* For compare */
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			RSPMoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

			RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
			RSPCondMoveGreater(x86_EAX, x86_ESI);
			RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
			RSPCondMoveLess(x86_EAX, x86_EDI);

			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
			RSPMoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, el * 2);
		}
	}

	RSPPop(x86_EBP);
}

void RSPCompile_Vector_VMADN ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, RSPCompilePC);

	#ifndef RSPCompileVmadn
	RSPCheat_r4300iOpcode(RSP_Vector_VMADN,"RSP_Vector_VMADN"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	} 
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x0000ffff, x86_ESI);
		RSPMoveConstToX86reg(0x00000000, x86_EDI);
	}

	RSPPush(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		/*sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);*/
		RSPMoveZxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPimulX86reg(x86_EBX);

		RSPMoveX86RegToX86Reg(x86_EAX, x86_EDX);
		RSPShiftRightSignImmed(x86_EDX, 16);
		RSPShiftLeftSignImmed(x86_EAX, 16);
		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].HW[0]");
		RSPAdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].HW[1]");

		if (bWriteToDest == TRUE) {
			/* For compare */
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			RSPMoveVariableToX86reg(&RSP_ACCUM[el].W[1], Reg, x86_EAX);

			/* For vector */
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			RSPMoveVariableToX86regHalf(&RSP_ACCUM[el].HW[1], Reg, x86_ECX);

			/* Weird eh */
			RSPCompConstToX86reg(x86_EAX, 0x7fff);
			RSPCondMoveGreater(x86_ECX, x86_ESI);
			RSPCompConstToX86reg(x86_EAX, -0x8000);
			RSPCondMoveLess(x86_ECX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	RSPPop(x86_EBP);
}

void RSPCompile_Vector_VMADH ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);

	#ifndef RSPCompileVmadh
	RSPCheat_r4300iOpcode(RSP_Vector_VMADH,"RSP_Vector_VMADH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	} 
	
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007fff, x86_ESI);
		RSPMoveConstToX86reg(0xFFFF8000, x86_EDI);
	}

	if (bWriteToDest == FALSE && bOptimize == TRUE) {
		RSPPush(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		/* 
		 * Pipe lined segment 0
		 */
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

		RSPImulX86RegToX86Reg(x86_EAX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ECX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_EDI, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ESI, x86_EBX);

		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 0);
		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[0].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 1);
		RSPAddX86regToVariable(x86_ECX, &RSP_ACCUM[1].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 2);
		RSPAddX86regToVariable(x86_EDI, &RSP_ACCUM[2].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 3);
		RSPAddX86regToVariable(x86_ESI, &RSP_ACCUM[3].W[1], Reg);

		/* 
		 * Pipe lined segment 1
		 */
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP,  8, x86_EAX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
		RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

		RSPImulX86RegToX86Reg(x86_EAX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ECX, x86_EBX);
		RSPImulX86RegToX86Reg(x86_EDI, x86_EBX);
		RSPImulX86RegToX86Reg(x86_ESI, x86_EBX);

		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 4);
		RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[4].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 5);
		RSPAddX86regToVariable(x86_ECX, &RSP_ACCUM[5].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 6);
		RSPAddX86regToVariable(x86_EDI, &RSP_ACCUM[6].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 7);
		RSPAddX86regToVariable(x86_ESI, &RSP_ACCUM[7].W[1], Reg);

		RSPPop(x86_EBP);
	} else {
		RSPPush(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		if (bWriteToDest) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
			RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
		} else if (!bOptimize) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
		}

		for (count = 0; count < 8; count++) {
			CPU_Message("     Iteration: %i", count);
			el = Indx[RSPOpC.rs].B[count];
			del = EleSpec[RSPOpC.rs].B[el];

			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
			RSPMoveSxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

			if (bOptimize == FALSE) {
				if (bWriteToDest == TRUE) {
					sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
					RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
				} else {
					RSPMoveSxX86RegPtrDispToX86RegHalf(x86_ECX, del * 2, x86_EBX);
				}
			}

			RSPimulX86reg(x86_EBX);
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			RSPAddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[1], Reg);

			if (bWriteToDest == TRUE) {
				RSPMoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

				RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
				RSPCondMoveGreater(x86_EAX, x86_ESI);
				RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
				RSPCondMoveLess(x86_EAX, x86_EDI);

				/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
				RSPMoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, el * 2);
			}
		}
		RSPPop(x86_EBP);
	}
}

BOOL RSPCompile_Vector_VADD_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 15) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPaddswRegToReg(x86_MM0, x86_MM2);
		RSPMmxPaddswRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 15) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMmxPaddswVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		RSPMmxPaddswVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPaddswRegToReg(x86_MM0, x86_MM2);
		RSPMmxPaddswRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VADD ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);
	BOOL bFlagUseage = UseRspFlags(RSPCompilePC);

	#ifndef RSPCompileVadd
	RSPCheat_r4300iOpcode(RSP_Vector_VADD,"RSP_Vector_VADD"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE && bFlagUseage == FALSE) {
		if (TRUE == RSPCompile_Vector_VADD_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007fff, x86_ESI);
		RSPMoveConstToX86reg(0xffff8000, x86_EDI);
	}
	
	/* Used for involking x86 carry flag */
	RSPXorX86RegToX86Reg(x86_ECX, x86_ECX);
	RSPPush(x86_EBP);
	RSPMoveVariableToX86reg(&RSP_Flags[0].UW, "RSP_Flags[0].UW", x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
		
		RSPMoveX86RegToX86Reg(x86_EBP, x86_EDX);
		RSPAndConstToX86Reg(x86_EDX, 1 << (7 - el));
		RSPCompX86RegToX86Reg(x86_ECX, x86_EDX);

		RSPAdcX86RegToX86Reg(x86_EAX, x86_EBX);

		if (bWriteToAccum == TRUE) {
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
		}
		if (bWriteToDest == TRUE) {
			RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
			RSPCondMoveGreater(x86_EAX, x86_ESI);
			RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
			RSPCondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	RSPMoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	RSPPop(x86_EBP);
}

void RSPCompile_Vector_VSUB ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);
	BOOL bZeroReg = ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rt == RSPOpC.rd)) ? TRUE : FALSE;

	#ifndef RSPCompileVsub
	RSPCheat_r4300iOpcode(RSP_Vector_VSUB,"RSP_Vector_VSUB"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPPush(x86_EBP);

	/* Used for involking the x86 carry flag */
	RSPXorX86RegToX86Reg(x86_ECX, x86_ECX);
	RSPMoveVariableToX86reg(&RSP_Flags[0].UW, "RSP_Flags[0].UW", x86_EBP);

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		RSPMoveConstToX86reg(0x00007fff, x86_ESI);
		RSPMoveConstToX86reg(0xffff8000, x86_EDI);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], "RSP_Vect[RSPOpC.rd].HW[el]", x86_EAX);
		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
	
		RSPMoveX86RegToX86Reg(x86_EBP, x86_EDX);
		RSPAndConstToX86Reg(x86_EDX, 1 << (7 - el));
		RSPCompX86RegToX86Reg(x86_ECX, x86_EDX);

		RSPSbbX86RegToX86Reg(x86_EAX, x86_EBX);

		if (bWriteToAccum == TRUE) {
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
		}
		if (bWriteToDest == TRUE) {
			RSPCompX86RegToX86Reg(x86_EAX, x86_ESI);
			RSPCondMoveGreater(x86_EAX, x86_ESI);
			RSPCompX86RegToX86Reg(x86_EAX, x86_EDI);
			RSPCondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}

	RSPMoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	RSPPop(x86_EBP);
}

void RSPCompile_Vector_VABS ( void ) {
	int count, el, del;
	char Reg[256];
	
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	#ifndef RSPCompileVabs
	RSPCheat_r4300iOpcode(RSP_Vector_VABS,"RSP_Vector_VABS"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		if (RSPOpC.rd == RSPOpC.rt && (RSPOpC.rs & 0xF) < 2) {
			/**
			** Optimize: EDI/ESI unused, and ECX is const etc
			***/

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

			/*** Obtain the negative of the source ****/
			RSPMoveX86RegToX86Reg(x86_EAX, x86_EBX);
			RSPNegateX86reg(x86_EBX);
		
			/**
			** determine negative value, 
			** note: negate(FFFF8000h) == 00008000h 
			***/

			RSPMoveConstToX86reg(0x7fff, x86_ECX);
			RSPCompConstToX86reg(x86_EBX, 0x00008000);
			RSPCondMoveEqual(x86_EBX, x86_ECX);

			/* sign clamp, dest = (eax >= 0) ? eax : ebx */
			RSPCompConstToX86reg(x86_EAX, 0);
			RSPCondMoveLess(x86_EAX, x86_EBX);

			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
			}
		} else {
			/**
			** Optimize: ESI unused, and EDX is const etc
			***/

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

			/*** Obtain the negative of the source ****/
			RSPMoveX86RegToX86Reg(x86_EBX, x86_ECX);
			RSPNegateX86reg(x86_EBX);

			/**
			** determine negative value, 
			** note: negate(FFFF8000h) == 00008000h 
			***/

			RSPMoveConstToX86reg(0x7fff, x86_EDX);
			RSPCompConstToX86reg(x86_EBX, 0x00008000);
			RSPCondMoveEqual(x86_EBX, x86_EDX);

			/* sign clamp, dest = (eax >= 0) ? ecx : ebx */
			RSPCompConstToX86reg(x86_EAX, 0);
			RSPCondMoveGreaterEqual(x86_EDI, x86_ECX);
			RSPCondMoveLess(x86_EDI, x86_EBX);

			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				RSPMoveX86regHalfToVariable(x86_EDI, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				RSPMoveX86regHalfToVariable(x86_EDI, &RSP_ACCUM[el].HW[1], Reg);	
			}
		}
	}
}

void RSPCompile_Vector_VADDC ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	
	#ifndef RSPCompileVaddc
	RSPCheat_r4300iOpcode(RSP_Vector_VADDC,"RSP_Vector_VADDC"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	/* Initialize flag register */
	RSPXorX86RegToX86Reg(x86_ECX, x86_ECX);

	RSPPush(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		RSPMoveZxX86RegPtrDispToX86RegHalf(x86_EBP, el * 2, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPAddX86RegToX86Reg(x86_EAX, x86_EBX);

		RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);
		RSPTestConstToX86Reg(0xFFFF0000, x86_EAX);
		RSPSetnz(x86_EDX);
		if ((7 - el) != 0) {
			RSPShiftLeftSignImmed(x86_EDX, 7 - el);
		}
		RSPOrX86RegToX86Reg(x86_ECX, x86_EDX);

		if (bWriteToAccum == TRUE) {
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
		}
		if (bWriteToDest == TRUE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	RSPMoveX86regToVariable(x86_ECX, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	RSPPop(x86_EBP);
}

void RSPCompile_Vector_VSUBC ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, RSPCompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	
	#ifndef RSPCompileVsubc
	RSPCheat_r4300iOpcode(RSP_Vector_VSUBC,"RSP_Vector_VSUBC"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	/* Initialize flag register */
	RSPXorX86RegToX86Reg(x86_ECX, x86_ECX);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		RSPSubX86RegToX86Reg(x86_EAX, x86_EBX);

		RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);
		RSPTestConstToX86Reg(0x0000FFFF, x86_EAX);
		RSPSetnz(x86_EDX);
		RSPShiftLeftSignImmed(x86_EDX, 15 - el);
		RSPOrX86RegToX86Reg(x86_ECX, x86_EDX);

		RSPXorX86RegToX86Reg(x86_EDX, x86_EDX);
		RSPTestConstToX86Reg(0xFFFF0000, x86_EAX);
		RSPSetnz(x86_EDX);
		RSPShiftLeftSignImmed(x86_EDX, 7 - el);
		RSPOrX86RegToX86Reg(x86_ECX, x86_EDX);

		if (bWriteToAccum == TRUE) {
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
		}
		if (bWriteToDest == TRUE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	RSPMoveX86regToVariable(x86_ECX, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
}

void RSPCompile_Vector_VSAW ( void ) {
	char Reg[256];
	DWORD Word;

	#ifndef RSPCompileVsaw
	RSPCheat_r4300iOpcode(RSP_Vector_VSAW,"RSP_Vector_VSAW"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	switch ((RSPOpC.rs & 0xF)) {
	case 8: Word = 3; break;
	case 9: Word = 2; break;
	case 10: Word = 1; break;
	default:
		RSPMoveConstToVariable(0, &RSP_Vect[RSPOpC.sa].DW[1], "RSP_Vect[RSPOpC.sa].DW[1]");
		RSPMoveConstToVariable(0, &RSP_Vect[RSPOpC.sa].DW[0], "RSP_Vect[RSPOpC.sa].DW[0]");
		return;
	}

	sprintf(Reg, "RSP_ACCUM[1].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[1].HW[Word], Reg, x86_EAX);
	sprintf(Reg, "RSP_ACCUM[3].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[3].HW[Word], Reg, x86_EBX);
	sprintf(Reg, "RSP_ACCUM[5].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[5].HW[Word], Reg, x86_ECX);
	sprintf(Reg, "RSP_ACCUM[7].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[7].HW[Word], Reg, x86_EDX);

	RSPShiftLeftSignImmed(x86_EAX, 16);
	RSPShiftLeftSignImmed(x86_EBX, 16);
	RSPShiftLeftSignImmed(x86_ECX, 16);
	RSPShiftLeftSignImmed(x86_EDX, 16);

	sprintf(Reg, "RSP_ACCUM[0].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[0].HW[Word], Reg, x86_EAX);
	sprintf(Reg, "RSP_ACCUM[2].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[2].HW[Word], Reg, x86_EBX);
	sprintf(Reg, "RSP_ACCUM[4].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[4].HW[Word], Reg, x86_ECX);
	sprintf(Reg, "RSP_ACCUM[6].HW[%i]", Word);
	RSPMoveVariableToX86regHalf(&RSP_ACCUM[6].HW[Word], Reg, x86_EDX);

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
	RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[2]", RSPOpC.sa);
	RSPMoveX86regToVariable(x86_EBX, &RSP_Vect[RSPOpC.sa].HW[2], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.sa);
	RSPMoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[4], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[6]", RSPOpC.sa);
	RSPMoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].HW[6], Reg);
}

void RSPCompile_Vector_VLT ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VLT,"RSP_Vector_VLT");
}

void RSPCompile_Vector_VEQ ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VEQ,"RSP_Vector_VEQ");
}

void RSPCompile_Vector_VNE ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VNE,"RSP_Vector_VNE");
}

BOOL RSPCompile_Vector_VGE_MMX(void) {
	char Reg[256];

	if ((RSPOpC.rs & 0xF) >= 2 && (RSPOpC.rs & 0xF) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPMoveConstToVariable(0, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].HW[4], Reg);
	RSPMmxMoveRegToReg(x86_MM2, x86_MM0);
	RSPMmxMoveRegToReg(x86_MM3, x86_MM1);

	if ((RSPOpC.rs & 0x0f) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM4, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		RSPMmxMoveQwordVariableToReg(x86_MM5, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
	} else if ((RSPOpC.rs & 0x0f) >= 8) {
		RSP_Element2Mmx(x86_MM4);
	} else {
		RSP_MultiElement2Mmx(x86_MM4, x86_MM5);
	}

	RSPMmxCompareGreaterWordRegToReg(x86_MM2, x86_MM4);
	RSPMmxCompareGreaterWordRegToReg(x86_MM3, ((RSPOpC.rs & 0x0f) >= 8) ? x86_MM4 : x86_MM5);

	RSPMmxPandRegToReg(x86_MM0, x86_MM2);
	RSPMmxPandRegToReg(x86_MM1, x86_MM3);
	RSPMmxPandnRegToReg(x86_MM2, x86_MM4);
	RSPMmxPandnRegToReg(x86_MM3, ((RSPOpC.rs & 0x0f) >= 8) ? x86_MM4 : x86_MM5);

	RSPMmxPorRegToReg(x86_MM0, x86_MM2);
	RSPMmxPorRegToReg(x86_MM1, x86_MM3);
	RSPMoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	return TRUE;
}

void RSPCompile_Vector_VGE ( void ) {
/*	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	/* FIXME: works ok, but needs careful flag analysis */
/*	#if defined (DLIST)
	if (bWriteToAccum == FALSE && TRUE == RSPCompile_Vector_VGE_MMX()) {
		return;
	}
	#endif
*/
	RSPCheat_r4300iOpcode(RSP_Vector_VGE,"RSP_Vector_VGE");
}

void RSPCompile_Vector_VCL ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VCL,"RSP_Vector_VCL");
}

void RSPCompile_Vector_VCH ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VCH,"RSP_Vector_VCH");
}

void RSPCompile_Vector_VCR ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VCR,"RSP_Vector_VCR");
}

void RSPCompile_Vector_VMRG ( void ) {
	char Reg[256];
	int count, el, del;

	#ifndef RSPCompileVmrg
	RSPCheat_r4300iOpcode(RSP_Vector_VMRG,"RSP_Vector_VMRG"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPMoveVariableToX86reg(&RSP_Flags[1].UW, "RSP_Flags[1].UW", x86_EDX);

	for (count = 0;count < 8; count++) {
		el = Indx[RSPOpC.rs].UB[count];
		del = EleSpec[RSPOpC.rs].UB[el];
		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

		RSPTestConstToX86Reg(1 << (7 - el), x86_EDX);
		RSPCondMoveNotEqual(x86_ECX, x86_EAX);
		RSPCondMoveEqual(x86_ECX, x86_EBX);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	}
}

BOOL RSPCompile_Vector_VAND_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPandRegToReg(x86_MM0, x86_MM2);
		RSPMmxPandRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		RSPMmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPandRegToReg(x86_MM0, x86_MM2);
		RSPMmxPandRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VAND ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	#ifndef RSPCompileVand
	RSPCheat_r4300iOpcode(RSP_Vector_VAND,"RSP_Vector_VAND"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VAND_MMX())
			return;
	}
	
	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
		
		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPAndVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			RSPAndX86RegHalfToX86RegHalf(x86_EAX, x86_EBX);
		}

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[el].HW[1]", el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
	}
}

void RSPCompile_Vector_VNAND ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VNAND,"RSP_Vector_VNAND");
}

BOOL RSPCompile_Vector_VOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		RSPMmxPorRegToReg(x86_MM0, x86_MM2);
		RSPMmxPorRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		RSPMmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		RSPMmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		RSPMmxPorRegToReg(x86_MM0, x86_MM2);
		RSPMmxPorRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VOR ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	#ifndef RSPCompileVor
	RSPCheat_r4300iOpcode(RSP_Vector_VOR,"RSP_Vector_VOR"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == RSPCompile_Vector_VOR_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
		
		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			RSPOrVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			RSPOrX86RegToX86Reg(x86_EAX, x86_EBX);
		}

		if (bWriteToAccum == TRUE) {
			sprintf(Reg, "RSP_ACCUM[el].HW[1]", el);
			RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		RSPMoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	}
}

void RSPCompile_Vector_VNOR ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VNOR,"RSP_Vector_VNOR");
}

BOOL RSPCompile_Vector_VXOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt)) {
		static DWORD VXOR_DynaRegCount = 0;
		RSPMmxXorRegToReg(VXOR_DynaRegCount, VXOR_DynaRegCount);

		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		RSPMmxMoveQwordRegToVariable(VXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		RSPMmxMoveQwordRegToVariable(VXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
		VXOR_DynaRegCount = (VXOR_DynaRegCount + 1) & 7;
	} else {		
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
		RSPMmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
		RSPMmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

		if ((RSPOpC.rs & 0xF) >= 8) {
			RSP_Element2Mmx(x86_MM2);
			RSPMmxXorRegToReg(x86_MM0, x86_MM2);
			RSPMmxXorRegToReg(x86_MM1, x86_MM2);
		} else if ((RSPOpC.rs & 0xF) < 2) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			RSPMmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			RSPMmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);

			RSPMmxXorRegToReg(x86_MM0, x86_MM2);
			RSPMmxXorRegToReg(x86_MM1, x86_MM3);
		} else {
			RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
			RSPMmxXorRegToReg(x86_MM0, x86_MM2);
			RSPMmxXorRegToReg(x86_MM1, x86_MM3);
		}

		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		RSPMmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		RSPMmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	}

	if (IsNextInstructionMmx(RSPCompilePC) == FALSE)
		RSPMmxEmptyMultimediaState();

	return TRUE;
}

void RSPCompile_Vector_VXOR ( void ) {	
	#ifdef RSPCompileVxor
	char Reg[256];
	DWORD count;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC)); 
	
	if (!bWriteToAccum || ((RSPOpC.rs & 0xF) < 2 && RSPOpC.rd == RSPOpC.rt)) {
		if (TRUE == RSPCompile_Vector_VXOR_MMX()) {
			if (bWriteToAccum == TRUE) {
				RSPXorX86RegToX86Reg(x86_EAX, x86_EAX);
				for (count = 0; count < 8; count++) {
					sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
					RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
				}
			}
			return;
		}
	}
	#endif

	RSPCheat_r4300iOpcodeNoMessage(RSP_Vector_VXOR,"RSP_Vector_VXOR");
}

void RSPCompile_Vector_VNXOR ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VNXOR,"RSP_Vector_VNXOR");
}

void RSPCompile_Vector_VRCP ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VRCP,"RSP_Vector_VRCP");
}

void RSPCompile_Vector_VRCPL ( void ) {
	RSPCheat_r4300iOpcode(RSP_Vector_VRCPL,"RSP_Vector_VRCPL");
}

void RSPCompile_Vector_VRCPH ( void ) {
	char Reg[256];
	int count, el, last = -1;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, RSPCompilePC);

	#ifndef RSPCompileVrcph
	RSPCheat_r4300iOpcode(RSP_Vector_VRCPH,"RSP_Vector_VRCPH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EDX);
	RSPMoveX86regHalfToVariable(x86_EDX, &Recp.UHW[1], "Recp.UHW[1]");
	
	RSPMoveConstHalfToVariable(0, &Recp.UHW[0], "Recp.UHW[0]");
	
	RSPMoveVariableToX86regHalf(&RecpResult.UHW[1], "RecpResult.UHW[1]", x86_ECX);
	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);

	if (bWriteToAccum == FALSE) return;

	for (count = 0; count < 8; count++) {
		el = EleSpec[RSPOpC.rs].B[count];

		if (el != last) {
			sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
			RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EAX);
			last = el;
		}

		sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
		RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
	}
}

void RSPCompile_Vector_VMOV ( void ) {
	char Reg[256];
	int el;

	#ifndef RSPCompileVmov
	RSPCheat_r4300iOpcode(RSP_Vector_VMOV,"RSP_Vector_VMOV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);

	RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_ECX);

	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);

	RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);
}

void RSPCompile_Vector_VRSQ ( void ) {
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPCheat_r4300iOpcodeNoMessage(RSP_Vector_VRSQ,"RSP_Vector_VRSQ");
}

void RSPCompile_Vector_VRSQL ( void ) {
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	RSPCheat_r4300iOpcodeNoMessage(RSP_Vector_VRSQL,"RSP_Vector_VRSQL");
}

void RSPCompile_Vector_VRSQH ( void ) {
	char Reg[256];
	int count, el;

	#ifndef RSPCompileVrsqh
	RSPCheat_r4300iOpcode(RSP_Vector_VRSQH,"RSP_Vector_VRSQH"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	
	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EDX);
	RSPMoveX86regHalfToVariable(x86_EDX, &SQroot.UHW[1], "SQroot.UHW[1]");

	RSPMoveVariableToX86regHalf(&SQrootResult.UHW[1], "SQrootResult.UHW[1]", x86_ECX);
	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);

	for (count = 0; count < 8; count++) {
		el = EleSpec[RSPOpC.rs].B[count];
		sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EAX);

		sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
		RSPMoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
	}
}

void RSPCompile_Vector_VNOOP ( void ) {

}

/************************** lc2 functions **************************/

void RSPCompile_Opcode_LBV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LBV,"RSP_Opcode_LBV");
}

void RSPCompile_Opcode_LSV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 1);

	if (RSPOpC.del > 14) {
		rsp_UnknownOpcode();
		return;
	}

	#ifndef RSPCompileLsv
	RSPCheat_r4300iOpcode(RSP_Opcode_LSV,"RSP_Opcode_LSV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));
	
	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 1) != 0) {
			sprintf(Reg, "Dmem + %Xh", (Addr + 0) ^ 3);
			RSPMoveVariableToX86regByte(DMEM + ((Addr + 0) ^ 3), Reg, x86_ECX);
			sprintf(Reg, "Dmem + %Xh", (Addr + 1) ^ 3);
			RSPMoveVariableToX86regByte(DMEM + ((Addr + 1) ^ 3), Reg, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
			RSPMoveX86regByteToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			RSPMoveX86regByteToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
		} else {
			sprintf(Reg, "Dmem + %Xh", Addr ^ 2);
			RSPMoveVariableToX86regHalf(DMEM + (Addr ^ 2), Reg, x86_EDX);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			RSPMoveX86regHalfToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
		}
		return;
	}
	
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) RSPAddConstToX86Reg(x86_EBX, offset);
	RSPAndConstToX86Reg(x86_EBX, 0x0FFF);

	if (Compiler.bAlignVector == TRUE) {
		RSPXorConstToX86Reg(x86_EBX, 2);
		RSPMoveN64MemToX86regHalf(x86_ECX, x86_EBX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		RSPMoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
	} else {
		RSPLeaSourceAndOffset(x86_EAX, x86_EBX, 1);
		RSPXorConstToX86Reg(x86_EBX, 3);
		RSPXorConstToX86Reg(x86_EAX, 3);

		RSPMoveN64MemToX86regByte(x86_ECX, x86_EBX);
		RSPMoveN64MemToX86regByte(x86_EDX, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
		RSPMoveX86regByteToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		RSPMoveX86regByteToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
	}
}

void RSPCompile_Opcode_LLV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 2);
	BYTE * Jump[2];

	#ifndef RSPCompileLlv
	RSPCheat_r4300iOpcode(RSP_Opcode_LLV,"RSP_Opcode_LLV"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if ((RSPOpC.del & 0x3) != 0) {
		rsp_UnknownOpcode();
		return;
	}

 	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			//CompilerWarning("Unaligned LLV at constant address");
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LLV,"RSP_Opcode_LLV");
			return;
		}

		sprintf(Reg, "Dmem + %Xh", Addr);
		RSPMoveVariableToX86reg(DMEM + Addr, Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
		return;
	}
	
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) RSPAddConstToX86Reg(x86_EBX, offset);
	
	RSPTestConstToX86Reg(3, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	/*
	 * Unaligned
	 */
	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	*((DWORD *)(Jump[0]))=(DWORD)(RSPRecompPos - Jump[0] - 4);
	RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LLV,"RSP_Opcode_LLV");
	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;

	CompilerToggleBuffer();

	/*
	 * Aligned
	 */
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPMoveN64MemToX86reg(x86_EAX, x86_EBX);
	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);

	CPU_Message("   Done:");
	*((DWORD *)(Jump[1]))=(DWORD)(RSPRecompPos - Jump[1] - 4);
}

void RSPCompile_Opcode_LDV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 3);
	BYTE * Jump[2], * LoopEntry;

	#ifndef RSPCompileLdv
	RSPCheat_r4300iOpcode(RSP_Opcode_LDV,"RSP_Opcode_LDV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	/* FIXME: Conker's hits this */
	//if ((RSPOpC.del & 0x7) != 0) {
	//	rsp_UnknownOpcode();
	//	return;
	//}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned LDV at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LDV,"RSP_Opcode_LDV");
			return;
		}

		sprintf(Reg, "Dmem + %Xh", Addr);
		RSPMoveVariableToX86reg(DMEM + Addr + 0, Reg, x86_EAX);
		sprintf(Reg, "Dmem + %Xh", Addr + 4);
		RSPMoveVariableToX86reg(DMEM + Addr + 4, Reg, x86_ECX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
		RSPMoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg);
		return;
	}
	
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		RSPAddConstToX86Reg(x86_EBX, offset);
	}
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPTestConstToX86Reg(3, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	RSPx86_SetBranch32b(Jump[0], RSPRecompPos);
	sprintf(Reg, "RSP_Vect[%i].UB[%i]", RSPOpC.rt, 15 - RSPOpC.del);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].UB[15 - RSPOpC.del], Reg, x86_EDI);
	RSPMoveConstToX86reg(8, x86_ECX);

/*    mov eax, ebx
      dec edi
      xor eax, 3h
      inc ebx
      mov dl, byte ptr [eax+Dmem]
      dec ecx
      mov byte ptr [edi+1], dl      
      jne $Loop */

	LoopEntry = RSPRecompPos;
	CPU_Message("   Loop:");	
	RSPMoveX86RegToX86Reg(x86_EBX, x86_EAX);
	RSPXorConstToX86Reg(x86_EAX, 3);
	RSPMoveN64MemToX86regByte(x86_EDX, x86_EAX);
	RSPMoveX86regByteToX86regPointer(x86_EDX, x86_EDI);
	RSPIncX86reg(x86_EBX); /* address constant */
	RSPDecX86reg(x86_EDI); /* vector pointer */
	RSPDecX86reg(x86_ECX); /* counter */
	RSPJneLabel8("Loop", 0);
	RSPx86_SetBranch8b(RSPRecompPos - 1, LoopEntry);

	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	RSPMoveN64MemToX86reg(x86_EAX, x86_EBX);
	RSPMoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);
	
	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
	RSPMoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg);

	CPU_Message("   Done:");
	RSPx86_SetBranch32b(Jump[1], RSPRecompPos);
}

void RSPCompile_Opcode_LQV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 4);
	BYTE * Jump[2];

	#ifndef RSPCompileLqv
	RSPCheat_r4300iOpcode(RSP_Opcode_LQV,"RSP_Opcode_LQV"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.del != 0) {
		rsp_UnknownOpcode();
		return;
	}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if (Addr & 15) {
			CompilerWarning("Unaligned LQV at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LQV,"RSP_Opcode_LQV");
			return;
		}
	
		/*
		 * Aligned store
		 */

		if (IsSseEnabled == FALSE) {
			sprintf(Reg, "Dmem+%Xh+0", Addr);
			RSPMoveVariableToX86reg(DMEM + Addr + 0, Reg, x86_EAX);
			sprintf(Reg, "Dmem+%Xh+4", Addr);
			RSPMoveVariableToX86reg(DMEM + Addr + 4, Reg, x86_EBX);
			sprintf(Reg, "Dmem+%Xh+8", Addr);
			RSPMoveVariableToX86reg(DMEM + Addr + 8, Reg, x86_ECX);
			sprintf(Reg, "Dmem+%Xh+C", Addr);
			RSPMoveVariableToX86reg(DMEM + Addr + 12, Reg, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
			RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[12], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
			RSPMoveX86regToVariable(x86_EBX, &RSP_Vect[RSPOpC.rt].B[8], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
			RSPMoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[4], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			RSPMoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[0], Reg);
		} else {
			sprintf(Reg, "Dmem+%Xh", Addr);
			RSPSseMoveUnalignedVariableToReg(DMEM + Addr, Reg, x86_XMM0);
			RSPSseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			RSPSseMoveAlignedRegToVariable(x86_XMM0, &RSP_Vect[RSPOpC.rt].B[0], Reg);
		}
		return;
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		RSPAddConstToX86Reg(x86_EBX, offset);
	}
	RSPTestConstToX86Reg(15, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	RSPx86_SetBranch32b(Jump[0], RSPRecompPos);

	RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LQV,"RSP_Opcode_LQV");
	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	if (IsSseEnabled == FALSE) {
		RSPMoveN64MemDispToX86reg(x86_EAX, x86_EBX, 0);
		RSPMoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);
		RSPMoveN64MemDispToX86reg(x86_EDX, x86_EBX, 8);
		RSPMoveN64MemDispToX86reg(x86_EDI, x86_EBX, 12);

		sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
		RSPMoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[12], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
		RSPMoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[8], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
		RSPMoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[4], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		RSPMoveX86regToVariable(x86_EDI, &RSP_Vect[RSPOpC.rt].B[0], Reg);
	} else {
		RSPSseMoveUnalignedN64MemToReg(x86_XMM0, x86_EBX);
		RSPSseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		RSPSseMoveAlignedRegToVariable(x86_XMM0, &RSP_Vect[RSPOpC.rt].B[0], Reg);
	}
	CPU_Message("   Done:");
	RSPx86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RSPRecompPos);
}

void RSPCompile_Opcode_LRV ( void ) {
	int offset = (RSPOpC.voffset << 4);
	BYTE * Loop, * Jump[2];

	#ifndef RSPCompileLrv
	RSPCheat_r4300iOpcode(RSP_Opcode_LRV,"RSP_Opcode_LRV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.del != 0) {
		rsp_UnknownOpcode();
		return;
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) RSPAddConstToX86Reg(x86_EBX, offset);

	if (Compiler.bAlignVector == FALSE) {
		RSPTestConstToX86Reg(1, x86_EBX);
		RSPJneLabel32("Unaligned", 0);
		Jump[0] = RSPRecompPos - 4;

		/* Unaligned */
		CompilerToggleBuffer();

		CPU_Message(" Unaligned:");
		RSPx86_SetBranch32b(Jump[0], RSPRecompPos);

		RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_LRV,"RSP_Opcode_LRV");
		RSPJmpLabel32("Done", 0);
		Jump[1] = RSPRecompPos - 4;

		CompilerToggleBuffer();
	}

	/* Aligned */
	RSPMoveX86RegToX86Reg(x86_EBX, x86_EAX);
	RSPAndConstToX86Reg(x86_EAX, 0x0F);
	RSPAndConstToX86Reg(x86_EBX, 0x0ff0);

	RSPMoveX86RegToX86Reg(x86_EAX, x86_ECX);
	RSPShiftRightUnsignImmed(x86_ECX, 1);

	RSPJeLabel8("Done", 0);
	Jump[0] = RSPRecompPos - 1;
/*
	RSPDecX86reg(x86_EAX);
	RSPLeaSourceAndOffset(x86_EAX, x86_EAX, (DWORD) &RSP_Vect[RSPOpC.rt].B[0]);
	RSPDecX86reg(x86_EAX);
*/
	RSPAddConstToX86Reg(x86_EAX, ((DWORD)&RSP_Vect[RSPOpC.rt].UB[0]) - 2);

	CPU_Message("   Loop:");
	Loop = RSPRecompPos;

	RSPMoveX86RegToX86Reg(x86_EBX, x86_ESI);
	RSPXorConstToX86Reg(x86_ESI, 2);
	RSPMoveN64MemToX86regHalf(x86_EDX, x86_ESI);
	RSPMoveX86regHalfToX86regPointer(x86_EDX, x86_EAX);

	RSPAddConstToX86Reg(x86_EBX, 2);	/* Dmem pointer	*/
	RSPSubConstFromX86Reg(x86_EAX, 2);	/* Vector pointer */	
	RSPDecX86reg(x86_ECX);				/* Loop counter	*/
	RSPJneLabel8("Loop", 0);
	RSPx86_SetBranch8b(RSPRecompPos - 1, Loop);

	if (Compiler.bAlignVector == FALSE) {
		CPU_Message("   Done:");
		RSPx86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RSPRecompPos);
	}

	RSPx86_SetBranch8b(Jump[0], RSPRecompPos);
}

void RSPCompile_Opcode_LPV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LPV,"RSP_Opcode_LPV");
}

void RSPCompile_Opcode_LUV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LUV,"RSP_Opcode_LUV");
}


void RSPCompile_Opcode_LHV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LHV,"RSP_Opcode_LHV");
}


void RSPCompile_Opcode_LFV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LFV,"RSP_Opcode_LFV");
}

void RSPCompile_Opcode_LTV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_LTV,"RSP_Opcode_LTV");
}

/************************** sc2 functions **************************/

void RSPCompile_Opcode_SBV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SBV,"RSP_Opcode_SBV");
}

void RSPCompile_Opcode_SSV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 1);

	if (RSPOpC.del > 14) {
		rsp_UnknownOpcode();
		return;
	}

	#ifndef RSPCompileSsv
	RSPCheat_r4300iOpcode(RSP_Opcode_SSV,"RSP_Opcode_SSV"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 1) != 0) {
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
			RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg, x86_ECX);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_EDX);

			sprintf(Reg, "Dmem + %Xh", (Addr + 0) ^ 3);
			RSPMoveX86regByteToVariable(x86_ECX, DMEM + ((Addr + 0) ^ 3), Reg);
			sprintf(Reg, "Dmem + %Xh", (Addr + 1) ^ 3);
			RSPMoveX86regByteToVariable(x86_EDX, DMEM + ((Addr + 1) ^ 3), Reg);
		} else {
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_ECX);
			sprintf(Reg, "Dmem + %Xh", Addr ^ 2);
			RSPMoveX86regHalfToVariable(x86_ECX, DMEM + (Addr ^ 2), Reg);
		}
		return;
	}

	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) RSPAddConstToX86Reg(x86_EBX, offset);
	RSPAndConstToX86Reg(x86_EBX, 0x0FFF);

	if (Compiler.bAlignVector == TRUE) {
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		RSPMoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_ECX);
		RSPXorConstToX86Reg(x86_EBX, 2);
		RSPMoveX86regHalfToN64Mem(x86_ECX, x86_EBX);		
	} else {
		RSPLeaSourceAndOffset(x86_EAX, x86_EBX, 1);
		RSPXorConstToX86Reg(x86_EBX, 3);
		RSPXorConstToX86Reg(x86_EAX, 3);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
		RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg, x86_ECX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		RSPMoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_EDX);

		RSPMoveX86regByteToN64Mem(x86_ECX, x86_EBX);
		RSPMoveX86regByteToN64Mem(x86_EDX, x86_EAX);
	}
}

void RSPCompile_Opcode_SLV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 2);
	BYTE * Jump[2];

	#ifndef RSPCompileSlv
	RSPCheat_r4300iOpcode(RSP_Opcode_SLV,"RSP_Opcode_SLV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

//	if ((RSPOpC.del & 0x3) != 0) {
//		rsp_UnknownOpcode();
//		return;
//	}

 	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			//CompilerWarning("Unaligned SLV at constant address");
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SLV,"RSP_Opcode_SLV");
			return;
		}

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
		sprintf(Reg, "Dmem + %Xh", Addr);
		RSPMoveX86regToVariable(x86_EAX, DMEM + Addr, Reg);
		return;
	}
	
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) RSPAddConstToX86Reg(x86_EBX, offset);
	
	RSPTestConstToX86Reg(3, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	/*
	 * Unaligned
	 */
	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	*((DWORD *)(Jump[0]))=(DWORD)(RSPRecompPos - Jump[0] - 4);
	RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SLV,"RSP_Opcode_SLV");
	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;

	CompilerToggleBuffer();

	/*
	 * Aligned
	 */

	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
	
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPMoveX86regToN64Mem(x86_EAX, x86_EBX);

	CPU_Message("   Done:");
	*((DWORD *)(Jump[1]))=(DWORD)(RSPRecompPos - Jump[1] - 4);
}

void RSPCompile_Opcode_SDV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 3);
	BYTE * Jump[2], * LoopEntry;

	//if ((RSPOpC.del & 0x7) != 0) {
	//	rsp_UnknownOpcode();
	//	return;
	//}

	#ifndef RSPCompileSdv
	RSPCheat_r4300iOpcode(RSP_Opcode_SDV,"RSP_Opcode_SDV"); return;
	#endif
	
	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned SDV at constant address PC = %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SDV,"RSP_Opcode_SDV");
			return;
		}

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg, x86_EBX);

		sprintf(Reg, "Dmem + %Xh", Addr);
		RSPMoveX86regToVariable(x86_EAX, DMEM + Addr, Reg);
		sprintf(Reg, "Dmem + %Xh", Addr + 4);
		RSPMoveX86regToVariable(x86_EBX, DMEM + Addr + 4, Reg);
		return;
	}
		
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		RSPAddConstToX86Reg(x86_EBX, offset);
	}
	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	RSPTestConstToX86Reg(3, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;
	
	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	RSPx86_SetBranch32b((DWORD*)Jump[0], (DWORD*)RSPRecompPos);
	
	sprintf(Reg, "RSP_Vect[%i].UB[%i]", RSPOpC.rt, 15 - RSPOpC.del);
	RSPMoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].UB[15 - RSPOpC.del], Reg, x86_EDI);
	RSPMoveConstToX86reg(8, x86_ECX);

	CPU_Message("   Loop:");
	LoopEntry = RSPRecompPos;
	RSPMoveX86RegToX86Reg(x86_EBX, x86_EAX);
	RSPXorConstToX86Reg(x86_EAX, 3);
	RSPMoveX86regPointerToX86regByte(x86_EDX, x86_EDI);
	RSPMoveX86regByteToN64Mem(x86_EDX, x86_EAX);
	RSPIncX86reg(x86_EBX); /* address constant */
	RSPDecX86reg(x86_EDI); /* vector pointer */
	RSPDecX86reg(x86_ECX); /* counter */
	RSPJneLabel8("Loop", 0);
	RSPx86_SetBranch8b(RSPRecompPos - 1, LoopEntry);

	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
	RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg, x86_ECX);
	RSPMoveX86regToN64Mem(x86_EAX, x86_EBX);
	RSPMoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);

	CPU_Message("   Done:");
	RSPx86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RSPRecompPos);
}

void RSPCompile_Opcode_SQV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 4);
	BYTE * Jump[2];

	#ifndef RSPCompileSqv
 	RSPCheat_r4300iOpcode(RSP_Opcode_SQV,"RSP_Opcode_SQV"); return;
	#endif

	CPU_Message("  %X %s",RSPCompilePC,RSPOpcodeName(RSPOpC.Hex,RSPCompilePC));

	if (RSPOpC.del != 0) {
		rsp_UnknownOpcode();
		return;
	}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if (Addr & 15) {
			CompilerWarning("Unaligned SQV at constant address %04X", RSPCompilePC);
			RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SQV,"RSP_Opcode_SQV");
			return;
		}
	
		/*
		 * Aligned store
		 */

		if (IsSseEnabled == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
			RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_EAX);
			sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
			RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_EBX);
			sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
			RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_ECX);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EDX);

			sprintf(Reg, "Dmem+%Xh+0", Addr);
			RSPMoveX86regToVariable(x86_EAX, DMEM + Addr + 0, Reg);
			sprintf(Reg, "Dmem+%Xh+4", Addr);
			RSPMoveX86regToVariable(x86_EBX, DMEM + Addr + 4, Reg);
			sprintf(Reg, "Dmem+%Xh+8", Addr);
			RSPMoveX86regToVariable(x86_ECX, DMEM + Addr + 8, Reg);
			sprintf(Reg, "Dmem+%Xh+C", Addr);
			RSPMoveX86regToVariable(x86_EDX, DMEM + Addr + 12, Reg);
		} else {
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			RSPSseMoveAlignedVariableToReg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_XMM0);
			RSPSseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
			sprintf(Reg, "Dmem+%Xh", Addr);
			RSPSseMoveUnalignedRegToVariable(x86_XMM0, DMEM + Addr, Reg);
		}
		return;
	}
	
	RSPMoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		RSPAddConstToX86Reg(x86_EBX, offset);
	}
	RSPTestConstToX86Reg(15, x86_EBX);
	RSPJneLabel32("Unaligned", 0);
	Jump[0] = RSPRecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	RSPx86_SetBranch32b((DWORD*)Jump[0], (DWORD*)RSPRecompPos);
	RSPCheat_r4300iOpcodeNoMessage(RSP_Opcode_SQV,"RSP_Opcode_SQV");
	RSPJmpLabel32("Done", 0);
	Jump[1] = RSPRecompPos - 4;
	CompilerToggleBuffer();

	RSPAndConstToX86Reg(x86_EBX, 0x0fff);
	if (IsSseEnabled == FALSE) {
		sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_ECX);
		sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_EDX);
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		RSPMoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EDI);

		RSPMoveX86regToN64MemDisp(x86_EAX, x86_EBX, 0);
		RSPMoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);
		RSPMoveX86regToN64MemDisp(x86_EDX, x86_EBX, 8);
		RSPMoveX86regToN64MemDisp(x86_EDI, x86_EBX, 12);
	} else {
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		RSPSseMoveAlignedVariableToReg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_XMM0);
		RSPSseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
		RSPSseMoveUnalignedRegToN64Mem(x86_XMM0, x86_EBX);
	}
	CPU_Message("   Done:");
	RSPx86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RSPRecompPos);
}

void RSPCompile_Opcode_SRV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SRV,"RSP_Opcode_SRV");
}

void RSPCompile_Opcode_SPV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SPV,"RSP_Opcode_SPV");
}

void RSPCompile_Opcode_SUV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SUV,"RSP_Opcode_SUV");
}

void RSPCompile_Opcode_SHV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SHV,"RSP_Opcode_SHV");
}

void RSPCompile_Opcode_SFV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SFV,"RSP_Opcode_SFV");
}

void RSPCompile_Opcode_STV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_STV,"RSP_Opcode_STV");
}

void RSPCompile_Opcode_SWV ( void ) {
	RSPCheat_r4300iOpcode(RSP_Opcode_SWV,"RSP_Opcode_SWV");
}

/************************** Other functions **************************/

void RSPCompile_UnknownOpcode (void) {
	CPU_Message("  %X Unhandled Opcode: %s",RSPCompilePC, RSPOpcodeName(RSPOpC.Hex,RSPCompilePC) );	
	RSPNextInstruction = FINISH_BLOCK;
	RSPMoveConstToVariable(RSPCompilePC,PrgCount,"RSP PC");
	RSPMoveConstToVariable(RSPOpC.Hex,&RSPOpC.Hex, "RSPOpC.Hex");
	RSPCall_Direct(rsp_UnknownOpcode, "rsp_UnknownOpcode" );
	RSPRet();
}
