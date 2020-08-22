#include <curses.h>
#include <signal.h>
#include <stdlib.h>

typedef struct {
    int row;
    int col;
} ordered_pair;

void initialize();
void finalize();
void run();

ordered_pair get_move();


int original_cursor_state;


int main() {
    initialize();
    run();
    finalize();
}

void initialize() {
    initscr();
    original_cursor_state = curs_set(0);
    signal(SIGINT, finalize);
}

void finalize() {
    signal(SIGINT, SIG_IGN);
    curs_set(original_cursor_state);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    exit(0);
}

void run() {
    int col = COLS / 2;
    int row = LINES / 2;
    box(stdscr, '|', '-');
    crmode();
    noecho();
    int input = 0;
    while (input != 'q') {
        move(row, col);
        addch('*');
//        move(0, 0);
        refresh();

        input = getch();
        ordered_pair delta = get_move(input);
        move(row, col);
        addch(' ');
        row += delta.row;
        col += delta.col;
    }
}

ordered_pair get_move(int input) {
    ordered_pair result = {.row = 0, .col = 0};
    switch (input) {
        case 'w':
            result.row = -1;
            break;
        case 'a':
            result.col = -1;
            break;
        case 's':
            result.row = 1;
            break;
        case 'd':
            result.col = 1;
            break;
        case 'q':
            break;
        default:
            printf("\a");
    }
    return result;
}