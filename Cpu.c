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
#include "rsp.h"
#include "audio.h"

int NextInstruction, JumpToLocation, CountPerOp;
char SaveAsFileName[255], LoadFileName[255];
int DlistCount, AlistCount;
enum SaveType SaveUsing;
CPU_ACTION CPU_Action;
SYSTEM_TIMERS Timers;
HANDLE hPauseMutex;
OPCODE Opcode;
HANDLE hCPU;
BOOL CPURunning, SPHack;
DWORD MemoryStack;
int WaitMode=0;

#ifdef CFB_READ
DWORD CFBStart = 0, CFBEnd = 0;
#endif

#ifdef Interpreter_StackTest
DWORD StackValue;
#endif

#ifdef CFB_READ
void __cdecl SetFrameBuffer (DWORD Address, DWORD Length) {
	DWORD NewStart, NewLength, OldProtect;

	NewStart = Address;
	NewLength = Length;

	if (CFBStart != 0) {
		VirtualProtect(N64MEM + CFBStart,CFBEnd - CFBStart,PAGE_READWRITE,&OldProtect);
	}
	if (Length == 0) {
		CFBStart = 0;
		CFBEnd   = 0;
		return;
	}
	CFBStart = Address & ~0xFFF;
	CFBEnd = ((CFBStart + Length + 0xFFC) & ~0xFFF) - 1;
	VirtualProtect(N64MEM + CFBStart,CFBEnd - CFBStart,PAGE_READONLY,&OldProtect);
}
#endif

char *TimeName[MaxTimers] = { "CompareTimer","SiTimer","PiTimer","ViTimer" };

void InitiliazeCPUFlags (void) {
	CPURunning   = FALSE;
	SPHack       = FALSE;
}

void ChangeCompareTimer(void) {
	DWORD NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) {  NextCompare = 0x7FFFFFFF; }
	if (NextCompare == 0) { NextCompare = 0x1; }
	ChangeTimer(CompareTimer,NextCompare);
}

void ChangeTimer(int Type, int Value) {
	if (Value == 0) {
		Timers.NextTimer[Type] = 0;
		Timers.Active[Type] = FALSE;
		return;
	}
	Timers.NextTimer[Type] = Value - Timers.Timer;
	Timers.Active[Type] = TRUE;
	CheckTimer();
}

void CheckTimer (void) {
	int count;

	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (!(count == CompareTimer && Timers.NextTimer[count] == 0x7FFFFFFF)) {
			Timers.NextTimer[count] += Timers.Timer;
		}
	}
	Timers.CurrentTimerType = -1;
	Timers.Timer = 0x7FFFFFFF;
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (Timers.NextTimer[count] >= Timers.Timer) { continue; }
		Timers.Timer = Timers.NextTimer[count];
		Timers.CurrentTimerType = count;
	}
	if (Timers.CurrentTimerType == -1) {
		DisplayError("No active timers ???\nEmulation Stoped");
		ExitThread(0);
	}
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (!(count == CompareTimer && Timers.NextTimer[count] == 0x7FFFFFFF)) {
			Timers.NextTimer[count] -= Timers.Timer;
		}
	}

	if (Timers.NextTimer[CompareTimer] == 0x7FFFFFFF) {
		DWORD NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
		if ((NextCompare & 0x80000000) == 0 && NextCompare != 0x7FFFFFFF) {
			ChangeCompareTimer();
		}
	}
}

void CloseCpu (void) {
	DWORD ExitCode, count, OldProtect;

	if (!CPURunning) { return; }

	for (count = 0; count < 2; count ++ ) {
		CPU_Action.CloseCPU = TRUE;
		CPU_Action.DoSomething = TRUE;
		Sleep(100);
		GetExitCodeThread(hCPU,&ExitCode);
		if (ExitCode != STILL_ACTIVE) {
			hCPU = NULL;
			break;
		}
	}
	if (hCPU != NULL) {
		DisplayError("CPU Thread did not stop, terminating");
		TerminateThread(hCPU,0); hCPU = NULL;
	}
	CPURunning = FALSE;
	VirtualProtect(N64MEM,RdramSize,PAGE_READWRITE,&OldProtect);
	VirtualProtect(N64MEM + 0x04000000,0x2000,PAGE_READWRITE,&OldProtect);
	AiClose();
	RSPRomClosed();
}

