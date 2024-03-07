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

#define MaxMaps	32

#include <windows.h>
#include "rsp.h"
#include "rsp main.h"
#include "RSP Registers.h"
#include "Recompiler CPU.h"
#include "main.h"
#include "types.h"
#include "memory.h"

DWORD NoOfMaps, MapsCRC[MaxMaps], Table;
BYTE * RSPRecompCode, * RSPRecompCodeSecondary, * RSPRecompPos, *RSPJumpTables;
void ** RSPJumpTable;

int RSPAllocateMemory (void) {
	RSPRecompCode=(BYTE *) VirtualAlloc( NULL, 0x00400004, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	RSPRecompCode=(BYTE *) VirtualAlloc( RSPRecompCode, 0x00400000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	
	if(RSPRecompCode == NULL) {
		DisplayError("Not enough memory for RSP RSPRecompCode!");
		return FALSE;
	}

	RSPRecompCodeSecondary = (BYTE *)VirtualAlloc( NULL, 0x00200000, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
	if(RSPRecompCodeSecondary == NULL) {
		DisplayError("Not enough memory for RSP RSPRecompCode Secondary!");
		return FALSE;
	}

	RSPJumpTables = (BYTE *)VirtualAlloc( NULL, 0x1000 * MaxMaps, MEM_COMMIT, PAGE_READWRITE );
	if( RSPJumpTables == NULL ) {  
		DisplayError("Not enough memory for Jump Table!");
		return FALSE;
	}

	RSPJumpTable = (void **)RSPJumpTables;
	RSPRecompPos = RSPRecompCode;
	NoOfMaps = 0;
	return TRUE;
}

void RSPFreeMemory (void) {
	if (RSPRecompCode != NULL) {
		VirtualFree( RSPRecompCode, 0 , MEM_RELEASE);
		RSPRecompCode=NULL;
	}
	if (RSPJumpTables != NULL) {
		VirtualFree( RSPJumpTables, 0 , MEM_RELEASE); // RSPJumpTable was a typo? fixes a memory leak
		RSPJumpTables=NULL;
	}
	if (RSPRecompCodeSecondary != NULL) {
		VirtualFree( RSPRecompCodeSecondary, 0 , MEM_RELEASE);
		RSPRecompCodeSecondary=NULL;
	}
}

void RSPSetJumpTable (void) {
	DWORD CRC, count;

	CRC = 0;
	for (count = 0; count < 0x800; count += 0x40) {
		CRC += *(DWORD *)(IMEM + count);		
	}

/*	
	for (count = 0; count < 0x1000; count += 0x4) {
		CRC += *(DWORD *)(IMEM + count) >> 10;
	}
*/	
	for (count = 0; count <	NoOfMaps; count++ ) {
		if (CRC == MapsCRC[count]) {
			RSPJumpTable = (void **)(RSPJumpTables + count * 0x1000);
			Table = count;
			return;
		}
	}
	//DisplayError("%X %X",NoOfMaps,CRC);
	if (NoOfMaps == MaxMaps) {
		DisplayError("Used up all the Jump tables in the rsp");
		ExitThread(0);
	}
	MapsCRC[NoOfMaps] = CRC;
	RSPJumpTable = (void **)(RSPJumpTables + NoOfMaps * 0x1000);
	Table = NoOfMaps;
	NoOfMaps += 1;
}

void RSP_LB_DMEM ( DWORD Addr, BYTE * Value ) {
	* Value = *(BYTE *)(DMEM + ((Addr ^ 3) & 0xFFF)) ;
}

void RSP_LBV_DMEM ( DWORD Addr, int vect, int element ) {
	RSP_Vect[vect].B[15 - element] = *(DMEM + ((Addr ^ 3) & 0xFFF));
}

void RSP_LDV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LFV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, count;
	VECTOR Temp;

	length = 8;
	if (length > 16 - element) {
		length = 16 - element;
	}
	
	Temp.HW[7] = *(DMEM + (((Addr + element) ^3) & 0xFFF)) << 7;
	Temp.HW[6] = *(DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[5] = *(DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[4] = *(DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[3] = *(DMEM + (((Addr + ((0x8 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[2] = *(DMEM + (((Addr + ((0xC - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[1] = *(DMEM + (((Addr + ((0x10 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	Temp.HW[0] = *(DMEM + (((Addr + ((0x4 - element) ^ 3) & 0xf)) & 0xFFF)) << 7;
	
	for (count = element; count < (length + element); count ++ ){
		RSP_Vect[vect].B[15 - count] = Temp.B[15 - count];
	}
}

void RSP_LH_DMEM ( DWORD Addr, WORD * Value ) {
	if ((Addr & 0x1) != 0) {
		if (Addr > 0xFFE) {
			DisplayError("hmmmm.... Problem with:\nRSP_LH_DMEM");
			return;
		}
		Addr &= 0xFFF;
		*Value = *(BYTE *)(DMEM + (Addr^ 3)) << 8;		
		*Value += *(BYTE *)(DMEM + ((Addr + 1)^ 3));
		return;
	}
	* Value = *(WORD *)(DMEM + ((Addr ^ 2 ) & 0xFFF));	
}

void RSP_LHV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(DMEM + ((Addr + ((0x10 - element) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(DMEM + ((Addr + ((0x10 - element + 2) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(DMEM + ((Addr + ((0x10 - element + 4) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(DMEM + ((Addr + ((0x10 - element + 6) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(DMEM + ((Addr + ((0x10 - element + 8) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(DMEM + ((Addr + ((0x10 - element + 10) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(DMEM + ((Addr + ((0x10 - element + 12) & 0xF) ^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(DMEM + ((Addr + ((0x10 - element + 14) & 0xF) ^3) & 0xFFF)) << 7;
}

void RSP_LLV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 4;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LPV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[6] = *(DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[5] = *(DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[4] = *(DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[3] = *(DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[2] = *(DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[1] = *(DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 8;
	RSP_Vect[vect].HW[0] = *(DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 8;
}

void RSP_LRV_DMEM ( DWORD Addr, int vect, int element ) {	
	int length, Count, offset;

	offset = (Addr & 0xF) - 1;
	length = (Addr & 0xF) - element;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[offset - Count] = *(DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LQV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = ((Addr + 0x10) & ~0xF) - Addr;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}

}

void RSP_LSV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = 2;
	if (length > 16 - element) {
		length = 16 - element;
	}
	for (Count = element; Count < (length + element); Count ++ ){
		RSP_Vect[vect].B[15 - Count] = *(DMEM + ((Addr ^ 3) & 0xFFF));
		Addr += 1;
	}
}

void RSP_LTV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	
	Addr = ((Addr + 8) & 0xFF0) + (element & 0x1);	
	for (count = 0; count < length; count ++) {
		del = ((8 - (element >> 1) + count) << 1) & 0xF;
		RSP_Vect[vect + count].B[15 - del] = *(DMEM + (Addr ^ 3));
		RSP_Vect[vect + count].B[14 - del] = *(DMEM + ((Addr + 1) ^ 3));
		Addr += 2;
	}
}

void RSP_LUV_DMEM ( DWORD Addr, int vect, int element ) {	
	RSP_Vect[vect].HW[7] = *(DMEM + ((Addr + ((0x10 - element) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[6] = *(DMEM + ((Addr + ((0x10 - element + 1) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[5] = *(DMEM + ((Addr + ((0x10 - element + 2) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[4] = *(DMEM + ((Addr + ((0x10 - element + 3) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[3] = *(DMEM + ((Addr + ((0x10 - element + 4) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[2] = *(DMEM + ((Addr + ((0x10 - element + 5) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[1] = *(DMEM + ((Addr + ((0x10 - element + 6) & 0xF)^3) & 0xFFF)) << 7;
	RSP_Vect[vect].HW[0] = *(DMEM + ((Addr + ((0x10 - element + 7) & 0xF)^3) & 0xFFF)) << 7;
}

void RSP_LW_DMEM ( DWORD Addr, DWORD * Value ) {
	if ((Addr & 0x3) != 0) {
		Addr &= 0xFFF;
		if (Addr > 0xFFC) {
			DisplayError("hmmmm.... Problem with:\nRSP_LW_DMEM");
			return;
		}
		*Value = *(BYTE *)(DMEM + (Addr^ 3)) << 0x18;
		*Value += *(BYTE *)(DMEM + ((Addr + 1)^ 3)) << 0x10;
		*Value += *(BYTE *)(DMEM + ((Addr + 2)^ 3)) << 8;
		*Value += *(BYTE *)(DMEM + ((Addr + 3)^ 3));
		return;
	}
	* Value = *(DWORD *)(DMEM + (Addr & 0xFFF));	
}

void RSP_LW_IMEM ( DWORD Addr, DWORD * Value ) {
	if ((Addr & 0x3) != 0) {
		DisplayError("Unaligned RSP_LW_IMEM");
	}
	* Value = *(DWORD *)(IMEM + (Addr & 0xFFF));	
}

void RSP_SB_DMEM ( DWORD Addr, BYTE Value ) {
	*(BYTE *)(DMEM + ((Addr ^ 3) & 0xFFF)) = Value;
}

void RSP_SBV_DMEM ( DWORD Addr, int vect, int element ) {
	*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - element];
}

void RSP_SDV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SFV_DMEM ( DWORD Addr, int vect, int element ) {	
	int offset = Addr & 0xF;
	Addr &= 0xFF0;

	switch (element) {
	case 0:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		break;
	case 1:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		break;
	case 2:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 3:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF)^3))) = 0;
		break;
	case 4:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[7] >> 7;
		break;
	case 5:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		break;
	case 6:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 7:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 8:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[3] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		break;
	case 9:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 10:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 11:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[4] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		break;
	case 12:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[2] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[1] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[0] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[3] >> 7;
		break;
	case 13:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 14:
		*(DMEM + ((Addr + offset)^3)) = 0;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = 0;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = 0;
		break;
	case 15:
		*(DMEM + ((Addr + offset)^3)) = RSP_Vect[vect].UHW[7] >> 7;
		*(DMEM + ((Addr + ((offset + 4) & 0xF))^3)) = RSP_Vect[vect].UHW[6] >> 7;
		*(DMEM + ((Addr + ((offset + 8) & 0xF))^3)) = RSP_Vect[vect].UHW[5] >> 7;
		*(DMEM + ((Addr + ((offset + 12) & 0xF))^3)) = RSP_Vect[vect].UHW[4] >> 7;
		break;
	}
}

void RSP_SH_DMEM ( DWORD Addr, WORD Value ) {
	if ((Addr & 0x1) != 0) {
		DisplayError("Unaligned RSP_SH_DMEM");
		return;
	}
	*(WORD *)(DMEM + ((Addr ^ 2) & 0xFFF)) = Value;
}

void RSP_SHV_DMEM ( DWORD Addr, int vect, int element ) {	
	*(DMEM + ((Addr^3) & 0xFFF)) = (RSP_Vect[vect].UB[(15 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(14 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 2)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(13 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(12 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 4)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(11 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(10 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 6)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(9 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(8 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 8)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(7 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(6 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 10)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(5 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(4 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 12)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(3 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(2 - element) & 0xF] >> 7);
	*(DMEM + (((Addr + 14)^3) & 0xFFF)) = (RSP_Vect[vect].UB[(1 - element) & 0xF] << 1) + 
		(RSP_Vect[vect].UB[(0 - element) & 0xF] >> 7);
}

void RSP_SLV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (4 + element); Count ++ ){
		*(DMEM + ((Addr ^3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SPV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0xF) << 1)];
		} else {
			*(DMEM + ((Addr ^ 3) & 0xFFF)) = (RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7);
		}
		Addr += 1;
	}
}

void RSP_SQV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count;
	
	length = ((Addr + 0x10) & ~0xF) - Addr;
	for (Count = element; Count < (length + element); Count ++ ){
		*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_SRV_DMEM ( DWORD Addr, int vect, int element ) {
	int length, Count, offset;

	length = (Addr & 0xF);
	offset = (0x10 - length) & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (length + element); Count ++ ){
		*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - ((Count + offset) & 0xF)];
		Addr += 1;
	}
}

void RSP_SSV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (2 + element); Count ++ ){
		*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		Addr += 1;
	}
}

void RSP_STV_DMEM ( DWORD Addr, int vect, int element ) {
	int del, count, length;
	
	length = 8;
	if (length > 32 - vect) {
		length = 32 - vect;
	}
	length = length << 1;
	del = element >> 1;
	for (count = 0; count < length; count += 2) {
		*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[15 - count];
		*(DMEM + (((Addr + 1) ^ 3) & 0xFFF)) = RSP_Vect[vect + del].UB[14 - count];
		del = (del + 1) & 7;
		Addr += 2;
	}
}

void RSP_SUV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count;

	for (Count = element; Count < (8 + element); Count ++ ){
		if (((Count) & 0xF) < 8) {
			*(DMEM + ((Addr ^ 3) & 0xFFF)) = (RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)] << 1) +
				(RSP_Vect[vect].UB[14 - ((Count & 0x7) << 1)] >> 7);
		} else {
			*(DMEM + ((Addr ^ 3) & 0xFFF)) = RSP_Vect[vect].UB[15 - ((Count & 0x7) << 1)];
		}
		Addr += 1;
	}
}

void RSP_SW_DMEM ( DWORD Addr, DWORD Value ) {
	Addr &= 0xFFF;
	if ((Addr & 0x3) != 0) {
		if (Addr > 0xFFC) {
			DisplayError("hmmmm.... Problem with:\nRSP_SW_DMEM");
			return;
		}
		*(BYTE *)(DMEM + (Addr ^ 3)) = (BYTE)(Value >> 0x18);
		*(BYTE *)(DMEM + ((Addr + 1) ^ 3)) = (BYTE)(Value >> 0x10);
		*(BYTE *)(DMEM + ((Addr + 2) ^ 3)) = (BYTE)(Value >> 0x8);
		*(BYTE *)(DMEM + ((Addr + 3) ^ 3)) = (BYTE)(Value);
		return;
	}
	*(DWORD *)(DMEM + Addr) = Value;
}

void RSP_SWV_DMEM ( DWORD Addr, int vect, int element ) {
	int Count, offset;

	offset = Addr & 0xF;
	Addr &= 0xFF0;
	for (Count = element; Count < (16 + element); Count ++ ){
		*(DMEM + ((Addr + (offset & 0xF)) ^ 3)) = RSP_Vect[vect].B[15 - (Count & 0xF)];
		offset += 1;
	}
}