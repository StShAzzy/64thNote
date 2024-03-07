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
#include "RSP x86.h"
#include "RSP memory.h"
#include "RSP registers.h"
#include "main.h"
#include "recompiler cpu.h"
#include "memory.h"
#include "cpu log.h"

#define PUTDST8(dest,value)  (*((BYTE *)(dest))=(BYTE)(value)); dest += 1;
#define PUTDST16(dest,value) (*((WORD *)(dest))=(WORD)(value)); dest += 2;
#define PUTDST32(dest,value) (*((DWORD *)(dest))=(DWORD)(value)); dest += 4;

char * x86_Strings[8] = {
	"eax", "ebx", "ecx", "edx", 
	"esi", "edi", "ebp", "esp"
};

char * x86_ByteStrings[8] = {
	"al", "bl", "cl", "dl", 
	"?4", "?5", "?6", "?7"
};

char * x86_HalfStrings[8] = {
	"ax", "bx", "cx", "dx", 
	"si", "di", "bp", "sp"
};

extern BOOL ConditionalMove;

#define x86Byte_Name(Reg) (x86_ByteStrings[(Reg)])
#define x86Half_Name(Reg) (x86_HalfStrings[(Reg)])

void RSPAdcX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      adc %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x0013; break;
	case x86_EBX: x86Command = 0x0313; break;
	case x86_ECX: x86Command = 0x0113; break;
	case x86_EDX: x86Command = 0x0213; break;
	case x86_ESI: x86Command = 0x0613; break;
	case x86_EDI: x86Command = 0x0713; break;
	case x86_ESP: x86Command = 0x0413; break;
	case x86_EBP: x86Command = 0x0513; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPAdcX86regToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      adc dword ptr [%s], %s",VariableName, x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0511); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D11); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D11); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1511); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3511); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D11); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2511); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D11); break;	
	default:
		DisplayError("AddVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPAdcX86regHalfToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      adc word ptr [%s], %s",VariableName, x86Half_Name(x86reg));

	PUTDST8(RSPRecompPos, 0x66);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0511); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D11); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D11); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1511); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3511); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D11); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2511); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D11); break;	
	default:
		DisplayError("AdcX86regHalfToVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPAdcConstToVariable(void *Variable, char *VariableName, BYTE Constant) {
	CPU_Message("      adc dword ptr [%s], %Xh", VariableName, Constant);
	PUTDST16(RSPRecompPos,0x1583);
    PUTDST32(RSPRecompPos,Variable);
	PUTDST8(RSPRecompPos,Constant);
}

void RSPAdcConstToX86reg( BYTE Constant, int x86reg ) {
	CPU_Message("      adc %s, %Xh",x86_Name(x86reg), Constant);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xD083); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xD383); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xD183); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xD283); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xD683); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xD783); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xD483); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xD583); break;
	default:
		DisplayError("AdcConstantToX86reg\nUnknown x86 Register");
	}
    PUTDST8(RSPRecompPos,Constant); 
}

void RSPAddConstToVariable (DWORD Const, void *Variable, char *VariableName) {
	CPU_Message("      add dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(RSPRecompPos,0x0581);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPAddConstToX86Reg (int x86Reg, DWORD Const) {
	CPU_Message("      add %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xC081); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xC381); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xC181); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xC281); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xC681); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xC781); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xC481); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xC581); break;
		}
		PUTDST32(RSPRecompPos, Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xC083); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xC383); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xC183); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xC283); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xC683); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xC783); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xC483); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xC583); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPAdcConstHalfToVariable(void *Variable, char *VariableName, BYTE Constant) {
	CPU_Message("      adc word ptr [%s], %Xh", VariableName, Constant);
	
	PUTDST8(RSPRecompPos,0x66);
	PUTDST8(RSPRecompPos,0x83);
	PUTDST8(RSPRecompPos,0x15);

    PUTDST32(RSPRecompPos,Variable);

	PUTDST8(RSPRecompPos,Constant);
}

