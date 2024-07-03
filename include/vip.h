#ifndef VIP_H_
#define VIP_H_

#include <termios.h>
#include <time.h>

/* CONFIG */
#define TAB_SIZE 4

#define VERSION "0.0.1"

/* number of times of warning before quitting when there is modified text */
#define QUIT_CONFIRM 1

/* THEME */
/* 38 and 48 is reversed as bar's color is reversed */

#define COLOR_LEN 19

#define SURFACE_1_BG "\x1b[38;2;049;050;068m"
#define BLACK_FG     "\x1b[48;2;000;000;000m"
#define BLACK_BG     "\x1b[38;2;000;000;000m"
#define WHITE_FG     "\x1b[48;2;205;214;244m"
#define WHITE_BG     "\x1b[38;2;205;214;244m"
#define BLUE_FG	     "\x1b[48;2;137;180;250m"
#define BLUE_BG      "\x1b[38;2;137;180;250m"
#define GREEN_FG     "\x1b[48;2;166;227;161m"
#define GREEN_BG     "\x1b[38;2;166;227;161m"
#define PEACH_FG     "\x1b[48;2;250;179;135m"
#define PEACH_BG     "\x1b[38;2;250;179;135m"
#define SKY_FG       "\x1b[48;2;137;220;235m"
#define SKY_BG       "\x1b[38;2;137;220;235m"

#define CTRL_KEY(k) ((k) & 0x1f)

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
	HL_NORMAL = 0,
	HL_NUMBER,
	HL_MATCH,
	HL_RESET
};

typedef struct row {
	int size;
	int render_size;
	char *chars;
	char *render;
	unsigned char *hl;
} row;

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
	struct termios termios;
} editor;

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len);

int read_key();
void refresh_screen();
void append_row(char *s, size_t len);
void row_insert_char(row *row, int at, int c);
void row_del_char(row *row, int at);

extern editor vip;

#endif