int DelaySlotEffectsCompare (DWORD PC, DWORD Reg1, DWORD Reg2) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(PC + 4, &Command.Hex)) {
		//DisplayError("Failed to load word 2");
		//ExitThread(0);
		return TRUE;
	}

	if (SelfModCheck == ModCode_ChangeMemory) {
		if ( (Command.Hex >> 16) == 0x7C7C) {
			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
		}
	}

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_SLL:
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (Command.rd == 0) { return FALSE; }
			if (Command.rd == Reg1) { return TRUE; }
			if (Command.rd == Reg2) { return TRUE; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			break;
		default:
#ifndef EXTERNAL_RELEASE
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_CP0:
		switch (Command.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		default:
			if ( (Command.rs & 0x10 ) != 0 ) {
				switch( Opcode.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default:
#ifndef EXTERNAL_RELEASE
					DisplayError("Does %s effect Delay slot at %X?\n6",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
					return TRUE;
				}
			} else {
#ifndef EXTERNAL_RELEASE
				DisplayError("Does %s effect Delay slot at %X?\n7",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
				return TRUE;
			}
		}
		break;
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (Command.rt == 0) { return FALSE; }
		if (Command.rt == Reg1) { return TRUE; }
		if (Command.rt == Reg2) { return TRUE; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	default:
#ifndef EXTERNAL_RELEASE
		DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
		return TRUE;
	}
	return FALSE;
}

int DelaySlotEffectsJump (DWORD JumpPC) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(JumpPC, &Command.Hex)) { return TRUE; }
	if (SelfModCheck == ModCode_ChangeMemory) {
		if ( (Command.Hex >> 16) == 0x7C7C) {
			Command.Hex = OrigMem[(Command.Hex & 0xFFFF)].OriginalValue;
		}
	}

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_JR:	return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,Command.rs,31);
		}
		break;
	case R4300i_REGIMM:
		switch (Command.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
			return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		}
		break;
	case R4300i_JAL:
	case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,31,0); break;
	case R4300i_J: return FALSE;
	case R4300i_BEQ:
	case R4300i_BNE:
	case R4300i_BLEZ:
	case R4300i_BGTZ:
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_BC:
			switch (Command.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				{
					int EffectDelaySlot;
					OPCODE NewCommand;

					if (!r4300i_LW_VAddr(JumpPC + 4, &NewCommand.Hex)) { return TRUE; }

					EffectDelaySlot = FALSE;
					if (NewCommand.op == R4300i_CP1) {
						if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						}
						if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						}
					}
					return EffectDelaySlot;
				}
				break;
			}
			break;
		}
		break;
	case R4300i_BEQL:
	case R4300i_BNEL:
	case R4300i_BLEZL:
	case R4300i_BGTZL:
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	}
	return TRUE;
}

void ProcessMessages (void) {
	HANDLE hEvent;
	MSG msg;

	hEvent =  CreateEvent(NULL,FALSE,FALSE,NULL);
	MsgWaitForMultipleObjects(1,&hEvent,FALSE,1000,QS_ALLINPUT);
	CloseHandle(hEvent);
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			PostMessage(msg.hwnd,msg.message,msg.wParam,msg.lParam);
			return;
		}
	}
}

