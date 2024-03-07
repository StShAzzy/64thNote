#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "in2.h"
#include "wa_ipc.h"
#include "main.h"
#include "rom.h"
#include "audio.h"
#include "psftag.h"
#include "resource.h"

// Last file name played (info fcns might be called with NULL, this fn is to be assumed)
char lastfn[MAX_PATH+1];

BOOL CALLBACK aboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Info Dialog
char infoDlgfn[MAX_PATH+1];
int infoDlgWriteNewTags;
char * infoDlgpsftag=NULL;

// used by the metadata query/save exports (5.x fun)
char extInf[MAX_PATH] = {0}, extInfwrite[MAX_PATH] = {0};
char *extInfpsftag = NULL, *extInfpsftagwrite = NULL;

BOOL IsYes(char * str) {
	if (!strcmp(str,"Yes")) return TRUE;
	if (!strcmp(str,"No")) return FALSE;
	else return FALSE;
}

char helptext[]="64th Note configuration help\r\n\r\n"
	"* Track Length options (mutually exclusive):\r\n"
	"- Play Forever - causes tracks to play forever\r\n"
	"- Always Use Default Length - all tracks will play for the specified time\r\n"
	"- Default Length - tracks without length information will use the specified time\r\n\r\n"
	"* Detect Silence - the track will end when a song is silent for the given length of time\r\n"
	"* Use Recompiler CPU - enables a fast recompiler CPU emulator, if disabled a slower interpreter will be used\r\n"
	"* Round Frequency - changes playback frequency to a more standard value, rather than the odd values that games use, uses in_usf.ini for specific values\r\n"
	"* Seek Backwards - enables the player to seek backwards\r\n"
	"* Fade Type:\r\n"
	"- linear - a basic linear fade\r\n"
	"- logarithmic - sounds smoother than linear\r\n"
	"- cosine (s-curve) - a fade preferred by some, slower at the beginning and end, often called \"s-shaped curve\" or \"S\" fade\r\n"
	"- no fade - simply cut off at end of track\r\n"
	"* Fask Seek - disables RSP emulation while seeking, faster but may cause glitches when play resumes\r\n"
	"* RSP Sections - attempts additional RSP optimization, faster but causes some emulation bugs\r\n"
	"* Soft Amplify - uses nonlinear amplification to avoid clipping, but distorts sound; NOT RECOMMENDED\r\n"
	"* Audio HLE - High Level Emulation, simulates the synthesis software to greatly improve speed, does not work well with all games\r\n"
	"* Auto Audio HLE - use the in_usf.ini file to automatically enable Audio HLE for well-supported games\r\n\r\n"
	"* CPU Thread Priority - how much CPU time 64th Note should use relative to other running processes\r\n"
	"* Display Errors - should be disabled unless you're testing for errors\r\n"
	"* Title Format - a string 64th note uses to generate track titles\r\nnames surrounded by \"%\" will be replaced by the value of the tag with that name in the USF\r\n ex. \"%title% - %game%\" becomes \"Intro - Super Game 64\"\r\n"
	"* Default to file name on missing field - if this is not checked missing tags specified in the title format will be ignored, if it is checked the file name will be used as the title when any tag is not found"
	"\r\n---\r\ncompatibility note:\r\n64th Note works best in XMPlay if you check Round Frequency and Seek Backwards\r\n";

char * fadetypes[4] = {"linear","logarithmic","cosine (s-curve)","no fade"};

char yesstring[]="Yes";
char nostring[]="No";

char * YesNoString(BOOL tf) {
	return (tf?yesstring:nostring);
}

// uses the configuration file plugin.ini in the same dir as the DLL (a la HE)
void GetINIFileName(char * iniFile){
	// if we've got a valid hwnd then we're running on a newer winamp version that better supports
	// saving of settings to a per-user directory - if not then just revert to the old behaviour
	if(IsWindow(mod.hMainWindow)) {
		strncpy(iniFile, (char*)SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETINIDIRECTORY), MAX_PATH);
		strcat(iniFile, "\\plugins\\" ExternalININame);
	}
	else {
		if (GetModuleFileName(GetModuleHandle(BinaryName), iniFile, MAX_PATH)) {
			char * lastSlash = strrchr(iniFile, '\\');

			*(lastSlash + 1) = 0;
			strcat(iniFile, ExternalININame);
		}
	}
}

void WriteSettings(void);

