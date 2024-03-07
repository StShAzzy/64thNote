#include <windows.h>
#include <math.h>
#include "main.h"
#include "in2.h"
#include "cpu.h"
#include "audio.h"
#include "plugin.h"
#include "rom.h"
                        /*576*/
#define BUFFER_SIZE		(576)
#define SEGMENTS 4
#define WINAMP_BUFFER_SIZE (BUFFER_SIZE*4)
#define MAXBUFFER (WINAMP_BUFFER_SIZE*SEGMENTS)

#define WM_WA_MPEG_EOF WM_USER+2

BYTE SoundBuffer[MAXBUFFER];

DWORD Dacrate = 0;
void AiCallBack (DWORD Status);
short sample_buffer[BUFFER_SIZE*2*2]; // DSP can return 2x samples
#define SILENTTHRESH 20

int realvolume; // set before we have a working output plugin to write to
double relvol = 1.0;
SHORT preamp[256*256];
int maxlatency;

DWORD writeLoc,readLoc;
DWORD remainingBytes;

long TrackLength,FadeLength;
int silentcount,silencelength;
long silencedetected;
double time,nexttime,seektime;
int SampleRate;
int audioIsPlaying;

void AudioDeInitialize (void);
void StopAudio (void);
void StartAudio (void);

void InitAmpTab(void) {
	unsigned long c;

	if (softsaturate) {
		// Amplification is done via "saturation", out=1-(1-in)^(amp level). This approximates linear amp.
		// for low amplitudes but flattens out at high levels
		// Distorts some sounds, flutes especially

		for (c=0; c < 128*256; c++) {
			preamp[(c^0xffff)+1]=-(preamp[c]=(1.0-pow(1.0-((double)c)/(128.0*256.0),relvol))*(128.0*256.0));
		}
	} else {
		// pure linear amp w/ clipping
		for (c=0; c < 128*256; c++) {
			preamp[(c^0xffff)+1]=-(preamp[c]=(relvol*c < 128*256)?(relvol*c):(128*256-1));
		}
	}
	// negative range is ever so slightly larger than positive range
	// obviously we'll want to scale this down if relvol < 1
	preamp[0x8000]=(relvol < 1.0)?(-128.0*256.0*relvol):(-128.0*256.0);
}

DWORD bufSize=5096, bufAt=0;

void InitWinampOutput(void) {
	silentcount=0;
	maxlatency=mod.outMod->Open(SampleRate,2,16, -1,-1);
	if (maxlatency < 0) {
		DisplayError("Failed to open output plugin");
		SampleRate=0;
		return;
	}
	mod.SetInfo(16,SampleRate/1000,2,1);
	mod.SAVSAInit(maxlatency,SampleRate);
	mod.VSASetInfo(SampleRate,2);
	mod.outMod->SetVolume(realvolume);
}
void CloseWinampOutput(void) {
	mod.outMod->Close();
	mod.SAVSADeInit();
}

DWORD lastword=0, doingwhat=0, vbuffer2=0;
DWORD laststatus = 0;


BOOL AudioInitialize (void) {
	AudioDeInitialize ();
	audioIsPlaying = FALSE;
	silentcount=0;
	time=nexttime=0;
	Dacrate=0;
    bufAt=0;
    vbuffer2=0;
	return TRUE;
}

void AudioDeInitialize () {
	StopAudio ();
}

#define AI_STATUS_FIFO_FULL	0x80000000		/* Bit 31: full */
#define AI_STATUS_DMA_BUSY	0x40000000		/* Bit 30: busy */
#define MI_INTR_AI			0x04			/* Bit 2: AI intr */