void DoSomething ( void ) {
	if (CPU_Action.CloseCPU) {
		ExitThread(0);
	}
	if (CPU_Action.CheckInterrupts) {
		CPU_Action.CheckInterrupts = FALSE;
		CheckInterrupts();
	}
	if (CPU_Action.DoInterrupt) {
		CPU_Action.DoInterrupt = FALSE;
		DoIntrException(FALSE);
	}

	CPU_Action.DoSomething = FALSE;

	if (CPU_Action.DoInterrupt == TRUE) { CPU_Action.DoSomething = TRUE; }
}

void InPermLoop (void) {
	// *** Changed ***/
	if (CPU_Action.DoInterrupt) { return; }

	//Timers.Timer -= 5;
	//COUNT_REGISTER +=5;

	/* Interrupts enabled */
	if (( STATUS_REGISTER & STATUS_IE  ) == 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_EXL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_ERL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & 0xFF00) == 0) { goto InterruptsDisabled; }

	/* check sound playing */
	//if (AiReadLength() != 0) { return; }

	/* check RSP running */
	/* check RDP running */
	if (Timers.Timer > 0) {
		COUNT_REGISTER += Timers.Timer + 1;
		Timers.Timer = -1;
	}
	return;

InterruptsDisabled:
	//if (UpdateScreen != NULL) { UpdateScreen(); }
	DisplayError(GS(MSG_PERM_LOOP));
	ExitThread(0);
}

void ReadFromMem(const void * source, void * target, DWORD length, DWORD *offset) {
	memcpy((BYTE*)target,((BYTE*)source)+*offset,length);
	*offset+=length;
}

