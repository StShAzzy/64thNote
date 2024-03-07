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
#include "memory.h"
#include "plugin.h"
#include "psftag.h"
#include "rom.h"
#include "audio.h"

#define SaveStateSize (4204380*2)

#define MenuLocOfUsedFiles	11
#define MenuLocOfUsedDirs	(MenuLocOfUsedFiles + 1)

char CurrentFileName[MAX_PATH+1] = {""}, RomName[MAX_PATH+1] = {""}, RomHeader[0x1000];
char LastRoms[10][MAX_PATH+1], LastDirs[10][MAX_PATH+1];
const DWORD MEG = 0x100000;
BYTE * savestatespace;

int ReadSparse( HANDLE hFile, BYTE * buffer );
int ReadSparseROM( HANDLE hFile );
int EnableCompare=0;
int EnableFIFOfull=0;

BOOL IsValidUSF ( BYTE Test[4] ) {
	if ( *((DWORD *)&Test[0]) == 0x21465350 ) { return TRUE; } // "PSF!"
	return FALSE;
}

BOOL IsValidSparseRom ( BYTE Test[4] ) {
	if ( *((DWORD *)&Test[0]) == 0x34365253 ) { return TRUE; }
	return FALSE;
}

BOOL IsTagPresent ( BYTE Test[5] ) {
	if ( *((DWORD *)&Test[0]) == 0x4741545b && Test[4]==0x5D) {return TRUE;}
	return FALSE;
}

unsigned long checksum32(HANDLE hFile,unsigned int length) {
	unsigned int c;
	unsigned char databuf[0x1000];
	DWORD dwRead;
	unsigned long sum = 0;

	if ( length == 0 ) return 0;

	while (length >= 0x1000 && ReadFile(hFile,databuf,0x1000,&dwRead,NULL) != 0) {
		for (c=0; c < 0x1000; c++) sum+=databuf[c];
		length-=0x1000;
	}

	if (length) {
		ReadFile(hFile,databuf,length,&dwRead,NULL);

		for (c=0; c < length; c++) sum+=databuf[c];
	}

	return sum;
}

