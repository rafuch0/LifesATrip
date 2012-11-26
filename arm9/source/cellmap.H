#ifndef CELLMAP_H
#define CELLMAP_H

#include "nds.h"

class cellmap
{
	public:
		cellmap(unsigned int h, unsigned int w);
		~cellmap(void);
		void set_cell(unsigned int x, unsigned int y);
		void clear_cell(unsigned int x, unsigned int y);
		bool cell_state(int x, int y);
		void next_generation(void);
		void init(void);
		void draw_pixel(unsigned int x, unsigned int y);
		void redraw();
		void clear();
		void toggle_state(int x, int y);
		void set_state(int x, int y);
		void clear_state(int x, int y);
		void setColors(unsigned int onR, unsigned int onG, unsigned int onB, unsigned int offR, unsigned int offG, unsigned int offB);
		void setRules(int starvez, int birthz, int deathz);
		void setWeights(unsigned int upleftz, unsigned int upz, unsigned int uprightz, unsigned int leftz, unsigned int rightz, unsigned int downleftz, unsigned int downz, unsigned int downrightz);
		void setRules(u16 birthrules,u16 deathrules);
	private:
		unsigned int width;
		unsigned int height;
		unsigned int length_in_bytes;

		u16 onColor;
		u16 offColor;
		unsigned int starve;
		unsigned int birth;
		unsigned int crowd;
		unsigned int upleft, up, upright, left, right, downleft, down, downright;

		//Static function variables
		unsigned char *cells;
		unsigned char *temp_cells;
		unsigned int x1,y1,x2,y2;
		unsigned int count;
		int xoleft, xoright, yoabove, yobelow;	
		unsigned char *cell_ptr1;
		unsigned char *cell_ptr2;
		unsigned int offset;
		u16 birthrule;
		u16 deathrule;
};

#endif