BOOL Machine_LoadStateFromRAM(void * savestatespace) {
	char LoadHeader[64];
	DWORD Value, count, SaveRDRAMSize, offset=0;

	// not ZIP
	{

		ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
		if (Value != 0x23D8A6C8) { return FALSE; }
		ReadFromMem( savestatespace,&SaveRDRAMSize,sizeof(SaveRDRAMSize),&offset);
		ReadFromMem( savestatespace,&LoadHeader,0x40,&offset);

		if (CPU_Type != CPU_Interpreter) {
			ResetRecompCode();
		}

		Timers.CurrentTimerType = -1;
		Timers.Timer = 0;
		for (count = 0; count < MaxTimers; count ++) { Timers.Active[count] = FALSE; }

		//fix rdram size
		if (SaveRDRAMSize != RdramSize) {
			if (RdramSize == 0x400000) {
				if (VirtualAlloc(N64MEM + 0x400000, 0x400000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
					DisplayError("Failed to Extend memory to 8mb");
					ExitThread(0);
				}
				if (VirtualAlloc((BYTE *)JumpTable + 0x400000, 0x400000, MEM_COMMIT, PAGE_READWRITE)==NULL) {
					DisplayError("Failed to Extend Jump Table to 8mb");
					ExitThread(0);
				}
				if (VirtualAlloc((BYTE *)DelaySlotTable + (0x400000 >> 0xA), (0x400000 >> 0xA), MEM_COMMIT, PAGE_READWRITE)==NULL) {
					DisplayError("Failed to Extend Delay Slot Table to 8mb");
					ExitThread(0);
				}
			} else {
				VirtualFree(N64MEM + 0x400000, 0x400000,MEM_DECOMMIT);
				VirtualFree((BYTE *)JumpTable + 0x400000, 0x400000,MEM_DECOMMIT);
				VirtualFree((BYTE *)DelaySlotTable + (0x400000 >> 0xA), (0x400000 >> 0xA),MEM_DECOMMIT);
			}
		}
		RdramSize = SaveRDRAMSize;

		ReadFromMem( savestatespace,&Value,sizeof(Value),&offset);
		ChangeTimer(ViTimer,Value);
		ReadFromMem( savestatespace,&PROGRAM_COUNTER,sizeof(PROGRAM_COUNTER),&offset);
		ReadFromMem( savestatespace,GPR,sizeof(_int64)*32,&offset);
		ReadFromMem( savestatespace,FPR,sizeof(_int64)*32,&offset);
		ReadFromMem( savestatespace,CP0,sizeof(DWORD)*32,&offset);
		ReadFromMem( savestatespace,FPCR,sizeof(DWORD)*32,&offset);
		ReadFromMem( savestatespace,&HI,sizeof(_int64),&offset);
		ReadFromMem( savestatespace,&LO,sizeof(_int64),&offset);
		ReadFromMem( savestatespace,RegRDRAM,sizeof(DWORD)*10,&offset);
		ReadFromMem( savestatespace,RegSP,sizeof(DWORD)*10,&offset);
		ReadFromMem( savestatespace,RegDPC,sizeof(DWORD)*10,&offset);
		ReadFromMem( savestatespace,RegMI,sizeof(DWORD)*4,&offset);
		ReadFromMem( savestatespace,RegVI,sizeof(DWORD)*14,&offset);
		ReadFromMem( savestatespace,RegAI,sizeof(DWORD)*6,&offset);
		ReadFromMem( savestatespace,RegPI,sizeof(DWORD)*13,&offset);
		ReadFromMem( savestatespace,RegRI,sizeof(DWORD)*8,&offset);
		ReadFromMem( savestatespace,RegSI,sizeof(DWORD)*4,&offset);
		ReadFromMem( savestatespace,tlb,sizeof(TLB)*32,&offset);
		ReadFromMem( savestatespace,PIF_Ram,0x40,&offset);
		ReadFromMem( savestatespace,RDRAM,SaveRDRAMSize,&offset);
		ReadFromMem( savestatespace,DMEM,0x1000,&offset);
		ReadFromMem( savestatespace,IMEM,0x1000,&offset);

		CP0[32]=0;

	}
	//memcpy(RomHeader,ROMPages[0],sizeof(RomHeader));
	ChangeCompareTimer();
	//AiClose();
	//RSPRomClosed();
	DlistCount = 0;
	AlistCount = 0;
	AI_STATUS_REG = 0;
	AiDacrateChanged(SYSTEM_NTSC);
	SetupTLB();
	StartAiInterrupt();

	SetFpuLocations(); // important if FR=1

	//Fix up Memory stack location
	MemoryStack = GPR[29].W[0];
	TranslateVaddr(&MemoryStack);
	MemoryStack += (DWORD)N64MEM;

	CheckInterrupts();
	DMAUsed = TRUE;
	strcpy(SaveAsFileName,"");
	strcpy(LoadFileName,"");

#ifdef Log_x86Code
	Stop_x86_Log();
	Start_x86_Log();
#endif
#ifndef EXTERNAL_RELEASE
	StopLog();
	StartLog();
#endif
	return TRUE;
}

void RefreshScreen (void ){
	static DWORD OLD_VI_V_SYNC_REG = 0, VI_INTR_TIME = 500000;

	if (OLD_VI_V_SYNC_REG != VI_V_SYNC_REG) {
		if (VI_V_SYNC_REG == 0) {
			VI_INTR_TIME = 500000;
		} else {
			VI_INTR_TIME = (VI_V_SYNC_REG + 1) * 1500;
			if ((VI_V_SYNC_REG % 1) != 0) {
				VI_INTR_TIME -= 38;
			}
		}
	}
	ChangeTimer(ViTimer,Timers.Timer + Timers.NextTimer[ViTimer] + VI_INTR_TIME);

	if ((VI_STATUS_REG & 0x10) != 0) {
		if (ViFieldNumber == 0) {
			ViFieldNumber = 1;
		} else {
			ViFieldNumber = 0;
		}
	} else {
		ViFieldNumber = 0;
	}

}

void RunRsp (void) {
	if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
		if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
			DWORD Task = *( DWORD *)(DMEM + 0xFC0);

			/*if (Task == 1 && (DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0)
			{
				return;
			}*/

			switch (Task) {
			case 1:
				DlistCount += 1;
				/*if ((DlistCount % 2) == 0) {
					SP_STATUS_REG |= (0x0203 );
					MI_INTR_REG |= MI_INTR_SP | MI_INTR_DP;
					CheckInterrupts();
					return;
				}*/
				//DisplayError("dlist");
				break;
			case 2:
				AlistCount += 1;
				//DisplayError("alist");
				break;
			}

			DoRspCycles(100);
#ifdef CFB_READ
			if (VI_ORIGIN_REG > 0x280) {
				SetFrameBuffer(VI_ORIGIN_REG, (DWORD)(VI_WIDTH_REG * (VI_WIDTH_REG *.75)));
			}
#endif
		}
	}
}