void RSPAddVariableToX86reg(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      add %s, dword ptr [%s]",x86_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0503); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D03); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D03); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1503); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3503); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D03); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2503); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D03); break;
	default:
		DisplayError("AddVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPAddX86regToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      add dword ptr [%s], %s",VariableName, x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0501); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D01); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D01); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1501); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3501); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D01); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2501); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D01); break;
	default:
		DisplayError("AddVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPAddX86regHalfToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      add word ptr [%s], %s",VariableName, x86Half_Name(x86reg));

	PUTDST8(RSPRecompPos,0x66);

	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0501); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D01); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D01); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1501); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3501); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D01); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2501); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D01); break;
	default:
		DisplayError("AddVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPAddX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      add %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x0003; break;
	case x86_EBX: x86Command = 0x0303; break;
	case x86_ECX: x86Command = 0x0103; break;
	case x86_EDX: x86Command = 0x0203; break;
	case x86_ESI: x86Command = 0x0603; break;
	case x86_EDI: x86Command = 0x0703; break;
	case x86_ESP: x86Command = 0x0403; break;
	case x86_EBP: x86Command = 0x0503; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPAndConstToVariable (DWORD Const, void *Variable, char *VariableName) {
	CPU_Message("      and dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(RSPRecompPos,0x2581);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPAndConstToX86Reg(int x86Reg, DWORD Const) {
	CPU_Message("      and %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xE081); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xE381); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xE181); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xE281); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xE681); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xE781); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xE481); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xE581); break;
		}
		PUTDST32(RSPRecompPos, Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xE083); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xE383); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xE183); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xE283); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xE683); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xE783); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xE483); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xE583); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPAndVariableToX86Reg(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      and %s, dword ptr [%s]",x86_Name(x86Reg),VariableName);
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0523); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D23); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D23); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1523); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3523); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D23); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2523); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D23); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPAndVariableToX86regHalf(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      and %s, word ptr [%s]",x86Half_Name(x86Reg),VariableName);
	PUTDST8(RSPRecompPos, 0x66);
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0523); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D23); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D23); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1523); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3523); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D23); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2523); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D23); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPAndX86RegToVariable(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      and dword ptr [%s], %s", VariableName, x86_Name(x86Reg));
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0521); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D21); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D21); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1521); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3521); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D21); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2521); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D21); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPAndX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      and %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Destination) {
	case x86_EAX: x86Command = 0x0021; break;
	case x86_EBX: x86Command = 0x0321; break;
	case x86_ECX: x86Command = 0x0121; break;
	case x86_EDX: x86Command = 0x0221; break;
	case x86_ESI: x86Command = 0x0621; break;
	case x86_EDI: x86Command = 0x0721; break;
	case x86_ESP: x86Command = 0x0421; break;
	case x86_EBP: x86Command = 0x0521; break;
	}
	switch (Source) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPAndX86RegHalfToX86RegHalf(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      and %s, %s",x86Half_Name(Destination),x86Half_Name(Source));
	PUTDST8(RSPRecompPos, 0x66);

	switch (Destination) {
	case x86_EAX: x86Command = 0x0021; break;
	case x86_EBX: x86Command = 0x0321; break;
	case x86_ECX: x86Command = 0x0121; break;
	case x86_EDX: x86Command = 0x0221; break;
	case x86_ESI: x86Command = 0x0621; break;
	case x86_EDI: x86Command = 0x0721; break;
	case x86_ESP: x86Command = 0x0421; break;
	case x86_EBP: x86Command = 0x0521; break;
	}
	switch (Source) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPCall_Direct(void * FunctAddress, char * FunctName) {
	CPU_Message("      call offset %s",FunctName);
	PUTDST8(RSPRecompPos,0xE8);
	PUTDST32(RSPRecompPos,(DWORD)FunctAddress-(DWORD)RSPRecompPos - 4);
}

void RSPCall_Indirect(void * FunctAddress, char * FunctName) {
	CPU_Message("      call [%s]",FunctName);
	PUTDST16(RSPRecompPos,0x15FF);
	PUTDST32(RSPRecompPos,FunctAddress);
}

void RSPCondMoveEqual(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("   [*]cmove %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJneLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmove %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x440F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCondMoveNotEqual(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("   [*]cmovne %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJeLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmovne %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x450F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCondMoveGreater(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("    [*]cmovg %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJleLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmovg %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x4F0F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCondMoveGreaterEqual(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("   [*]cmovge %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJlLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmovge %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x4D0F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCondMoveLess(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("   [*]cmovl %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJgeLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmovl %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x4C0F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCondMoveLessEqual(int Destination, int Source) {
	if (ConditionalMove == FALSE) {
		BYTE * Jump;
		CPU_Message("   [*]cmovle %s, %s",x86_Name(Destination),x86_Name(Source));

		RSPJgLabel8("label", 0);
		Jump = RSPRecompPos - 1;
		RSPMoveX86RegToX86Reg(Source, Destination);
		CPU_Message("     label:");
		RSPx86_SetBranch8b(Jump, RSPRecompPos);
	} else {
		BYTE x86Command;
		CPU_Message("      cmovle %s, %s",x86_Name(Destination),x86_Name(Source));

		PUTDST16(RSPRecompPos,0x4E0F);

		switch (Source) {
		case x86_EAX: x86Command = 0x00; break;
		case x86_EBX: x86Command = 0x03; break;
		case x86_ECX: x86Command = 0x01; break;
		case x86_EDX: x86Command = 0x02; break;
		case x86_ESI: x86Command = 0x06; break;
		case x86_EDI: x86Command = 0x07; break;
		case x86_ESP: x86Command = 0x04; break;
		case x86_EBP: x86Command = 0x05; break;
		}

		switch (Destination) {
		case x86_EAX: x86Command += 0xC0; break;
		case x86_EBX: x86Command += 0xD8; break;
		case x86_ECX: x86Command += 0xC8; break;
		case x86_EDX: x86Command += 0xD0; break;
		case x86_ESI: x86Command += 0xF0; break;
		case x86_EDI: x86Command += 0xF8; break;
		case x86_ESP: x86Command += 0xE0; break;
		case x86_EBP: x86Command += 0xE8; break;
		}

		PUTDST8(RSPRecompPos, x86Command);
	}
}

void RSPCompConstToVariable(DWORD Const, void * Variable, char * VariableName) {
	CPU_Message("      cmp dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(RSPRecompPos,0x3D81);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPCompConstHalfToVariable(WORD Const, void * Variable, char * VariableName) {
	CPU_Message("      cmp word ptr [%s], 0x%X",VariableName, Const);
	PUTDST8(RSPRecompPos,0x66);
	PUTDST8(RSPRecompPos,0x81);
	PUTDST8(RSPRecompPos,0x3D);

	PUTDST32(RSPRecompPos,Variable);
	PUTDST16(RSPRecompPos,Const);
}

void RSPCompConstToX86reg(int x86Reg, DWORD Const) {
	CPU_Message("      cmp %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xF881); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xFB81); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xF981); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xFA81); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xFE81); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xFF81); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xFC81); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xFD81); break;
		default:
			DisplayError("CompConstToX86reg\nUnknown x86 Register");
		}
		PUTDST32(RSPRecompPos,Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xF883); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xFB83); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xF983); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xFA83); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xFE83); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xFF83); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xFC83); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xFD83); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPCompX86regToVariable(int x86Reg, void * Variable, char * VariableName) {
	CPU_Message("      cmp %s, dword ptr [%s]",x86_Name(x86Reg),VariableName);
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x053B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D3B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D3B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x153B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x353B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D3B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x253B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D3B); break;
	default:
		DisplayError("Unknown x86 Register");
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPCompVariableToX86reg(int x86Reg, void * Variable, char * VariableName) {
	CPU_Message("      cmp dword ptr [%s], %s",VariableName, x86_Name(x86Reg));
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0539); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D39); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D39); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1539); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3539); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D39); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2539); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D39); break;
	default:
		DisplayError("Unknown x86 Register");
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPCompX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      cmp %s, %s",x86_Name(Destination),x86_Name(Source));
	
	switch (Source) {
	case x86_EAX: x86Command = 0x003B; break;
	case x86_EBX: x86Command = 0x033B; break;
	case x86_ECX: x86Command = 0x013B; break;
	case x86_EDX: x86Command = 0x023B; break;
	case x86_ESI: x86Command = 0x063B; break;
	case x86_EDI: x86Command = 0x073B; break;
	case x86_ESP: x86Command = 0x043B; break;
	case x86_EBP: x86Command = 0x053B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPCwd(void) {
	CPU_Message("      cwd");
	PUTDST16(RSPRecompPos, 0x9966);
}

void RSPCwde(void) {
	CPU_Message("      cwde");
	PUTDST8(RSPRecompPos, 0x98);
}

void RSPDecX86reg(x86Reg) {
	CPU_Message("      dec %s",x86_Name(x86Reg));
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xC8FF); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xCBFF); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xC9FF); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xCAFF); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xCEFF); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xCFFF); break;
	case x86_ESP: PUTDST8 (RSPRecompPos,0x4C);   break;
	case x86_EBP: PUTDST8 (RSPRecompPos,0x4D);   break;
	default:
		DisplayError("DecX86reg\nUnknown x86 Register");
	}
}

void RSPDivX86reg(int x86reg) {
	CPU_Message("      div %s",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EBX: PUTDST16(RSPRecompPos,0xf3F7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xf1F7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xf2F7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xf6F7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xf7F7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xf4F7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xf5F7); break;
	default:
		DisplayError("divX86reg\nUnknown x86 Register");
	}
}

void RSPidivX86reg(int x86reg) {
	CPU_Message("      idiv %s",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EBX: PUTDST16(RSPRecompPos,0xfbF7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xf9F7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xfaF7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xfeF7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xffF7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xfcF7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xfdF7); break;
	default:
		DisplayError("idivX86reg\nUnknown x86 Register");
	}
}

void RSPimulX86reg(int x86reg) {
	CPU_Message("      imul %s",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE8F7); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xEBF7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE9F7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xEAF7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xEEF7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xEFF7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xECF7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xEDF7); break;
	default:
		DisplayError("imulX86reg\nUnknown x86 Register");
	}
}

