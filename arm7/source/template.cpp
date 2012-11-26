#include <nds.h>

void handleVBlank() 
{
	static int heartbeat = 0;
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt=0, aux=0;
	int t1=0, t2=0;
	uint32 temp=0;
	uint8 ct[sizeof(IPC->curtime)];
		
	++heartbeat;
		
	but = REG_KEYXY;
	if (~but & 0x40) {
			// Read the touch screen
		x = touchReadXY().x;
		y = touchReadXY().y;
		xpx = touchReadXY().px;
		ypx =touchReadXY().py;
		z1 = touchRead(TSC_MEASURE_Z1);
		z2 = touchRead(TSC_MEASURE_Z2);
	}
		
	batt = touchRead(TSC_MEASURE_BATTERY);
	aux  = touchRead(TSC_MEASURE_AUX);
		
    rtcGetTime((uint8 *)ct);
    BCDToInteger((uint8 *)&(ct[1]), 7);
		
	temp = touchReadTemperature(&t1, &t2);

	//ready to exit to loader if needed (gbamp loader)
	if (*((vu32*)0x027FFE24) == (u32)0x027FFE04)
	{ // Check for ARM9 reset
		REG_IME = IME_DISABLE; // Disable interrupts
		REG_IF = REG_IF; // Acknowledge interrupt
		*((vu32*)0x027FFE34) = (u32)0x08000000; // Bootloader start address
		swiSoftReset(); // Jump to boot loader
	}
		
	IPC->heartbeat = heartbeat;
	IPC->buttons   = but;
	IPC->touchX    = x;
	IPC->touchY    = y;
	IPC->touchXpx  = xpx;
	IPC->touchYpx  = ypx;
	IPC->touchZ1   = z1;
	IPC->touchZ2   = z2;
	IPC->battery   = batt;
	IPC->aux       = aux;
		
	for(u32 i=0; i<sizeof(ct); i++) {
		IPC->curtime[i] = ct[i];
	}
		
	IPC->temperature = temp;
	IPC->tdiode1 = t1;
	IPC->tdiode2 = t2;
}

int main(int argc, char ** argv)
{
	rtcReset();
		
	irqInit();
	irqSet(IRQ_VBLANK, handleVBlank);
	irqEnable(IRQ_VBLANK);
		
	while (1)
	{		
		swiWaitForVBlank();
	}
		
	return 0;
}