void StartEmulationFromSave ( void * savestate ) {
	DWORD ThreadID, count;
	CloseCpu();

	memset(&CPU_Action,0,sizeof(CPU_Action));
	//memcpy(RomHeader,ROMPages[0],sizeof(RomHeader));
	WrittenToRom = FALSE;

	InitilizeTLB();
	//InitalizeR4300iRegisters(LoadPifRom(*(ROMPages[0] + 0x3D)),*(ROM + 0x3D),GetCicChipID(ROM));
	InitalizeR4300iRegisters();

	BuildInterpreter();

	RecompPos = RecompCode;

    DlistCount = 0;
	AlistCount = 0;

	Timers.CurrentTimerType = -1;
	Timers.Timer = 0;
	for (count = 0; count < MaxTimers; count ++) { Timers.Active[count] = FALSE; }
	ChangeTimer(ViTimer,5000);
	ChangeCompareTimer();
	ViFieldNumber = 0;
	DMAUsed = FALSE;
	strcpy(LoadFileName,"");
	strcpy(SaveAsFileName,"");
	CPURunning = TRUE;

	WaitMode=0;

	Machine_LoadStateFromRAM(savestate);

	switch (CPU_Type) {
	case CPU_Interpreter: hCPU = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartInterpreterCPU,NULL,0, &ThreadID); break;
	case CPU_Recompiler: hCPU = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartRecompilerCPU,NULL,0, &ThreadID);	break;
	default:
		DisplayError("Unhandled CPU %d",CPU_Type);
	}

	SetThreadPriority(hCPU, priarray[CPUPriority]);
}

void TimerDone (void) {
	switch (Timers.CurrentTimerType) {
	case CompareTimer:
		// this timer must be agknowledged, even with compare int disabled,
		// otherwise we get stuck in an endless loop here
		if (EnableCompare) FAKE_CAUSE_REGISTER |= CAUSE_IP7;
		CheckInterrupts();
		ChangeCompareTimer();
		break;
	case SiTimer:
		ChangeTimer(SiTimer,0);
		MI_INTR_REG |= MI_INTR_SI;
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		CheckInterrupts();
		break;
	case PiTimer:
		ChangeTimer(PiTimer,0);
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		break;
	case ViTimer:
		RefreshScreen();
		MI_INTR_REG |= MI_INTR_VI;
		CheckInterrupts();
		WaitMode=0;
		break;
	case RspTimer:
		ChangeTimer(RspTimer,0);
		RunRsp();
		break;
	case AiTimer:
         /*
		//AI_STATUS_REG&=~0x80000000;
		if (AI_STATUS_REG&0x80000000) {
			AI_STATUS_REG&=~0x80000000;
			StartAiInterrupt(); // since we were full another sample is playing now,
			                    // assume it is of the same length
		} else {
			AI_STATUS_REG&=~0x40000000;
			ChangeTimer(AiTimer,0);
		}
		AudioIntrReg|=4;
		CheckInterrupts();
        */
        AI_STATUS_REG=0;
		ChangeTimer(AiTimer,0);
        AudioIntrReg|=4;
		CheckInterrupts();
		break;
	}
	CheckTimer();
}
