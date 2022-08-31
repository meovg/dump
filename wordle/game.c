/* a wordle clone with basic interface run on windows terminal */

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include <windows.h>
#include <stdarg.h>

#include "dict.h"

typedef dict_of(char) Pool;

/* special keyboard code (enter, backspace) */
#define KB_ENTER    13
#define KB_BACKS    8

/* wchar_t typed values for tile borders */
#define W_HOR       196
#define W_VER       179
#define W_TOPL      218
#define W_BOTL      192
#define W_TOPR      191
#define W_BOTR      217

/* console screen buffer text attributes (color) */
#define B_BLACK     0
#define B_GREEN     37
#define B_YELLOW    110
#define B_GREY      128
#define F_WHITE     15

/* mask defines the color to be placed on the tiles as hints */
#define MSK_GREY    0
#define MSK_YELLOW  1
#define MSK_GREEN   2
#define MSK_WIN     682

/* letter tile sizes */
#define TILE_H      3
#define TILE_W      5

/* on screen keyboard offsets */
#define KEY_W       3
#define KEY_H       3
#define KEY_TOP_X   0
#define KEY_MID_X   2
#define KEY_BOT_X   4
#define KEY_TOP_Y   0
#define KEY_MID_Y   3
#define KEY_BOT_Y   6

/* get keypress and return the character */
char from_keyboard(void) {
    while (!kbhit());
    return (char)getch();
}

char uppercase(char c) {
    return (c >= 97 && c <= 122 ? c - 32 : c);
}

char lowercase(char c) {
    return (c >= 65 && c <= 90 ? c + 32 : c);
}

/* a sequence of bits are used to mask the color of each letter of guessed word
   color mask of a letter takes 2 bits
   using macros instead of functions as mask can be of any integer type */
#define mask_set(maskptr, pos, color) \
    __typeof__(*(maskptr)) color_inv = (color) ^ 3; \
    *(maskptr) |= ((__typeof__(*(maskptr)))3 << ((pos) << 1)); \
    *(maskptr) ^= (color_inv << ((pos) << 1))

#define mask_get(mask, pos) \
    (int)(((mask) >> ((pos) << 1)) & (__typeof__(mask))(3))

/* check whether guessed word is in the word pool */
int is_valid_guess(const char guess[5], Pool *p) {
    return dict_get(p, guess) != NULL;
}

/* get color mask based on how the guessed word matches the answer */
uint16_t check_guess(const char guess[5], const char answer[5]) {
    uint16_t color_mask = 0;
    int vis = 0;

    for (int i = 0; i < 5; i++) {
        if (answer[i] == guess[i]) {
            mask_set(&color_mask, i, MSK_GREEN);
            vis |= 1 << i;
        }
    }

    for (int i = 0; i < 5; i++) {
        if ((vis >> i) & 1) {
            continue;
        }

        for (int j = 0; j < 5; j++) {
            int tmp = mask_get(color_mask, j);
            if (tmp == MSK_GREY && answer[i] == guess[j]) {
                mask_set(&color_mask, j, MSK_YELLOW);
                vis |= 1 << i;
                break;
            }
        }
    }
    return color_mask;
}

WORD init_attrs;
CONSOLE_CURSOR_INFO cursor_info;
CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;

/* get initial console cursor & screen buffer info */
void cons_get_init_info(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(h, &scrbuf_info);
    GetConsoleCursorInfo(h, &cursor_info);
}

void curs_hide(void) {
    CONSOLE_CURSOR_INFO i;
    i.dwSize = 100;
    i.bVisible = FALSE;

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
}

void curs_show(void) {
    CONSOLE_CURSOR_INFO i;
    i.dwSize = cursor_info.dwSize;
    i.bVisible = TRUE;

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
}

/* set the back/foreground colord of output screen buffer on console */
void cons_set_color(int fore, int back) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(h, &i);

    /* stores the initial text color attributes */
    init_attrs = i.wAttributes;
    SetConsoleTextAttribute(h, fore | back);
}