void UpdateStatus()
{
	if(vbuffer2==0) {
	//	vbuffer2=1;
	//	DisplayError("%08x\n",AI_STATUS_REG);
	}
 //if(mod.outMod->CanWrite() < (remainingBytes)) {  // The FIFO is still full and DMA is busy...

 if(AI_LEN_REG>2048) {
    if(AI_STATUS_REG&0x40000000)
    	AI_STATUS_REG|=0x80000000;
    AudioIntrReg|=4;
	CheckInterrupts();
 }

if((bufAt>=bufSize)) {


 if(AI_LEN_REG>2848) {
    //if(AI_STATUS_REG&0x40000000) AI_STATUS_REG|=0x80000000;
 	//AudioIntrReg|=4;
	//CheckInterrupts();
 }
 //if(!(laststatus&0x80000000)) {
 	//AI_STATUS_REG=0xC0000000;
 	//AI_LEN_REG=remainingBytes;
 	//laststatus=0xC0000000;
 	return;
 }

 if(laststatus==0xC0000000) { // Then we need to generate an interrupt now...
 	AI_LEN_REG=remainingBytes;
	AI_STATUS_REG=0x40000000;
	AudioIntrReg|=4;
	CheckInterrupts();
	laststatus=0x40000000;
	return;
 }

 if(laststatus==0x40000000) {
 	if(mod.outMod->CanWrite() < remainingBytes) {
 		AI_LEN_REG=0;
 		AudioIntrReg|=4;
		CheckInterrupts();
		laststatus=0x0;
		AI_STATUS_REG=0;
 	}
	return;
 }

}

 int qt=0;