void RSPImulX86RegToX86Reg(int Destination, int Source) {
	BYTE x86Command;

	CPU_Message("      imul %s, %s",x86_Name(Destination), x86_Name(Source));

	switch (Source) {
	case x86_EAX: x86Command = 0x00; break;
	case x86_EBX: x86Command = 0x03; break;
	case x86_ECX: x86Command = 0x01; break;
	case x86_EDX: x86Command = 0x02; break;
	case x86_ESI: x86Command = 0x06; break;
	case x86_EDI: x86Command = 0x07; break;
	case x86_ESP: x86Command = 0x04; break;
	case x86_EBP: x86Command = 0x05; break;
	}

	switch (Destination) {
	case x86_EAX: x86Command += 0xC0; break;
	case x86_EBX: x86Command += 0xD8; break;
	case x86_ECX: x86Command += 0xC8; break;
	case x86_EDX: x86Command += 0xD0; break;
	case x86_ESI: x86Command += 0xF0; break;
	case x86_EDI: x86Command += 0xF8; break;
	case x86_ESP: x86Command += 0xE0; break;
	case x86_EBP: x86Command += 0xE8; break;
	}
	
	PUTDST16(RSPRecompPos, 0xAF0F);
	PUTDST8(RSPRecompPos, x86Command);
}

void RSPIncX86reg(int x86Reg) {
	CPU_Message("      inc %s",x86_Name(x86Reg));
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xC0FF); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xC3FF); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xC1FF); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xC2FF); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xC6FF); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xC7FF); break;
	case x86_ESP: PUTDST8 (RSPRecompPos,0x44);   break;
	case x86_EBP: PUTDST8 (RSPRecompPos,0x45);   break;
	default:
		DisplayError("IncX86reg\nUnknown x86 Register");
	}
}

