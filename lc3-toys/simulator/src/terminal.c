#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <conio.h>

#elif defined(__linux__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>

#endif

#include "common.h"

#if defined(_WIN32) || defined(__CYGWIN__)
HANDLE input_handle;
DWORD fdw_mode, fdw_original_mode;

#elif defined(__linux__)
struct termios original_tio;

#endif

// saves the original input mode and
// disables line-by-line input and shows input in terminal
void disable_input_buffering(void) {
    atexit(restore_input_buffering);

#if defined(_WIN32) || defined(__CYGWIN__)
    input_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(input_handle, &fdw_original_mode); // save original mode

    fdw_mode = fdw_original_mode ^
               ENABLE_ECHO_INPUT ^  // no input echo
               ENABLE_LINE_INPUT;   // return when one or more characters are available

    SetConsoleMode(input_handle, fdw_mode); // set new mode
    FlushConsoleInputBuffer(input_handle); // clear buffer

#elif defined(__linux__)
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;

    new_tio.c_lflag &= ~(ICANON |   // turn off canonical mode & read byte by byte
                         ECHO);     // no input echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

#endif
}

// restores the original input mode, used after the program exits
void restore_input_buffering(void) {
#if defined(_WIN32) || defined(__CYGWIN__)
    SetConsoleMode(input_handle, fdw_original_mode);

#elif defined(__linux__)
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);

#endif
}

// checks if a key is pressed
uint16_t check_key(void) {
#if defined(_WIN32) || defined(__CYGWIN__)
    return WaitForSingleObject(input_handle, 1000) == WAIT_OBJECT_0 && _kbhit();

#elif defined(__linux__)
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(1, &readfds, NULL, NULL, &timeout) != 0;

#endif
}

void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
}