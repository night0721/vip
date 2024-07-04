#ifndef ROW_H_
#define ROW_H_

typedef struct row {
	int idx;
	int size;
	int render_size;
	char *chars;
	char *render;
	unsigned char *hl;
	int opened_comment;
} row;

int row_cx_to_rx(row *row, int cx);
int row_rx_to_cx(row *row, int rx);
void update_row(row *row);
void insert_row(int at, char *s, size_t len);
void free_row(row *row);
void del_row(int at);
void row_insert_char(row *row, int at, int c);
void row_append_str(row *row, char *s, size_t len);
void row_del_char(row *row, int at);
char *rows_to_str(int *buflen);

#endif