void RSPJaeLabel32(char * Label,DWORD Value) {
	CPU_Message("      jae $%s",Label);
	PUTDST16(RSPRecompPos,0x830F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJaLabel8(char * Label, BYTE Value) {
	CPU_Message("      ja $%s",Label);
	PUTDST8(RSPRecompPos,0x77);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJaLabel32(char * Label,DWORD Value) {
	CPU_Message("      ja $%s",Label);
	PUTDST16(RSPRecompPos,0x870F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJbLabel8(char * Label, BYTE Value) {
	CPU_Message("      jb $%s",Label);
	PUTDST8(RSPRecompPos,0x72);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJbLabel32(char * Label,DWORD Value) {
	CPU_Message("      jb $%s",Label);
	PUTDST16(RSPRecompPos,0x820F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJeLabel8(char * Label, BYTE Value) {
	CPU_Message("      je $%s",Label);
	PUTDST8(RSPRecompPos,0x74);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJeLabel32(char * Label,DWORD Value) {
	CPU_Message("      je $%s",Label);
	PUTDST16(RSPRecompPos,0x840F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJgeLabel8(char * Label, BYTE Value) {
	CPU_Message("      jge $%s",Label);
	PUTDST8(RSPRecompPos,0x7D);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJgeLabel32(char * Label,DWORD Value) {
	CPU_Message("      jge $%s",Label);
	PUTDST16(RSPRecompPos,0x8D0F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJgLabel8(char * Label, BYTE Value) {
	CPU_Message("      jg $%s",Label);
	PUTDST8(RSPRecompPos,0x7F);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJgLabel32(char * Label,DWORD Value) {
	CPU_Message("      jg $%s",Label);
	PUTDST16(RSPRecompPos,0x8F0F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJleLabel8(char * Label, BYTE Value) {
	CPU_Message("      jle $%s",Label);
	PUTDST8(RSPRecompPos,0x7E);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJleLabel32(char * Label,DWORD Value) {
	CPU_Message("      jle $%s",Label);
	PUTDST16(RSPRecompPos,0x8E0F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJlLabel8(char * Label, BYTE Value) {
	CPU_Message("      jl $%s",Label);
	PUTDST8(RSPRecompPos,0x7C);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJlLabel32(char * Label,DWORD Value) {
	CPU_Message("      jl $%s",Label);
	PUTDST16(RSPRecompPos,0x8C0F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJumpX86Reg( int x86reg ) {
	CPU_Message("      jmp %s",x86_Name(x86reg));

	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xe0ff); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xe3ff); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xe1ff); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xe2ff); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xe6ff); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xe7ff); break;
	default: DisplayError("JumpX86Reg: Unknown reg");
	}
}

void RSPJmpLabel8(char * Label, BYTE Value) {
	CPU_Message("      jmp $%s",Label);
	PUTDST8(RSPRecompPos,0xEB);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJmpLabel32(char * Label, DWORD Value) {
	CPU_Message("      jmp $%s",Label);
	PUTDST8(RSPRecompPos,0xE9);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJneLabel8(char * Label, BYTE Value) {
	CPU_Message("      jne $%s",Label);
	PUTDST8(RSPRecompPos,0x75);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJneLabel32(char *Label, DWORD Value) {
	CPU_Message("      jne $%s",Label);
	PUTDST16(RSPRecompPos,0x850F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJnsLabel8(char * Label, BYTE Value) {
	CPU_Message("      jns $%s",Label);
	PUTDST8(RSPRecompPos,0x79);
	PUTDST8(RSPRecompPos,Value);
}

void RSPJnsLabel32(char *Label, DWORD Value) {
	CPU_Message("      jns $%s",Label);
	PUTDST16(RSPRecompPos,0x890F);
	PUTDST32(RSPRecompPos,Value);
}

void RSPJsLabel32(char *Label, DWORD Value) {
	CPU_Message("      js $%s",Label);
	PUTDST16(RSPRecompPos,0x880F);
	PUTDST32(RSPRecompPos,Value);
}

/* 
** NOTE: this op can get really complex with muls
** if we need this rewrite it into 1 function
**/

void RSPLeaSourceAndOffset(int x86DestReg, int x86SourceReg, int offset) {
	WORD x86Command;

	CPU_Message("      lea %s, [%s + %0Xh]",x86_Name(x86DestReg),x86_Name(x86SourceReg),offset);
	switch (x86DestReg) {
	case x86_EAX: x86Command = 0x808D; break;
	case x86_EBX: x86Command = 0x988D; break;
	case x86_ECX: x86Command = 0x888D; break;
	case x86_EDX: x86Command = 0x908D; break;
	case x86_ESI: x86Command = 0xB08D; break;
	case x86_EDI: x86Command = 0xB88D; break;
	case x86_ESP: x86Command = 0xA08D; break;
	case x86_EBP: x86Command = 0xA88D; break;
	default:
		DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
	}
	switch (x86SourceReg) {
	case x86_EAX: x86Command += 0x0000; break;
	case x86_EBX: x86Command += 0x0300; break;
	case x86_ECX: x86Command += 0x0100; break;
	case x86_EDX: x86Command += 0x0200; break;
	case x86_ESI: x86Command += 0x0600; break;
	case x86_EDI: x86Command += 0x0700; break;
	case x86_ESP: x86Command += 0x0400; break;
	case x86_EBP: x86Command += 0x0500; break;
	default:
		DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
	}

	if ((offset & 0xFFFFFF80) != 0 && (offset & 0xFFFFFF80) != 0xFFFFFF80) {
		PUTDST16(RSPRecompPos,x86Command);
		PUTDST32(RSPRecompPos,offset);
	} else {
		PUTDST16(RSPRecompPos,(x86Command & 0x7FFF) | 0x4000);
		PUTDST8(RSPRecompPos,offset);
	}
}

void RSPMoveConstByteToVariable (BYTE Const,void *Variable, char *VariableName) {
	CPU_Message("      mov byte ptr [%s], %Xh",VariableName,Const);
	PUTDST16(RSPRecompPos,0x05C6);
    PUTDST32(RSPRecompPos,Variable);
    PUTDST8(RSPRecompPos,Const);
}

void RSPMoveConstHalfToVariable (WORD Const,void *Variable, char *VariableName) {
	CPU_Message("      mov word ptr [%s], %Xh",VariableName,Const);
	PUTDST8(RSPRecompPos,0x66);
	PUTDST16(RSPRecompPos,0x05C7);
    PUTDST32(RSPRecompPos,Variable);
    PUTDST16(RSPRecompPos,Const);
}

void RSPMoveConstToVariable (DWORD Const,void *Variable, char *VariableName) {
	CPU_Message("      mov dword ptr [%s], %Xh",VariableName,Const);
	PUTDST16(RSPRecompPos,0x05C7);
    PUTDST32(RSPRecompPos,Variable);
    PUTDST32(RSPRecompPos,Const);
}

void RSPMoveConstToX86reg(DWORD Const, int x86reg) {
	CPU_Message("      mov %s, %Xh",x86_Name(x86reg),Const);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xC0C7); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xC3C7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xC1C7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xC2C7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xC6C7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xC7C7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xC4C7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xC5C7); break;
	default:
		DisplayError("MoveConstToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Const); 
}

void RSPMoveOffsetToX86reg(DWORD Const, char * VariableName, int x86reg) {
	CPU_Message("      mov %s, offset %s",x86_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xC0C7); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xC3C7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xC1C7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xC2C7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xC6C7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xC7C7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xC4C7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xC5C7); break;
	default:
		DisplayError("MoveOffsetToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Const); 
}

void RSPMoveX86regPointerToX86regByte(int Destination, int AddrReg) {
	BYTE x86Command = 0;

	CPU_Message("      mov %s, byte ptr [%s]",x86Byte_Name(Destination), x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00; break;
	case x86_EBX: x86Command = 0x03; break;
	case x86_ECX: x86Command = 0x01; break;
	case x86_EDX: x86Command = 0x02; break;
	case x86_ESI: x86Command = 0x06; break;
	case x86_EDI: x86Command = 0x07; break;
	default: DisplayError("MoveX86regPointerToX86regByte\nUnknown x86 Register");
	}

	switch (Destination) {
	case x86_EAX: x86Command += 0x00; break;
	case x86_EBX: x86Command += 0x18; break;
	case x86_ECX: x86Command += 0x08; break;
	case x86_EDX: x86Command += 0x10; break;
	default: DisplayError("MoveX86regPointerToX86regByte\nUnknown x86 Register");
	}

	PUTDST8(RSPRecompPos, 0x8A);
	PUTDST8(RSPRecompPos, x86Command);
}

void RSPMoveX86regPointerToX86regHalf(int Destination, int AddrReg) {
	unsigned char x86Amb = 0;

	CPU_Message("      mov %s, word ptr [%s]",x86Half_Name(Destination), x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	default: DisplayError("MoveX86regPointerToX86regHalf\nUnknown x86 Register");
	}

	switch (Destination) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regPointerToX86regHalf\nUnknown x86 Register");
	}

	PUTDST16(RSPRecompPos, 0x8B66);
	PUTDST8(RSPRecompPos, x86Amb);
}

void RSPMoveX86regPointerToX86reg(int Destination, int AddrReg) {
	BYTE x86Amb;
	CPU_Message("      mov %s, dword ptr [%s]",x86_Name(Destination), x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: x86Amb = 0x04; break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regPointerToX86reg\nUnknown x86 Register");
	}

	switch (Destination) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regPointerToX86reg\nUnknown x86 Register");
	}

	PUTDST8(RSPRecompPos, 0x8B);
	PUTDST8(RSPRecompPos, x86Amb);
}

void RSPMoveX86regByteToX86regPointer(int Source, int AddrReg) {
	BYTE x86Amb;
	CPU_Message("      mov byte ptr [%s], %s",x86_Name(AddrReg), x86Byte_Name(Source));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: x86Amb = 0x04; break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	PUTDST8(RSPRecompPos, 0x88);
	PUTDST8(RSPRecompPos, x86Amb);
}

void RSPMoveX86regHalfToX86regPointer(int Source, int AddrReg) {
	BYTE x86Amb;

	CPU_Message("      mov word ptr [%s], %s",x86_Name(AddrReg), x86Half_Name(Source));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: x86Amb = 0x04; break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	PUTDST16(RSPRecompPos, 0x8966);
	PUTDST8(RSPRecompPos, x86Amb);
}

void RSPMoveX86regHalfToX86regPointerDisp(int Source, int AddrReg, BYTE Disp) {
	BYTE x86Amb;

	CPU_Message("      mov word ptr [%s+%X], %s",x86_Name(AddrReg), Disp, x86Half_Name(Source));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: x86Amb = 0x04; break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regBytePointer\nUnknown x86 Register");
	}

	PUTDST16(RSPRecompPos, 0x8966);
	PUTDST8(RSPRecompPos, x86Amb | 0x40);
	PUTDST8(RSPRecompPos, Disp);
}

void RSPMoveX86regToX86regPointer(int Source, int AddrReg) {
	BYTE x86Amb;
	CPU_Message("      mov dword ptr [%s], %s",x86_Name(AddrReg), x86_Name(Source));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: x86Amb = 0x04; break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regToX86regPointer\nUnknown x86 Register");
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regToX86regPointer\nUnknown x86 Register");
	}

	PUTDST8(RSPRecompPos, 0x89);
	PUTDST8(RSPRecompPos, x86Amb);
}

void RSPMoveX86RegToX86regPointerDisp ( int Source, int AddrReg, BYTE Disp ) {
	BYTE x86Amb;
	CPU_Message("      mov dword ptr [%s+%X], %s",x86_Name(AddrReg), Disp, x86_Name(Source));

	switch (AddrReg) {
	case x86_EAX: x86Amb = 0x00; break;
	case x86_EBX: x86Amb = 0x03; break;
	case x86_ECX: x86Amb = 0x01; break;
	case x86_EDX: x86Amb = 0x02; break;
	case x86_ESI: x86Amb = 0x06; break;
	case x86_EDI: x86Amb = 0x07; break;
	case x86_ESP: DisplayError("MoveX86RegToX86regPointerDisp: ESP is invalid"); break;
	case x86_EBP: x86Amb = 0x05; break;
	default: DisplayError("MoveX86regToX86regPointer\nUnknown x86 Register");
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	default: DisplayError("MoveX86regToX86regPointer\nUnknown x86 Register");
	}

	PUTDST8(RSPRecompPos, 0x89);
	PUTDST8(RSPRecompPos, x86Amb | 0x40);
	PUTDST8(RSPRecompPos, Disp);
}

void RSPMoveN64MemDispToX86reg(int x86reg, int AddrReg, BYTE Disp) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s+Dmem+%Xh]",x86_Name(x86reg),x86_Name(AddrReg),Disp);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM + Disp);
}

void RSPMoveN64MemToX86reg(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s+Dmem]",x86_Name(x86reg),x86_Name(AddrReg));
	
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveN64MemToX86regByte(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, byte ptr [%s+Dmem]",x86Byte_Name(x86reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008A; break;
	case x86_EBX: x86Command = 0x038A; break;
	case x86_ECX: x86Command = 0x018A; break;
	case x86_EDX: x86Command = 0x028A; break;
	case x86_ESI: x86Command = 0x068A; break;
	case x86_EDI: x86Command = 0x078A; break;
	case x86_ESP: x86Command = 0x048A; break;
	case x86_EBP: x86Command = 0x058A; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	default:
		DisplayError("MoveN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveN64MemToX86regHalf(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, word ptr [%s+Dmem]",x86Half_Name(x86reg),x86_Name(AddrReg));
	
	PUTDST8(RSPRecompPos,0x66);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveX86regByteToN64Mem(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov byte ptr [%s+Dmem], %s",x86_Name(AddrReg),x86Byte_Name(x86reg));
	
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0088; break;
	case x86_EBX: x86Command = 0x0388; break;
	case x86_ECX: x86Command = 0x0188; break;
	case x86_EDX: x86Command = 0x0288; break;
	case x86_ESI: x86Command = 0x0688; break;
	case x86_EDI: x86Command = 0x0788; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveX86regHalfToN64Mem(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov word ptr [%s+Dmem], %s",x86_Name(AddrReg),x86Half_Name(x86reg));
	PUTDST8(RSPRecompPos,0x66);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveX86regToN64Mem(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s+N64mem], %s",x86_Name(AddrReg),x86_Name(x86reg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveX86regToN64MemDisp(int x86reg, int AddrReg, BYTE Disp) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s+N64mem+%d], %s",x86_Name(AddrReg),Disp,x86_Name(x86reg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM+Disp);
}

void RSPMoveVariableToX86reg(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      mov %s, dword ptr [%s]",x86_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x058B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D8B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D8B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x158B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x358B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D8B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x258B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D8B); break;
	default: DisplayError("MoveVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveVariableToX86regByte(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      mov %s, byte ptr [%s]",x86Byte_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x058A); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D8A); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D8A); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x158A); break;
	default: DisplayError("MoveVariableToX86regByte\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveVariableToX86regHalf(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      mov %s, word ptr [%s]",x86Half_Name(x86reg),VariableName);
	PUTDST8(RSPRecompPos,0x66);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x058B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D8B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D8B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x158B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x358B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D8B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x258B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D8B); break;
	default: DisplayError("MoveVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveX86regByteToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      mov byte ptr [%s], %s",VariableName,x86Byte_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0588); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D88); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D88); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1588); break;
	default:
		DisplayError("MoveX86regByteToVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveX86regHalfToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      mov word ptr [%s], %s",VariableName,x86Half_Name(x86reg));
	PUTDST8(RSPRecompPos,0x66);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0589); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D89); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D89); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1589); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3589); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D89); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2589); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D89); break;
	default:
		DisplayError("MoveX86regHalfToVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveX86regToVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      mov dword ptr [%s], %s",VariableName,x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0589); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D89); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D89); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1589); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3589); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D89); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2589); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D89); break;
	default:
		DisplayError("MoveX86regToVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveX86RegToX86Reg(int Source, int Destination) {
	WORD x86Command;
	
	CPU_Message("      mov %s, %s",x86_Name(Destination),x86_Name(Source));

	switch (Destination) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	
	switch (Source) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPMoveSxX86RegHalfToX86Reg(int Source, int Destination) {
	WORD x86Command;
	
	CPU_Message("      movsx %s, %s",x86_Name(Destination),x86Half_Name(Source));

	switch (Source) {
	case x86_EAX: x86Command = 0x00BF; break;
	case x86_EBX: x86Command = 0x03BF; break;
	case x86_ECX: x86Command = 0x01BF; break;
	case x86_EDX: x86Command = 0x02BF; break;
	case x86_ESI: x86Command = 0x06BF; break;
	case x86_EDI: x86Command = 0x07BF; break;
	case x86_ESP: x86Command = 0x04BF; break;
	case x86_EBP: x86Command = 0x05BF; break;
	}
	
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST8(RSPRecompPos, 0x0f);
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPMoveSxX86RegPtrDispToX86RegHalf(int AddrReg, BYTE Disp, int Destination) {
	BYTE x86Command;
	
	CPU_Message("      movsx %s, [%s+%X]",x86_Name(Destination), x86_Name(AddrReg),Disp);

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00; break;
	case x86_EBX: x86Command = 0x03; break;
	case x86_ECX: x86Command = 0x01; break;
	case x86_EDX: x86Command = 0x02; break;
	case x86_ESI: x86Command = 0x06; break;
	case x86_EDI: x86Command = 0x07; break;
	case x86_ESP: x86Command = 0x04; break;
	case x86_EBP: x86Command = 0x05; break;
	}
	
	switch (Destination) {
	case x86_EAX: x86Command += 0x40; break;
	case x86_EBX: x86Command += 0x58; break;
	case x86_ECX: x86Command += 0x48; break;
	case x86_EDX: x86Command += 0x50; break;
	case x86_ESI: x86Command += 0x70; break;
	case x86_EDI: x86Command += 0x78; break;
	case x86_ESP: x86Command += 0x60; break;
	case x86_EBP: x86Command += 0x68; break;
	}
	PUTDST16(RSPRecompPos, 0xBF0F);
	PUTDST8(RSPRecompPos, x86Command);
	PUTDST8(RSPRecompPos, Disp);
}

void RSPMoveSxVariableToX86regHalf(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      movsx %s, word ptr [%s]",x86_Name(x86reg),VariableName);

	PUTDST16(RSPRecompPos, 0xbf0f);

	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0x05); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0x1D); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0x0D); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0x15); break;
	case x86_ESI: PUTDST8(RSPRecompPos,0x35); break;
	case x86_EDI: PUTDST8(RSPRecompPos,0x3D); break;
	case x86_ESP: PUTDST8(RSPRecompPos,0x25); break;
	case x86_EBP: PUTDST8(RSPRecompPos,0x2D); break;
	default: DisplayError("MoveSxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveSxN64MemToX86regByte(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      movsx %s, byte ptr [%s+Dmem]",x86_Name(x86reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00BE; break;
	case x86_EBX: x86Command = 0x03BE; break;
	case x86_ECX: x86Command = 0x01BE; break;
	case x86_EDX: x86Command = 0x02BE; break;
	case x86_ESI: x86Command = 0x06BE; break;
	case x86_EDI: x86Command = 0x07BE; break;
	case x86_ESP: x86Command = 0x04BE; break;
	case x86_EBP: x86Command = 0x05BE; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	default:
		DisplayError("MoveSxN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST8(RSPRecompPos,0x0f);
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveSxN64MemToX86regHalf(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      movsx %s, word ptr [%s+Dmem]",x86_Name(x86reg),x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00BF; break;
	case x86_EBX: x86Command = 0x03BF; break;
	case x86_ECX: x86Command = 0x01BF; break;
	case x86_EDX: x86Command = 0x02BF; break;
	case x86_ESI: x86Command = 0x06BF; break;
	case x86_EDI: x86Command = 0x07BF; break;
	case x86_ESP: x86Command = 0x04BF; break;
	case x86_EBP: x86Command = 0x05BF; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}

	PUTDST8(RSPRecompPos, 0x0f);
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveZxX86RegHalfToX86Reg(int Source, int Destination) {
	WORD x86Command;
	
	CPU_Message("      movzx %s, %s",x86_Name(Destination),x86Half_Name(Source));

	switch (Source) {
	case x86_EAX: x86Command = 0x00B7; break;
	case x86_EBX: x86Command = 0x03B7; break;
	case x86_ECX: x86Command = 0x01B7; break;
	case x86_EDX: x86Command = 0x02B7; break;
	case x86_ESI: x86Command = 0x06B7; break;
	case x86_EDI: x86Command = 0x07B7; break;
	case x86_ESP: x86Command = 0x04B7; break;
	case x86_EBP: x86Command = 0x05B7; break;
	}
	
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST8(RSPRecompPos, 0x0f);
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPMoveZxX86RegPtrDispToX86RegHalf(int AddrReg, BYTE Disp, int Destination) {
	BYTE x86Command;
	
	CPU_Message("      movzx %s, [%s+%X]",x86_Name(Destination), x86_Name(AddrReg), Disp);

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00; break;
	case x86_EBX: x86Command = 0x03; break;
	case x86_ECX: x86Command = 0x01; break;
	case x86_EDX: x86Command = 0x02; break;
	case x86_ESI: x86Command = 0x06; break;
	case x86_EDI: x86Command = 0x07; break;
	case x86_ESP: x86Command = 0x04; break;
	case x86_EBP: x86Command = 0x05; break;
	}
	
	switch (Destination) {
	case x86_EAX: x86Command += 0x40; break;
	case x86_EBX: x86Command += 0x58; break;
	case x86_ECX: x86Command += 0x48; break;
	case x86_EDX: x86Command += 0x50; break;
	case x86_ESI: x86Command += 0x70; break;
	case x86_EDI: x86Command += 0x78; break;
	case x86_ESP: x86Command += 0x60; break;
	case x86_EBP: x86Command += 0x68; break;
	}
	PUTDST16(RSPRecompPos, 0xB70F);
	PUTDST8(RSPRecompPos, x86Command);
	PUTDST8(RSPRecompPos, Disp);
}

void RSPMoveZxVariableToX86regHalf(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      movzx %s, word ptr [%s]",x86_Name(x86reg),VariableName);

	PUTDST16(RSPRecompPos, 0xb70f);

	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0x05); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0x1D); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0x0D); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0x15); break;
	case x86_ESI: PUTDST8(RSPRecompPos,0x35); break;
	case x86_EDI: PUTDST8(RSPRecompPos,0x3D); break;
	case x86_ESP: PUTDST8(RSPRecompPos,0x25); break;
	case x86_EBP: PUTDST8(RSPRecompPos,0x2D); break;
	default: DisplayError("MoveZxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPMoveZxN64MemToX86regByte(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      movzx %s, byte ptr [%s+Dmem]",x86_Name(x86reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00B6; break;
	case x86_EBX: x86Command = 0x03B6; break;
	case x86_ECX: x86Command = 0x01B6; break;
	case x86_EDX: x86Command = 0x02B6; break;
	case x86_ESI: x86Command = 0x06B6; break;
	case x86_EDI: x86Command = 0x07B6; break;
	case x86_ESP: x86Command = 0x04B6; break;
	case x86_EBP: x86Command = 0x05B6; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	default:
		DisplayError("MoveZxN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST8(RSPRecompPos,0x0f);
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMoveZxN64MemToX86regHalf(int x86reg, int AddrReg) {
	WORD x86Command;

	CPU_Message("      movzx %s, word ptr [%s+Dmem]",x86_Name(x86reg),x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00B7; break;
	case x86_EBX: x86Command = 0x03B7; break;
	case x86_ECX: x86Command = 0x01B7; break;
	case x86_EDX: x86Command = 0x02B7; break;
	case x86_ESI: x86Command = 0x06B7; break;
	case x86_EDI: x86Command = 0x07B7; break;
	case x86_ESP: x86Command = 0x04B7; break;
	case x86_EBP: x86Command = 0x05B7; break;
	}
	switch (x86reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}

	PUTDST8(RSPRecompPos, 0x0f);
	PUTDST16(RSPRecompPos,x86Command);
	PUTDST32(RSPRecompPos,DMEM);
}

void RSPMulX86reg(int x86reg) {
	CPU_Message("      mul %s",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE0F7); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xE3F7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE1F7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xE2F7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xE6F7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xE7F7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xE4F7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xE5F7); break;
	default:
		DisplayError("MulX86reg\nUnknown x86 Register");
	}
}

void RSPNegateX86reg(int x86reg) {
	CPU_Message("      neg %s", x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xd8f7); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xdbf7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xd9f7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xdaf7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xdef7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xdff7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xdcf7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xddf7); break;
	default:
		DisplayError("NegateX86reg\nUnknown x86 Register");
	}
}

