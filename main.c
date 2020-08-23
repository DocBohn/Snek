#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INITIAL_TIMEOUT 500
#define TIMEOUT_DECREMENT 5
#define MESSAGE_DELAY 1500
#define INITIAL_SNAKE_LENGTH 2

typedef struct {
    int row;
    int col;
} ordered_pair;

struct segment {
    ordered_pair position;
    chtype marker;
    struct segment *cranial;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
    struct segment *caudad;
#pragma clang diagnostic pop
};

void initialize();
void finalize();
void finalize_on_error( int sig );
void run();

void convert_message_to_apples( int row, int col, const char *message, int message_length );
ordered_pair get_move( int input );
bool check_for_collision( int row, int col );
void eat_apple( int row, int col );
struct segment *create_new_head( int row, int col, struct segment *old_head, chtype head_marker, chtype tail_marker );
struct segment *destroy_old_tail( struct segment *old_tail, chtype replacement_marker );

int original_cursor_state;
const char game_start_message[] = "Ready Player One";
const size_t game_start_message_length = strlen(game_start_message);
const char game_over_message[] = "Game Over!";
const size_t game_over_message_length = strlen(game_over_message);

chtype head_marker = '>';
const chtype tail_marker = '*';
int growth = INITIAL_SNAKE_LENGTH - 1;
int pause = INITIAL_TIMEOUT;
const chtype apple_marker = 'O';
int apples = 0;
int apples_eaten = 0;


int main() {
    initialize();
    run();
    finalize();
}

void initialize() {
    srandom((unsigned int)time(0));
    initscr();
    original_cursor_state = curs_set(0);
    box(stdscr, '|', '-');
    crmode();
    noecho();
    signal(SIGINT, finalize);
    signal(SIGABRT, finalize_on_error);
    signal(SIGSEGV, finalize_on_error);
}

void finalize() {
    signal(SIGINT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    curs_set(original_cursor_state);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    exit(0);
}

void finalize_on_error( int sig ) {
    signal(SIGINT, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    char *error_message;
    switch (sig) {
        case SIGABRT:
            error_message = "Game terminated due to abort signal.";
            break;
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
    convert_message_to_apples(row, col + 1, game_start_message, (int)game_start_message_length);
    refresh();

    timeout(pause);
    int input = 0;
    napms(pause);

    while (input != 'q') {
        bool collision = check_for_collision(row, col);
        eat_apple(row, col);
        head = create_new_head(row, col, head, head_marker, tail_marker);
        if (growth) {
            growth--;
        } else {
            chtype replacement_marker = ' ';
            /* snek cannot bite the tip of a non-growing tail */
            if (collision && (row == tail->position.row) && (col == tail->position.col)) {
                collision = FALSE;
                replacement_marker = head_marker;
            }
            tail = destroy_old_tail(tail, replacement_marker);
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
    return new_head;
}

struct segment *destroy_old_tail( struct segment *old_tail, chtype replacement_marker ) {
    mvaddch(old_tail->position.row, old_tail->position.col, replacement_marker);
    struct segment *new_tail = old_tail->cranial;
    free(old_tail);
    return new_tail;
}

bool check_for_collision( int row, int col ) {
    if ((row == 0 || row == LINES - 1)              // collide with boundary
        || (col == 0 || col == COLS - 1)            // collide with boundary
        || (mvinch(row, col) == tail_marker)) {     // collide with tail
        return TRUE;
    } else {
        return FALSE;
    }
}

void eat_apple( int row, int col ) {
    chtype existing_char = mvinch(row, col);
    if (existing_char == apple_marker) {
        mvprintw(0, 5, "Apples eaten: %d", ++apples_eaten);
        growth++;
        apples--;
        /* non-linear acceleration of player's reaction time */
        if (pause == 0) {
            /* superhuman skillz! */
            // pause = pause;
        } else if (pause <= TIMEOUT_DECREMENT) {
            pause -= 1;
        } else if (pause <= INITIAL_TIMEOUT / 8) {
            pause -= TIMEOUT_DECREMENT / 4;
        } else if (pause <= INITIAL_TIMEOUT / 4) {
            pause -= TIMEOUT_DECREMENT / 2;
        } else {
            pause -= TIMEOUT_DECREMENT;
        }
        timeout(pause);
    }
    /* if there are no apples on the screen, randomly place an apple */
    while (!apples) {
        /* strictly within box, not on boundary */
        long candidate_row = 1 + random() % (LINES - 1);
        long candidate_col = 1 + random() % (COLS - 1);
        if (mvinch(candidate_row, candidate_col) == ' ') {
            addch(apple_marker);
            mvprintw(0, COLS / 2, "Next apple: (%d,%d)--", candidate_row, candidate_col);
            apples++;
        }
    }
}

void convert_message_to_apples( int row, int col, const char *message, int message_length ) {
    for (int i = 0; i < message_length; i++) {
        if (message[i] != ' ') {
            mvaddch(row, col + i, apple_marker);
            apples++;
        }
    }
}


ordered_pair get_move( int input ) {
    ordered_pair result = {.row = 0, .col = 0};
    switch (input) {
        case 'w':
            if (head_marker != 'v') {
                result.row = -1;
                head_marker = '^';
            } else {
                result.row = 1;
                printf("\a");
            }
            break;
        case 'a':
            if (head_marker != '>') {
                result.col = -1;
                head_marker = '<';
            } else {
                result.col = 1;
                printf("\a");
            }
            break;
        case 's':
            if (head_marker != '^') {
                result.row = 1;
                head_marker = 'v';
            } else {
                result.row = -1;
                printf("\a");
            }
            break;
        case 'd':
            if (head_marker != '<') {
                result.col = 1;
                head_marker = '>';
            } else {
                result.col = -1;
                printf("\a");
            }
            break;
        case 'q':
            break;
        default:
            switch(head_marker) {
                case '^':
                    result.row = -1;
                    break;
                case '<':
                    result.col = -1;
                    break;
                case 'v':
                    result.row = 1;
                    break;
                case '>':
                    result.col = 1;
                    break;
                default:
                    printf("\a\a\a");
            }
            printf("\a");
    }
    return result;
}