#include "nds.h"
#include <nds/arm9/console.h>
#include <string.h>
#include "utils.H"
#include "screenshot.h"
#include <stdio.h>

#include "cellmap.H"

using namespace std;

#define WIDTH 256
#define HEIGHT 192

touchPosition touchXY;
int intPressure;
u16 keysH;
u16 keysD;
int i, j, k, i2, j2;
char screenshotFile[] = "Life0000.bmp";
int screenshotCount = 0;

typedef struct neighborIter
{
	int left, right, up, down, upleft, upright, downleft, downright;
};

typedef struct settings
{
	int DELAY, colorR, colorG, colorB, baseColorR, baseColorG, baseColorB, starve, birth, crowd, mirror, dotType, scale, scale_count, scrollx, scrolly, cycleChoice, cycleFromR, cycleToR, cycleFromG, cycleToG, cycleFromB, cycleToB, cycleRate;
	bool swapped, cycleColors, paused, cycleBaseDirR, cycleBaseDirG, cycleBaseDirB, cycleCellDirR, cycleCellDirG, cycleCellDirB, autoXray, filter;
	unsigned long generation;
	int x,y;
	int offset;
	u16 filtered;
	int filtermode;
	u16 fillColor;
	u16 birthrules;
	u16 deathrules;
};

int scale_settings[] = {1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8};

void draw_clear();
void fill_clear();
void processInput();
void processTouch();
bool areaTouched(int x1, int y1, int x2, int y2);
void draw2MirrorsX(int x, int y);
void draw2MirrorsY(int x, int y);
void draw2MirrorsZ(int x, int y);
void draw4Mirrors(int x, int y);
void initMenu();
void doDelay(int delay);
void cycleColors();
void DrawPt(int x, int y);
void DrawSquare(int x, int y);
void DrawPlus(int x, int y);
void Draw3PtsV(int x, int y);
void Draw3PtsH(int x, int y);
void DrawSquareBig(int x, int y);
void DrawX(int x, int y);
void DrawShape(int x, int y);
void initGame();
void drawStyle();
void FillScreen();

settings set;
neighborIter neighborWeights;
cellmap theMap(HEIGHT, WIDTH);
bool theShape[5][5];

void filterScreen()
{
	switch(set.mirror)
	{
		case 0:
			for(set.y = 96; set.y >=0; set.y--)
			for(set.x = 128; set.x >=0; set.x--)
			{
				set.offset = set.y*WIDTH+set.x;
				set.filtered = ((BG_GFX[set.offset+1]+BG_GFX[set.offset-1]+BG_GFX[set.offset - WIDTH] + BG_GFX[set.offset + WIDTH])>>2)|BIT(15);
				BG_GFX[set.offset] = set.filtered;
				BG_GFX[set.y*WIDTH + (WIDTH - set.x)] = set.filtered;
				BG_GFX[(HEIGHT-set.y)*WIDTH + set.x] = set.filtered;
				BG_GFX[(HEIGHT-set.y)*WIDTH + (WIDTH - set.x)] = set.filtered;
			}
		break;

		default:
			for(set.offset = 0;set.offset < WIDTH*HEIGHT; set.offset++)
			BG_GFX[set.offset]  = ((BG_GFX[set.offset+1]+BG_GFX[set.offset-1]+BG_GFX[set.offset - WIDTH] + BG_GFX[set.offset + WIDTH])>>2)|BIT(15);
	}
}

int main(void)
{
	irqInit();
	irqSet(IRQ_VBLANK, 0);

	videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x6000000);
	BG2_CR = BG_BMP16_256x256 | BG_WRAP_ON;

	BG2_XDX = 1 << 8;
	BG2_YDY = 1 << 8;
	BG2_XDY = 0;
	BG2_YDX = 0;
	BG2_CY = 0;
	BG2_CX = 0;

	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(31, 31, 31);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	lcdSwap();
	draw_clear();

	initMenu();
	initGame();

	memset(BG_GFX,0,256*256);

	while (1)
	{
		scanKeys();
		if (!set.paused)
		{
			set.generation++;
			if(set.filter)
			{
				if(set.autoXray)
				{
					if(set.generation % (set.cycleRate*5+1) == 0) {theMap.next_generation(); filterScreen();}
					else filterScreen();
				}
				else {theMap.next_generation(); filterScreen(); swiWaitForVBlank();}
			}
			else if (set.autoXray)
			{
				theMap.next_generation();
				swiWaitForVBlank();
				draw_clear();
			}
			else theMap.next_generation();

			if ((set.cycleColors) && (set.generation % (set.cycleRate*5+1) == 0)) cycleColors();

			//iprintf("\x1b[0;0HGen:%12d", set.generation);
		}

		if (keysHeld())
		{
			keysH = keysHeld();
			if (keysH & KEY_TOUCH) processTouch();
			else processInput();
		}
		else if ((set.DELAY > 0) && !set.autoXray) doDelay(set.DELAY*10);
	}
	return 0;
}