void RSPOrConstToVariable(DWORD Const, void * Variable, char * VariableName) {
	CPU_Message("      or dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(RSPRecompPos,0x0D81);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPOrConstToX86Reg(DWORD Const, int  x86Reg) {
	CPU_Message("      or %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xC881); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xCB81); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xC981); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xCA81); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xCE81); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xCF81); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xCC81); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xCD81); break;
		}
		PUTDST32(RSPRecompPos, Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xC883); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xCB83); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xC983); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xCA83); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xCE83); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xCF83); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xCC83); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xCD83); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPOrVariableToX86Reg(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      or %s, dword ptr [%s]",x86_Name(x86Reg),VariableName);
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x050B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D0B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D0B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x150B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x350B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D0B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x250B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D0B); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPOrVariableToX86regHalf(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      or %s, word ptr [%s]",x86Half_Name(x86Reg),VariableName);
	PUTDST8(RSPRecompPos,0x66);
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x050B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D0B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D0B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x150B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x350B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D0B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x250B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D0B); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPOrX86RegToVariable(void * Variable, char * VariableName, int x86Reg) {
	CPU_Message("      or dword ptr [%s], %s",VariableName, x86_Name(x86Reg));
	switch (x86Reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0509); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D09); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D09); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1509); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3509); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D09); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2509); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D09); break;
	}
	PUTDST32(RSPRecompPos,Variable);
}

