DWORD IsFreqSet(void);
BOOL __cdecl AudioInitialize (void);
void __cdecl AiClose (void);
void AiDacrateChanged (int  SystemType);
void AiLenChanged (void);
void StartAiInterrupt(void);
DWORD AiReadLength (void);
DWORD IsSeeking(void);
long PlayTime();
void SeekTo(int seek_dest);

extern int realvolume;
extern long TrackLength, FadeLength;
extern double relvol;