void AddBuffer (BYTE *start, DWORD length) {
	DWORD c;
	int t;
	//int fadetype=1; // 0=linear, 1=logarithmic, 2=cosine
	static int seeklast=0;
	static short lastsample=0;

	if(qt==0) {
	//	AllocConsole();
		qt=1;
	}
	//cprintf("%08x %08x %08x %08x\n", AI_LEN_REG,AI_STATUS_REG,);

	bufAt+=length;

	if (length == 0) return;

	if (!audioIsPlaying) StartAudio ();

	time = nexttime;
	nexttime += (double)length*(double)1000.0/((double)SampleRate*4.0);

	if (nexttime >= seektime) {
		// Copy from AI buffer to internal buffer

		// process fade
		if (FadeLength && nexttime >= TrackLength-FadeLength) {
			for (c=0;c<length/2;c++) {
				if (time+c*2*1000.0/SampleRate < TrackLength-FadeLength) {
					// no fade yet
					*((short*)(SoundBuffer+writeLoc))=preamp[((unsigned short*)start)[c^1]];
				} else if (time+c*2*1000.0/SampleRate >= TrackLength) {
					// insert silence after end of track (after fade)
					*((short*)(SoundBuffer+writeLoc))=0;
				} else {
					// do fade

					switch (FadeType) {
					case 0:
						// linear
						*((short*)(SoundBuffer+writeLoc))=((double)preamp[((unsigned short*)start)[c^1]])*
(TrackLength-(time+c*2*1000.0/SampleRate))/FadeLength;
						break;
					case 1:
						// logarithmic
						*((short*)(SoundBuffer+writeLoc))=((double)preamp[((unsigned short*)start)[c^1]])*
(1.0-log(-(exp(32767.0/32768.0)-1.0)*(TrackLength-FadeLength-(time+c*2*1000.0/SampleRate))/FadeLength+1.0));
//exp(-log(50.0/32678)*(TrackLength-FadeLength-(time+c*2*1000.0/SampleRate))/FadeLength);
						break;
					case 2:
						// cosine
						*((short*)(SoundBuffer+writeLoc))=((double)preamp[((unsigned short*)start)[c^1]])*
((cos((TrackLength-FadeLength-(time+c*2*1000.0/SampleRate))*3.14159/FadeLength)+1.0)/2.0);
						break;
					case 3:
						// none
						*((short*)(SoundBuffer+writeLoc))=preamp[((unsigned short*)start)[c^1]];
						break;
					}
				}
				if ((writeLoc+=2)==MAXBUFFER) writeLoc=0;
			}
		// insert silence after end of track
		} else if (TrackLength > 0 && nexttime >= TrackLength) {
			for (c=0;c<length/2;c++) {
				if (time+c*2*1000.0/SampleRate < TrackLength-FadeLength)
					// no fade, normal copy
					*((short*)(SoundBuffer+writeLoc))=preamp[((unsigned short*)start)[c^1]];
				else
					// past end of track
					*((WORD*)(SoundBuffer+writeLoc))=0;
				if ((writeLoc+=2)==MAXBUFFER) writeLoc=0;
			}
		// no end of track condition
		} else {
			if (!DetectSilence) {
				for (c=0;c<length/2;c++) {
					*((short*)(SoundBuffer+writeLoc))=preamp[((unsigned short*)start)[c^1]];
					if ((writeLoc+=2)==MAXBUFFER) writeLoc=0;
				}
			} else {
				for (c=0;c<length/2;c++) {
					if (abs(lastsample-((short*)start)[c^1]) < SILENTTHRESH) {
						silentcount++;
					} else {
						silentcount=0;
						// only update last sample when no silence, this should prevent a continuous
						// small delta from being detected as silence
						lastsample=*((short*)(SoundBuffer+writeLoc))=((short*)start)[c^1];
					}
					*((short*)(SoundBuffer+writeLoc))=preamp[((unsigned short*)start)[c^1]];
					if ((writeLoc+=2)==MAXBUFFER) writeLoc=0;

					// set silencedetected as the time when the silence started, not
					// just a flag (+1 so it'll be nonzero even if the whole track is
					// silent)
					// Then we can check to make sure we're actually past the start of the
					// silence before we kill the track, in case the user has huge
					// buffers
					if (silentcount >= silencelength*SampleRate*2) silencedetected=nexttime-silentcount*1000/SampleRate/2+1;
				}
			}
		}


		remainingBytes+=length;
        UpdateStatus();
		if (remainingBytes >= WINAMP_BUFFER_SIZE) {

			// Send from internal buffer to Winamp


			while (mod.outMod->CanWrite() < (remainingBytes/WINAMP_BUFFER_SIZE)*WINAMP_BUFFER_SIZE) {
				UpdateStatus();
				Sleep(1);
			}

			while (remainingBytes >= WINAMP_BUFFER_SIZE) {
				t=mod.outMod->GetWrittenTime();
				mod.SAAddPCMData((char*)SoundBuffer+readLoc,2,16,t);
				mod.VSAAddPCMData((char*)SoundBuffer+readLoc,2,16,t);

				// DSP?
				if (mod.dsp_isactive()) {
					int samples;
					memcpy(sample_buffer,(char*)SoundBuffer+readLoc,WINAMP_BUFFER_SIZE);
					samples=mod.dsp_dosamples(sample_buffer,BUFFER_SIZE,16,2,SampleRate);
					mod.outMod->Write((char*)sample_buffer,samples*4);
				} else mod.outMod->Write((char*)SoundBuffer+readLoc,WINAMP_BUFFER_SIZE);
				remainingBytes-=WINAMP_BUFFER_SIZE;
				UpdateStatus();
				bufAt-=WINAMP_BUFFER_SIZE;
				if ((readLoc+=WINAMP_BUFFER_SIZE)==MAXBUFFER) readLoc=0;
			}
		}

		// go back to configured priority when done seeking
		if (seeklast) SetThreadPriority(hCPU, priarray[CPUPriority]);
		seeklast=0;
	} else {
		mod.outMod->Flush((int)time);
		writeLoc = 0x0000;
		readLoc = 0x0000;
		remainingBytes = 0;
		// be friendly while seeking
		if (!seeklast) SetThreadPriority(hCPU, THREAD_PRIORITY_NORMAL);
		seeklast=1;
	}
}

void StopAudio () {
	if (!audioIsPlaying) return;
	audioIsPlaying = FALSE;
	CloseWinampOutput();
}

void StartAudio () {

	if (audioIsPlaying) return;
	audioIsPlaying = TRUE;

	InitWinampOutput();

	writeLoc = 0x0000;
	readLoc = 0x0000;
	remainingBytes = 0;
	time=nexttime=0;
	silencedetected=0;

	InitAmpTab();
}