void RSPOrX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;

	CPU_Message("      or %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x000B; break;
	case x86_EBX: x86Command = 0x030B; break;
	case x86_ECX: x86Command = 0x010B; break;
	case x86_EDX: x86Command = 0x020B; break;
	case x86_ESI: x86Command = 0x060B; break;
	case x86_EDI: x86Command = 0x070B; break;
	case x86_ESP: x86Command = 0x040B; break;
	case x86_EBP: x86Command = 0x050B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPPopad(void) {
	CPU_Message("      popad");
	PUTDST8(RSPRecompPos,0x61);
}

void RSPPushad(void) {
	CPU_Message("      pushad");
	PUTDST8(RSPRecompPos,0x60);
}

void RSPPush(int x86reg) {
	CPU_Message("      push %s", x86_Name(x86reg));

	switch(x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos, 0x50); break;
	case x86_EBX: PUTDST8(RSPRecompPos, 0x53); break;
	case x86_ECX: PUTDST8(RSPRecompPos, 0x51); break;
	case x86_EDX: PUTDST8(RSPRecompPos, 0x52); break;
	case x86_ESI: PUTDST8(RSPRecompPos, 0x56); break;
	case x86_EDI: PUTDST8(RSPRecompPos, 0x57); break;
	case x86_ESP: PUTDST8(RSPRecompPos, 0x54); break;
	case x86_EBP: PUTDST8(RSPRecompPos, 0x55); break;
	}
}