void doDelay(int delay) {swiDelay(1000*delay);}

void initGame()
{
	set.scrollx = 0;
	set.scrolly = 0;
	set.scale = 4;
	set.scale_count = 5;

	set.generation = 0;
	set.mirror = 0;
	set.dotType = 0;

	set.cycleChoice = 0;
	set.cycleColors = false;
	set.cycleRate = 4;
	set.cycleBaseDirR = true;
	set.cycleBaseDirG = true;
	set.cycleBaseDirB = true;
	set.cycleCellDirR = true;
	set.cycleCellDirG = true;
	set.cycleCellDirB = true;
	set.paused = false;
	set.swapped = false;
	set.autoXray = false;
	set.filter = false;

	neighborWeights.upleft = neighborWeights.up = neighborWeights.upright = neighborWeights.left = neighborWeights.right = neighborWeights.downleft = neighborWeights.down = neighborWeights.downright = 1;
	theMap.setWeights(neighborWeights.upleft, neighborWeights.up, neighborWeights.upright, neighborWeights.left, neighborWeights.right, neighborWeights.downleft, neighborWeights.down, neighborWeights.downright);

	set.colorR = 31;
	set.colorG = 31;
	set.colorB = 31;
	set.baseColorR = 0;
	set.baseColorG = 0;
	set.baseColorB = 0;
	set.cycleFromR = 0;
	set.cycleFromG = 0;
	set.cycleFromB = 0;
	set.cycleToR = 31;
	set.cycleToG = 31;
	set.cycleToB = 31;
	theMap.setColors(set.colorR, set.colorG, set.colorB, set.baseColorR, set.baseColorG, set.baseColorB);
	set.fillColor = RGB15(set.baseColorR,set.baseColorG,set.baseColorB)|BIT(15);

	set.birthrules = 0 | BIT(3);
	set.deathrules = 0 | BIT(0)|BIT(1)|BIT(4)|BIT(5)|BIT(6)|BIT(7)|BIT(8)|BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15);
	theMap.setRules(set.birthrules,set.deathrules);
//	set.starve = 2;
//	set.birth = 3;
//	set.crowd = 4;
//	theMap.setRules(set.starve, set.birth, set.crowd);

	for (i = 0;i < 5;i++)for (j = 0;j < 5;j++) theShape[j][i] = true;

	draw_clear();
}

void initMenu()
{
	iprintf("\x1b[1;1H--Mirror++ --Point++");
	iprintf("\x1b[2;1H--Mode 0++ --Type0++");

	iprintf("\x1b[4;1H--Red++ --Grn++ --Blu++");
	iprintf("\x1b[5;1H--F00++ --F00++ --F00++");

	iprintf("\x1b[7;1H--Red++ --Grn++ --Blu++");
	iprintf("\x1b[8;1H--T00++ --T00++ --T00++");

	iprintf("\x1b[0;21H||\\/|/");
	iprintf("\x1b[1;21H||/\\|\\");

//	iprintf("\x1b[22;1H--Starve++ --Birth++ --Crowd++");
//	iprintf("\x1b[23;1H--R1  02++ --R2 03++ --R3 04++");
	iprintf("\x1b[21;1H      0123456789ABCDEF");
	iprintf("\x1b[22;1HBirth ...*............");
	iprintf("\x1b[23;1HDeath **..************");

	iprintf("\x1b[2;25HLIV DIE");
	for (k = 3;k < 21;k++) iprintf("\x1b[%d;25HRGB RGB", k);

	for (i = 10;i < 21;)
	{
		for (j = 1;j < 12;)
		{
			iprintf("\x1b[%d;%dH-|+", i, j);
			iprintf("\x1b[%d;%dH-|+", i + 1, j);
			iprintf("\x1b[%d;%dH-1+", i + 2, j);
			j += 4;
		}
		i += 4;
	}

	iprintf("\x1b[13;4H*****");
	iprintf("\x1b[14;4H*****");
	iprintf("\x1b[15;4H*****");
	iprintf("\x1b[16;4H*****");
	iprintf("\x1b[17;4H*****");
	iprintf("\x1b[0;30HC-");
	iprintf("\x1b[1;27H-4+BG");
	iprintf("\x1b[0;29HF");
	iprintf("\x1b[14;18H+");
	iprintf("\x1b[23;0HR");
	iprintf("\x1b[0;28HX");
}