DWORD IsFreqSet(void) {
	return SampleRate!=0;
}

DWORD GetReadStatus () {
	return 0;
}

void AiClose (void){
	StopAudio ();
	Dacrate = 0;
	bufAt=0;
	AudioDeInitialize ();
}

void AiDacrateChanged (int  SystemType) {
	if (Dacrate != AI_DACRATE_REG) {
		Dacrate = AI_DACRATE_REG;
		switch (SystemType) {
			case SYSTEM_NTSC:

				SampleRate = 48681812 / (Dacrate + 1);

				if (round_frequency) {
					char inikey[5],iniFile[MAX_PATH+1];
					DWORD tempsrate;

					// calculate a more even freq, algorithm suggested by Chris Moeller
					SampleRate = SampleRate+25;
					SampleRate -= SampleRate % 50;

					// look up a value in INI to get something exact

					if (GetModuleFileName(GetModuleHandle(BinaryName), iniFile, MAX_PATH)) {
						char * lastSlash = strrchr(iniFile, '\\');

						*(lastSlash + 1) = 0;
						strcat(iniFile, InternalININame);
					}

					sprintf(inikey,"%#04x",Dacrate);

					tempsrate = GetPrivateProfileInt("roundfreq",inikey,0,iniFile);
					if (tempsrate) SampleRate = tempsrate;
				}
				break;
			case SYSTEM_PAL:  SampleRate = 49656530 / (Dacrate + 1); break;
			case SYSTEM_MPAL: SampleRate = 48628316 / (Dacrate + 1); break;
		}
	}
}

void AiLenChanged (void) {
	if ((TrackLength > 0 && time > TrackLength)||(silencedetected && time > silencedetected)) {
		// a) avoid stopping more than once (StopAudio will clear audioIsPlaying)
		// b) don't send any more buffers after track has ended
		// c) don't return from this function until play has stopped, otherwise since
		//    we're not waiting for a buffer to clear up we emulate as fast as possible,
		//    especially bad with > Normal priority
		if (audioIsPlaying) {
			while (mod.outMod->IsPlaying()) Sleep(50);
			StopAudio();
			PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
		} else DisplayError("track end AiLenChanged but audioIsPlaying not set");
	} else {
		AddBuffer (
			(RDRAM + (AI_DRAM_ADDR_REG & 0x00FFFFF8)),
			AI_LEN_REG & 0x3FFF8);
	}


	//DisplayError("length=%08x",AI_LEN_REG);

	if (EnableFIFOfull)
	{
	//	if (AI_STATUS_REG&0x40000000) AI_STATUS_REG|=0x80000000;
	//	else AI_STATUS_REG|=0x40000000;
	AI_STATUS_REG|=0x40000000;
	}

	StartAiInterrupt();
}

void StartAiInterrupt(void) {
	// delay calculation from PJ64 v1.7
	const float VSyncTiming = 789000.0f;
	double BytesPerSecond = 48681812.0 / (AI_DACRATE_REG + 1) * 4;
	double CountsPerSecond = (double)(((double)VSyncTiming) * (double)60.0);
	double CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
	DWORD IntScheduled = (DWORD)((double)AI_LEN_REG * CountsPerByte);

	if (!AI_LEN_REG) return;

	ChangeTimer(AiTimer,IntScheduled);
	//ChangeTimer(AiTimer,IntScheduled*95/100);
	//ChangeTimer(AiTimer,IntScheduled*88/100);

	//AI_STATUS_REG|=0x80000000;
}

DWORD AiReadLength (void){
	AI_LEN_REG = GetReadStatus ();

	return AI_LEN_REG;
}

DWORD IsSeeking(void) {
	if (FastSeek && time < seektime) return 1;
	return 0;
}


long PlayTime() {
	if (audioIsPlaying && IsFreqSet()) return mod.outMod->GetOutputTime();
	else return 0;
}

void SeekTo(int seek_dest) {
	seektime=seek_dest;
}