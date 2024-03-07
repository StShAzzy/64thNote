#define MAX_STRING_LEN	400

/*********************************************************************************
* Meta Information                                                               *
*********************************************************************************/
#define LANGUAGE_NAME	        1
#define LANGUAGE_AUTHOR	        2
#define LANGUAGE_VERSION        3
#define LANGUAGE_DATE	        4

/*********************************************************************************
* Numbers                                                                        *
*********************************************************************************/
#define NUMBER_0		        50
#define NUMBER_1		        51
#define NUMBER_2		        52
#define NUMBER_3		        53
#define NUMBER_4		        54
#define NUMBER_5		        55
#define NUMBER_6		        56
#define NUMBER_7		        57
#define NUMBER_8		        58
#define NUMBER_9		        59

/*********************************************************************************
* Messages                                                                       *
*********************************************************************************/
#define MSG_CPU_PAUSED			2000
#define MSG_CPU_RESUMED			2001
#define MSG_PERM_LOOP           2002
#define MSG_MEM_ALLOC_ERROR     2003
#define MSG_FAIL_INIT_GFX       2004
#define MSG_FAIL_INIT_AUDIO     2005
#define MSG_FAIL_INIT_RSP       2006
#define MSG_FAIL_INIT_CONTROL   2007
#define MSG_FAIL_LOAD_PLUGIN    2008
#define MSG_FAIL_LOAD_WORD      2009
#define MSG_FAIL_OPEN_SAVE      2010
#define MSG_FAIL_OPEN_EEPROM    2011
#define MSG_FAIL_OPEN_FLASH     2012
#define MSG_FAIL_OPEN_MEMPAK    2013
#define MSG_FAIL_OPEN_ZIP       2014
#define MSG_FAIL_OPEN_IMAGE     2015
#define MSG_FAIL_ZIP            2016
#define MSG_FAIL_IMAGE          2017
#define MSG_UNKNOWN_COUNTRY     2018
#define MSG_UNKNOWN_CIC_CHIP    2019
#define MSG_UNKNOWN_FILE_FORMAT 2020
#define MSG_UNKNOWN_MEM_ACTION  2021
#define MSG_UNHANDLED_OP        2022
#define MSG_NONMAPPED_SPACE     2023
#define MSG_SAVE_STATE_HEADER   2024
#define MSG_MSGBOX_TITLE        2025
#define MSG_PIF2_ERROR          2026
#define MSG_PIF2_TITLE          2027
#define MSG_PLUGIN_CHANGE       2028
#define MSG_PLUGIN_CHANGE_TITLE 2029
#define MSG_EMULATION_ENDED     2030
#define MSG_EMULATION_STARTED   2031
#define MSG_UNABLED_LOAD_STATE  2032
#define MSG_LOADED_STATE        2033
#define MSG_SAVED_STATE         2034
#define MSG_SAVE_SLOT           2035
#define MSG_BYTESWAP            2036
#define MSG_CHOOSE_IMAGE        2037
#define MSG_LOADED              2038
#define MSG_LOADING             2039
#define MSG_PLUGIN_NOT_INIT     2040
#define MSG_DEL_SURE            2041
#define MSG_DEL_TITLE           2042
#define MSG_CHEAT_NAME_IN_USE   2043
#define MSG_MAX_CHEATS          2044

char * GS (int StringID);
