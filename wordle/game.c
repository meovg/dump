// a wordle clone with basic interface run on windows terminal 

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include <stdarg.h>
#include <windows.h>

#include "dict.h"

dict_of(char) pool;

// special keyboard code (enter, backspace) 
#define KB_ENTER    13
#define KB_BACKS    8

// wchar_t typed values for tile borders 
#define W_HOR       196
#define W_VER       179
#define W_TOPL      218
#define W_BOTL      192
#define W_TOPR      191
#define W_BOTR      217

// console screen buffer text attributes (color)
#define B_BLACK     0
#define B_GREEN     37
#define B_YELLOW    110
#define B_GREY      128
#define F_WHITE     15

// mask defines the color to be placed on the tiles as hints
#define MSK_GREY    0       // 00
#define MSK_YELLOW  1       // 01
#define MSK_GREEN   2       // 10
#define MSK_WIN     682     // 1010101010

// letter tile sizes 
#define TILE_H      3
#define TILE_W      5

// on screen keyboard offsets
#define KEY_W       3
#define KEY_H       3
#define KEY_TOP_X   0
#define KEY_MID_X   2
#define KEY_BOT_X   4
#define KEY_TOP_Y   0
#define KEY_MID_Y   3
#define KEY_BOT_Y   6

// get keypress and return the character 
char from_keyboard(void) {
    while (!kbhit());
    return (char)getch();
}

// getting uppercase and lowercase of characters
char uppercase(char c) {
    return (c >= 97 && c <= 122 ? c - 32 : c);
}

char lowercase(char c) {
    return (c >= 65 && c <= 90 ? c + 32 : c);
}

// a sequence of bits are used to mask the color of each letter of guessed word
// color mask of a letter takes 2 bits
// using macros instead of functions as mask can be of any integer type

// setting a color mask at a position (unchecked)
#define mask_set(maskptr, pos, color) \
    __typeof__(*(maskptr)) color_inv = (color) ^ 3; \
    *(maskptr) |= ((__typeof__(*(maskptr)))3 << ((pos) << 1)); \
    *(maskptr) ^= (color_inv << ((pos) << 1))

// getting a color mask at a position (unchecked)
#define mask_get(mask, pos) \
    (int)(((mask) >> ((pos) << 1)) & (__typeof__(mask))(3))

// check whether guessed word is in the word pool 
int is_valid_guess(const char guess[5]) {
    return dict_get(&pool, guess) != NULL;
}

// get color mask based on how the guessed word matches the answer
uint16_t check_guess(const char guess[5], const char answer[5]) {
    uint16_t color_mask = 0;

    // used to mark "colored" positions in guessed words
    int visited_mask = 0;

    // start with checking the right letter in the right place (colored green) 
    for (int i = 0; i < 5; i++) {
        if (answer[i] == guess[i]) {
            mask_set(&color_mask, i, MSK_GREEN);
            visited_mask |= 1 << i;
        }
    }

    // then check the right letter in the wrong place (colored yellow)
    for (int i = 0; i < 5; i++) {
        if ((visited_mask >> i) & 1) {
            continue;
        }
        for (int j = 0; j < 5; j++) {
            int tmp = mask_get(color_mask, j);
            if (tmp == MSK_GREY && answer[i] == guess[j]) {
                mask_set(&color_mask, j, MSK_YELLOW);
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

// get initial console cursor & screen buffer info
void screen_get_original_state(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(h, &scrbuf_info);
    init_attrs = scrbuf_info.wAttributes;

    GetConsoleCursorInfo(h, &cursor_info);
}

// set the back/foreground color of output screen buffer on console 
void screen_set_color(int fore, int back) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fore | back);
}

void screen_reset_color(void) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), init_attrs);
}

// hide / show the cursor (actually return it to the state before the program starts)
void cursor_hide(void) {
    CONSOLE_CURSOR_INFO i;
    i.dwSize = 100;
    i.bVisible = FALSE;

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
}

void cursor_show(void) {
    CONSOLE_CURSOR_INFO i;
    i.dwSize = cursor_info.dwSize;
    i.bVisible = TRUE;

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &i);
}

// move the cursor to some location in the console screen
void cursor_move(int x, int y) {
    COORD loc = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
}

// write a wchar_t character to the console with color
void screen_write_char(int x, int y, wchar_t c) {
    cursor_move(x, y);
    // screen_set_color(fore, back); // maybe unnecessary -> DIY
    printf("%lc", c);
    // screen_reset_color(); // maybe unnecessary -> DIY
}

