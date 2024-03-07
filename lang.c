#include "lang.h"

typedef struct {
	int    ID;
	char   Str[MAX_STRING_LEN];
} LANG_STR;

LANG_STR DefaultString[] = {

/*********************************************************************************
* Numbers                                                                        *
*********************************************************************************/
	{ NUMBER_0,             "0"                      },
	{ NUMBER_1,             "1"                      },
	{ NUMBER_2,             "2"                      },
	{ NUMBER_3,             "3"                      },
	{ NUMBER_4,             "4"                      },
	{ NUMBER_5,             "5"                      },
	{ NUMBER_6,             "6"                      },
	{ NUMBER_7,             "7"                      },
	{ NUMBER_8,             "8"                      },
	{ NUMBER_9,             "9"                      },

/*********************************************************************************
* Messages                                                                       *
*********************************************************************************/
	{ MSG_CPU_PAUSED,         "*** CPU PAUSED ***"},
	{ MSG_CPU_RESUMED,        "CPU Resumed"},
	{ MSG_PERM_LOOP,          "In a permanent loop that cannot be exited. \nEmulation will now stop. \n\nVerify ROM and ROM Settings."},
	{ MSG_MEM_ALLOC_ERROR,    "Failed to allocate Memory"},
	{ MSG_FAIL_INIT_GFX,      "The default or selected video plugin is missing or invalid.\n\nYou need to go into Settings and select a video (graphics) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder."},
	{ MSG_FAIL_INIT_AUDIO,    "The default or selected audio plugin is missing or invalid.\n\nYou need to go into Settings and select a audio (sound) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder."},
	{ MSG_FAIL_INIT_RSP,      "The default or selected RSP plugin is missing or invalid. \n\nYou need to go into Settings and select an RSP plugin.\nCheck that you have at least one compatible plugin file in your plugin folder."},
	{ MSG_FAIL_INIT_CONTROL,  "The default or selected input plugin is missing or invalid. \n\nYou need to go into Settings and select a video (graphics) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder."},
	{ MSG_FAIL_LOAD_PLUGIN,   "Failed to load plugin:"},
	{ MSG_FAIL_LOAD_WORD,     "Failed to load word\n\nVerify ROM and ROM Settings."},
	{ MSG_FAIL_OPEN_SAVE,     "Failed to open Save File"},
	{ MSG_FAIL_OPEN_EEPROM,   "Failed to open Eeprom"},
	{ MSG_FAIL_OPEN_FLASH,    "Failed to open Flashram"},
	{ MSG_FAIL_OPEN_MEMPAK,   "Failed to open mempak"},
	{ MSG_FAIL_OPEN_ZIP,      "Attempt to open zip file failed. \n\nProbably a corrupt zip file - try unzipping ROM manually."},
	{ MSG_FAIL_OPEN_IMAGE,    "Attempt to open file failed."},
	{ MSG_FAIL_ZIP,           "Error occured when trying to open zip file."},
	{ MSG_FAIL_IMAGE,         "File loaded does not appear to be a valid Nintendo64 ROM. \n\nVerify your ROMs with GoodN64."},
	{ MSG_UNKNOWN_COUNTRY,    "Unknown country"},
	{ MSG_UNKNOWN_CIC_CHIP,   "Unknown Cic Chip"},
	{ MSG_UNKNOWN_FILE_FORMAT,"Unknown file format"},
	{ MSG_UNKNOWN_MEM_ACTION, "Unknown memory action\n\nEmulation stop"},
	{ MSG_UNHANDLED_OP,       "Unhandled R4300i OpCode at"},
	{ MSG_NONMAPPED_SPACE,    "Executing from non-mapped space.\n\nVerify ROM and ROM Settings."},
	{ MSG_SAVE_STATE_HEADER,  "State save does not appear to match the running ROM. \n\nState saves must be saved & loaded between 100% identical ROMs, \nin particular the REGION and VERSION need to be the same. \nLoading this state is likely to cause the game and/or emulator to crash. \n\nAre you sure you want to continue loading?"},
	{ MSG_MSGBOX_TITLE,       "Error"},
	{ MSG_PIF2_ERROR,         "Copyright sequence not found in LUT.  Game will no longer function."},
	{ MSG_PIF2_TITLE,         "Copy Protection Failure"},
	{ MSG_PLUGIN_CHANGE,      "Changing a plugin requires Project64 to reset a running ROM. \nIf you don't want to lose your place, answer No and make a state save first. \n\nChange plugins and restart game now?"},
	{ MSG_PLUGIN_CHANGE_TITLE,"Change Plugins"},
	{ MSG_EMULATION_ENDED,    "Emulation ended"},
	{ MSG_EMULATION_STARTED,  "Emulation started"},
	{ MSG_UNABLED_LOAD_STATE, "Unable to load save state"},
	{ MSG_LOADED_STATE,       "Loaded save state"},
	{ MSG_SAVED_STATE,        "Saved current state to"},
	{ MSG_SAVE_SLOT,          "Save state slot"},
	{ MSG_BYTESWAP,           "Byte swapping image"},
	{ MSG_CHOOSE_IMAGE,       "Choosing N64 image"},
	{ MSG_LOADED,             "Loaded"},
	{ MSG_LOADING,            "Loading image"},
	{ MSG_PLUGIN_NOT_INIT,    "Cannot open a rom because plugins have not successfully initialised"},
	{ MSG_DEL_SURE,           "Are you sure you really want to delete this?"},
	{ MSG_DEL_TITLE,          "Delete Cheat"},
	{ MSG_CHEAT_NAME_IN_USE,  "Cheat Name is already in use"},
	{ MSG_MAX_CHEATS,         "You Have reached the Maxiumn amount of cheats for this rom"},

};

char * GS (int StringID) {
	int count;

	for (count = 0; count < (sizeof(DefaultString) / sizeof(LANG_STR)); count ++) {
		if (DefaultString[count].ID == StringID) { return DefaultString[count].Str; }
	}
	return "";
}