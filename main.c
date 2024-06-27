#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INITIAL_TIMEOUT 500
#define TIMEOUT_DECREMENT 5
#define MESSAGE_DELAY 1500
#define INITIAL_SNAKE_LENGTH 2

#define HEAD_UP '^'
#define HEAD_LEFT '<'
#define HEAD_DOWN 'v'
#define HEAD_RIGHT '>'
#define TAIL_VERTICAL '|'
#define TAIL_HORIZONTAL '-'
#define TAIL_DIAGONAL_LEFT_UP '/'
#define TAIL_DIAGONAL_RIGHT_UP '\\'

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
void finalize_on_error(int sig);
void run();

void convert_message_to_apples(int row, int col, const char *message, int message_length);
ordered_pair get_move(int input);
bool check_for_collision(int row, int col);
void eat_apple(int row, int col);
struct segment *create_new_head(int row, int col, struct segment *old_head);
struct segment *destroy_old_tail(struct segment *old_tail, chtype replacement_marker);

int original_cursor_state;
const char game_start_message[] = "Ready Player One";
const size_t game_start_message_length = 16;    /* strlen(game_start_message) */    // clang15 doesn't pre-determine strlen(...) as a compile-time constant
const char game_over_message[] = "Game Over!";
const size_t game_over_message_length = 10;     /* strlen(game_over_message) */

enum {
    UP, DOWN, LEFT, RIGHT
} direction = RIGHT;
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
    srand((unsigned int) time(0));
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

void finalize_on_error(int sig) {
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
    mvaddstr(LINES / 2 + 2, COLS / 2 - (int) strlen(error_message) / 2, error_message);
    refresh();
    napms(MESSAGE_DELAY);
    curs_set(original_cursor_state);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    printf("%s\n", error_message);
    exit(sig);
}


void run() {
    int col = COLS / 2 - (int) game_start_message_length / 2;
    int row = LINES / 2;
    mvaddstr(row, col, game_start_message);
    refresh();
    napms(MESSAGE_DELAY);

    col -= 2;
    struct segment *head = create_new_head(row, col++, NULL);
    struct segment *tail = head;
    ordered_pair delta = {.row = 0, .col = 1};  // initially move to the right
    convert_message_to_apples(row, col + 1, game_start_message, (int) game_start_message_length);
    refresh();

    timeout(pause);
    int input = 0;
    napms(pause);

    while (input != 'q') {
        bool collision = check_for_collision(row, col);
        eat_apple(row, col);
        head = create_new_head(row, col, head);
        if (growth) {
            growth--;
        } else {
            chtype replacement_marker = ' ';
            /* snek cannot bite the tip of a non-growing tail */
            if (collision && (row == tail->position.row) && (col == tail->position.col)) {
                collision = FALSE;
                replacement_marker = head->marker;
            }
            tail = destroy_old_tail(tail, replacement_marker);
        }
        if (collision) {
            int message_row = row == LINES / 2 ? row + 2 : LINES / 2;
            mvaddstr(message_row, COLS / 2 - (int) game_over_message_length / 2, game_over_message);
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


struct segment *create_new_head(int row, int col, struct segment *old_head) {
    struct segment *new_head = (struct segment *) malloc(sizeof(struct segment));
    new_head->position.row = row;
    new_head->position.col = col;
    new_head->caudad = old_head;
    new_head->cranial = NULL;
    if (old_head == NULL) {
        new_head->marker = HEAD_RIGHT;
    } else if (row - old_head->position.row < 0) {
        new_head->marker = HEAD_UP;
    } else if (row - old_head->position.row > 0) {
        new_head->marker = HEAD_DOWN;
    } else if (col - old_head->position.col < 0) {
        new_head->marker = HEAD_LEFT;
    } else if (col - old_head->position.col > 0) {
        new_head->marker = HEAD_RIGHT;
    } else {
        new_head->marker = '?';
    }
    mvaddch(new_head->position.row, new_head->position.col, new_head->marker);
    if (old_head != NULL) {
        old_head->cranial = new_head;
        if (old_head->caudad == NULL) {
            old_head->marker = TAIL_HORIZONTAL;
        } else if (row - old_head->caudad->position.row == 0) {
            old_head->marker = TAIL_HORIZONTAL;
        } else if (col - old_head->caudad->position.col == 0) {
            old_head->marker = TAIL_VERTICAL;
        } else {
            int orientation = (row - old_head->caudad->position.row) * (col - old_head->caudad->position.col);
            if (orientation > 0) {
                old_head->marker = TAIL_DIAGONAL_RIGHT_UP;
            } else {
                old_head->marker = TAIL_DIAGONAL_LEFT_UP;
            }
        }
        mvaddch(old_head->position.row, old_head->position.col, old_head->marker);
    }
    return new_head;
}

struct segment *destroy_old_tail(struct segment *old_tail, chtype replacement_marker) {
    mvaddch(old_tail->position.row, old_tail->position.col, replacement_marker);
    struct segment *new_tail = old_tail->cranial;
    free(old_tail);
    return new_tail;
}

bool check_for_collision(int row, int col) {
    if ((row == 0 || row == LINES - 1)                          // collide with boundary
        || (col == 0 || col == COLS - 1)                        // collide with boundary
        || (mvinch(row, col) == TAIL_VERTICAL)                  // collide with tail
        || (mvinch(row, col) == TAIL_HORIZONTAL)                // collide with tail
        || (mvinch(row, col) == TAIL_DIAGONAL_LEFT_UP)          // collide with tail
        || (mvinch(row, col) == TAIL_DIAGONAL_RIGHT_UP)) {      // collide with tail
        return TRUE;
    } else {
        return FALSE;
    }
}

void eat_apple(int row, int col) {
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
        long candidate_row = 1 + rand() % (LINES - 1);
        long candidate_col = 1 + rand() % (COLS - 1);
        if (mvinch(candidate_row, candidate_col) == ' ') {
            addch(apple_marker);
            mvprintw(0, COLS / 2, "Next apple: (%d,%d)--", candidate_row, candidate_col);
            apples++;
        }
    }
}

void convert_message_to_apples(int row, int col, const char *message, int message_length) {
    for (int i = 0; i < message_length; i++) {
        if (message[i] != ' ') {
            mvaddch(row, col + i, apple_marker);
            apples++;
        }
    }
}


ordered_pair get_move(int input) {
    ordered_pair result = {.row = 0, .col = 0};
    switch (input) {
        case 'w':
            if (direction != DOWN) {
                result.row = -1;
                direction = UP;
            } else {
                result.row = 1;
                printf("\a");
            }
            break;
        case 'a':
            if (direction != RIGHT) {
                result.col = -1;
                direction = LEFT;
            } else {
                result.col = 1;
                printf("\a");
            }
            break;
        case 's':
            if (direction != UP) {
                result.row = 1;
                direction = DOWN;
            } else {
                result.row = -1;
                printf("\a");
            }
            break;
        case 'd':
            if (direction != LEFT) {
                result.col = 1;
                direction = RIGHT;
            } else {
                result.col = -1;
                printf("\a");
            }
            break;
        case 'q':
            break;
        default:
            switch (direction) {
                case UP:
                    result.row = -1;
                    break;
                case LEFT:
                    result.col = -1;
                    break;
                case DOWN:
                    result.row = 1;
                    break;
                case RIGHT:
                    result.col = 1;
                    break;
                default:
                    printf("\a\a\a");
            }
            printf("\a");
    }
    return result;
}