void processTouch()
{
	touchXY = touchReadXY();
	intPressure = (IPC->touchX * IPC->touchZ2) / (IPC->touchZ1 << 6) - IPC->touchX >> 6;

	if (!set.swapped)		//Drawing mode
	{
		if((intPressure < 192) && (touchXY.py > 0) && (touchXY.px > 0) && (touchXY.px < WIDTH - 1) && (touchXY.py < HEIGHT - 1)) drawStyle();
		if ((set.cycleColors) && (set.generation++ % (set.cycleRate*5+1) == 0)) cycleColors();
		return ;
	}
	else
	if ((intPressure < 192) && (touchXY.py > 0) && (touchXY.px > 0) && (touchXY.px < 255) && (touchXY.py < 191))
	{
		if (areaTouched(80, 104, 168, 192))
		{
			touchXY.px -= 20;
			touchXY.py -= 22;
			drawStyle();
			if ((set.cycleColors) && (set.generation++ % (set.cycleRate*5+1) == 0)) cycleColors();
			return ;
		}
		else if (areaTouched(0, 184, 16, 200))
		{
			if (set.paused)
			{
				set.generation++;
				//iprintf("\x1b[0;0HGen:%12d", set.generation);

				if(set.filter)
				{
					filterScreen();
				}
				else if (set.autoXray)
				{
					draw_clear();
					theMap.next_generation();
					swiWaitForVBlank();
				}
				else theMap.next_generation();

				if ((set.cycleColors) && (set.generation % (set.cycleRate*5+1) == 0)) cycleColors();
			}
			else
			{
				if (!set.autoXray)
				{
					swiWaitForVBlank();
					fill_clear();
				}
				theMap.redraw();
			}
		}
		else if (areaTouched(0, 248, 8, 256))
		{
			//Toggle Border Position
		}
		else if (areaTouched(0, 200, 16, 216))	//X  - Clears life, keeps background
		{
			theMap.init();
			doDelay(1500);
		}
		else if (areaTouched(8, 244, 16, 252))	//B  - cycle base or cell color
		{
			set.cycleChoice++;
			if (set.cycleChoice > 2) set.cycleChoice = 0;

			if (set.cycleChoice == 0) printf("\x1b[1;30HB");
			else if (set.cycleChoice == 1) printf("\x1b[1;30HC");
			else iprintf("\x1b[1;30HA");

			doDelay(1500);
		}
		else	if (areaTouched(8,252,16,260))
		{
			set.filter ^= 1;
			doDelay(1000);
		}
		else	if (areaTouched(0, 244, 8, 252))
		{
			set.cycleColors ^= 1;
			if (set.cycleColors)
			{
				if ((set.cycleChoice == 0) || (set.cycleChoice == 2))
				{
					set.baseColorR = set.cycleFromR;
					set.baseColorG = set.cycleFromG;
					set.baseColorB = set.cycleFromB;
				}

				if ((set.cycleChoice == 1) || (set.cycleChoice == 2))
				{
					set.colorR = set.cycleFromR;
					set.colorG = set.cycleFromG;
					set.colorB = set.cycleFromB;
				}

				set.cycleBaseDirR = true;
				set.cycleBaseDirG = true;
				set.cycleBaseDirB = true;
				set.cycleCellDirR = true;
				set.cycleCellDirG = true;
				set.cycleCellDirB = true;

				iprintf("\x1b[0;30HO");
			}
			else iprintf("\x1b[0;30HC");
			doDelay(1500);
		}
		else	if (areaTouched(16, 200, 168, 208))
		{
			set.colorR = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[5;4H%02d", set.colorR);
		}
		else	if (areaTouched(16, 208, 168, 216))
		{
			set.colorG = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[5;12H%02d", set.colorG);
		}
		else	if (areaTouched(16, 216, 168, 224))
		{
			set.colorB = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[5;20H%02d", set.colorB);
		}
		else	if (areaTouched(16, 232, 168, 240))
		{
			set.baseColorR = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[8;4H%02d", set.baseColorR);
		}
		else	if (areaTouched(16, 240, 168, 248))
		{
			set.baseColorG = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[8;12H%02d", set.baseColorG);
		}
		else	if (areaTouched(16, 248, 168, 256))
		{
			set.baseColorB = (touchXY.py - 16) / 4.75;
			iprintf("\x1b[8;20H%02d", set.baseColorB);
		}
		else if (areaTouched(32, 8, 48, 24))
		{
			set.cycleFromR--;
			if (set.cycleFromR < 0) set.cycleFromR = 0;
			iprintf("\x1b[5;4H%02d", set.cycleFromR);
			doDelay(750);
		}
		else if (areaTouched(32, 48, 48, 64))
		{
			set.cycleFromR++;
			if (set.cycleFromR > 31) set.cycleFromR = 31;
			iprintf("\x1b[5;4H%02d", set.cycleFromR);
			doDelay(750);
		}
		else if (areaTouched(32, 72, 48, 88))
		{
			set.cycleFromG--;
			if (set.cycleFromG < 0) set.cycleFromG = 0;
			iprintf("\x1b[5;12H%02d", set.cycleFromG);
			doDelay(750);
		}
		else if (areaTouched(32, 112, 48, 128))
		{
			set.cycleFromG++;
			if (set.cycleFromG > 31) set.cycleFromG = 31;
			iprintf("\x1b[5;12H%02d", set.cycleFromG);
			doDelay(750);
		}
		else if (areaTouched(32, 136, 48, 160))
		{
			set.cycleFromB--;
			if (set.cycleFromB < 0) set.cycleFromB = 0;
			iprintf("\x1b[5;20H%02d", set.cycleFromB);
			doDelay(750);
		}
		else if (areaTouched(32, 176, 48, 192))
		{
			set.cycleFromB++;
			if (set.cycleFromB > 31) set.cycleFromB = 31;
			iprintf("\x1b[5;20H%02d", set.cycleFromB);
			doDelay(750);
		}
		else if (areaTouched(56, 8, 72, 24))
		{
			set.cycleToR--;
			if (set.cycleToR < 0) set.cycleToR = 0;
			iprintf("\x1b[8;4H%02d", set.cycleToR);
			doDelay(750);
		}
		else if (areaTouched(56, 48, 72, 64))
		{
			set.cycleToR++;
			if (set.cycleToR > 31) set.cycleToR = 31;
			iprintf("\x1b[8;4H%02d", set.cycleToR);
			doDelay(750);
		}
		else if (areaTouched(56, 72, 72, 88))
		{
			set.cycleToG--;
			if (set.cycleToG < 0) set.cycleToG = 0;
			iprintf("\x1b[8;12H%02d", set.cycleToG);
			doDelay(750);
		}
		else if (areaTouched(56, 112, 72, 128))
		{
			set.cycleToG++;
			if (set.cycleToG > 31) set.cycleToG = 31;
			iprintf("\x1b[8;12H%02d", set.cycleToG);
			doDelay(750);
		}
		else if (areaTouched(56, 136, 72, 152))
		{
			set.cycleToB--;
			if (set.cycleToB < 0) set.cycleToB = 0;
			iprintf("\x1b[8;20H%02d", set.cycleToB);
			doDelay(750);
		}
		else if (areaTouched(56, 176, 72, 192))
		{
			set.cycleToB++;
			if (set.cycleToB > 31) set.cycleToB = 31;
			iprintf("\x1b[8;20H%02d", set.cycleToB);
			doDelay(750);
		}
/*		else if (areaTouched(176, 8, 192, 24))
		{
			set.starve--;
			if (set.starve < 0) set.starve = 0;
			iprintf("\x1b[23;7H%02d", set.starve);
			doDelay(750);
		}
		else if (areaTouched(176, 72, 192, 88))
		{
			set.starve++;
			if (set.starve > 24 ) set.starve = 24;
			iprintf("\x1b[23;7H%02d", set.starve);
			doDelay(750);
		}
		else if (areaTouched(176, 96, 192, 112))
		{
			set.birth--;
			if (set.birth < 0) set.birth = 0;
			iprintf("\x1b[23;17H%02d", set.birth);
			doDelay(750);
		}
		else if (areaTouched(176, 152, 192, 168))
		{
			set.birth++;
			if (set.birth > 24) set.birth = 24;
			iprintf("\x1b[23;17H%02d", set.birth);
			doDelay(750);
		}
		else if (areaTouched(176, 176, 192, 192))
		{
			set.crowd--;
			if (set.crowd < 0) set.crowd = 0;
			iprintf("\x1b[23;27H%02d", set.crowd);
			doDelay(750);
		}
		else if (areaTouched(176, 232, 192, 256))
		{
			set.crowd++;
			if (set.crowd > 24 ) set.crowd = 24;
			iprintf("\x1b[23;27H%02d", set.crowd);
			doDelay(750);
		}
*/
		else if(areaTouched(176,56, 176+8, 56 + 16*8))
		{
			int index = (touchXY.px - 56) / 8;
			if(index != 16)
			{
				if(set.birthrules & (0|BIT(index)))
				{
					set.birthrules = set.birthrules & ~(0|BIT(index));
					iprintf("\x1b[22;%dH.",7+index);
				}
				else
				{
					set.birthrules = set.birthrules | BIT(index);
					iprintf("\x1b[22;%dH*",7+index);
				}
			}
			doDelay(750);
		}
		else if(areaTouched(184, 56, 184+8, 56 + 16*8))
		{
			int index = (touchXY.px - 56) / 8;
			if(index != 16)
			{
				if(set.deathrules & (0|BIT(index)))
				{
					set.deathrules = set.deathrules & ~(0|BIT(index));
					iprintf("\x1b[23;%dH.",7+index);
				}
				else
				{
					set.deathrules = set.deathrules | BIT(index);
					iprintf("\x1b[23;%dH*",7+index);
				}
			}
			doDelay(750);
		}
		else if (areaTouched(8, 8, 24, 24))
		{
			set.mirror--;
			if (set.mirror < 0) set.mirror = 0;
			iprintf("\x1b[2;8H%01d", set.mirror);
			doDelay(750);
		}
		else if (areaTouched(8, 72, 24, 88))
		{
			set.mirror++;
			if (set.mirror > 4) set.mirror = 4;
			iprintf("\x1b[2;8H%01d", set.mirror);
			doDelay(750);
		}
		else if (areaTouched(8, 96, 24, 112))
		{
			set.dotType--;
			if (set.dotType < 0) set.dotType = 0;
			iprintf("\x1b[2;18H%1d", set.dotType);
			doDelay(750);
		}
		else if (areaTouched(8, 152, 24, 168))
		{
			set.dotType++;
			if (set.dotType > 7) set.dotType = 7;
			iprintf("\x1b[2;18H%1d", set.dotType);
			doDelay(750);
		}
		else if (areaTouched(80, 8, 104, 16))
		{
			neighborWeights.upleft -= 1;
			if (neighborWeights.upleft < 0) neighborWeights.upleft = 0;
			iprintf("\x1b[12;2H%x", neighborWeights.upleft);
			doDelay(750);
		}
		else if (areaTouched(80, 24, 104, 32))
		{
			neighborWeights.upleft += 1;
			if (neighborWeights.upleft > 15) neighborWeights.upleft = 15;
			iprintf("\x1b[12;2H%x", neighborWeights.upleft);
			doDelay(750);
		}
		else if (areaTouched(80, 40, 104, 48))
		{
			neighborWeights.up -= 1;
			if (neighborWeights.up < 0) neighborWeights.up = 0;
			iprintf("\x1b[12;6H%x", neighborWeights.up);
			doDelay(750);
		}
		else if (areaTouched(80, 56, 104, 64))
		{
			neighborWeights.up += 1;
			if (neighborWeights.up > 15) neighborWeights.up = 15;
			iprintf("\x1b[12;6H%x", neighborWeights.up);
			doDelay(750);
		}
		else if (areaTouched(80, 72, 104, 80))
		{
			neighborWeights.upright -= 1;
			if (neighborWeights.upright < 0) neighborWeights.upright = 0;
			iprintf("\x1b[12;10H%x", neighborWeights.upright);
			doDelay(750);
		}
		else if (areaTouched(80, 88, 104, 96))
		{
			neighborWeights.upright += 1;
			if (neighborWeights.upright > 15) neighborWeights.upright = 15;
			iprintf("\x1b[12;10H%x", neighborWeights.upright);
			doDelay(750);
		}
		else if (areaTouched(112, 8, 136, 16))
		{
			neighborWeights.left -= 1;
			if (neighborWeights.left < 0) neighborWeights.left = 0;
			iprintf("\x1b[16;2H%x", neighborWeights.left);
			doDelay(750);
		}
		else if (areaTouched(112, 24, 136, 32))
		{
			neighborWeights.left += 1;
			if (neighborWeights.left > 15) neighborWeights.left = 15;
			iprintf("\x1b[16;2H%x", neighborWeights.left);
			doDelay(750);
		}
		else if (areaTouched(112, 72, 136, 80))
		{
			neighborWeights.right -= 1;
			if (neighborWeights.right < 0) neighborWeights.right = 0;
			iprintf("\x1b[16;10H%x", neighborWeights.right);
			doDelay(750);
		}
		else if (areaTouched(112, 88, 136, 96))
		{
			neighborWeights.right += 1;
			if (neighborWeights.right > 15) neighborWeights.right = 15;
			iprintf("\x1b[16;10H%x", neighborWeights.right);
			doDelay(750);
		}
		else if (areaTouched(144, 8, 168, 16))
		{
			neighborWeights.downleft -= 1;
			if (neighborWeights.downleft < 0) neighborWeights.downleft = 0;
			iprintf("\x1b[20;2H%x", neighborWeights.downleft);
			doDelay(750);
		}
		else if (areaTouched(144, 24, 168, 32))
		{
			neighborWeights.downleft += 1;
			if (neighborWeights.downleft > 15) neighborWeights.downleft = 15;
			iprintf("\x1b[20;2H%x", neighborWeights.downleft);
			doDelay(750);
		}
		else if (areaTouched(144, 40, 168, 48))
		{
			neighborWeights.down -= 1;
			if (neighborWeights.down < 0) neighborWeights.down = 0;
			iprintf("\x1b[20;6H%x", neighborWeights.down);
			doDelay(750);
		}
		else if (areaTouched(144, 56, 168, 64))
		{
			neighborWeights.down += 1;
			if (neighborWeights.down > 15) neighborWeights.down = 15;
			iprintf("\x1b[20;6H%x", neighborWeights.down);
			doDelay(750);
		}
		else if (areaTouched(144, 72, 168, 80))
		{
			neighborWeights.downright -= 1;
			if (neighborWeights.downright < 0) neighborWeights.downright = 0;
			iprintf("\x1b[20;10H%x", neighborWeights.downright);
			doDelay(750);
		}
		else if (areaTouched(144, 88, 168, 96))
		{
			neighborWeights.downright += 1;
			if (neighborWeights.downright > 15) neighborWeights.downright = 15;
			iprintf("\x1b[20;10H%x", neighborWeights.downright);
			doDelay(750);
		}
		else if (areaTouched(0, 168, 16, 184))
		{
			set.paused ^= 1;
			if (set.paused)
			{
				iprintf("\x1b[0;23H\\\\");
				iprintf("\x1b[1;23H//");
				doDelay(1000);
			}
			else
			{
				iprintf("\x1b[0;23H\\/");
				iprintf("\x1b[1;23H/\\");
				doDelay(1000);
			}
			if (set.autoXray) {theMap.next_generation();}
		}
		else if (areaTouched(8, 216, 16, 224))
		{
			set.cycleRate--;
			if (set.cycleRate < 0) set.cycleRate = 0;
			iprintf("\x1b[1;28H%01X", set.cycleRate);
			doDelay(750);
		}
		else if (areaTouched(8, 232, 16, 240))
		{
			set.cycleRate++;
			if (set.cycleRate > 15) set.cycleRate = 15;
			iprintf("\x1b[1;28H%01X", set.cycleRate);
			doDelay(750);
		}
		else if (areaTouched(104, 32, 144, 72))
		{
			i = (touchXY.px - 32) >> 3;
			j = (touchXY.py - 104) >> 3;
			
			if ((i < 5) && (j < 5))
			{
				theShape[j][i] ^= 1;
				if (theShape[j][i]) iprintf("\x1b[%d;%dH*", j + 13, i + 4);
				else iprintf("\x1b[%d;%dH.", j + 13, i + 4);
			}
			doDelay(2500);
		}
		else if (areaTouched(0, 232, 8, 240))
		{
			FillScreen();
			doDelay(750);
		}
		else if (areaTouched(184, 0, 192, 8))
		{
			bool temp = set.swapped;
			initGame();
			initMenu();
			set.swapped = temp;
			doDelay(750);
		}
		else if (areaTouched(0, 224, 8, 232))
		{
			set.autoXray ^= 1;
			doDelay(1000);
		}
		theMap.setWeights(neighborWeights.upleft, neighborWeights.up, neighborWeights.upright, neighborWeights.left, neighborWeights.right, neighborWeights.downleft, neighborWeights.down, neighborWeights.downright);
		theMap.setColors(set.colorR, set.colorG, set.colorB, set.baseColorR, set.baseColorG, set.baseColorB);
		set.fillColor = RGB15(set.baseColorR,set.baseColorG,set.baseColorB)|BIT(15);
		//theMap.setRules(set.starve, set.birth, set.crowd);
		theMap.setRules(set.birthrules,set.deathrules);
	}
}
void drawStyle()
{
	switch (set.mirror)		//set.mirror drawing type
	{
			case 0:
			draw4Mirrors(touchXY.px, touchXY.py);
			break;
			case 1:
			draw2MirrorsX(touchXY.px, touchXY.py);
			break;
			case 2:
			draw2MirrorsY(touchXY.px, touchXY.py);
			break;
			case 3:
			draw2MirrorsZ(touchXY.px, touchXY.py);
			break;
			case 4:
			DrawPt(touchXY.px, touchXY.py);
			break;
	}
}

void processInput()
{
	keysD = keysDown();

	if (keysD & KEY_START) loadNDS("wifc2.nds");
	else if (keysH & KEY_A) {set.DELAY++; if (set.DELAY == 255) doDelay(6400); if (set.DELAY > 254) set.DELAY = 0; iprintf("\x1b[0;0HDelay: %03d", set.DELAY);doDelay(100); return ;}
	else if (keysH & KEY_B) {set.DELAY--; if (set.DELAY == 0) doDelay(6400); else if (set.DELAY < 0) set.DELAY = 255; iprintf("\x1b[0;0HDelay: %03d", set.DELAY);doDelay(100); return ;}
	else if (keysD & KEY_SELECT)
	{
		sprintf(screenshotFile, "Life%04d.bmp", screenshotCount);
		iprintf("\x1b[0;0HSS: %04d  ", screenshotCount);
		screenshotCount++;
		screenshotbmpA(screenshotFile);
		return ;
	}
	else if ((set.scale != 4) && (keysH & (KEY_DOWN | KEY_UP | KEY_RIGHT | KEY_LEFT)))	//Any key can scroll
	{
		swiWaitForVBlank();
		if (keysH & KEY_DOWN)
		{
			set.scrolly += 2;
			if (set.scrolly >= (HEIGHT - (scale_settings[set.scale]>>2) * 3)) set.scrolly = (HEIGHT - (scale_settings[set.scale]>>2) * 3);
		}
		else if (keysH & KEY_UP)
		{
			set.scrolly -= 2;
			if (set.scrolly < 0) set.scrolly = 0;
		}
		if(keysH & KEY_RIGHT)
		{
			set.scrollx += 2;
			if (set.scrollx >= (WIDTH - scale_settings[set.scale])) set.scrollx = (WIDTH - scale_settings[set.scale]);
		}
		else if (keysH & KEY_LEFT)
		{
			set.scrollx -= 2;
			if (set.scrollx < 0) set.scrollx = 0;
		}
			BG2_CY = (set.scrolly << 8);
			BG2_CX = (set.scrollx << 8);
	}
	else if (keysD & KEY_L) {set.swapped ^= 1; lcdSwap(); return ;}
	else if (keysD & KEY_R) {set.swapped ^= 1; lcdSwap(); return ;}
	else if ((keysD == KEY_X) || (keysD == KEY_Y))
	{
		swiWaitForVBlank();
		if (keysD == KEY_Y)
		{
			set.scale++;
			if (set.scale >= set.scale_count) set.scale = set.scale_count - 1;
		}
		else
		{
			set.scale--;
			if (set.scale <= 1) set.scale = 1;
		}

		if (set.scrollx > (WIDTH - scale_settings[set.scale])){ set.scrollx = (WIDTH - scale_settings[set.scale]);BG2_CX = (set.scrollx << 8);}
		if (set.scrollx < 0){ set.scrollx = 0;BG2_CX = (set.scrollx << 8);}
		if (set.scrolly > (HEIGHT - (scale_settings[set.scale]>>2) * 3)) {set.scrolly = (HEIGHT - (scale_settings[set.scale]>>2) * 3);BG2_CY = (set.scrolly << 8);}
		if (set.scrolly < 0) {set.scrolly = 0;BG2_CY = (set.scrolly << 8);}
		BG2_XDX = scale_settings[set.scale];
		BG2_YDY = scale_settings[set.scale];
	}
}

void draw_clear()
{
	memset(BG_GFX, 0, 258*194<<1);
}

void fill_clear()
{
	int x,y;
	for(x = 0;x<256;x++)
	for(y = 0;y<192;y++)
	BG_GFX[y*256+x] = set.fillColor;
	swiWaitForVBlank();
}

bool areaTouched(int y1, int x1, int y2, int x2)
{
	return ((touchXY.px >= x1) && (touchXY.px <= x2) && (touchXY.py >= y1) && (touchXY.py <= y2));
}

void draw2MirrorsX(int x, int y)
{
	DrawPt(x, y);
	DrawPt(x, HEIGHT - y);
}

void draw2MirrorsY(int x, int y)
{
	DrawPt(x, y);
	DrawPt(WIDTH - x, y);
}

void draw2MirrorsZ(int x, int y)
{
	DrawPt(x, y);
	DrawPt(WIDTH - x, HEIGHT - y);
}

void draw4Mirrors(int x, int y)
{
	DrawPt(x, y);
	DrawPt(WIDTH - x, y);
	DrawPt(x, HEIGHT - y);
	DrawPt(WIDTH - x, HEIGHT - y);
}

void DrawPt(int x, int y)
{
	switch (set.dotType)
	{
			case 0: DrawSquare(x, y);
			break;
			case 1: DrawPlus(x, y);
			break;
			case 2: Draw3PtsV(x, y);
			break;
			case 3: Draw3PtsH(x, y);
			break;
			case 4: DrawSquareBig(x, y);
			break;
			case 5: DrawX(x, y);
			break;
			case 6: theMap.toggle_state(x, y);
			break;
			case 7: DrawShape(x, y);
			break;
	}
}

void Draw3PtsH(int x, int y)
{
	theMap.toggle_state(x, y);
	theMap.toggle_state(x - 1, y);
	theMap.toggle_state(x + 1, y);
}

void Draw3PtsV(int x, int y)
{
	theMap.toggle_state(x, y);
	theMap.toggle_state(x, y - 1);
	theMap.toggle_state(x, y + 1);
}

void DrawSquare(int x, int y)
{
	theMap.toggle_state(x, y);
	theMap.toggle_state(x - 1, y);
	theMap.toggle_state(x, y - 1);
	theMap.toggle_state(x - 1, y - 1);
}

void DrawPlus(int x, int y)
{
	theMap.toggle_state(x, y);
	theMap.toggle_state(x, y + 1);
	theMap.toggle_state(x, y - 1);
	theMap.toggle_state(x + 1, y);
	theMap.toggle_state(x - 1, y);
}

void DrawSquareBig(int x, int y)
{
	theMap.toggle_state(x - 1, y + 1);
	theMap.toggle_state(x, y + 1);
	theMap.toggle_state(x + 1, y + 1);
	theMap.toggle_state(x - 1, y);
	theMap.toggle_state(x + 1, y);
	theMap.toggle_state(x - 1, y - 1);
	theMap.toggle_state(x, y - 1);
	theMap.toggle_state(x + 1, y - 1);
}

void DrawX(int x, int y)
{
	theMap.toggle_state(x - 1, y + 1);
	theMap.toggle_state(x + 1, y + 1);
	theMap.toggle_state(x, y);
	theMap.toggle_state(x - 1, y - 1);
	theMap.toggle_state(x + 1, y - 1);
}

void DrawShape(int x, int y)
{
	for (i = 0;i < 5;i++)for (j = 0;j < 5;j++)
			if ((theShape[j][i]) && (x - 2 + i > -1) && (x - 2 + i < WIDTH) && (y - 2 + j > -1) && (y - 2 + j < HEIGHT)) theMap.toggle_state(x - 2 + i, y - 2 + j);
}

void FillScreen()
{
	swiWaitForVBlank();
	theMap.init();
	draw_clear();
	for (i2 = 2; i2 < WIDTH-2;i2 += 5)
	{
		for (j2 = 2; j2 < HEIGHT-2;j2 += 5)
			DrawShape(i2, j2);
			
		if (set.cycleColors) cycleColors();
	}
}

void cycleColors()
{
	switch (set.cycleChoice)
	{
			case 0:
			if (set.cycleBaseDirR) set.baseColorR++;
			else set.baseColorR--;
			if (set.cycleBaseDirG) set.baseColorG++;
			else set.baseColorG--;
			if (set.cycleBaseDirB) set.baseColorB++;
			else set.baseColorB--;
			if ((set.baseColorR >= set.cycleToR) || (set.baseColorR <= set.cycleFromR)) set.cycleBaseDirR ^= 1;
			if ((set.baseColorG >= set.cycleToG) || (set.baseColorG <= set.cycleFromG)) set.cycleBaseDirG ^= 1;
			if ((set.baseColorB >= set.cycleToB) || (set.baseColorB <= set.cycleFromB)) set.cycleBaseDirB ^= 1;
			break;
			
			case 1:
			if (set.cycleCellDirR) set.colorR++;
			else set.colorR--;
			if (set.cycleCellDirG) set.colorG++;
			else set.colorG--;
			if (set.cycleCellDirB) set.colorB++;
			else set.colorB--;
			if ((set.colorR >= set.cycleToR) || (set.colorR <= set.cycleFromR)) set.cycleCellDirR ^= 1;
			if ((set.colorG >= set.cycleToG) || (set.colorG <= set.cycleFromG)) set.cycleCellDirG ^= 1;
			if ((set.colorB >= set.cycleToB) || (set.colorB <= set.cycleFromB)) set.cycleCellDirB ^= 1;
			break;
			
			case 2:
			if (set.cycleBaseDirR) set.baseColorR++;
			else set.baseColorR--;
			if (set.cycleBaseDirG) set.baseColorG++;
			else set.baseColorG--;
			if (set.cycleBaseDirB) set.baseColorB++;
			else set.baseColorB--;
			if ((set.baseColorR >= set.cycleToR) || (set.baseColorR <= set.cycleFromR)) set.cycleBaseDirR ^= 1;
			if ((set.baseColorG >= set.cycleToG) || (set.baseColorG <= set.cycleFromG)) set.cycleBaseDirG ^= 1;
			if ((set.baseColorB >= set.cycleToB) || (set.baseColorB <= set.cycleFromB)) set.cycleBaseDirB ^= 1;
			
			if (set.cycleCellDirR) set.colorR++;
			else set.colorR--;
			if (set.cycleCellDirG) set.colorG++;
			else set.colorG--;
			if (set.cycleCellDirB) set.colorB++;
			else set.colorB--;
			if ((set.colorR >= set.cycleToR) || (set.colorR <= set.cycleFromR)) set.cycleCellDirR ^= 1;
			if ((set.colorG >= set.cycleToG) || (set.colorG <= set.cycleFromG)) set.cycleCellDirG ^= 1;
			if ((set.colorB >= set.cycleToB) || (set.colorB <= set.cycleFromB)) set.cycleCellDirB ^= 1;
			break;
	}
	
	theMap.setColors(set.colorR, set.colorG, set.colorB, set.baseColorR, set.baseColorG, set.baseColorB);
	set.fillColor = RGB15(set.baseColorR,set.baseColorG,set.baseColorB)|BIT(15);
}
