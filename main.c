#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_TIMEOUT 500
#define MESSAGE_DELAY 1500
#define INITIAL_SNAKE_LENGTH 6

typedef struct {
    int row;
    int col;
} ordered_pair;

struct segment {
    ordered_pair position;
    chtype marker;
    struct segment *cranial;
    struct segment *caudad;
};

void initialize();
void finalize();
void run();
void report_memory_problem( int sig );

ordered_pair get_move( int input );
bool check_for_collision( int row, int col );
struct segment *create_new_head( int row, int col, struct segment *old_head, chtype head_marker, chtype tail_marker );
struct segment *destroy_old_tail( struct segment *old_tail );

int original_cursor_state;
const char game_start_message[] = "Ready Player One";
const size_t game_start_message_length = strlen(game_start_message);
const char game_over_message[] = "Game Over!";
const size_t game_over_message_length = strlen(game_over_message);

chtype head_marker = '>';
const chtype tail_marker = '*';
int growth = INITIAL_SNAKE_LENGTH - 1;


int main() {
    initialize();
    run();
    finalize();
}

void initialize() {
    initscr();
    original_cursor_state = curs_set(0);
    box(stdscr, '|', '-');
    crmode();
    noecho();
    signal(SIGINT, finalize);
    signal(SIGSEGV, report_memory_problem);
}

void finalize() {
    signal(SIGINT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    curs_set(original_cursor_state);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    exit(0);
}

void report_memory_problem( int sig ) {
    signal(SIGINT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    char *error_message;
    switch (sig) {
        case SIGSEGV:
            error_message = "Game terminated due to segmentation fault.";
            break;
        default:
            error_message = "Game terminated due to unknown memory error.";
    }
    mvaddstr(LINES / 2 + 2, COLS / 2 - (int)strlen(error_message) / 2, error_message);
    refresh();
    napms(MESSAGE_DELAY);
    curs_set(original_cursor_state);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    printf("%s\n", error_message);
    exit(sig);
}

void run() {
    int col = COLS / 2 - (int)game_start_message_length / 2;
    int row = LINES / 2;
    mvaddstr(row, col, game_start_message);
    refresh();
    napms(MESSAGE_DELAY);

    col -= 2;
    struct segment *head = create_new_head(row, col++, NULL, head_marker, tail_marker);
    struct segment *tail = head;

    ordered_pair delta = {.row = 0, .col = 1};  // initially move to the right
    timeout(INITIAL_TIMEOUT);
    refresh();
    int input = 0;
    napms(INITIAL_TIMEOUT);

    while (input != 'q') {
        bool collision = check_for_collision(row, col);
        head = create_new_head(row, col, head, head_marker, tail_marker);
        if (growth) {
            growth--;
        } else {
            /* snek cannot bite the tip of a non-growing tail */
            if (collision && (row == tail->position.row) && (col == tail->position.col)) {
                collision = FALSE;
            }
            tail = destroy_old_tail(tail);
        }
        if (collision) {
            int message_row = row == LINES / 2 ? row + 2 : LINES / 2;
            mvaddstr(message_row, COLS / 2 - (int)game_over_message_length / 2, game_over_message);
            refresh();
            napms(MESSAGE_DELAY);
            input = 'q';
        } else {
            refresh();
            /* if player enters a direction, use it; otherwise, use previous direction after timeout */
            if ((input = getch()) != ERR) {
                delta = get_move(input);
            }
            row += delta.row;
            col += delta.col;
        }
    }
}



struct segment *create_new_head( int row, int col, struct segment *old_head, chtype head_marker, chtype tail_marker ) {
    struct segment *new_head = (struct segment *)malloc(sizeof(struct segment));
    new_head->position.row = row;
    new_head->position.col = col;
    new_head->caudad = old_head;
    new_head->cranial = NULL;
    new_head->marker = head_marker;
    mvaddch(new_head->position.row, new_head->position.col, new_head->marker);
    if (old_head != NULL) {
        old_head->cranial = new_head;
        old_head->marker = tail_marker;
        mvaddch(old_head->position.row, old_head->position.col, old_head->marker);
    }
//    mvprintw(1 + head_count, 1, "%p", old_head);
//    mvprintw(1 + head_count++, 21, "%p", new_head);
//    refresh();
    return new_head;
}

struct segment *destroy_old_tail( struct segment *old_tail ) {
//    mvprintw(1 + tail_count++, 41, "%p", old_tail);
//    refresh();
    mvaddch(old_tail->position.row, old_tail->position.col, ' ');
    struct segment *new_tail = old_tail->cranial;
    free(old_tail);
    return new_tail;
}

bool check_for_collision( int row, int col ) {
    chtype existing_char = mvinch(row, col);
    if ((existing_char == '|') || (existing_char == '-') || (existing_char == tail_marker)) {
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
            head_marker = '^';
            break;
        case 'a':
            result.col = -1;
            head_marker = '<';
            break;
        case 's':
            result.row = 1;
            head_marker = 'v';
            break;
        case 'd':
            result.col = 1;
            head_marker = '>';
            break;
        case 'q':
            break;
        default:
            printf("\a");
    }
    return result;
}