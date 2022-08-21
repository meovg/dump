/* a wordle clone run on windows terminal
 * it has basic interface like a 6x5 grid and an onscreen keyboard
 * to show guesses and color hints
 */ 

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include <windows.h>

#include "map.h"

typedef MapOf(int) poolmap;

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

char uppercase(char u) {
    return (u >= 97 && u <= 122 ? u - 32 : u);
}

char lowercase(char u) {
    return (u >= 65 && u <= 90 ? u + 32 : u);
}

/* a sequence of bits are used to mask the color of each letter of guessed word
 * color mask of a letter takes 2 bits
 */
void word_mask_set_color(uint16_t *mask, int pos, int color) {
    *mask |= ((uint16_t)(color) << (pos << 1));
}

int word_mask_get_color(uint16_t mask, int pos) {
    return (int)((mask >> (pos << 1)) & (uint16_t)3);
}

/* checks whether guessed word is in the word pool using binary search */
int word_is_in_pool(const char guess[5], poolmap *pool) {
    return map_get(pool, guess) != NULL;
}

/* gets color mask based on how the guessed word matches the answer */
uint16_t word_get_mask(const char guess[5], const char answer[5]) {
    uint16_t color_mask = 0;
    int visited_mask = 0;

    for (int i = 0; i < 5; i++) {
        if (answer[i] == guess[i]) {
            word_mask_set_color(&color_mask, i, MSK_GREEN);
            visited_mask |= 1 << i;
        }
    }
    for (int i = 0; i < 5; i++) {
        if ((visited_mask >> i) & 1) {
            continue;
        }
        for (int j = 0; j < 5; j++) {
            int tmp = word_mask_get_color(color_mask, j);
            if (tmp == MSK_GREY && answer[i] == guess[j]) {
                word_mask_set_color(&color_mask, j, MSK_YELLOW);
                visited_mask |= 1 << i;
                break;
            }
        }
    }
    return color_mask;
}

WORD init_attrs;
CONSOLE_CURSOR_INFO cursor_info;
CONSOLE_SCREEN_BUFFER_INFO scrbuf_info;

/* gets initial console cursor & screen buffer info */
void console_get_init_info(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(h, &scrbuf_info);
    GetConsoleCursorInfo(h, &cursor_info);
}

void console_hide_cursor(void) {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

void console_show_cursor(void) {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = cursor_info.dwSize;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

/* sets the back/foreground colord of output screen buffer on console */
void console_set_output_color(int fore, int back) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);

    /* stores the initial text color attributes */
    init_attrs = csbi.wAttributes;
    SetConsoleTextAttribute(h, fore | back);
}

void console_reset_output_color(void) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), init_attrs);
}

