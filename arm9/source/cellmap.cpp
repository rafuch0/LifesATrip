#ifndef CELLMAP_CPP
#define CELLMAP_CPP

#include "cellmap.H"
#include <string.h>
#include <stdio.h>

using namespace std;

cellmap::cellmap(unsigned int h, unsigned int w)
{
	width = w;
	height = h;

	length_in_bytes = w * h;
	cells = new unsigned char[length_in_bytes];
	temp_cells = new unsigned char[length_in_bytes];

	birthrule = 0 | BIT(3);
	deathrule = 0 | BIT(0)|BIT(1)|BIT(4)|BIT(5)|BIT(6)|BIT(7)|BIT(8)|BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15);

	init();
}

void cellmap::setWeights(unsigned int upleftz, unsigned int upz, unsigned int uprightz, unsigned int leftz, unsigned int rightz, unsigned int downleftz, unsigned int downz, unsigned int downrightz)
{
	upleft = upleftz;
	up = upz;
	upright = uprightz;
	left = leftz;
	right = rightz;
	downleft = downleftz;
	down = downz;
	downright = downrightz;
}

void cellmap::setRules(u16 birthrules, u16 deathrules)
{
	birthrule = birthrules;
	deathrule = deathrules;
}

void cellmap::setRules(int starvez, int birthz, int crowdz)
{
	starve = starvez;
	birth = birthz;
	crowd = crowdz;
}

cellmap::~cellmap(void)
{
	delete[] cells;
	delete[] temp_cells;
}

void cellmap::set_cell(unsigned int x, unsigned int y)
{
	offset = (y*width) + x;
	cell_ptr2 = cells + offset;

	xoleft = x == 0 ? width - 1 : -1;
	yoabove = y == 0 ? length_in_bytes - width : -width;
	xoright = x == (width - 1) ? -(width - 1) : 1;
	yobelow = y == (height - 1) ? -(length_in_bytes - width) : width;

	*(cell_ptr2) |= 0x01;
	*(cell_ptr2 + yoabove + xoleft) += upleft << 1;
	*(cell_ptr2 + yoabove) += up << 1;
	*(cell_ptr2 + yoabove + xoright) += upright << 1;
	*(cell_ptr2 + xoleft) += left << 1;
	*(cell_ptr2 + xoright) += right << 1;
	*(cell_ptr2 + yobelow + xoleft) += downleft << 1;
	*(cell_ptr2 + yobelow) += down << 1;
	*(cell_ptr2 + yobelow + xoright) += downright << 1;

	BG_GFX[offset] = onColor;
}

void cellmap::clear_cell(unsigned int x, unsigned int y)
{
	offset = (y*width) + x;
	cell_ptr2 = cells + offset;

	xoleft = x == 0 ? width - 1 : -1;
	yoabove = y == 0 ? length_in_bytes - width : -width;
	xoright = x == (width - 1) ? -(width - 1) : 1;
	yobelow = y == (height - 1) ? -(length_in_bytes - width) : width;

	*(cell_ptr2) &= ~0x01;
	*(cell_ptr2 + yoabove + xoleft) -= upleft << 1;
	*(cell_ptr2 + yoabove) -= up << 1;
	*(cell_ptr2 + yoabove + xoright) -= upright << 1;
	*(cell_ptr2 + xoleft) -= left << 1;
	*(cell_ptr2 + xoright) -= right << 1;
	*(cell_ptr2 + yobelow + xoleft) -= downleft << 1;
	*(cell_ptr2 + yobelow) -= down << 1;
	*(cell_ptr2 + yobelow + xoright) -= downright << 1;

	BG_GFX[offset] = offColor;
}

bool cellmap::cell_state(int x, int y)
{
	cell_ptr2 = cells + (y * width) + x;
	return ((*cell_ptr2 & 0x01) == 1);
}

void cellmap::set_state(int x, int y)
{
	if(!cell_state(x,y))
	{
		set_cell(x,y);
	}
}

void cellmap::clear_state(int x, int y)
{
	if (cell_state(x, y))
	{
		clear_cell(x, y);
	}
}

void cellmap::toggle_state(int x, int y)
{
	if (cell_state(x, y))	clear_cell(x, y);
	else	set_cell(x, y);
}

void cellmap::draw_pixel(unsigned int x, unsigned int y)
{
	BG_GFX[width * y + x] = onColor;
}

void cellmap::redraw()
{
	for (y1 = 0; y1 < height; y1++)for (x1 = 0; x1 < width; x1++)if (cell_state(x1, y1)) draw_pixel(x1, y1);
}


void cellmap::next_generation(void)
{
	memcpy(temp_cells, cells, length_in_bytes);

	cell_ptr1 = temp_cells;
	for (y2 = 0; y2 < height; y2++)
	{
		x2 = 0;
		do
		{
			while (*cell_ptr1 == 0)
			{
				cell_ptr1++;
				if (++x2 >= width) goto RowDone;
			}

			count = *cell_ptr1 >> 1;

			if ((*cell_ptr1 & 0x01))
			{
				if(deathrule&(0|BIT(count))) clear_cell(x2,y2);
//				if ((count < starve) || (count >= crowd))	//Standard
//				if ((count != 2) && (count != 3))	//High Life
//				if ((count != 3) && (count != 4) && (count != 6) && (count != 7) && (count != 8))	//Day and Night
//				if ((count != 5) && (count != 6) && (count != 7) && (count != 8))	//Diamoeba
//				if ((count != 2) && (count != 4) && (count != 5))
//				{
//					clear_cell(x2, y2);
//				}
			}
			else
			{
				if(birthrule&(0|BIT(count))) set_cell(x2,y2);
//				if ((count == birth))						//Standard
//				if ((count == 3) || (count == 6))	//High Life
//				if ((count == 3) || (count == 6) || (count == 7) || (count == 8))	//Day and Night
//				if ((count == 3) || (count == 5) || (count == 6) || (count == 7) || (count == 8))   //Diamoeba
//				if ((count == 3) || (count == 6))
//				{
//					set_cell(x2, y2);
//				}
			}
			cell_ptr1++;
		}
		while (++x2 < width);
RowDone:
		continue;
	}
}

void cellmap::setColors(unsigned int onR, unsigned int onG, unsigned int onB, unsigned int offR, unsigned int offG, unsigned int offB)
{
	onColor = RGB15(onR, onG, onB) | BIT(15);
	offColor = RGB15(offR,offG,offB) | BIT(15);
}

void cellmap::clear()
{
	init();
}

void cellmap::init()
{
	memset(cells, 0, length_in_bytes);
}

#endif