int ReadUSF( const char * FileName ) {
	DWORD dwRead;
	DWORD reservestart;
	DWORD tagstart;
	DWORD taglen;
	DWORD codesize;
	DWORD reservesize;
	HANDLE hFile;
	BYTE length[256],fade[256],buf[256],rvol[256];
	char lib[_MAX_PATH],drive[_MAX_DRIVE],usfFileName[_MAX_DIR],dir[_MAX_DIR], ext[_MAX_EXT];
	char libpath[_MAX_PATH];

	BYTE Test[5];
	char *psftag;

	_splitpath( FileName, drive, dir, usfFileName, ext );

	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		DisplayError("Failed to open file\n%s",FileName);
		return 1;
	}

	ReadFile(hFile,Test,4,&dwRead,NULL);
	if (!IsValidUSF(Test)) {
		DisplayError("Not a USF file\n%s",FileName);
		CloseHandle(hFile);
		return 1;
	}
	// Read USF contents
	ReadFile(hFile,&reservesize,4,&dwRead,NULL); // size of reserved area
	ReadFile(hFile,&codesize,4,&dwRead,NULL); // size of program
	ReadFile(hFile,Test,4,&dwRead,NULL); // CRC (discarded)

	reservestart=SetFilePointer(hFile,0,0,FILE_CURRENT);
	tagstart=reservestart+codesize+reservesize;

	// check for AutoAudioHLE
	if (AutoAudioHLE && !AutoAudioHLEtriggered) {
		char inientry[MAX_PATH+16+1],iniFile[MAX_PATH+1];

		if (GetModuleFileName(GetModuleHandle(BinaryName), iniFile, MAX_PATH)) {
			char * lastSlash = strrchr(iniFile, '\\');

			*(lastSlash + 1) = 0;
			strcat(iniFile, InternalININame);
		}

		// check for proper INI version
		if (GetPrivateProfileInt("meta","HLEversion",0,iniFile) == AudioHLEVersion) {

			SetFilePointer(hFile,reservestart,0,FILE_BEGIN);
			sprintf(inientry,"%s%s_%08x",usfFileName,ext,checksum32(hFile,reservesize));

			if (GetPrivateProfileInt(inientry,"useHLE",0,iniFile)) {
				//DisplayError("triggered for %s",inientry);
				AutoAudioHLEtriggered=TRUE;
			}
		}
	}

	SetFilePointer(hFile,tagstart,0,FILE_BEGIN);

	ReadFile(hFile,Test,5,&dwRead,NULL);
	// read tags if there are any
	if (IsTagPresent(Test)) {
		tagstart+=5;
		taglen=SetFilePointer(hFile,0,0,FILE_END)-tagstart;
		SetFilePointer(hFile,tagstart,0,FILE_BEGIN);
		if (taglen > 0) {
			//psftag=malloc(taglen+2);
			psftag=VirtualAlloc(NULL, 50001, MEM_COMMIT, PAGE_READWRITE); //malloc(50001);

			ReadFile(hFile,psftag,taglen,&dwRead,NULL);
			psftag[taglen]='\0';

			psftag_raw_getvar(psftag,"_lib",lib,sizeof(lib)-1); // no _libn support

			// revised so tags in usflib don't reset length
			psftag_raw_getvar(psftag,"length",length,sizeof(length)-1);
			if (strlen(length)) TrackLength=LengthFromString(length);

			psftag_raw_getvar(psftag,"fade",fade,sizeof(fade)-1);
			if (strlen(fade)) {
				FadeLength=LengthFromString(fade);
				TrackLength+=FadeLength; // comply with PSF standard timing:
			}							 // "length" tag specifies length before fade,
										 // "fade" specifies length of fade
			if (SetLength) {
				TrackLength=(deflen+deffade)*1000;
				FadeLength=deffade*1000;
			}

			if (IgnoreTrackLength) {
				TrackLength=0;
				FadeLength=0;
			}

			psftag_raw_getvar(psftag,"volume",rvol,sizeof(rvol)-1);
			if (strlen(rvol)) {
				sscanf(rvol,"%lf",&relvol);
				relvol*=defrelvol;
			}

			// Check for bug tags:
			// _64thbug1: Pilot Wings64 can't be run under the recompiler as of 9/15/04
			// _64thbug2: Goldeneye can't be run under the recompiler as of 9/15/04

			//psftag_raw_getvar(psftag,"_64thbug1",buf,sizeof(buf)-1);
			//if (strlen(buf)>0) forceinterpreter=1;
			//psftag_raw_getvar(psftag,"_64thbug2",buf,sizeof(buf)-1);
			//if (strlen(buf)>0) forceinterpreter=1;

			// Most often you don't want the count=compare interrupt, but sometimes... (i.e. PD,BK!)
			//EnableCompare=0;
			psftag_raw_getvar(psftag,"_enablecompare",buf,sizeof(buf)-1);
			if (strlen(buf)>0) EnableCompare=1;

			// emulation of the AI FIFO full flag breaks some sets, but Excitebike needs it
		//	EnableFIFOfull=1;
			psftag_raw_getvar(psftag,"_enableFIFOfull",buf,sizeof(buf)-1);
			if (strlen(buf)>0) EnableFIFOfull=1;

			//free(psftag);
			VirtualFree(psftag,0,MEM_RELEASE);
			psftag=NULL;

			// load usflib if lib tag found
			if (strlen(lib) > 0) {
				sprintf(libpath,"%s%s%s",drive,dir,lib);

				if (ReadUSF(libpath)) return 1;
			}
		}
	}

	if (DefaultLength && TrackLength == 0) {
		TrackLength=(deflen+deffade)*1000;
		FadeLength=deffade*1000;
	}


	SetFilePointer(hFile,reservestart,0,FILE_BEGIN);

	if (codesize > 0) {
		DisplayError("What's in code?\nYou are probably using an old format USF, download a new version.");
		return 1;
	}
	if (reservesize > 0) {
		if (ReadSparseROM(hFile)) return 1;
		if (ReadSparse(hFile,savestatespace)) return 1;
	}

	CloseHandle(hFile);
	return 0;
}