void ReadSettings(void) {
	char iniFile[MAX_PATH+1];
	char buf[256];

	GetINIFileName(iniFile);

	// check version

	GetPrivateProfileString(AppName,"version","",buf,sizeof(buf),iniFile);
	if (strcmp(buf,AppName AppVer)) WriteSettings(); // if not matched write defaults

	GetPrivateProfileString(AppName,"title format","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') strcpy(titlefmt,buf);

	GetPrivateProfileString(AppName,"use recompiler","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') UseRecompiler = IsYes(buf);

	GetPrivateProfileString(AppName,"Audio HLE","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') AudioHLE = IsYes(buf);

	GetPrivateProfileString(AppName,"Auto Audio HLE","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') AutoAudioHLE = IsYes(buf);

	GetPrivateProfileString(AppName,"RSP Sections","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') RSPSECTIONS = IsYes(buf);

	GetPrivateProfileString(AppName,"round frequency","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') round_frequency = IsYes(buf);

	GetPrivateProfileString(AppName,"soft amplify","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') softsaturate = IsYes(buf);

	GetPrivateProfileString(AppName,"detect silence","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') DetectSilence = IsYes(buf);

	GetPrivateProfileString(AppName,"fast seek","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') FastSeek = IsYes(buf);

	GetPrivateProfileString(AppName,"seek backwards","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') EnableBackwards = IsYes(buf);

	GetPrivateProfileString(AppName,"ignore track length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') IgnoreTrackLength = IsYes(buf);

	GetPrivateProfileString(AppName,"use default track length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') DefaultLength = IsYes(buf);

	GetPrivateProfileString(AppName,"always use given track length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') SetLength = IsYes(buf);

	GetPrivateProfileString(AppName,"title defaults to file name","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') title_default_to_file_name = IsYes(buf);

	GetPrivateProfileString(AppName,"CPU priority","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf,"%i",&CPUPriority);

	GetPrivateProfileString(AppName, "fade type","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf, "%i",&FadeType);

	GetPrivateProfileString(AppName,"default length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf,"%li",&deflen);

	GetPrivateProfileString(AppName,"default fade length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf,"%li",&deffade);

	GetPrivateProfileString(AppName,"detect silence length","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf,"%i",&silencelength);

	GetPrivateProfileString(AppName,"default volume","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') sscanf(buf,"%lf",&defrelvol);

	GetPrivateProfileString(AppName,"display errors","",buf,sizeof(buf),iniFile);
	if (buf[0]!='\0') DisplayErrors = IsYes(buf);
}

void WriteSettings(void) {
	char iniFile[MAX_PATH+1];
	char buf[256];

	GetINIFileName(iniFile);

	WritePrivateProfileString(AppName,"version",AppName AppVer,iniFile);
	WritePrivateProfileString(AppName,"title format",titlefmt,iniFile);
	WritePrivateProfileString(AppName,"use recompiler",YesNoString(UseRecompiler),iniFile);
	WritePrivateProfileString(AppName,"Audio HLE",YesNoString(AudioHLE),iniFile);
	WritePrivateProfileString(AppName,"Auto Audio HLE",YesNoString(AutoAudioHLE),iniFile);
	WritePrivateProfileString(AppName,"RSP Sections",YesNoString(RSPSECTIONS),iniFile);
	WritePrivateProfileString(AppName,"round frequency",YesNoString(round_frequency),iniFile);
	WritePrivateProfileString(AppName,"soft amplify",YesNoString(softsaturate),iniFile);
	WritePrivateProfileString(AppName,"detect silence",YesNoString(DetectSilence),iniFile);
	WritePrivateProfileString(AppName,"fast seek",YesNoString(FastSeek),iniFile);
	WritePrivateProfileString(AppName,"seek backwards",YesNoString(EnableBackwards),iniFile);
	WritePrivateProfileString(AppName,"ignore track length",YesNoString(IgnoreTrackLength),iniFile);
	WritePrivateProfileString(AppName,"use default track length",YesNoString(DefaultLength),iniFile);
	WritePrivateProfileString(AppName,"always use given track length",YesNoString(SetLength),iniFile);
	WritePrivateProfileString(AppName,"title defaults to file name",YesNoString(title_default_to_file_name),iniFile);
	sprintf(buf,"%i",CPUPriority);
	WritePrivateProfileString(AppName,"CPU priority",buf,iniFile);
	sprintf(buf,"%i",FadeType);
	WritePrivateProfileString(AppName,"fade type",buf,iniFile);
	sprintf(buf,"%li",deflen);
	WritePrivateProfileString(AppName,"default length",buf,iniFile);
	sprintf(buf,"%li",deffade);
	WritePrivateProfileString(AppName,"default fade length",buf,iniFile);
	sprintf(buf,"%i",silencelength);
	WritePrivateProfileString(AppName,"detect silence length",buf,iniFile);
	sprintf(buf,"%lf",defrelvol);
	WritePrivateProfileString(AppName,"default volume",buf,iniFile);
	WritePrivateProfileString(AppName,"display errors",YesNoString(DisplayErrors),iniFile);
}

BOOL CALLBACK helpDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_HELPTEXT,helptext);
		break;
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		return 0;
	case WM_COMMAND:
		if (GET_WM_COMMAND_ID(wParam,lParam) == IDC_ABOUT) DialogBox(mod.hDllInstance, (const char *)IDD_ABOUTBOX,hDlg, aboutDlgProc);
		if (GET_WM_COMMAND_ID(wParam,lParam) == IDCANCEL) EndDialog(hDlg,TRUE);
		break;
	default:
		return 0;
	}

	return 1;
}

BOOL CALLBACK configDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int mypri;
	int i;
	char buf[256];
	HANDLE hSlider,hFadeBox;
	switch (uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		return 0;
	case WM_INITDIALOG:
		if (FastSeek) CheckDlgButton(hDlg,IDC_FASTSEEK,BST_CHECKED);
		if (EnableBackwards) CheckDlgButton(hDlg,IDC_BACKWARDS,BST_CHECKED);
		if (AudioHLE) CheckDlgButton(hDlg,IDC_AUDIOHLE,BST_CHECKED);
		if (AutoAudioHLE) CheckDlgButton(hDlg,IDC_AUTOAUDIOHLE,BST_CHECKED);
		if (IgnoreTrackLength) CheckDlgButton(hDlg,IDC_NOLENGTH,BST_CHECKED);
		if (DefaultLength) CheckDlgButton(hDlg,IDC_DEFLEN,BST_CHECKED);
		if (SetLength) CheckDlgButton(hDlg,IDC_SETLEN,BST_CHECKED);
		if (DetectSilence) CheckDlgButton(hDlg,IDC_DETSIL,BST_CHECKED);
		if (UseRecompiler) CheckDlgButton(hDlg,IDC_RECOMPILER,BST_CHECKED);
		if (RSPSECTIONS) CheckDlgButton(hDlg,IDC_SECTIONS,BST_CHECKED);
		if (softsaturate) CheckDlgButton(hDlg,IDC_SOFTAMP,BST_CHECKED);
		if (title_default_to_file_name) CheckDlgButton(hDlg,IDC_FNONMISSINGTAG,BST_CHECKED);
		if (round_frequency) CheckDlgButton(hDlg,IDC_ROUNDFREQ,BST_CHECKED);
		if (DisplayErrors) CheckDlgButton(hDlg,IDC_DISPERROR,BST_CHECKED);


		SetDlgItemText(hDlg,IDC_TITLEFMT,titlefmt);
		sprintf(buf,"%i",deflen);
		SetDlgItemText(hDlg,IDC_DEFLENVAL,buf);
		sprintf(buf,"%i",deffade);
		SetDlgItemText(hDlg,IDC_DEFFADEVAL,buf);
		sprintf(buf,"%i",silencelength);
		SetDlgItemText(hDlg,IDC_DETSILVAL,buf);
		sprintf(buf,"%.2lf",defrelvol);
		SetDlgItemText(hDlg,IDC_RELVOL,buf);

		// set CPU Priority slider
		hSlider=GetDlgItem(hDlg,IDC_PRISLIDER);
		SendMessage(hSlider, TBM_SETRANGE, (WPARAM) TRUE,                   // redraw flag
			(LPARAM) MAKELONG(1, 7));  // min. & max. positions
		SendMessage(hSlider, TBM_SETPOS,
        (WPARAM) TRUE,                   // redraw flag
        (LPARAM) CPUPriority+1);
		mypri=CPUPriority;
		SetDlgItemText(hDlg,IDC_CPUPRI,pristr[CPUPriority]);

		// set fade type dropdown

		hFadeBox=GetDlgItem(hDlg,IDC_FADETYPE);
		SendMessage(hFadeBox, CB_RESETCONTENT, 0,0);
		for (i=0;i<4;i++) SendMessage(hFadeBox, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)(fadetypes[i]));
		SendMessage(hFadeBox, CB_SELECTSTRING, -1, (LPARAM)(LPCTSTR)(fadetypes[FadeType]));

		break;
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			FastSeek=(IsDlgButtonChecked(hDlg, IDC_FASTSEEK) == BST_CHECKED) ? TRUE : FALSE;
			EnableBackwards=(IsDlgButtonChecked(hDlg, IDC_BACKWARDS) == BST_CHECKED) ? TRUE : FALSE;
			AudioHLE=(IsDlgButtonChecked(hDlg, IDC_AUDIOHLE) == BST_CHECKED) ? TRUE : FALSE;
			AutoAudioHLE=(IsDlgButtonChecked(hDlg, IDC_AUTOAUDIOHLE) == BST_CHECKED) ? TRUE : FALSE;
			IgnoreTrackLength=(IsDlgButtonChecked(hDlg, IDC_NOLENGTH) == BST_CHECKED) ? TRUE : FALSE;
			DefaultLength=(IsDlgButtonChecked(hDlg, IDC_DEFLEN) == BST_CHECKED) ? TRUE : FALSE;
			SetLength=(IsDlgButtonChecked(hDlg,IDC_SETLEN) == BST_CHECKED) ? TRUE : FALSE;
			UseRecompiler=(IsDlgButtonChecked(hDlg, IDC_RECOMPILER) == BST_CHECKED) ? TRUE : FALSE;
			DetectSilence=(IsDlgButtonChecked(hDlg, IDC_DETSIL) == BST_CHECKED) ? TRUE : FALSE;
			RSPSECTIONS=(IsDlgButtonChecked(hDlg, IDC_SECTIONS) == BST_CHECKED) ? TRUE : FALSE;
			softsaturate=(IsDlgButtonChecked(hDlg, IDC_SOFTAMP) == BST_CHECKED) ? TRUE : FALSE;
			title_default_to_file_name=(IsDlgButtonChecked(hDlg, IDC_FNONMISSINGTAG) == BST_CHECKED) ? TRUE : FALSE;
			round_frequency=(IsDlgButtonChecked(hDlg, IDC_ROUNDFREQ) == BST_CHECKED) ? TRUE : FALSE;
			DisplayErrors=(IsDlgButtonChecked(hDlg, IDC_DISPERROR) == BST_CHECKED) ? TRUE : FALSE;


			GetDlgItemText(hDlg,IDC_TITLEFMT,titlefmt,sizeof(titlefmt)-1);
			GetDlgItemText(hDlg,IDC_DEFLENVAL,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&deflen);
			GetDlgItemText(hDlg,IDC_DEFFADEVAL,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&deffade);
			GetDlgItemText(hDlg,IDC_DETSILVAL,buf,sizeof(buf)-1);
			sscanf(buf,"%i",&silencelength);
			GetDlgItemText(hDlg,IDC_RELVOL,buf,sizeof(buf)-1);
			sscanf(buf,"%lf",&defrelvol);
			CPUPriority=mypri;
			WriteSettings();

			FadeType = SendMessage(GetDlgItem(hDlg,IDC_FADETYPE),CB_GETCURSEL,0,0);
		case IDCANCEL:
			EndDialog(hDlg,TRUE);
			break;
		case IDHELPBUTTON:
			DialogBox(mod.hDllInstance, (const char *)IDD_HELPBOX, hDlg, helpDlgProc);
			break;
		}
	case WM_HSCROLL:
		if ((struct HWND__ *)lParam==GetDlgItem(hDlg,IDC_PRISLIDER)) {
			//DisplayError("HScroll=%i",HIWORD(wParam));

			if (LOWORD(wParam)==TB_THUMBPOSITION || LOWORD(wParam)==TB_THUMBTRACK) mypri=HIWORD(wParam)-1;
			else mypri=SendMessage(GetDlgItem(hDlg,IDC_PRISLIDER),TBM_GETPOS,0,0)-1;
			SetDlgItemText(hDlg,IDC_CPUPRI,pristr[mypri]);
		}
		break;
	default:
		return 0;
	}

	return 1;
}

void config(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, (const char *)IDD_CONFIG_WINDOW, hwndParent, configDlgProc);
}

BOOL CALLBACK aboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		return 0;
	case WM_COMMAND:
		if (GET_WM_COMMAND_ID(wParam,lParam) == IDCANCEL) EndDialog(hDlg,TRUE);
		return 0;
	default:
		return 0;
	}
}

void about(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, (const char *)IDD_ABOUTBOX,hwndParent, aboutDlgProc);
}

void init()
{
	ReadSettings();
}

void quit()
{
	// cleanup just incase anything was left open from metadata queries/saves
	if (extInfpsftag) {
		free(extInfpsftag);
		extInfpsftag = NULL;
	}

	if (extInfpsftagwrite) {
		free(extInfpsftagwrite);
		extInfpsftagwrite = NULL;
	}

	WriteSettings();
}

int isourfile(char *fn)
{
	return 0;
}

int play(char *fn)
{
	int retval;

	strcpy(lastfn,fn);

	paused=0;

	if (!InitalizeApplication ( mod.hDllInstance )) return 1;
	hMainWindow = mod.hMainWindow;

	strcpy(CurrentFileName,fn);

	SeekTo(0);

	// should return the error status winamp is looking for if the file is missing
	if (retval=OpenChosenUSF()) { // intentional use of assignment operator
		ShutdownApplication();
	}
	return retval;
}

void pause()
{
	if (!paused) mod.outMod->Pause(1);
	paused=1;
}

void unpause()
{
	if (paused) mod.outMod->Pause(0);
	paused=0;
}

int ispaused()
{
	return paused;
}

void stop()
{
	unpause();
	ShutdownApplication();
}

int getlength()
{
	return TrackLength;
}

int getoutputtime()
{
	return PlayTime();
}

void setoutputtime(int time_in_ms)
{
	// maybe this should check with Audio.c's time?
	if (time_in_ms < getoutputtime()) {
		if (EnableBackwards) {
			SeekTo(time_in_ms);
			// shutdown
			unpause();
			ShutdownApplication();

			// start again
			if (!InitalizeApplication ( mod.hDllInstance )) return;

			// should return the error status winamp is looking for if the file is missing
			if (OpenChosenUSF()!=0) ShutdownApplication();
		}
	} else SeekTo(time_in_ms);
}

void setvolume(int volume)
{
	realvolume=volume;
	mod.outMod->SetVolume(volume);
}
void setpan(int pan)
{
	mod.outMod->SetPan(pan);
}

BOOL CALLBACK rawDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		return 0;
	case WM_INITDIALOG:
		{
			char *LFpsftag;
			DWORD c,d;

			LFpsftag = malloc(strlen(infoDlgpsftag)*2+1);

			// <= includes terminating null
			for (c=0, d=0; c <= strlen(infoDlgpsftag); c++, d++) {
				LFpsftag[d]=infoDlgpsftag[c];
				if (infoDlgpsftag[c]=='\n') {
					LFpsftag[d++]='\r';
					LFpsftag[d]='\n';
				}
			}

			SetDlgItemText(hDlg,IDC_RAWTAG,LFpsftag);
			free(LFpsftag);
			break;
		}
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			{
				char *psftag;
				DWORD c,d;
				psftag = malloc(50000*2+1);
				GetDlgItemText(hDlg,IDC_RAWTAG,psftag,50000*2);

				// remove 0x0d (in PSF a newline is 0x0a, the 0x0d's are for windows)
				for (c=0,d=0; c < strlen(psftag); c++) {
					if (psftag[c] != 0x0d) {
						infoDlgpsftag[d]=psftag[c];
						d++;
					}
				}
				infoDlgpsftag[d]='\0';
				free(psftag);

				infoDlgWriteNewTags=1;

				EndDialog(GetParent(hDlg), TRUE);
			}
			case IDCANCEL:
			EndDialog(hDlg,TRUE);
			break;
		}
	default:
		return 0;
	}
	return 1;
}

BOOL CALLBACK infoDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char Buffer[1024];
	char *LFpsftag;
	//char LFpsftag[50001];
	DWORD c,d;

	switch (uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg,TRUE);
		break;
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_FILENAME,infoDlgfn);

		psftag_raw_getvar(infoDlgpsftag,"track",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_TRACK,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"title",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_TITLE,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"artist",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_ARTIST,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"game",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_GAME,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"year",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_YEAR,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"copyright",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_COPYRIGHT,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"usfby",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_USFBY,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"length",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_LENGTH,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"fade",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_FADE,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"volume",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_VOLUME,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"genre",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_GENRE,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"tagger",Buffer,sizeof(Buffer)-1);
		SetDlgItemText(hDlg,IDC_TAGGER,Buffer);

		psftag_raw_getvar(infoDlgpsftag,"comment",Buffer,sizeof(Buffer)-1);

		LFpsftag = malloc(strlen(Buffer)*3+1);

		for (c=0, d=0; c <= strlen(Buffer); c++, d++) {
			LFpsftag[d]=Buffer[c];
			if (Buffer[c]=='\n') {
				LFpsftag[d++]='\r';
				LFpsftag[d]='\r';
				LFpsftag[d]='\n';
			}
		}

		SetDlgItemText(hDlg,IDC_COMMENT,LFpsftag);
		free(LFpsftag);

		SetFocus(GetDlgItem(hDlg,IDOK));

		break;
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			GetDlgItemText(hDlg,IDC_TRACK,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"track",Buffer);

			GetDlgItemText(hDlg,IDC_TITLE,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"title",Buffer);

			GetDlgItemText(hDlg,IDC_ARTIST,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"artist",Buffer);

			GetDlgItemText(hDlg,IDC_GAME,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"game",Buffer);

			GetDlgItemText(hDlg,IDC_YEAR,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"year",Buffer);

			GetDlgItemText(hDlg,IDC_COPYRIGHT,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"copyright",Buffer);

			GetDlgItemText(hDlg,IDC_USFBY,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"usfby",Buffer);

			GetDlgItemText(hDlg,IDC_LENGTH,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"length",Buffer);

			GetDlgItemText(hDlg,IDC_FADE,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"fade",Buffer);

			GetDlgItemText(hDlg,IDC_VOLUME,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"volume",Buffer);

			GetDlgItemText(hDlg,IDC_GENRE,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"genre",Buffer);

			GetDlgItemText(hDlg,IDC_TAGGER,Buffer,1024);
			psftag_raw_setvar(infoDlgpsftag,50000-1,"tagger",Buffer);

			GetDlgItemText(hDlg,IDC_COMMENT,Buffer,1024);

			// remove 0x0d (in PSF a newline is 0x0a)
			for (c=0,d=0; c < strlen(Buffer); c++) {
				if (Buffer[c] != 0x0d) {
					Buffer[d]=Buffer[c];
					d++;
				}
			}
			Buffer[d]='\0';

			psftag_raw_setvar(infoDlgpsftag,50000,"comment",Buffer);

			infoDlgWriteNewTags=1;

			EndDialog(hDlg, TRUE);
			break;
		case IDCANCEL:
		case IDC_CANCEL:
			EndDialog(hDlg, TRUE);
			break;
		case IDC_NOWBUT:
			sprintf(Buffer,"%i:%02i.%i",getoutputtime()/60000,(getoutputtime()%60000)/1000,getoutputtime()%1000);
			SetDlgItemText(hDlg,IDC_LENGTH,Buffer);
			break;
		case IDC_LAUNCHCONFIG:
			DialogBox(mod.hDllInstance, (const char *)IDD_CONFIG_WINDOW, hDlg, configDlgProc);
			break;
		case IDC_VIEWRAW:
			DialogBox(mod.hDllInstance, (const char *)IDD_RAWTAG_WINDOW, hDlg, rawDlgProc);
			break;
		default:
			return 0;
		}
		break;
	default:
		return 0;
	}

	return 1;
}

int infoDlg(char *fn, HWND hwnd)
{
	// ensure that under no circumstances will two infoDlgs be up
	if (infoDlgpsftag) return 0;
	// not just for decoration, this must be copied because Winamp might change fn
	// willy-nilly
	strcpy(infoDlgfn,fn);
	infoDlgpsftag=malloc(50001); //malloc(taglen+2); // for safety when editing
	infoDlgpsftag[0]='\0';
	psftag_readfromfile(infoDlgpsftag,infoDlgfn);

	infoDlgWriteNewTags=0;

	DialogBox(mod.hDllInstance, (const char *)IDD_INFO_WINDOW, hwnd, infoDlgProc);

	if (infoDlgWriteNewTags) {
		psftag_writetofile(infoDlgpsftag,infoDlgfn);
	}
	free(infoDlgpsftag);
	infoDlgpsftag=NULL;

	return 0;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	DWORD dwRead;
	DWORD FileSize;
	DWORD tagstart;
	DWORD taglen;
	DWORD reservesize;
	HANDLE hFile;
	BYTE buffer[256],length[256],fade[256],titlefmtcopy[256];
	char drive[_MAX_DRIVE] ,FileName[_MAX_DIR],dir[_MAX_DIR], ext[_MAX_EXT];
	BYTE Test[5];
	char * getfileinfopsftag;
	//char *psftag;

	if (!filename || !*filename) filename=lastfn;
	//if (title) sprintf(title,"%s",filename);

	_splitpath( filename, drive, dir, FileName, ext );

	hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	ReadFile(hFile,Test,4,&dwRead,NULL);
	if (!IsValidUSF(Test)) {
		CloseHandle(hFile);
		return;
	}
	// Read USF contents
	ReadFile(hFile,&reservesize,4,&dwRead,NULL); // size of reserved area
	ReadFile(hFile,&FileSize,4,&dwRead,NULL); // size of program
	ReadFile(hFile,Test,4,&dwRead,NULL); // CRC (no check yet)

	tagstart=SetFilePointer(hFile,0,0,FILE_CURRENT)+FileSize+reservesize;
	SetFilePointer(hFile,tagstart,0,FILE_BEGIN);

	ReadFile(hFile,Test,5,&dwRead,NULL);
	// read tags if there are any
	if (IsTagPresent(Test)) {
		tagstart+=5;
		taglen=SetFilePointer(hFile,0,0,FILE_END)-tagstart;
		SetFilePointer(hFile,tagstart,0,FILE_BEGIN);
		getfileinfopsftag=malloc(taglen+2);

		ReadFile(hFile,getfileinfopsftag,taglen,&dwRead,NULL);
		getfileinfopsftag[taglen]='\0';

		psftag_raw_getvar(getfileinfopsftag,"length",length,sizeof(length)-1);
		psftag_raw_getvar(getfileinfopsftag,"fade",fade,sizeof(fade)-1);

		if (length_in_ms) *length_in_ms=LengthFromString(length)+LengthFromString(fade);
		if (IgnoreTrackLength) *length_in_ms=0;

		// create title from PSF tag
		if (title) {
			DWORD c=0,tagnamestart;
			char tempchar;
			sprintf(title,"");
			strcpy(titlefmtcopy,titlefmt);
			while (c < strlen(titlefmtcopy)) {
				if (titlefmtcopy[c]=='%') {
					tagnamestart=++c;
					for (;c<strlen(titlefmtcopy) && titlefmtcopy[c] != '%'; c++);
					if (c == strlen(titlefmtcopy)) {
						//DisplayError("Bad title format string (unterminated token)");
						return;
					}
					titlefmtcopy[c]='\0';
					if (!strcmp(titlefmtcopy+tagnamestart,"file")) {
						strcpy(buffer,FileName);
					} else {
						psftag_raw_getvar(getfileinfopsftag,titlefmtcopy+tagnamestart,buffer,sizeof(buffer)-1);
					}
					strcat(title,buffer);
					titlefmtcopy[c]='%';
					if (strlen(buffer)==0 && title_default_to_file_name) {
						strcpy(title,FileName);
						c=strlen(titlefmtcopy);
					}
				} else {
					tempchar=titlefmtcopy[c+1];
					titlefmtcopy[c+1]='\0';
					strcat(title,titlefmtcopy+c);
					titlefmtcopy[c+1]=tempchar;
				}
				c++;
			}
		}
		//DisplayError("%s",title);

		free(getfileinfopsftag);
		getfileinfopsftag=NULL;
	} else if (title) strcpy(title,FileName);

	if ((DefaultLength && *length_in_ms <= 0) || SetLength) *length_in_ms=(deflen+deffade)*1000;
	CloseHandle(hFile);
}

void eq_set(int on, char data[10], int preamp)
{
}

In_Module mod =
{
	IN_VER,
	AppName AppVer,
	0,	// hMainWindow
	0,  // hDllInstance
	"usf\0Nintendo Ultra 64 Sound Format (*.usf)\0miniusf\0Mini-USF (*.miniusf)\0",
	1,	// is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp

	eq_set,

	NULL,		// setinfo

	0 // out_mod

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}

// the following are extra exported functions from the dll which allow winamp/it's media library
// to query and edit metadata within the files passed in to it (bringing it upto in_* 5.x specs)
__declspec(dllexport) int winampGetExtendedFileInfo(const char *queryfn, const char *data, char *dest, int destlen){

	if (!queryfn || (queryfn && !*queryfn) || !destlen || (data && !*data)) {
		return 0;
	}
	else {
		if (strcmpi(extInf, queryfn)) {
			strncpy(extInf, queryfn, MAX_PATH);
			if (extInfpsftag) {
				free(extInfpsftag);
				extInfpsftag = NULL;
			}

			extInfpsftag = malloc(50001);
			extInfpsftag[0] = '\0';
			psftag_readfromfile(extInfpsftag, extInf);
		}

		// supported tags to be queryable
		// artist, title, track, game, year, copyright, usfby, length, fade, volume, genre, tagger, comment

		if (!strcmpi(data, "artist")) {
				return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));

		}

		else if (!strcmpi(data, "title")) {
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "track")) {
			if(psftag_raw_getvar(extInfpsftag,data,dest,destlen-1)) {
			char *slash = 0, *slashs = 0;
				slash = strrchr(queryfn, '\\');
				if(slash && *slash && (*slash == '\\')) {
					slashs = slash = CharNext(slash);
					while(slash && *slash && *slash >= '0' && *slash <= '9') {slash = CharNext(slash);}
					if(slash && *slash) {
						*slash = 0;
						strncpy(dest, slashs, destlen);
						return 1;
					}
				}
				return 0;
			}
			return 1;
		}

		else if (!strcmpi(data, "game") || !strcmpi(data, "album")){
			return (!psftag_raw_getvar(extInfpsftag,"game",dest,destlen-1));
		}

		else if (!strcmpi(data, "year")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "copyright") || !strcmpi(data, "publisher")){
			return (!psftag_raw_getvar(extInfpsftag,"copyright",dest,destlen-1));
		}

		else if (!strcmpi(data, "usfby")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		// have to form the length in milliseconds and not seconds
		// (otherwise it'll show as zero most of the time)

		else if (!strcmpi(data, "length")) {
			int min=0;
			if (IgnoreTrackLength) {
				min=0;
			} else {
				char *d = dest;
				int temp;
				psftag_raw_getvar(extInfpsftag,"length",dest,destlen-1);
				min=LengthFromString(dest);
				psftag_raw_getvar(extInfpsftag,"fade",dest,destlen-1);
				min+=LengthFromString(dest);

			}
			if ((DefaultLength && min <= 0) || SetLength) min=(deflen+deffade)*1000;
			itoa(min, dest, 10);
			return 0;
		}

		else if (!strcmpi(data, "fade")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "volume")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "genre")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "tagger")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		else if (!strcmpi(data, "comment")){
			return (!psftag_raw_getvar(extInfpsftag,data,dest,destlen-1));
		}

		// return "0" to indicate that we are a supported audio format
		else if(!strcmpi(data, "type")){
			strncpy(dest, "0", destlen);
			return 0;
		}
	}

	return 0;
}


