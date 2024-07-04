#ifndef CONFIG_H_
#define CONFIG_H_

#include "syntax.h"

#define TAB_SIZE 4

/* THEME */
/* 38 and 48 is reversed as bar's color is reversed */

#define SURFACE_1_BG "\x1b[38;2;049;050;068m"
#define OVERLAY_0_BG "\x1b[38;2;108;112;134m"
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
#define MAUVE_BG     "\x1b[38;2;203;166;247m"
#define YELLOW_BG    "\x1b[38;2;249;226;175m"

#endif
