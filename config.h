#define VERSION "0.0.1"

#define TAB_SIZE 4

/* THEME */
/* 38 and 48 is reversed as bar's color is reversed */

#define SURFACE_1_BG "\033[38;2;049;050;068m"
#define OVERLAY_0_BG "\033[38;2;108;112;134m"
#define BLACK_FG     "\033[48;2;000;000;000m"
#define BLACK_BG     "\033[38;2;000;000;000m"
#define WHITE_FG     "\033[48;2;205;214;244m"
#define WHITE_BG     "\033[38;2;205;214;244m"
#define BLUE_FG	     "\033[48;2;137;180;250m"
#define BLUE_BG      "\033[38;2;137;180;250m"
#define GREEN_FG     "\033[48;2;166;227;161m"
#define GREEN_BG     "\033[38;2;166;227;161m"
#define PEACH_FG     "\033[48;2;250;179;135m"
#define PEACH_BG     "\033[38;2;250;179;135m"
#define SKY_FG       "\033[48;2;137;220;235m"
#define SKY_BG       "\033[38;2;137;220;235m"
#define MAUVE_FG     "\033[48;2;203;166;247m"
#define MAUVE_BG     "\033[38;2;203;166;247m"
#define YELLOW_BG    "\033[38;2;249;226;175m"
#define RED_FG		 "\033[48;2;243;139;168m"
#define RED_BG		 "\033[38;2;243;139;168m"

/* ERROR is red with bold and italic */
#define ERROR "\033[38;2;243;139;168m\033[1m\033[3m"

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

typedef struct {
	int idx;
	int size;
	int render_size;
	char *chars;
	char *render;
	unsigned char *hl;
	int opened_comment;
} row_t;

#define MAX_KEYWORDS 100
#define MAX_EXTENSIONS 10

typedef struct {
	char *filetype;
	int flags;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	char *keywords[MAX_KEYWORDS];
	char *extensions[MAX_EXTENSIONS];
} language_t;

typedef struct {
	int x, y; /* chars x, y */
	int rx; /* render x */
	int rowoff;
	int coloff;
	int rows;
	row_t *row;
	int dirty;
	int mode;
	char filename[PATH_MAX];
	char cwd[PATH_MAX];
	language_t *syntax;
} editor_t;

#define HL_NUMBERS (1 << 0)
#define HL_STRINGS (1 << 1)

language_t langs[] = {
	{
		"c",
		HL_NUMBERS | HL_STRINGS,
		"//",
		"/*",
		"*/",
		{ "switch", "if", "while", "for", "break", "continue", "return", "else", "struct", "union", "typedef", "static", "enum", "case", "sizeof", "#include", "#define", "#if", "#elseif", "#endif", "int|", "long|", "double|", "float|", "char|", "unsigned|", "void|", NULL },
		{ ".c", ".h", ".cpp", NULL },
	},
};

#define LANGS_LEN (sizeof(langs) / sizeof(langs[0]))

#define CTRL_KEY(k) ((k) & 0x1f)
#define COLOR_LEN 19
