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

enum x86RegValues {
	x86_EAX = 0, x86_EBX = 1, x86_ECX = 2, x86_EDX = 3,
	x86_ESI = 4, x86_EDI = 5, x86_EBP = 6, x86_ESP = 7
};

enum mmxRegValues {
	x86_MM0 = 0, x86_MM1 = 1, x86_MM2 = 2, x86_MM3 = 3, 
	x86_MM4 = 4, x86_MM5 = 5, x86_MM6 = 6, x86_MM7 = 7
};

enum sseRegValues {
	x86_XMM0 = 0, x86_XMM1 = 1, x86_XMM2 = 2, x86_XMM3 = 3, 
	x86_XMM4 = 4, x86_XMM5 = 5, x86_XMM6 = 6, x86_XMM7 = 7
};

void RSPAdcX86RegToX86Reg				( int Destination, int Source );
void RSPAdcX86regToVariable			( int x86reg, void * Variable, char * VariableName );
void RSPAdcX86regHalfToVariable		( int x86reg, void * Variable, char * VariableName );
void RSPAdcConstToX86reg				( BYTE Constant, int x86reg );
void RSPAdcConstToVariable				( void *Variable, char *VariableName, BYTE Constant );
void RSPAdcConstHalfToVariable			( void *Variable, char *VariableName, BYTE Constant );
void RSPAddConstToVariable				( DWORD Const, void *Variable, char *VariableName );
void RSPAddConstToX86Reg				( int x86Reg, DWORD Const );
void RSPAddVariableToX86reg			( int x86reg, void * Variable, char * VariableName );
void RSPAddX86regToVariable			( int x86reg, void * Variable, char * VariableName );
void RSPAddX86regHalfToVariable		( int x86reg, void * Variable, char * VariableName );
void RSPAddX86RegToX86Reg				( int Destination, int Source );
void RSPAndConstToVariable				( DWORD Const, void *Variable, char *VariableName );
void RSPAndConstToX86Reg				( int x86Reg, DWORD Const );
void RSPAndVariableToX86Reg			( void * Variable, char * VariableName, int x86Reg );
void RSPAndVariableToX86regHalf		( void * Variable, char * VariableName, int x86Reg );
void RSPAndX86RegToVariable			( void * Variable, char * VariableName, int x86Reg );
void RSPAndX86RegToX86Reg				( int Destination, int Source );
void RSPAndX86RegHalfToX86RegHalf		( int Destination, int Source );
void RSPCall_Direct					( void * FunctAddress, char * FunctName );
void RSPCall_Indirect					( void * FunctAddress, char * FunctName );
void RSPCondMoveEqual					( int Destination, int Source );
void RSPCondMoveNotEqual				( int Destination, int Source );
void RSPCondMoveGreater				( int Destination, int Source );
void RSPCondMoveGreaterEqual			( int Destination, int Source );
void RSPCondMoveLess					( int Destination, int Source );
void RSPCondMoveLessEqual				( int Destination, int Source );
void RSPCompConstToVariable			( DWORD Const, void * Variable, char * VariableName );
void RSPCompConstHalfToVariable		( WORD Const, void * Variable, char * VariableName );
void RSPCompConstToX86reg				( int x86Reg, DWORD Const );
void RSPCompX86regToVariable			( int x86Reg, void * Variable, char * VariableName );
void RSPCompVariableToX86reg			( int x86Reg, void * Variable, char * VariableName );
void RSPCompX86RegToX86Reg				( int Destination, int Source );
void RSPCwd							( void );
void RSPCwde							( void );
void RSPDecX86reg						( int x86Reg );
void RSPDivX86reg						( int x86reg );
void RSPidivX86reg						( int x86reg );
void RSPimulX86reg						( int x86reg );
void RSPImulX86RegToX86Reg				( int Destination, int Source );
void RSPIncX86reg						( int x86Reg );
void RSPJaeLabel32						( char * Label, DWORD Value );
void RSPJaLabel8						( char * Label, BYTE Value );
void RSPJaLabel32						( char * Label, DWORD Value );
void RSPJbLabel8						( char * Label, BYTE Value );
void RSPJbLabel32						( char * Label, DWORD Value );
void RSPJeLabel8						( char * Label, BYTE Value );
void RSPJeLabel32						( char * Label, DWORD Value );
void RSPJgeLabel8						( char * Label, BYTE Value );
void RSPJgeLabel32						( char * Label, DWORD Value );
void RSPJgLabel8						( char * Label, BYTE Value );
void RSPJgLabel32						( char * Label, DWORD Value );
void RSPJleLabel8						( char * Label, BYTE Value );
void RSPJleLabel32						( char * Label, DWORD Value );
void RSPJlLabel8						( char * Label, BYTE Value );
void RSPJlLabel32						( char * Label, DWORD Value );
void RSPJumpX86Reg						( int x86reg );
void RSPJmpLabel8						( char * Label, BYTE Value );
void RSPJmpLabel32						( char * Label, DWORD Value );
void RSPJneLabel8						( char * Label, BYTE Value );
void RSPJneLabel32						( char * Label, DWORD Value );
void RSPJnsLabel8						( char * Label, BYTE Value );
void RSPJnsLabel32						( char * Label, DWORD Value );
void RSPJsLabel32						( char * Label, DWORD Value );
void RSPLeaSourceAndOffset				( int x86DestReg, int x86SourceReg, int offset );
void RSPMoveConstByteToN64Mem			( BYTE Const, int AddrReg );
void RSPMoveConstHalfToN64Mem			( WORD Const, int AddrReg );
void RSPMoveConstByteToVariable		( BYTE Const,void *Variable, char *VariableName );
void RSPMoveConstHalfToVariable		( WORD Const, void *Variable, char *VariableName );
void RSPMoveConstToN64Mem				( DWORD Const, int AddrReg );
void RSPMoveConstToN64MemDisp			( DWORD Const, int AddrReg, BYTE Disp );
void RSPMoveConstToVariable			( DWORD Const, void *Variable, char *VariableName );
void RSPMoveConstToX86reg				( DWORD Const, int x86reg );
void RSPMoveOffsetToX86reg				( DWORD Const, char * VariableName, int x86reg );
void RSPMoveX86regByteToX86regPointer	( int Source, int AddrReg );
void RSPMoveX86regHalfToX86regPointer	( int Source, int AddrReg );
void RSPMoveX86regHalfToX86regPointerDisp ( int Source, int AddrReg, BYTE Disp);
void RSPMoveX86regToX86regPointer		( int Source, int AddrReg );
void RSPMoveX86RegToX86regPointerDisp	( int Source, int AddrReg, BYTE Disp );
void RSPMoveX86regPointerToX86regByte	( int Destination, int AddrReg );
void RSPMoveX86regPointerToX86regHalf	( int Destination, int AddrReg );
void RSPMoveX86regPointerToX86reg		( int Destination, int AddrReg );
void RSPMoveN64MemDispToX86reg			( int x86reg, int AddrReg, BYTE Disp );
void RSPMoveN64MemToX86reg				( int x86reg, int AddrReg );
void RSPMoveN64MemToX86regByte			( int x86reg, int AddrReg );
void RSPMoveN64MemToX86regHalf			( int x86reg, int AddrReg );
void RSPMoveX86regByteToN64Mem			( int x86reg, int AddrReg );
void RSPMoveX86regByteToVariable		( int x86reg, void * Variable, char * VariableName );
void RSPMoveX86regHalfToN64Mem			( int x86reg, int AddrReg );
void RSPMoveX86regHalfToVariable		( int x86reg, void * Variable, char * VariableName );
void RSPMoveX86regToN64Mem				( int x86reg, int AddrReg );
void RSPMoveX86regToN64MemDisp			( int x86reg, int AddrReg, BYTE Disp );
void RSPMoveX86regToVariable			( int x86reg, void * Variable, char * VariableName );
void RSPMoveX86RegToX86Reg				( int Source, int Destination );
void RSPMoveVariableToX86reg			( void *Variable, char *VariableName, int x86reg );
void RSPMoveVariableToX86regByte		( void *Variable, char *VariableName, int x86reg );
void RSPMoveVariableToX86regHalf		( void *Variable, char *VariableName, int x86reg );
void RSPMoveSxX86RegHalfToX86Reg		( int Source, int Destination );
void RSPMoveSxX86RegPtrDispToX86RegHalf( int AddrReg, BYTE Disp, int Destination );
void RSPMoveSxN64MemToX86regByte		( int x86reg, int AddrReg );
void RSPMoveSxN64MemToX86regHalf		( int x86reg, int AddrReg );
void RSPMoveSxVariableToX86regHalf		( void *Variable, char *VariableName, int x86reg );
void RSPMoveZxX86RegHalfToX86Reg		( int Source, int Destination );
void RSPMoveZxX86RegPtrDispToX86RegHalf( int AddrReg, BYTE Disp, int Destination );
void RSPMoveZxN64MemToX86regByte		( int x86reg, int AddrReg );
void RSPMoveZxN64MemToX86regHalf		( int x86reg, int AddrReg );
void RSPMoveZxVariableToX86regHalf		( void *Variable, char *VariableName, int x86reg );
void RSPMulX86reg						( int x86reg );
void RSPNegateX86reg					( int x86reg );
void RSPOrConstToVariable				( DWORD Const, void * Variable, char * VariableName );
void RSPOrConstToX86Reg				( DWORD Const, int  x86Reg );
void RSPOrVariableToX86Reg				( void * Variable, char * VariableName, int x86Reg );
void RSPOrVariableToX86regHalf			( void * Variable, char * VariableName, int x86Reg );
void RSPOrX86RegToVariable				( void * Variable, char * VariableName, int x86Reg );
void RSPOrX86RegToX86Reg				( int Destination, int Source );
void RSPPopad							( void );
void RSPPushad							( void );
void RSPPush							( int x86reg );
void RSPPop							( int x86reg );
void RSPPushImm32						( char * String, DWORD Value );
void RSPRet							( void );
void RSPSeta							( int x86reg );
void RSPSetae							( int x86reg );
void RSPSetl							( int x86reg );
void RSPSetb							( int x86reg );
void RSPSetg							( int x86reg );
void RSPSetz							( int x86reg );
void RSPSetnz							( int x86reg );
void RSPSetlVariable					( void * Variable, char * VariableName );
void RSPSetleVariable					( void * Variable, char * VariableName );
void RSPSetgVariable					( void * Variable, char * VariableName );
void RSPSetgeVariable					( void * Variable, char * VariableName );
void RSPSetbVariable					( void * Variable, char * VariableName );
void RSPSetaVariable					( void * Variable, char * VariableName );
void RSPSetzVariable					( void * Variable, char * VariableName );
void RSPSetnzVariable					( void * Variable, char * VariableName );
void RSPShiftLeftSign					( int x86reg );
void RSPShiftLeftSignImmed				( int x86reg, BYTE Immediate );
void RSPShiftLeftSignVariableImmed		( void *Variable, char *VariableName, BYTE Immediate );
void RSPShiftRightSignImmed			( int x86reg, BYTE Immediate );
void RSPShiftRightSignVariableImmed	( void *Variable, char *VariableName, BYTE Immediate );
void RSPShiftRightUnsign				( int x86reg );
void RSPShiftRightUnsignImmed			( int x86reg, BYTE Immediate );
void RSPShiftRightUnsignVariableImmed	( void *Variable, char *VariableName, BYTE Immediate );
void RSPShiftLeftDoubleImmed			( int Destination, int Source, BYTE Immediate );
void RSPShiftRightDoubleImmed			( int Destination, int Source, BYTE Immediate );
void RSPSubConstFromVariable			( DWORD Const, void *Variable, char *VariableName );
void RSPSubConstFromX86Reg				( int x86Reg, DWORD Const );
void RSPSubVariableFromX86reg			( int x86reg, void * Variable, char * VariableName );
void RSPSubX86RegToX86Reg				( int Destination, int Source );
void RSPSubX86regFromVariable			( int x86reg, void * Variable, char * VariableName );
void RSPSbbX86RegToX86Reg				( int Destination, int Source );
void RSPTestConstToVariable			( DWORD Const, void * Variable, char * VariableName );
void RSPTestConstToX86Reg				( DWORD Const, int x86reg );
void RSPTestX86RegToX86Reg				( int Destination, int Source );
void RSPXorConstToX86Reg				( int x86Reg, DWORD Const );
void RSPXorX86RegToX86Reg				( int Source, int Destination );
void RSPXorVariableToX86reg			( void *Variable, char *VariableName, int x86reg );
void RSPXorX86RegToVariable			( void *Variable, char *VariableName, int x86reg );
void RSPXorConstToVariable				( void *Variable, char *VariableName, DWORD Const );

