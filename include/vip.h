#ifndef VIP_H_
#define VIP_H_

#include <termios.h>
#include <time.h>

#define VERSION "0.0.1"

#define COLOR_LEN 19

enum keys {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

enum modes {
	NORMAL,
	INSERT,
	VISUAL,
	COMMAND
};

enum highlight {
	DEFAULT = 0,
	COMMENT,
	MLCOMMENT,
	KEYWORD1, /* default */
	KEYWORD2, /* types */
	STRING,
	NUMBER,
	MATCH,
	RESET
};

#include "row.h"
#include "syntax.h"
#include "config.h"

typedef struct editor {
	int cx, cy; /* chars x, y */
	int rx; /* render x */
	int rowoff;
	int coloff;
	int screenrows, screencols;
	int rows;
	row *row;
	int dirty;
	int mode;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	language *syntax;
	struct termios termios;
} editor;

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT { NULL, 0 }
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

extern editor vip;

#endif