void cons_reset_color(void) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), init_attrs);
}

void curs_set_pos(int x, int y) {
    COORD loc = {(short)x, (short)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
}

COORD curs_get_pos(void) {
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
    return i.dwCursorPosition;
}

/* write a wchar_t character to the console with color */
void cons_write(int x, int y, wchar_t c, int fore, int back) {
    curs_set_pos(x, y);
    cons_set_color(fore, back);

    printf("%lc", c);

    cons_reset_color();
}

/* draw a square tile with
   (xa, ya), (xb, yb): the coordinates of the top left and bottom right corners
   (fore, back): fore/background colors of the tile */
void tile_draw(int xa, int ya, int xb, int yb, int fore, int back) {
    for (int i = xa + 1; i < xb; i++) {
        for (int j = ya + 1; j < yb; j++) {
            cons_write(i, j, 0, fore, back);
        }
    }

    for (int i = xa + 1; i < xb; i++) {
        cons_write(i, ya, W_HOR, fore, back);
        cons_write(i, yb, W_HOR, fore, back);
    }

    for (int i = ya + 1; i < yb; i++) {
        cons_write(xa, i, W_VER, fore, back);
        cons_write(xb, i, W_VER, fore, back);
    }

    cons_write(xa, ya, W_TOPL, fore, back);
    cons_write(xa, yb, W_BOTL, fore, back);
    cons_write(xb, ya, W_TOPR, fore, back);
    cons_write(xb, yb, W_BOTR, fore, back);
}

int grid_st_x, grid_st_y;

void grid_place(int x, int y, char c, int fore, int back) {
    tile_draw(x, y, x + 4, y + 2, fore, back);
    cons_write(x + 2, y + 1, c, fore, back);
}

/* draw the game playing area - a 6x5 board of black tiles */
void grid_draw(void) {
    int cx = grid_st_x;
    int cy = grid_st_y;

    for (int i = 0; i < 6; i++) {
        cx = grid_st_x;
        for (int j = 0; j < 5; j++) {
            tile_draw(cx, cy, cx + TILE_W - 1, cy + TILE_H - 1, F_WHITE, B_BLACK);
            cx += TILE_W + 1;
        }
        cy += TILE_H;
    }
}

/* location offsets of 26 letter keys in the keyboard */
static const size_t keys_x[] = {
    KEY_MID_X,              KEY_BOT_X + 4 * KEY_W,  KEY_BOT_X + 2 * KEY_W,
    KEY_MID_X + 2 * KEY_W,  KEY_TOP_X + 2 * KEY_W,  KEY_MID_X + 3 * KEY_W,
    KEY_MID_X + 4 * KEY_W,  KEY_MID_X + 5 * KEY_W,  KEY_TOP_X + 7 * KEY_W,
    KEY_MID_X + 6 * KEY_W,  KEY_MID_X + 7 * KEY_W,  KEY_MID_X + 8 * KEY_W,
    KEY_BOT_X + 6 * KEY_W,  KEY_BOT_X + 5 * KEY_W,  KEY_TOP_X + 8 * KEY_W,
    KEY_TOP_X + 9 * KEY_W,  KEY_TOP_X,              KEY_TOP_X + 3 * KEY_W,
    KEY_MID_X +     KEY_W,  KEY_TOP_X + 4 * KEY_W,  KEY_TOP_X + 6 * KEY_W,
    KEY_BOT_X + 3 * KEY_W,  KEY_TOP_X +     KEY_W,  KEY_BOT_X +     KEY_W,
    KEY_TOP_X + 5 * KEY_W,  KEY_BOT_X
};

static const size_t keys_y[] = {
    KEY_MID_Y,  KEY_BOT_Y,  KEY_BOT_Y,
    KEY_MID_Y,  KEY_TOP_Y,  KEY_MID_Y,
    KEY_MID_Y,  KEY_MID_Y,  KEY_TOP_Y,
    KEY_MID_Y,  KEY_MID_Y,  KEY_MID_Y,
    KEY_BOT_Y,  KEY_BOT_Y,  KEY_TOP_Y,
    KEY_TOP_Y,  KEY_TOP_Y,  KEY_TOP_Y,
    KEY_MID_Y,  KEY_TOP_Y,  KEY_TOP_Y,
    KEY_BOT_Y,  KEY_TOP_Y,  KEY_BOT_Y,
    KEY_TOP_Y,  KEY_BOT_Y
};

/* require 52 bits to store the color mask of 26 keys */
uint64_t keys_mask;

int keys_st_x, keys_st_y;

void keys_place(int x, int y, char c, int fore, int back) {
    tile_draw(x, y, x + 2, y + 2, fore, back);
    cons_write(x + 1, y + 1, c, fore, back);
}

/* draw an onscreen keyboard starting at (x, y) */
void keys_draw(void) {
    for (int i = 0; i < 26; i++) {
        keys_place(keys_st_x + keys_x[i], keys_st_y + keys_y[i], i + 'A', F_WHITE, B_BLACK);
    }
}

/* recolor a tile (key) in the onscreen keyboard */
void keys_color_tile(char c, int color) {
    int i = c - 'A';

    switch (mask_get(keys_mask, i)) {
        case MSK_GREEN: break;
        case MSK_YELLOW: {
            if (color == B_GREEN) {
                keys_place(keys_st_x + keys_x[i], keys_st_y + keys_y[i], c, F_WHITE, B_GREEN);
                mask_set(&keys_mask, i, MSK_GREEN);
            }
            break;
        }
        default: {
            int color_mask = (color == B_GREEN ? MSK_GREEN
                : (color == B_YELLOW ? MSK_YELLOW
                    : MSK_GREY));
            keys_place(keys_st_x + keys_x[i], keys_st_y + keys_y[i], c, F_WHITE, color);
            mask_set(&keys_mask, i, color_mask);
        }
    }
}

int tbox_st_x, tbox_st_y, tbox_ed_x, tbox_ed_y;
char message[50];

/* draw text box at the bottom right of game area */
void tbox_draw(void) {
    tbox_ed_x = tbox_st_x + KEY_W * 10;
    tbox_ed_y = tbox_st_y + 4;
    tile_draw(tbox_st_x, tbox_st_y, tbox_ed_x, tbox_ed_y, F_WHITE, B_BLACK);
}

#define tbox_line_limit (KEY_W * 10 - 1)

/* function like strrchr for substrings */
char *strrchr_(const char *str, size_t len, int c) {
    char *res = NULL;

    while (len-- && *str != '\0') {
        if (*str == c) {
            res = (char *)str;
        }
        str++;
    }
    return res;
}

/* find the end of the line (usually right before the last space or newline
   so that it fits the textbox) */
int tbox_line_end(const char *msg) {
    char *pspace = strrchr_(msg, tbox_line_limit, ' ');
    char *pnewln = strrchr_(msg, tbox_line_limit, '\n');

    if (strlen(msg) <= tbox_line_limit || (pspace == NULL && pnewln == NULL)) {
        return tbox_line_limit;
    } else if (pnewln == NULL) {
        return pspace - msg;
    } else {
        return pnewln - msg;
    }
}

/* show the text in the textbox */
void tbox_show(const char *format, ...) {
    memset(message, 0, 50);

    /* store the formatted text into a buffer */
    va_list args;
    va_start(args, format);
    vsnprintf(message, 50, format, args);
    va_end(args);

    /* separate text into lines and print them in the textbox */
    cons_set_color(F_WHITE, B_BLACK);
    curs_set_pos(tbox_st_x + 1, tbox_st_y + 1);

    char *line = message;
    int endpos = tbox_line_end(line);
    printf("%.*s", endpos, line);

    curs_set_pos(tbox_st_x + 1, tbox_st_y + 2);

    line += endpos + 1;
    endpos = tbox_line_end(line);
    printf("%.*s", endpos, line);

    cons_reset_color();
}

/* clear the textbox */
void tbox_clean(void) {
    memset(message, ' ', 50);

    cons_set_color(F_WHITE, B_BLACK);
    curs_set_pos(tbox_st_x + 1, tbox_st_y + 1);
    printf("%.*s", tbox_line_limit, message);

    curs_set_pos(tbox_st_x + 1, tbox_st_y + 2);
    printf("%.*s", tbox_line_limit, message);

    cons_reset_color();
}

void init_screen(void) {
    cons_get_init_info();
    curs_hide();

    COORD loc = curs_get_pos();

    grid_st_x = loc.X;
    grid_st_y = loc.Y;
    grid_draw();

    keys_st_x = loc.X + 6 * TILE_W + 1;
    keys_st_y = loc.Y;
    keys_draw();

    tbox_st_x = keys_st_x;
    tbox_st_y = loc.Y + 3 * KEY_W + 1;
    tbox_draw();
}


void game(const char answer[5], Pool *p) {
    uint16_t mask = 0;
    char guess[6] = {[5] = '\0'};
    int x = grid_st_x;
    int y = grid_st_y;
    int i;

    for (i = 1; i <= 6; i++) {
        int pos = 0;

        /* get guess from keyboard */
        for (;;) {
            char ch = lowercase(from_keyboard());
            tbox_clean();

            if (ch == KB_ENTER) {
                if (pos != 5) {
                    tbox_show("Not enough letters");
                } else if (!is_valid_guess(guess, p)) {
                    tbox_show("Not in word list");
                } else {
                    break;
                }
            } else if (ch == KB_BACKS && pos > 0) {
                /* backspace key is pressed: remove the letter in the last tile */
                grid_place(x - TILE_W - 1, y, 0, F_WHITE, B_BLACK);
                x -= TILE_W + 1;
                guess[--pos] = '\0';
            } else if (ch >= 'a' && ch <= 'z' && pos < 5) {
                /* letter key is pressed: put it on tile */
                grid_place(x, y, uppercase(ch), F_WHITE, B_BLACK);
                x += TILE_W + 1;
                guess[pos++] = ch;
            }
        }

        /* move the cursor to the first tile of the row */
        x = grid_st_x;
        mask = check_guess(guess, answer);

        /* color each guessed letters in the grid as well as the keyboard */
        int tile_color;
        for (int j = 0; j < 5; j++) {
            switch (mask_get(mask, j)) {
                case MSK_GREEN: tile_color = B_GREEN; break;
                case MSK_YELLOW: tile_color = B_YELLOW; break;
                default: tile_color = B_GREY;
            }

            grid_place(x, y, uppercase(guess[j]), F_WHITE, tile_color);
            keys_color_tile(uppercase(guess[j]), tile_color);
            x += TILE_W + 1;
        }

        if (mask == MSK_WIN) {
            break;
        }

        /* player does not win, move to the next row in grid */
        x = grid_st_x;
        y += TILE_H;
    }

    if (mask == MSK_WIN) {
        tbox_show("Solved after %d guess(es). Press any key to exit", i);
    } else {
        tbox_show("The answer is %s. Press any key to exit", answer);
    }

    /* read the key and exit without doing anything */
    from_keyboard();
    /* move the cursor away from the play area */
    curs_set_pos(0, grid_st_y + 6 * TILE_H);
}

/* putting word list into the hash table */
void import_words(const char *file_name, Pool *p) {
    FILE *f = fopen(file_name, "r");

    char token[7] = {[6] = '\0'};
    while (fgets(token, 7, f)) {
        token[5] = '\0';
        dict_set(p, token, 0);
    }

    fclose(f);
}

int main(void) {
    Pool p;
    dict_init(&p);

    import_words("answers.txt", &p);
    const char *answer = dict_rand(&p)->key;
    import_words("words.txt", &p);

    init_screen();
    game(answer, &p);

    dict_clear(&p);
    curs_show();

    return 0;
}