// draw a square tile with
// (xa, ya), (xb, yb): the coordinates of the top left and bottom right corners
void tile_draw(int xa, int ya, int xb, int yb) {
    for (int i = xa + 1; i < xb; i++) {
        for (int j = ya + 1; j < yb; j++) {
            screen_write_char(i, j, 0);
        }
    }

    for (int i = xa + 1; i < xb; i++) {
        screen_write_char(i, ya, W_HOR);
        screen_write_char(i, yb, W_HOR);
    }

    for (int i = ya + 1; i < yb; i++) {
        screen_write_char(xa, i, W_VER);
        screen_write_char(xb, i, W_VER);
    }

    screen_write_char(xa, ya, W_TOPL);
    screen_write_char(xa, yb, W_BOTL);
    screen_write_char(xb, ya, W_TOPR);
    screen_write_char(xb, yb, W_BOTR);
}

int grid_start_x, grid_start_y;

// place the tile with letter c at (x, y) in screen color (fore, back)
void grid_place(int x, int y, char c, int fore, int back) {
    screen_set_color(fore, back);
    tile_draw(x, y, x + TILE_W - 1, y + TILE_H - 1);

    // write the character at the center, so it should look like this
    // ┌───┐
    // │ A │
    // └───┘
    screen_write_char(x + 2, y + 1, c);
    screen_reset_color();
}

// draw the game playing area - a 6x5 board of black tiles 
void grid_draw(void) {
    int x = grid_start_x;
    int y = grid_start_y;

    screen_set_color(F_WHITE, B_BLACK);

    for (int i = 0; i < 6; i++) {
        x = grid_start_x;

        for (int j = 0; j < 5; j++) {
            tile_draw(x, y, x + TILE_W - 1, y + TILE_H - 1);
            x += TILE_W + 1;
        }

        y += TILE_H;
    }

    screen_reset_color();
}