#define _MMX_SHUFFLE(a, b, c, d)	\
	((BYTE)(((a) << 6) | ((b) << 4) | ((c) << 2) | (d)))

void RSPMmxMoveRegToReg				( int Dest, int Source );
void RSPMmxMoveQwordRegToVariable		( int Dest, void *Variable, char *VariableName );
void RSPMmxMoveQwordVariableToReg		( int Dest, void *Variable, char *VariableName );
void RSPMmxPandRegToReg				( int Dest, int Source );
void RSPMmxPandnRegToReg				( int Dest, int Source );
void RSPMmxPandVariableToReg			( void * Variable, char * VariableName, int Dest );
void RSPMmxPorRegToReg					( int Dest, int Source );
void RSPMmxPorVariableToReg			( void * Variable, char * VariableName, int Dest );
void RSPMmxXorRegToReg					( int Dest, int Source );
void RSPMmxShuffleMemoryToReg			( int Dest, void * Variable, char * VariableName, BYTE Immed );
void RSPMmxPmullwRegToReg				( int Dest, int Source );
void RSPMmxPmullwVariableToReg			( int Dest, void * Variable, char * VariableName );
void RSPMmxPmulhuwRegToReg				( int Dest, int Source );
void RSPMmxPmulhwRegToReg				( int Dest, int Source );
void RSPMmxPmulhwRegToVariable			( int Dest, void * Variable, char * VariableName );
void RSPMmxPsrlwImmed					( int Dest, BYTE Immed );
void RSPMmxPsrawImmed					( int Dest, BYTE Immed );
void RSPMmxPsllwImmed					( int Dest, BYTE Immed );
void RSPMmxPaddswRegToReg				( int Dest, int Source );
void RSPMmxPaddswVariableToReg			( int Dest, void * Variable, char * VariableName );
void RSPMmxPaddwRegToReg				( int Dest, int Source );
void RSPMmxPackSignedDwords			( int Dest, int Source );
void RSPMmxUnpackLowWord				( int Dest, int Source );
void RSPMmxUnpackHighWord				( int Dest, int Source );
void RSPMmxCompareGreaterWordRegToReg	( int Dest, int Source );
void RSPMmxEmptyMultimediaState		( void );

