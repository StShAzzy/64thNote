#ifndef RSP_H
#define RSP_H

DWORD DoRspCycles (DWORD Cycles);
void __cdecl CloseRSP (void);
void __cdecl InitiateRSP ( void );
void RSPRomClosed (void);

#endif