int ReadSparse( HANDLE hFile, BYTE * buffer ) {
	DWORD dwRead;
	DWORD len;
	DWORD start;
	BYTE Test[5];

	ReadFile(hFile,Test,4,&dwRead,NULL); // SR64 header
	if (!IsValidSparseRom(Test)) {
		if (*((unsigned long*)Test)==0) return 0; // empty section
		DisplayError("Bad sparse ROM");
		CloseHandle(hFile);
		return 1;
	}

	ReadFile(hFile,&len,4,&dwRead,NULL); // first len
	while (len) {
		ReadFile(hFile,&start,4,&dwRead,NULL); // start addr
		//DisplayError("len=%08x start=%08x",len,start);
		if (!ReadFile(hFile,&buffer[start],len,&dwRead,NULL)) {
			DisplayError("Failed to copy sparse to memory\nlen=%08x",len);
			CloseHandle( hFile );
			return 1;
		}
		ReadFile(hFile,&len,4,&dwRead,NULL);
	}
	return 0;
}

int ReadSparseROM( HANDLE hFile ) {
	DWORD dwRead;
	DWORD len;
	DWORD start;
	BYTE Test[5];
	DWORD startpage,readlen;

	ReadFile(hFile,Test,4,&dwRead,NULL); // SR64 header
	if (!IsValidSparseRom(Test)) {
		if (*((unsigned long*)Test)==0) return 0; // empty section
		CloseHandle(hFile);
		return 1;
	}

	ReadFile(hFile,&len,4,&dwRead,NULL); // first len
	while (len) {
		ReadFile(hFile,&start,4,&dwRead,NULL); // start addr
		//DisplayError("len=%08x start=%08x",len,start);
		while (len) {
			startpage=start/MEG;
			readlen=((start+len)/MEG>startpage)?(startpage+1)*MEG-start:len;
			if (!ROMPages[startpage]) {
				if (!(ROMPages[startpage]=VirtualAlloc(NULL,MEG,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))) {
					DisplayError("Failed to load ROM page %i",startpage);
					return 1;
				}
			}
			if (!ReadFile(hFile,ROMPages[startpage]+(start%MEG),readlen,&dwRead,NULL)) {
				DisplayError("Failed to copy sparse to memory\nlen=%08x",len);
				CloseHandle( hFile );
				return 1;
			}
			len-=readlen;
			start+=readlen;
		}
		ReadFile(hFile,&len,4,&dwRead,NULL);
	}
	return 0;
}

int OpenChosenUSF ( void ) {
	int count;

	CloseCpu();

	RomFileSize = 0x4000000;

	for (count=0; count < 0x40; count++) ROMPages[count]=0;

	savestatespace = VirtualAlloc(NULL,SaveStateSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (!savestatespace) {
		DisplayError("Not enough memory to load save state");
	  return 1;
	}

	relvol=defrelvol;
	TrackLength=0;
	FadeLength=0;
	AutoAudioHLEtriggered=FALSE;

	EnableCompare=0;
	EnableFIFOfull=0;

	// this should return the "file not found" error, since we *should* be able to
	// recover from a failed load
	if (ReadUSF( CurrentFileName )) {
		DisplayError("error reading USF");
		if (savestatespace!=NULL) {VirtualFree(savestatespace,0,MEM_RELEASE); savestatespace=NULL;}
		return -1;
	}

	StartEmulationFromSave(savestatespace);

	if (savestatespace!=NULL) {VirtualFree(savestatespace,0,MEM_RELEASE); savestatespace=NULL;}

	return 0;
}