void RSPPop(int x86reg) {
	CPU_Message("      pop %s", x86_Name(x86reg));

	switch(x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos, 0x58); break;
	case x86_EBX: PUTDST8(RSPRecompPos, 0x5B); break;
	case x86_ECX: PUTDST8(RSPRecompPos, 0x59); break;
	case x86_EDX: PUTDST8(RSPRecompPos, 0x5A); break;
	case x86_ESI: PUTDST8(RSPRecompPos, 0x5E); break;
	case x86_EDI: PUTDST8(RSPRecompPos, 0x5F); break;
	case x86_ESP: PUTDST8(RSPRecompPos, 0x5C); break;
	case x86_EBP: PUTDST8(RSPRecompPos, 0x5D); break;
	}
}

void RSPPushImm32(char * String, DWORD Value) {
	CPU_Message("      push %s",String);
	PUTDST8(RSPRecompPos,0x68);
	PUTDST32(RSPRecompPos,Value);
}

void RSPRet(void) {
	CPU_Message("      ret");
	PUTDST8(RSPRecompPos,0xC3);
}

void RSPSetl(int x86reg) {
	CPU_Message("      setl %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x9C0F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Setl\nUnknown x86 Register");
	}
}

void RSPSetlVariable(void * Variable, char * VariableName) {
	CPU_Message("      setl byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x9C0F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetleVariable(void * Variable, char * VariableName) {
	CPU_Message("      setle byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x9E0F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetb(int x86reg) {
	CPU_Message("      setb %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x920F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Setb\nUnknown x86 Register");
	}
}

void RSPSetbVariable(void * Variable, char * VariableName) {
	CPU_Message("      setb byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x920F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetg(int x86reg) {
	CPU_Message("      setg %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x9F0F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Setg\nUnknown x86 Register");
	}
}

void RSPSetgVariable(void * Variable, char * VariableName) {
	CPU_Message("      setg byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x9F0F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetgeVariable(void * Variable, char * VariableName) {
	CPU_Message("      setge byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x9D0F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSeta(int x86reg) {
	CPU_Message("      seta %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x970F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Seta\nUnknown x86 Register");
	}
}

void RSPSetaVariable(void * Variable, char * VariableName) {
	CPU_Message("      seta byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x970F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetae(int x86reg) {
	CPU_Message("      setae %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x930F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Seta\nUnknown x86 Register");
	}
}

void RSPSetz(int x86reg) {
	CPU_Message("      setz %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x940F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Setz\nUnknown x86 Register");
	}
}

void RSPSetzVariable(void * Variable, char * VariableName) {
	CPU_Message("      setz byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x940F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPSetnz(int x86reg) {
	CPU_Message("      setnz %s",x86Byte_Name(x86reg));
	PUTDST16(RSPRecompPos,0x950F);
	switch (x86reg) {
	case x86_EAX: PUTDST8(RSPRecompPos,0xC0); break;
	case x86_EBX: PUTDST8(RSPRecompPos,0xC3); break;
	case x86_ECX: PUTDST8(RSPRecompPos,0xC1); break;
	case x86_EDX: PUTDST8(RSPRecompPos,0xC2); break;
	default:
		DisplayError("Setnz\nUnknown x86 Register");
	}
}

void RSPSetnzVariable(void * Variable, char * VariableName) {
	CPU_Message("      setnz byte ptr [%s]",VariableName);
	PUTDST16(RSPRecompPos,0x950F);
	PUTDST8(RSPRecompPos,0x05);
	PUTDST32(RSPRecompPos,Variable);
}

void RSPShiftLeftDoubleImmed(int Destination, int Source, BYTE Immediate) {
	BYTE x86Amb = 0xC0;

	CPU_Message("      shld %s, %s, %Xh", x86_Name(Destination),x86_Name(Source), Immediate);
	PUTDST16(RSPRecompPos,0xA40F);

	switch (Destination) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_EBX: x86Amb += 0x03; break;
	case x86_ECX: x86Amb += 0x01; break;
	case x86_EDX: x86Amb += 0x02; break;
	case x86_ESI: x86Amb += 0x06; break;
	case x86_EDI: x86Amb += 0x07; break;
	case x86_ESP: x86Amb += 0x04; break;
	case x86_EBP: x86Amb += 0x05; break;
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	}

	PUTDST8(RSPRecompPos,x86Amb);
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftRightDoubleImmed(int Destination, int Source, BYTE Immediate) {
	BYTE x86Amb = 0xC0;

	CPU_Message("      shrd %s, %s, %Xh", x86_Name(Destination),x86_Name(Source), Immediate);
	PUTDST16(RSPRecompPos,0xAC0F);

	switch (Destination) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_EBX: x86Amb += 0x03; break;
	case x86_ECX: x86Amb += 0x01; break;
	case x86_EDX: x86Amb += 0x02; break;
	case x86_ESI: x86Amb += 0x06; break;
	case x86_EDI: x86Amb += 0x07; break;
	case x86_ESP: x86Amb += 0x04; break;
	case x86_EBP: x86Amb += 0x05; break;
	}

	switch (Source) {
	case x86_EAX: x86Amb += 0x00; break;
	case x86_EBX: x86Amb += 0x18; break;
	case x86_ECX: x86Amb += 0x08; break;
	case x86_EDX: x86Amb += 0x10; break;
	case x86_ESI: x86Amb += 0x30; break;
	case x86_EDI: x86Amb += 0x38; break;
	case x86_ESP: x86Amb += 0x20; break;
	case x86_EBP: x86Amb += 0x28; break;
	}

	PUTDST8(RSPRecompPos,x86Amb);
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftLeftSign(int x86reg) {
	CPU_Message("      shl %s, cl",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE0D3); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xE3D3); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE1D3); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xE2D3); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xE6D3); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xE7D3); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xE4D3); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xE5D3); break;
	}
}

void RSPShiftLeftSignImmed(int x86reg, BYTE Immediate) {
	CPU_Message("      shl %s, %Xh",x86_Name(x86reg),Immediate);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE0C1); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xE3C1); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE1C1); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xE2C1); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xE6C1); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xE7C1); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xE4C1); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xE5C1); break;
	}
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftLeftSignVariableImmed(void *Variable, char *VariableName, BYTE Immediate) {
	CPU_Message("      shl dword ptr [%s], %Xh",VariableName, Immediate);

	PUTDST16(RSPRecompPos,0x25C1)
	PUTDST32(RSPRecompPos, Variable);
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftRightSignImmed(int x86reg, BYTE Immediate) {
	CPU_Message("      sar %s, %Xh",x86_Name(x86reg),Immediate);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xF8C1); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xFBC1); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xF9C1); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xFAC1); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xFEC1); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xFFC1); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xFCC1); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xFDC1); break;
	default:
		DisplayError("ShiftRightSignImmed\nUnknown x86 Register");
	}
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftRightSignVariableImmed(void *Variable, char *VariableName, BYTE Immediate) {
	CPU_Message("      sar dword ptr [%s], %Xh",VariableName, Immediate);

	PUTDST16(RSPRecompPos,0x3DC1)
	PUTDST32(RSPRecompPos, Variable);
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftRightUnsign(int x86reg) {
	CPU_Message("      shr %s, cl",x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE8D3); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xEBD3); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE9D3); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xEAD3); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xEED3); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xEFD3); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xECD3); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xEDD3); break;
	}
}

