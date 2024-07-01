#include "vip.h"

extern editor vip;

void insert_char(int c)
{
	if (vip.cy == vip.rows) {
		append_row("", 0);
	}
	row_insert_char(&vip.row[vip.cy], vip.cx, c);
	vip.cx++;
}