void RSPSseMoveAlignedVariableToReg	( void *Variable, char *VariableName, int sseReg );
void RSPSseMoveAlignedRegToVariable	( int sseReg, void *Variable, char *VariableName );
void RSPSseMoveAlignedN64MemToReg		( int sseReg, int AddrReg );
void RSPSseMoveAlignedRegToN64Mem		( int sseReg, int AddrReg );
void RSPSseMoveUnalignedVariableToReg	( void *Variable, char *VariableName, int sseReg );
void RSPSseMoveUnalignedRegToVariable	( int sseReg, void *Variable, char *VariableName );
void RSPSseMoveUnalignedN64MemToReg	( int sseReg, int AddrReg );
void RSPSseMoveUnalignedRegToN64Mem	( int sseReg, int AddrReg );
void RSPSseMoveRegToReg				( int Dest, int Source );
void RSPSseXorRegToReg					( int Dest, int Source );

typedef struct {
	union {
		struct {
			unsigned Reg0 : 2;
			unsigned Reg1 : 2;
			unsigned Reg2 : 2;
			unsigned Reg3 : 2;
		};
		unsigned UB:8;
	};
} SHUFFLE;

void RSPSseShuffleReg					( int Dest, int Source, BYTE Immed );

void RSPx86_SetBranch32b(void * JumpByte, void * Destination);
void RSPx86_SetBranch8b(void * JumpByte, void * Destination);