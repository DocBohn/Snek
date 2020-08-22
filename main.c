#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int row;
    int col;
} ordered_pair;

void initialize();
void finalize();
void run();

ordered_pair get_move( int input );
bool check_for_collision();

int original_cursor_state;
const char game_over_message[] = "Game Over!";
const size_t game_over_message_length = strlen(game_over_message);


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
    ordered_pair delta = {.row = 0, .col = 1};  // initially move to the right
    int col = COLS / 2;
    int row = LINES / 2;
    box(stdscr, '|', '-');
    crmode();
    noecho();
    timeout(500);
    int input = 0;
    while (input != 'q') {
        move(row, col);

        bool collision = check_for_collision();
        addch('*');
        if (collision) {
            move(LINES/2, COLS/2 - (int)game_over_message_length / 2);
            addstr(game_over_message);
        }
        refresh();

        if (!collision) {
            /* if player enters a direction, use it; otherwise, use previous direction after timeout */
            if ((input = getch()) != ERR) {
                delta = get_move(input);
            }
            move(row, col);
            addch(' ');
            row += delta.row;
            col += delta.col;
        } else {
            napms(1500);
            input = 'q';
        }
    }
}

bool check_for_collision() {
    chtype existing_char = inch();
    if ((existing_char == '|') || (existing_char == '-')) {
        return TRUE;
    } else {
        return FALSE;
    }
}

ordered_pair get_move( int input ) {
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