void RSPShiftRightUnsignImmed(int x86reg, BYTE Immediate) {
	CPU_Message("      shr %s, %Xh",x86_Name(x86reg),Immediate);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0xE8C1); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xEBC1); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xE9C1); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xEAC1); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xEEC1); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xEFC1); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xECC1); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xEDC1); break;
	}
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPShiftRightUnsignVariableImmed(void *Variable, char *VariableName, BYTE Immediate) {
	CPU_Message("      shr dword ptr [%s], %Xh",VariableName, Immediate);

	PUTDST16(RSPRecompPos,0x2DC1)
	PUTDST32(RSPRecompPos, Variable);
	PUTDST8(RSPRecompPos,Immediate);
}

void RSPSubConstFromVariable (DWORD Const, void *Variable, char *VariableName) {
	CPU_Message("      sub dword ptr [%s], 0x%X",VariableName, Const);\
	PUTDST16(RSPRecompPos,0x2D81);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPSubConstFromX86Reg (int x86Reg, DWORD Const) {
	CPU_Message("      sub %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xE881); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xEB81); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xE981); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xEA81); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xEE81); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xEF81); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xEC81); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xED81); break;
		}
		PUTDST32(RSPRecompPos, Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xE883); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xEB83); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xE983); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xEA83); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xEE83); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xEF83); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xEC83); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xED83); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPSubVariableFromX86reg(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      sub %s, dword ptr [%s]",x86_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x052B); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D2B); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D2B); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x152B); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x352B); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D2B); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x252B); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D2B); break;
	default:
		DisplayError("SubVariableFromX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPSubX86regFromVariable(int x86reg, void * Variable, char * VariableName) {
	CPU_Message("      sub dword ptr [%s], %s",VariableName, x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0529); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D29); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D29); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1529); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3529); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D29); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2529); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D29); break;
	default:
		DisplayError("SubX86regFromVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable); 
}

void RSPSubX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;
	CPU_Message("      sub %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x002B; break;
	case x86_EBX: x86Command = 0x032B; break;
	case x86_ECX: x86Command = 0x012B; break;
	case x86_EDX: x86Command = 0x022B; break;
	case x86_ESI: x86Command = 0x062B; break;
	case x86_EDI: x86Command = 0x072B; break;
	case x86_ESP: x86Command = 0x042B; break;
	case x86_EBP: x86Command = 0x052B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPSbbX86RegToX86Reg(int Destination, int Source) {
	WORD x86Command;
	CPU_Message("      sbb %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x001B; break;
	case x86_EBX: x86Command = 0x031B; break;
	case x86_ECX: x86Command = 0x011B; break;
	case x86_EDX: x86Command = 0x021B; break;
	case x86_ESI: x86Command = 0x061B; break;
	case x86_EDI: x86Command = 0x071B; break;
	case x86_ESP: x86Command = 0x041B; break;
	case x86_EBP: x86Command = 0x051B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPTestConstToVariable(DWORD Const, void * Variable, char * VariableName) {
	CPU_Message("      test dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(RSPRecompPos,0x05F7);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos,Const);
}

void RSPTestConstToX86Reg(DWORD Const, int x86reg) {
	CPU_Message("      test %s, 0x%X",x86_Name(x86reg), Const);
	
	switch (x86reg) {
	case x86_EAX: PUTDST8 (RSPRecompPos,0xA9); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0xC3F7); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0xC1F7); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0xC2F7); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0xC6F7); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0xC7F7); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0xC4F7); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0xC5F7); break;
	}
	PUTDST32(RSPRecompPos,Const);
}

void RSPTestX86RegToX86Reg(int Destination, int Source) {	
	WORD x86Command;

	CPU_Message("      test %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x0085; break;
	case x86_EBX: x86Command = 0x0385; break;
	case x86_ECX: x86Command = 0x0185; break;
	case x86_EDX: x86Command = 0x0285; break;
	case x86_ESI: x86Command = 0x0685; break;
	case x86_EDI: x86Command = 0x0785; break;
	case x86_ESP: x86Command = 0x0485; break;
	case x86_EBP: x86Command = 0x0585; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPXorConstToX86Reg(int x86Reg, DWORD Const) {
	CPU_Message("      xor %s, %Xh",x86_Name(x86Reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xF081); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xF381); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xF181); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xF281); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xF681); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xF781); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xF481); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xF581); break;
		}
		PUTDST32(RSPRecompPos, Const);
	} else {
		switch (x86Reg) {
		case x86_EAX: PUTDST16(RSPRecompPos,0xF083); break;
		case x86_EBX: PUTDST16(RSPRecompPos,0xF383); break;
		case x86_ECX: PUTDST16(RSPRecompPos,0xF183); break;
		case x86_EDX: PUTDST16(RSPRecompPos,0xF283); break;
		case x86_ESI: PUTDST16(RSPRecompPos,0xF683); break;
		case x86_EDI: PUTDST16(RSPRecompPos,0xF783); break;
		case x86_ESP: PUTDST16(RSPRecompPos,0xF483); break;
		case x86_EBP: PUTDST16(RSPRecompPos,0xF583); break;
		}
		PUTDST8(RSPRecompPos, Const);
	}
}

void RSPXorConstToVariable(void *Variable, char *VariableName, DWORD Const) {

	CPU_Message("      xor dword ptr [%s], 0x%X",VariableName, Const);

	PUTDST16(RSPRecompPos, 0x3581);
	PUTDST32(RSPRecompPos,Variable);
	PUTDST32(RSPRecompPos, Const);
}

void RSPXorX86RegToX86Reg(int Source, int Destination) {
	WORD x86Command;

	CPU_Message("      xor %s, %s",x86_Name(Source),x86_Name(Destination));
		
	switch (Source) {
	case x86_EAX: x86Command = 0x0031; break;
	case x86_EBX: x86Command = 0x0331; break;
	case x86_ECX: x86Command = 0x0131; break;
	case x86_EDX: x86Command = 0x0231; break;
	case x86_ESI: x86Command = 0x0631; break;
	case x86_EDI: x86Command = 0x0731; break;
	case x86_ESP: x86Command = 0x0431; break;
	case x86_EBP: x86Command = 0x0531; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(RSPRecompPos,x86Command);
}

void RSPXorVariableToX86reg(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      xor %s, dword ptr [%s]",x86_Name(x86reg),VariableName);
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0533); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D33); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D33); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1533); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3533); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D33); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2533); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D33); break;
	default: DisplayError("XorVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}

void RSPXorX86RegToVariable(void *Variable, char *VariableName, int x86reg) {
	CPU_Message("      xor dword ptr [%s], %s",VariableName, x86_Name(x86reg));
	switch (x86reg) {
	case x86_EAX: PUTDST16(RSPRecompPos,0x0531); break;
	case x86_EBX: PUTDST16(RSPRecompPos,0x1D31); break;
	case x86_ECX: PUTDST16(RSPRecompPos,0x0D31); break;
	case x86_EDX: PUTDST16(RSPRecompPos,0x1531); break;
	case x86_ESI: PUTDST16(RSPRecompPos,0x3531); break;
	case x86_EDI: PUTDST16(RSPRecompPos,0x3D31); break;
	case x86_ESP: PUTDST16(RSPRecompPos,0x2531); break;
	case x86_EBP: PUTDST16(RSPRecompPos,0x2D31); break;
	default: DisplayError("XorX86RegToVariable\nUnknown x86 Register");
	}
    PUTDST32(RSPRecompPos,Variable);
}
