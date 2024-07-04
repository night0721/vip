#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "vip.h"
#include "term.h"
#include "editor.h"
#include "bar.h"
#include "io.h"

editor vip;

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

int main(int argc, char **argv)
{
	setup_term();
	init_editor();
	if (argc >= 2) {
		open_editor(argv[1]);
	}

	set_status_bar_message("By night0721 and gnucolas");

	while (1) {
		refresh_screen();
		process_key();
	}
	return 0;
}