void console_set_cursor_pos(int x, int y) {
    COORD loc = {(short)x, (short)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
}

COORD console_get_cursor_pos(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.dwCursorPosition;
}

/* writes a wchar_t typed character at the position (x, y) with fore/background color */
void console_write_wchar(int x, int y, wchar_t ch, int fore, int back) {
    console_set_cursor_pos(x, y);
    console_set_output_color(fore, back);
    printf("%lc", ch);
    console_reset_output_color();
}

/* draws a square tile
 * with (xa, ya), (xb, yb): the coordinates of the top left and bottom right corners
 *      (fore, back): fore/background colors of the tile
 * note: at the end of the function, the cursor location is (xa, ya)
 */
void tile_draw(int xa, int ya, int xb, int yb, int fore, int back) {
    for (int i = xa + 1; i < xb; i++) {
        for (int j = ya + 1; j < yb; j++) {
            console_write_wchar(i, j, 0, fore, back);
        }
    }
    for (int i = xa + 1; i < xb; i++) {
        console_write_wchar(i, ya, W_HOR, fore, back);
        console_write_wchar(i, yb, W_HOR, fore, back);
    }
    for (int i = ya + 1; i < yb; i++) {
        console_write_wchar(xa, i, W_VER, fore, back);
        console_write_wchar(xb, i, W_VER, fore, back);
    }
    console_write_wchar(xa, ya, W_TOPL, fore, back);
    console_write_wchar(xa, yb, W_BOTL, fore, back);
    console_write_wchar(xb, ya, W_TOPR, fore, back);
    console_write_wchar(xb, yb, W_BOTR, fore, back);

    console_set_cursor_pos(xa, ya);
}

void grid_place(int x, int y, char ch, int fore, int back) {
    tile_draw(x, y, x + 4, y + 2, fore, back);
    console_write_wchar(x + 2, y + 1, ch, fore, back);
    console_set_cursor_pos(x, y);
}

/* draws the game playing area - a 6x5 board of black tiles */
void grid_draw(int x, int y) {
    int ox = x;
    int oy = y;

    for (int i = 0; i < 6; i++) {
        ox = x;
        for (int j = 0; j < 5; j++) {
            tile_draw(ox, oy, ox + TILE_W - 1, oy + TILE_H - 1, F_WHITE, B_BLACK);
            ox += TILE_W + 1;
        }
        oy += TILE_H;
    }
    console_set_cursor_pos(x, y);
}

/* location offsets of 26 letter keys in the keyboard */
static const size_t keyb_offset_x[] = {
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

static const size_t keyb_offset_y[] = {
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

/* stores the color mask of 26 characters in the alphabet to color the keyboard
 * requires at least 26 * 2 = 52 bits 
 */
uint64_t keyb_color_mask;

void keyb_mask_set_color(uint64_t *mask, int pos, int color) {
    int color_inv = color ^ 3;
    *mask |= ((uint64_t)3 << (pos << 1));
    *mask ^= ((uint64_t)color_inv << (pos << 1));
}

int keyb_mask_get_color(uint64_t mask, int pos) {
    return (int)((mask >> (pos << 1)) & (uint64_t)3);
}

void keyb_place(int x, int y, char ch, int fore, int back) {
    tile_draw(x, y, x + 2, y + 2, fore, back);
    console_write_wchar(x + 1, y + 1, ch, fore, back);
    console_set_cursor_pos(x, y);
}

/* draws an onscreen keyboard starting at (x, y) */
void keyb_draw(int x, int y) {
    for (int i = 0; i < 26; i++) {
        keyb_place(x + keyb_offset_x[i], y + keyb_offset_y[i], i + 'A', F_WHITE, B_BLACK);
    }
}

/* recolors a tile in the onscreen keyboard */
void keyb_recolor_tile(int x, int y, char ch, int color) {
    int i = ch - 'A';

    switch (keyb_mask_get_color(keyb_color_mask, i)) {
        int mask;
    case MSK_GREEN:
        break;
    case MSK_YELLOW:
        if (color == B_GREEN) {
            keyb_place(x + keyb_offset_x[i], y + keyb_offset_y[i], ch, F_WHITE, B_GREEN);
            keyb_mask_set_color(&keyb_color_mask, i, MSK_GREEN);
        }
        break;
    default:
        mask = (color == B_GREEN ? MSK_GREEN
            : (color == B_YELLOW ? MSK_YELLOW 
            : MSK_GREY));
        keyb_place(x + keyb_offset_x[i], y + keyb_offset_y[i], ch, F_WHITE, color);
        keyb_mask_set_color(&keyb_color_mask, i, mask);
    }
}

void gameplay(const char answer[5], poolmap *pool) {
    COORD loc = console_get_cursor_pos();

    int grid_start_row = loc.Y;
    int keyb_start_row = loc.Y + 6 * TILE_H + 1;
    grid_draw(0, grid_start_row);
    keyb_draw(0, keyb_start_row);

    uint16_t mask = 0;
    char guess[6] = {[5] = '\0'};
    int x = loc.X;
    int y = loc.Y;
    int i;

    for (i = 1; i <= 6; i++) {
        int pos = 0;

        for (;;) {
            if (kbhit()) {
                int ch = getch();

                if (ch == KB_ENTER && pos == 5 && word_is_in_pool(guess, pool)) {
                    break;
                } else if (ch == KB_BACKS && pos > 0) {
                    grid_place(x - TILE_W - 1, y, 0, F_WHITE, B_BLACK);
                    x -= TILE_W + 1; 
                    guess[--pos] = '\0';
                } else if (((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) && pos < 5) {
                    grid_place(x, y, uppercase(ch), F_WHITE, B_BLACK);
                    x += TILE_W + 1;
                    guess[pos++] = lowercase((char)ch);
                }
            }
        }

        /* moves the cursor to the first tile of the row */
        x = 0;
        mask = word_get_mask(guess, answer);

        int tile_color;
        for (int j = 0; j < 5; j++) {
            switch (word_mask_get_color(mask, j)) {
            case MSK_GREEN:
                tile_color = B_GREEN; break;
            case MSK_YELLOW:
                tile_color = B_YELLOW; break;
            default:
                tile_color = B_GREY; 
            }
            grid_place(x, y, uppercase(guess[j]), F_WHITE, tile_color);
            keyb_recolor_tile(0, keyb_start_row, uppercase(guess[j]), tile_color);
            x += TILE_W + 1;
        }

        if (mask == MSK_WIN) {
            break;
        }

        x = 0;
        y += TILE_H;
    }

    /* game is over, moves the cursor to the end of the playing area */
    int message_row = keyb_start_row + 3 * KEY_H + 1;
    console_set_cursor_pos(0, message_row);
    if (mask == MSK_WIN) {
        printf("Solved after %d guess(es)\n", i);
    } else {
        printf("The answer is %s\n", answer);
    }

    printf("Press any key to exit");
    getch();
    fflush(stdout);
}

void import_words(const char *file_name, poolmap *pool) {
    FILE *f = fopen(file_name, "r");

    char token[7] = {[6] = '\0'};
    while (fgets(token, 7, f)) {
        token[5] = '\0';
        map_set(pool, token, 0);
    }

    fclose(f);
}

int main(void) {
    console_get_init_info();
    console_hide_cursor();

    poolmap pool;
    map_init(&pool);

    import_words("answers.txt", &pool);
    const char *answer = map_rand(&pool)->key;
    import_words("words.txt", &pool);

    gameplay(answer, &pool);

    map_clear(&pool);
    console_show_cursor();

    return 0;
}