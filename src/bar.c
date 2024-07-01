#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "vip.h"

extern editor vip;

void draw_status_bar(struct abuf *ab)
{
	abAppend(ab, "\x1b[7m", 4);
	abAppend(ab, "\x1b[1m", 4);
	abAppend(ab, "\x1b[38;2;137;180;250m", 19);

	char mode[9] = " NORMAL ";
	int mode_len = 8;
	abAppend(ab, mode, 8);

	char file[80], lines[80];
	int file_len = snprintf(file, sizeof(file), " %.20s ", 
			vip.filename ? vip.filename : "[No Name]");
	int lines_len = snprintf(lines, sizeof(lines), "%dL | %d:%d", vip.rows,
			vip.cy + 1, vip.rx + 1);

	abAppend(ab, "\x1b[38;2;49;50;68m", 16);
	abAppend(ab, "\x1b[22m", 5);
	abAppend(ab, "\x1b[48;2;137;180;250m", 19);
	abAppend(ab, file, file_len);
	abAppend(ab, "\x1b[38;2;0;0;0m", 13);


	while (file_len < vip.screencols) {
		if (vip.screencols - mode_len - file_len == lines_len) {
			abAppend(ab, "\x1b[48;2;255;255;255m", 19);
			abAppend(ab, lines, lines_len);
			break;
		} else {
			abAppend(ab, " ", 1);
			file_len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

void draw_message_bar(struct abuf *ab)
{
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(vip.statusmsg);
	if (msglen > vip.screencols) msglen = vip.screencols;
	if (msglen && time(NULL) - vip.statusmsg_time < 5)
		abAppend(ab, vip.statusmsg, msglen);
}

void set_status_bar_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(vip.statusmsg, sizeof(vip.statusmsg), fmt, ap);
	va_end(ap);
	vip.statusmsg_time = time(NULL);
}
