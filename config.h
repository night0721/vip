#define VERSION "0.0.1"

#define TAB_SIZE 4

/* THEME */
/* 38 and 48 is reversed as bar's color is reversed */

#define SURFACE_0_BG "\033[38;2;49;50;68m"
#define SURFACE_1_BG "\033[38;2;69;71;90m"
#define OVERLAY_2_BG "\033[38;2;147;153;178m"
#define BLACK_FG     "\033[40m"
#define BLACK_BG     "\033[30m"
#define WHITE_FG     "\033[48;2;205;214;244m"
#define WHITE_BG     "\033[38;2;205;214;244m"
#define BLUE_FG	     "\033[44m"
#define BLUE_BG      "\033[34m"
#define GREEN_FG     "\033[42m"
#define GREEN_BG     "\033[32m"
#define PEACH_FG     "\033[48;2;250;179;135m"
#define PEACH_BG     "\033[38;2;250;179;135m"
#define SKY_FG       "\033[48;2;137;220;235m"
#define SKY_BG       "\033[38;2;137;220;235m"
#define MAUVE_FG     "\033[48;2;203;166;247m"
#define MAUVE_BG     "\033[38;2;203;166;247m"
#define YELLOW_BG    "\033[33m"
#define RED_FG		 "\033[41m"
#define RED_BG		 "\033[31m"
#define TEAL_FG		 "\033[46m"
#define TEAL_BG		 "\033[36m"
#define PINK_FG		 "\033[45m"
#define PINK_BG		 "\033[35m"

/* ERROR is red with bold and italic */
#define ERROR "\033[31;1;3m"

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
	SYMBOL,
	COMMENT,
	TERMINATOR,
	MLCOMMENT,
	KW,
	KW_TYPE,
	KW_FN,
	KW_BRACKET,
	STRING,
	CHAR,
	ESCAPE,
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

language_t langs[] = {
	{
		"c",
		"//",
		"/*",
		"*/",
		{ "const", "switch", "if", "while", "for", "break", "continue", "return", "else", "struct", "union", "typedef", "static", "enum", "case", "sizeof", "#include", "#define", "#if", "#elseif", "#endif", "int|", "long|", "double|", "float|", "char|", "unsigned|", "void|", "size_t|", "uint8_t|", NULL },
		{ ".c", ".h", ".cpp", NULL },
	},
};

#define LANGS_LEN (sizeof(langs) / sizeof(langs[0]))

#define CTRL_KEY(k) ((k) & 0x1f)
#define COLOR_LEN 19
