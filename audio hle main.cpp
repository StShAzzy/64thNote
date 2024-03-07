#include <windows.h>
#include "audio hle.h"

// "Mupen64 HLE RSP plugin v0.2 with Azimers code by Hacktarux"

static int audio_ucode_detect(OSTask_t *task)
{
   if (*(unsigned long*)(RDRAM + task->ucode_data + 0) != 0x1)
     {
	if (*(RDRAM + task->ucode_data + (0 ^ (3-S8))) == 0xF)
	  return 4;
	else
	  return 3;
     }
   else
     {
	if (*(unsigned long*)(RDRAM + task->ucode_data + 0x30) == 0xF0000F00)
	  return 1;
	else
	  return 2;
     }
}

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

void (*ABI[0x20])();

u32 inst1, inst2;

int audio_ucode(OSTask_t *task)
{
	unsigned long *p_alist = (unsigned long*)(RDRAM + task->data_ptr);
	unsigned int i;

	switch(audio_ucode_detect(task))
	{
	case 1: // mario ucode
		memcpy( ABI, ABI1, sizeof(ABI[0])*0x20 );
		break;
	case 2: // banjo kazooie ucode
		memcpy( ABI, ABI2, sizeof(ABI[0])*0x20 );
		break;
	case 3: // zelda ucode
		memcpy( ABI, ABI3, sizeof(ABI[0])*0x20 );
		break;
	default:
		{
/*		char s[1024];
		sprintf(s, "unknown audio\n\tsum:%x", sum);
#ifdef __WIN32__
		MessageBox(NULL, s, "unknown task", MB_OK);
#else
		printf("%s\n", s);
#endif*/
		return -1;
		}
	}

//	data = (short*)(rsp.RDRAM + task->ucode_data);

	for (i = 0; i < (task->data_size/4); i += 2)
	{
		inst1 = p_alist[i];
		inst2 = p_alist[i+1];
		ABI[inst1 >> 24]();
	}

	return 0;
}