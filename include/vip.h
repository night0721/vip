#ifndef VIP_H_
#define VIP_H_

#include <termios.h>
#include <time.h>

/* CONFIG */
#define TAB_SIZE 4

#define VERSION "0.0.1"

typedef struct row {
	int size;
	int render_size;
	char *chars;
	char *render;
} row;

typedef struct editor {
	int cx, cy; /* chars x, y */
	int rx; /* render x, y */
	int rowoff;
	int coloff;
	int screenrows, screencols;
	int rows;
	row *row;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct termios termios;
} editor;

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len);

void append_row(char *s, size_t len);
void row_insert_char(row *row, int at, int c);

extern editor vip;

#endif
