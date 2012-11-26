/*
* 
*   DSWifi Chat - Copyright 2006 Bafio
*   This software is completely free. No warranty is provided.
*   If you use it, please give me credit and email me about your
*   project at buzurro@gmail.com
*   Contributions are appriciated too.
*
*/

#include "nds.h"
#include <stdio.h>
#include <nds/reload.h>
#include "gba_nds_fat/gba_nds_fat.h"

#define IME REG_IME
#define IME_DISABLED (0)

bool loadNDS(char* fn)
{
	if (!FAT_InitFiles ())
		return FALSE;
	FAT_FILE* handle = FAT_fopen(fn, "R");

	if (handle == NULL)
	{
		return false;
	}

	u32 fileCluster = FAT_GetFileCluster();
	FAT_fclose(handle);

	// Use this code to start an NDS file
	IME = IME_DISABLED;	// Disable interrupts
	WAIT_CR |= (0x8080);  // ARM7 has access to GBA cart
	*((vu32*)0x027FFFFC) = fileCluster;  // Start cluster of NDS to load
	*((vu32*)0x027FFE04) = (u32)0xE59FF018;  // ldr pc, 0x027FFE24
	*((vu32*)0x027FFE24) = (u32)0x027FFE04;  // Set ARM9 Loop address
	swiSoftReset();  // Reset
	//**********************************************************************
	while (1){};
}