int write_needed = 0;
__declspec(dllexport) int winampSetExtendedFileInfo(char* filename, char* data, char* value){
char dest[50001]= {0};

	// if not the same file as the last call then need to free the tag and load from the new file
	if (strcmpi(extInfwrite, filename)) {
		strncpy(extInfwrite, filename, MAX_PATH);
		if (extInfpsftagwrite) {
			free(extInfpsftagwrite);
			extInfpsftagwrite = NULL;
		}
		write_needed = 0;
		extInfpsftagwrite = malloc(50001);
		extInfpsftagwrite[0] = '\0';
		psftag_readfromfile(extInfpsftagwrite, extInfwrite);
	}

	// supported tags to be saveable (not all will - just what's supported by the ml
	// at the time though 3rd party plugins _could_ use this api but it's doubtful)
	// artist, title, track, game, year, copyright, usfby, length, fade, volume, genre, tagger, comment

	// will check against the current tag and if there's no difference it won't set an update request

	if (!strcmpi(data, "artist")) {
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "title")) {
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "track")) {
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "game") || !strcmpi(data, "album")){
		psftag_raw_getvar(extInfpsftagwrite, "game", dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, "game", value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "year")){
		psftag_raw_getvar(extInfpsftagwrite,data,dest,50000);
		if(strcmp(dest,value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "copyright") || !strcmpi(data, "publisher")){
		psftag_raw_getvar(extInfpsftagwrite, "copyright", dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, "copyright", value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "usfby")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "fade")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "volume")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "genre")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "tagger")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	else if (!strcmpi(data, "comment")){
		psftag_raw_getvar(extInfpsftagwrite, data, dest, 50000);
		if(strcmp(dest, value)){
			psftag_setvar(extInfpsftagwrite, data, value);
			write_needed++;
		}
	}

	return 1;
}


__declspec(dllexport) int winampWriteExtendedFileInfo(void){int ret = 0;
	// only attempt to write if we have a valid tag to work with
	if(extInfwrite[0] && extInfpsftagwrite){
		if(write_needed){
			ret = !psftag_writetofile(extInfpsftagwrite,extInfwrite);
		}
		// if the tag was fine but no update was needed then no need to throw an error
		else{
			ret = 1;
		}
	}
	// cleanup of things
	if (extInfpsftagwrite) {
		free(extInfpsftagwrite);
		extInfpsftagwrite = NULL;
	}
	extInfwrite[0] = '\0';
	write_needed = 0;
	return ret;
}