// location offsets of 26 letter keys in the keyboard 
static const size_t keyboard_x[] = {
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

static const size_t keyboard_y[] = {
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

// require 52 bits to store the color mask of 26 keys, so 64-bit then
uint64_t keyboard_mask;

int keyboard_start_x, keyboard_start_y;

void keyboard_place(int x, int y, char c, int fore, int back) {
    screen_set_color(fore, back);

    // write the character at the center, so it should look like this
    // ┌─┐
    // │A│
    // └─┘
    tile_draw(x, y, x + KEY_W - 1, y + KEY_H - 1);
    screen_write_char(x + 1, y + 1, c);

    screen_reset_color();
}

// draw an onscreen keyboard starting at (x, y) 
void keyboard_draw(void) {
    for (int i = 0; i < 26; i++) {
        keyboard_place(keyboard_start_x + keyboard_x[i], keyboard_start_y + keyboard_y[i],
                       i + 'A', F_WHITE, B_BLACK);
    }
}

// recolor a tile (key) in the onscreen keyboard 
void keyboard_color_tile(char c, int color) {
    int pos = c - 'A';

    switch (mask_get(keyboard_mask, pos)) {
    case MSK_GREEN: break;
    case MSK_YELLOW:
        if (color == B_GREEN) {
            keyboard_place(keyboard_start_x + keyboard_x[pos],
                           keyboard_start_y + keyboard_y[pos], c, F_WHITE, color);
            mask_set(&keyboard_mask, pos, MSK_GREEN);
        }
        break;
    default: {
        int color_mask =
            (color == B_GREEN ? MSK_GREEN : (color == B_YELLOW ? MSK_YELLOW : MSK_GREY));
        keyboard_place(keyboard_start_x + keyboard_x[pos],
                       keyboard_start_y + keyboard_y[pos], c, F_WHITE, color);
        mask_set(&keyboard_mask, pos, color_mask);
        }
    }
}

int textbox_start_x, textbox_start_y, textbox_ed_x, textbox_ed_y;
char message[50];

int textbox_line_limit = KEY_W * 10 - 1;

// draw text box at the bottom right of game area 
void textbox_draw(void) {
    textbox_ed_x = textbox_start_x + KEY_W * 10;
    textbox_ed_y = textbox_start_y + 4;

    screen_set_color(F_WHITE, B_BLACK);
    tile_draw(textbox_start_x, textbox_start_y, textbox_ed_x, textbox_ed_y);
    screen_reset_color();
}

// function like strrchr for substrings 
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

// find the end of the line (usually right before the last space or newline
// so that it fits the textbox 
int textbox_line_end(const char *str) {
    char *pspace = strrchr_(str, textbox_line_limit, ' ');
    char *pnewln = strrchr_(str, textbox_line_limit, '\n');

    if (strlen(str) <= textbox_line_limit || (pspace == NULL && pnewln == NULL)) {
        return textbox_line_limit;
    } else if (pnewln == NULL) {
        return pspace - str;
    } else {
        return pnewln - str;
    }
}

// show the text in the textbox
void textbox_show(const char *format, ...) {
    memset(message, 0, 50);

    // store the formatted text into a buffer 
    va_list args;
    va_start(args, format);
    vsnprintf(message, 50, format, args);
    va_end(args);

    // separate text into lines and print them in the textbox
    screen_set_color(F_WHITE, B_BLACK);
    cursor_move(textbox_start_x + 1, textbox_start_y + 1);

    char *line = message;
    int endpos = textbox_line_end(line);
    printf("%.*s", endpos, line);

    cursor_move(textbox_start_x + 1, textbox_start_y + 2);

    line += endpos + 1;
    endpos = textbox_line_end(line);
    printf("%.*s", endpos, line);

    screen_reset_color();
}

// clear the textbox
void textbox_clean(void) {
    memset(message, ' ', 50);

    screen_set_color(F_WHITE, B_BLACK);
    cursor_move(textbox_start_x + 1, textbox_start_y + 1);
    printf("%.*s", textbox_line_limit, message);

    cursor_move(textbox_start_x + 1, textbox_start_y + 2);
    printf("%.*s", textbox_line_limit, message);

    screen_reset_color();
}

void cleanup(void);

void init(void) {
    // remember to call for cleanup before safe exit
    atexit(cleanup);

    screen_get_original_state();
    cursor_hide();

    // get the initial cursor location
    COORD loc = scrbuf_info.dwCursorPosition;

    // draw the grid
    grid_start_x = loc.X;
    grid_start_y = loc.Y;
    grid_draw();

    // then the keyboard
    keyboard_start_x = loc.X + 6 * TILE_W + 1;
    keyboard_start_y = loc.Y;
    keyboard_draw();

    // and then the textbox
    textbox_start_x = keyboard_start_x;
    textbox_start_y = loc.Y + 3 * KEY_W + 1;
    textbox_draw();

    dict_init(&pool);
}

void game(const char answer[5]) {
    uint16_t mask = 0;
    char guess[6] = {[5] = '\0'};
    int x = grid_start_x;
    int y = grid_start_y;

    int attempt;

    for (attempt = 1; attempt <= 6; attempt++) {
        int pos = 0;

        for (;;) {
            char ch = lowercase(from_keyboard());
            textbox_clean();

            if (ch == KB_ENTER) {
                // player is now desperate to submit their guess
                if (pos != 5) {
                    textbox_show("Not enough letters");
                } else if (!is_valid_guess(guess)) {
                    textbox_show("Not in word list");
                } else {
                    // congrats, time for evaluation
                    break;
                }

            } else if (ch == KB_BACKS && pos > 0) {
                // backspace key is pressed: remove the letter in the last tile 
                grid_place(x - TILE_W - 1, y, 0, F_WHITE, B_BLACK);
                x -= TILE_W + 1;
                guess[--pos] = '\0';
            } else if (ch >= 'a' && ch <= 'z' && pos < 5) {
                // letter key is pressed: put it on tile
                grid_place(x, y, uppercase(ch), F_WHITE, B_BLACK);
                x += TILE_W + 1;
                guess[pos++] = ch;
            }
        }

        // move the cursor to the first tile of the row
        x = grid_start_x;
        mask = check_guess(guess, answer);

        // color each guessed letters in the grid as well as the keyboard 
        int tile_color;

        for (int j = 0; j < 5; j++) {
            switch (mask_get(mask, j)) {
            case MSK_GREEN:
                tile_color = B_GREEN;
                break;
            case MSK_YELLOW:
                tile_color = B_YELLOW;
                break;
            default:
                tile_color = B_GREY;
            }

            grid_place(x, y, uppercase(guess[j]), F_WHITE, tile_color);
            keyboard_color_tile(uppercase(guess[j]), tile_color);
            x += TILE_W + 1;
        }

        if (mask == MSK_WIN) {
            break;
        }

        // player does not win, move to the next row in grid for the next attempt
        x = grid_start_x;
        y += TILE_H;
    }

    if (mask == MSK_WIN) {
        textbox_show("Solved after %d guess(es). Press any key to exit", attempt);
    } else {
        textbox_show("The answer is %s. Press any key to exit", answer);
    }

    // read the key and exit without doing anything
    from_keyboard();
}

// putting word list into the pool
void import_words(const char *file_name) {
    FILE *f = fopen(file_name, "r");

    if (f == NULL) {
        textbox_show("Failed to load the word list");
        abort();
    }

    char token[7] = {[6] = '\0'};
    while (fgets(token, 7, f)) {
        token[5] = '\0';
        dict_set(&pool, token, 0);
    }

    fclose(f);
}

void cleanup(void) {
    dict_clear(&pool);

    // move the cursor away from the game area
    // return the original state of the cursor as well
    cursor_move(0, grid_start_y + 6 * TILE_H);
    cursor_show();
}

int main(void) {
    init();

    import_words("answers.txt");
    const char *answer = dict_rand(&pool)->key;
    import_words("words.txt");

    game(answer);

    return 0;
}