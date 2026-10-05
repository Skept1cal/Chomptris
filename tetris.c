#include "tetris.h"
#include "highscore.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>



const uint8_t PIECE_SHAPES4[][4][4] = {
    [PIECE_NULL]={
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    [PIECE_I]={
        {0, 0, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 0}
    }
};

const uint8_t PIECE_SHAPES3[][3][3] = {
    [PIECE_NULL]={
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    },
    [PIECE_T]={
        {0, 1, 0},
        {0, 1, 1},
        {0, 1, 0},
    },
    [PIECE_J]={
        {0, 1, 0},
        {0, 1, 0},
        {1, 1, 0},
    },
    [PIECE_L]={
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 1},
    },
    [PIECE_S]={
        {0, 1, 0},
        {0, 1, 1},
        {0, 0, 1},
    },
    [PIECE_Z]={
        {0, 0, 1},
        {0, 1, 1},
        {0, 1, 0},
    }
};

const uint8_t PIECE_SHAPES2[][2][2] = {
    [PIECE_NULL]={
        {0, 0},
        {0, 0}
    },
    [PIECE_O]={
        {1, 1},
        {1, 1}
    }
};

#ifndef _WIN32
    struct termios orgterm;
#endif

const Pos WALL_KICKS[] = {
    (Pos){.x =  1, .y =  0},
    (Pos){.x = -1, .y =  0},
    (Pos){.x =  2, .y =  0},
    (Pos){.x = -2, .y =  0},
    (Pos){.x =  0, .y = -1},
    (Pos){.x =  0, .y = -2}
};


const char* BUTTON_LABELS[] = {
    STRT_LVL_LBL,
    NAME_LBL,

    SORT_BY_LBL,
    SORT_ORDER_LBL,
    TOGGLE_LOG_LBL,
    VIEW_LOG_LBL,
    EXPORT_LOG_LBL,

    CLR_LOG_LBL,

    STRT_LBL,
    HELP_LBL,
    QUIT_LBL
};

const uint8_t MAX_INPUT_LENS[] = {
    [STRT_LVL - 1] = MAX_LVL_INPUT_LEN,
    [NAME     - 1] = MAX_NME_INPUT_LEN
};

void (*BUTTON_FUNCTIONS[])(void*) = {
      [VIEW_LOG - 1] =   viewLogBtnPressed,
    [EXPORT_LOG - 1] = exportLogBtnPressed,
       [CLR_LOG - 1] =  clearLogBtnPressed,
          [STRT - 1] =     startBtnPressed,
          [HELP - 1] =      helpBtnPressed,
          [QUIT - 1] =      quitBtnPressed
};



const char* SB_BTN_CYCLE_LBLS[] = {
    [SB_CYCLE_SCORE]      = SB_BTN_SCR_LBL,
    [SB_CYCLE_LINES]      = SB_BTN_LN_LBL,
    [SB_CYCLE_LEVEL]      = SB_BTN_LVL_LBL,
    [SB_CYCLE_STRT_LVL]   = SB_BTN_STRT_LBL,
    [SB_CYCLE_TETRIS_CNT] = SB_BTN_TETR_LBL
};

const char* TLB_BTN_CYCLE_LBLS[] = {
    [TLB_CYCLE_OFF] = BTN_OFF_LBL,
    [TLB_CYCLE_ON]  = BTN_ON_LBL,
};

const char* SOB_CYCLE_LBLS[] = {
    [SOB_CYCLE_DESCENDING] = SOB_DESCEND_LBL,
    [SOB_CYCLE_ASCENDING]  = SOB_ASCEND_LBL
};



int main(int argc, char** argv) {
    if (argc == 1) {
        Vector* ARENA = createArena();

        Player player;
        initPlayer(&player);

        struct timespec framerate = {
            .tv_sec  = 0,
            .tv_nsec = (int)(1e9/FPS)
        };
        struct timespec animationTimeout = {
            .tv_sec  = 0,
            .tv_nsec = (int)(ANIMATION_TIMEOUT * 1e8)
        };
        #ifdef _WIN32
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            LARGE_INTEGER start;
            LARGE_INTEGER end;
        #else
            clock_t start;
            clock_t end;
        #endif
        double  diff;

        uint8_t c;

        #ifndef _WIN32
            setsigs();
        #else
            setWinHandler();
        #endif
        enableraw();

        goToMenu(&player, ARENA);

        while (true) {
            #ifdef _WIN32
                QueryPerformanceCounter(&start);
            #else
                start = clock();
            #endif

            if (player.quit) {
                quit(&player, ARENA);
                goto quit;
            }

            readInput(&player, ARENA, &c);

            if (player.mainMenuOpen && !player.helpTextOpen) {
                executeSelected(&player);
            }
            if (player.paused || player.helpTextOpen || player.mainMenuOpen) {
                suspend(&framerate);
                continue;
            }

            if (!player.gameOver) {

                if (player.currToBeFrozen) {
                    player.lockDelayFrames++;

                    if (canMoveDown(&player.currPiece, ARENA)) {
                        player.currToBeFrozen = false;
                        player.lockDelayFrames = 0;
                    }
                    if (player.lockDelayFrames >= player.lockDelay) {
                        engrainPiece(&player.currPiece, ARENA);
                        setPieceToNull(&player.currPiece);

                        if (markLines(ARENA)) { // Found lines to clear
                            renderGame(&player, ARENA);
                            suspend(&player.lnTimeout);
                            flushstdin(&c);

                            int clearedLines = clearLines(ARENA);
                            player.score            += lineClearFormula(clearedLines, player.level);
                            player.linesCleared     += clearedLines;
                            player.currLinesCleared += clearedLines;
                            if (clearedLines == 4) player.tetrisCount++;

                            uint32_t levelThreshold = (
                                (player.firstLevel) ? LEVEL_THRESHOLD + player.startLevel * LEVEL_THRESHOLD : LEVEL_THRESHOLD
                            );

                            if (player.currLinesCleared >= levelThreshold) {
                                levelUp(&animationTimeout);
                                player.level++;
                                player.currLinesCleared -= levelThreshold; // Keeps overflowing lines from last level
    
                                player.firstLevel = false;
                                player.score     += levelUpFormula(player.level);
                                player.speed      = speedFormula(player.level);
                                player.lnTimeout  = lnTimeoutFormula(&player, player.level);
                                player.lockDelay  = lockDelayFormula(&player);

                                flushstdin(&c);
                            }
                        }

                        useNextPiece(&player);
                        player.score           += pieceFreezeFormula();
                        player.swappedThisRound = false;
                        player.currToBeFrozen   = false;
                        player.lockDelayFrames  = 0;
                        player.framesPassed     = 0; // Must reset this so the piece always takes 1 whole tick to move from its spawn position

                        if (pieceCollides(&player.currPiece, ARENA)) {
                            player.gameOver = true;
                            gameOver(&player, ARENA, &animationTimeout);
                            flushstdin(&c);
                        }
                    }
                }

                player.speed = speedFormula(player.level);
                player.lockDelay = lockDelayFormula(&player);

                if (!canMoveDown(&player.currPiece, ARENA)) player.currToBeFrozen = true;

                player.framesPassed++;
                if (player.framesPassed >= player.speed) {
                    player.framesPassed = 0;
                    if (!player.gameOver) moveDown(&player.currPiece, ARENA);
                }

                if (!player.gameOver) renderGame(&player, ARENA);
            }

            #ifdef _WIN32
                QueryPerformanceCounter(&end);
                diff = ((double)1e9) * (end.QuadPart - start.QuadPart) / freq.QuadPart;
            #else
                end  = clock();
                diff = ((double)1e9) * (end - start) / CLOCKS_PER_SEC; // We only end up suspending for as long as there is left from the ~16.67ms window.
            #endif

            long tempNsec = framerate.tv_nsec; // We'll need to reset the framerate after suspension to always compensate for the target itself
            framerate.tv_nsec -= (long)diff;

            suspend(&framerate);

            framerate.tv_nsec = tempNsec;
        }

    } else if (argc == 2) {
        int len = strlen(argv[1]); // Don't need to include '\0'
        strToLower(argv[1], len, argv[1], len);

        if (!strmtch(argv[1], "help")) goto invalidarg;
        else {
            printHelpText();
            fputs("\r\n", stdout); // The help text doesn't contain a new-line, so we add it here
            fflush(stdout);
            goto exitsuccess;
        }

    } else goto invalidarg;



    quit:
        clearterm();
        fputs("Exited.\r\n", stdout);
        fflush(stdout);
        goto exitsuccess;

    invalidarg:
        fputs(
            "Invalid argument(s) for game.\r\n\r\nValid argument(s):\r\n"
            "\"help\"\r\n",
            stdout
        );
        fflush(stdout);
        goto exitfailure;

    exitsuccess: return 0;
    exitfailure: return 1;
}



static inline void readInput(Player* player, Vector* ARENA, uint8_t* c) {
    #ifdef _WIN32
        if (!_kbhit()) return;
    #else
        if (read(STDIN_FILENO, c, 1) == 0) return;
    #endif

    #ifdef _WIN32
        *c = _getch();
        if (!player->paused && !player->helpTextOpen && !player->logOpen && *c == 0xe0) {
            *c = _getch();
            switch (*c) {
                case 72:    upPressed(player, ARENA); break;             // H
                case 80:  downPressed(player, ARENA); break;             // P
                case 77: rightPressed(player, ARENA); break;             // M
                case 75:  leftPressed(player, ARENA); break;             // K
            }
            return;
        } else if (*c == 27) {                                           // ESC
            goToMenu(player, ARENA);
            return;
        }
    #else
        if (*c == '\x1b') {
            if (!player->paused && !player->helpTextOpen && !player->logOpen && read(STDIN_FILENO, c, 1) == 1 && read(STDIN_FILENO, c, 1) == 1) {
                switch (*c) {
                    case 65:    upPressed(player, ARENA); break;         // A
                    case 66:  downPressed(player, ARENA); break;         // B
                    case 67: rightPressed(player, ARENA); break;         // C
                    case 68:  leftPressed(player, ARENA); break;         // D
                }
            } else if (read(STDIN_FILENO, c, 1) == 0) {                  // Checks whether ESC was actually pressed and not something beginning with ESC
                goToMenu(player, ARENA);
            }
            // Pressing shift or ctrl AND an arrow key sends bytes which aren't immediately parsed to valid keybinds,
            // and would later map to completely unrelated keybinds due to the sent bytes,
            // so we empty the input if there is any.
            if (read(STDIN_FILENO, c, 1) == 1) flushstdin(c);

            return;
        }
    #endif

    switch (*c) {
        case  1: clearterm(); break;                               // ^A
        case 17: player->quit = true; break;                       // ^Q
        case 18: {                                                 // ^R
            if (!player->mainMenuOpen) {
                reset(player, ARENA);
            }
            break;
        }
        case  8: {                                                 // ^H
            if (!player->gameOver && !player->logOpen) {
                toggleHelp(player, ARENA);
            }
            break;
        }
        case 32: {                                                 // SPACE
            if (!player->helpTextOpen && !player->mainMenuOpen) {
                togglePause(player);
            }
            break;
        }
        case 13: {                                                 // ^M/ENTER
            if (player->mainMenuOpen && !player->helpTextOpen && !player->logOpen) {
                selectBtn(player);
                flushstdin(c);
            }
            break;
        }
    }

    if (!player->paused && !player->helpTextOpen && !player->mainMenuOpen) {
        *c = tolower(*c);
        switch (*c) {
            case 110:                                              // n/N
            case 119:                                              // w/W
            case 120: rotateCW(&player->currPiece, ARENA); break;  // x/X

            case  98:                                              // b/B
            case 121:                                              // y/Y
            case 122: rotateCCW(&player->currPiece, ARENA); break; // z/Z

            case 109:                                              // m/M
            case  99: swapHeldPiece(player); break;                // c/C

            case 115:  downPressed(player, ARENA); break;          // s/S
            case 100: rightPressed(player, ARENA); break;          // d/D
            case  97:  leftPressed(player, ARENA); break;          // a/A
        }
    }

    if (player->mainMenuOpen && !player->helpTextOpen && !player->logOpen) {
        if (player->buttons[player->selected].valueType == BVTYPE_CHAR) {
            if (*c >= 32 && *c <= 126) writeCharToInput(player, *c);
            else if (*c == 127 || *c == 4) removeCharFromInput(player); // DEL and ^D
        } else if (player->buttons[player->selected].valueType == BVTYPE_NUM) {
            if (*c >= 48 && *c <= 57) writeNumToInput(player, *c - '0');
        }
    }
}



static inline bool strmtch(const char* str1, const char* str2) {
    return strcmp(str1, str2) == 0;
}

static inline void writeField(char** buff, MARKERS marker, const char* str) {
    // Set up color
    *((*buff)++) = '\x1b';
    *((*buff)++) = '[';
    *((*buff)++) = '3';
    *((*buff)++) = '8';
    *((*buff)++) = ';';
    *((*buff)++) = '5';
    *((*buff)++) = ';';
    switch (marker) {
        case EMPTY_MRKR: *((*buff)++) = '0'; *((*buff)++) = '0'; break;
        case LINE_MRKR:
        case GAME_OVER_MRKR:
        case BOUND_MRKR:
        case TEXT_MRKR:  *((*buff)++) = '1'; *((*buff)++) = '5'; break;
        case GOLD_MRKR:  *((*buff)++) = '0'; *((*buff)++) = '3'; break;
        case CYAN_MRKR:  *((*buff)++) = '8'; *((*buff)++) = '7'; break;
        case YLLW_MRKR:  *((*buff)++) = '1'; *((*buff)++) = '1'; break;
        case MGNT_MRKR:  *((*buff)++) = '1'; *((*buff)++) = '3'; break;
        case GREEN_MRKR: *((*buff)++) = '1'; *((*buff)++) = '0'; break;
        case RED_MRKR:   *((*buff)++) = '0'; *((*buff)++) = '9'; break;
        case BLUE_MRKR:  *((*buff)++) = '1'; *((*buff)++) = '2'; break;
        default: {
            printf("\r\nINVALID MARKER IN ARENA OR PIECE: %i\r\n", marker);
            fflush(stdout);
            abort();
        }
    }
    *((*buff)++) = 'm';

    // Draw field
    switch (marker) {
        case EMPTY_MRKR: {
            *((*buff)++) = EMPTY_STR[0];
            *((*buff)++) = EMPTY_STR[1];
            break;
        }
        case LINE_MRKR: {
            *((*buff)++) = LINE_CLEAR_STR[0];
            *((*buff)++) = LINE_CLEAR_STR[1];
            break;
        }
        case GAME_OVER_MRKR: {
            *((*buff)++) = GAME_OVER_STR[0];
            *((*buff)++) = GAME_OVER_STR[1];
            break;
        }
        case BOUND_MRKR: {
            *((*buff)++) = BOUND_STR[0];
            *((*buff)++) = BOUND_STR[1];
            break;
        }
        case TEXT_MRKR: {
            *((*buff)++) = str[0];
            *((*buff)++) = str[1];
            break;
        }
        case CYAN_MRKR:
        case YLLW_MRKR:
        case MGNT_MRKR:
        case GREEN_MRKR:
        case RED_MRKR:
        case BLUE_MRKR:
        case GOLD_MRKR: {
            *((*buff)++) = BLOCK_STR[0];
            *((*buff)++) = BLOCK_STR[1];
            break;
        }
    }

    // Reset color
    *((*buff)++) = '\x1b';
    *((*buff)++) = '[';
    *((*buff)++) = '0';
    *((*buff)++) = 'm';
}



static inline uint8_t lockDelayFormula(Player* player) {
    return MAX( (uint8_t)(player->speed / 2), MIN_LOCK_DELAY );
}

static inline struct timespec lnTimeoutFormula(Player* player, uint8_t currLevel) {
    return (struct timespec){
        .tv_sec  = 0,
        .tv_nsec = (int)(MAX( (float)BASE_ANIM_TIMEOUT - ((float)currLevel / 10.0), 2.0 ) * 1e8)
    };
}

static inline uint8_t speedFormula(uint8_t currLevel) {
    if (currLevel < 10) return MAX(START_SPEED - currLevel, MIN_SPEED);
    return MAX(START_SPEED - currLevel * 2, MIN_SPEED);
}

static inline uint64_t pieceFreezeFormula() {
    return 50;
}

static inline uint64_t lineClearFormula(double lines, uint8_t currLevel) {
    return lines * lines * 100 * (1.0 + 0.075 * currLevel);
}

static inline uint64_t levelUpFormula(uint8_t levels) {
    return 200 * levels;
}



static inline uint8_t getGlobalShapeField(SHP_MATR_SZS MATR_SZ, int pInd, int i, int j) {
    switch (MATR_SZ) {
        case MATR_SZ_FOUR:  return PIECE_SHAPES4[pInd][i][j];
        case MATR_SZ_THREE: return PIECE_SHAPES3[pInd][i][j];
        case MATR_SZ_TWO:   return PIECE_SHAPES2[pInd][i][j];
        default:            return 0;
    }
}

static inline void setPieceShapeField(Piece* p, int i, int j, int field) {
    switch (p->MATR_SZ) {
        case MATR_SZ_FOUR:  p->shape4[i][j] = field; break;
        case MATR_SZ_THREE: p->shape3[i][j] = field; break;
        case MATR_SZ_TWO:   p->shape2[i][j] = field; break;
    }
}

static inline uint8_t getPieceShapeField(Piece* p, int i, int j) {
    switch (p->MATR_SZ) {
        case MATR_SZ_FOUR:  return p->shape4[i][j];
        case MATR_SZ_THREE: return p->shape3[i][j];
        case MATR_SZ_TWO:   return p->shape2[i][j];
        default:            return 0;
    }
}

static inline SHP_MATR_SZS requiredPieceMatrixSize(Piece* p) {
    switch (p->type) {
        case PIECE_NULL:
        case PIECE_I: return MATR_SZ_FOUR;
        case PIECE_O: return MATR_SZ_TWO;
        default:      return MATR_SZ_THREE;
    }
}



static inline void suspend(struct timespec* timeout) {
    #ifdef _WIN32
        HANDLE hTimer;
        if (!(hTimer = CreateWaitableTimerW(NULL, FALSE, NULL))) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO CREATE TIMER");

        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -(long long)(timeout->tv_nsec / 100); // Chunks of 100 nanoseconds, negative means relative

        if (!SetWaitableTimer(hTimer, &dueTime, 0, NULL, NULL, FALSE)) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO SET TIMER");
        if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO WAIT TIMER");

        CloseHandle(hTimer);
    #else
        nanosleep(timeout, NULL);
    #endif
}


// Piece field-pos to arena-pos
static inline Pos FPOStoAPOS(Piece* p, int row, int col) {
    return (Pos){
        .x = p->pos.x + col,
        .y = p->pos.y + row
    };
}
// Arena-pos to field-pos
static inline Pos APOStoFPOS(Piece* p, int row, int col) {
    return (Pos){
        .x = col - p->pos.x,
        .y = row - p->pos.y
    };
}

bool APOSinpiece(Piece* p, Pos apos) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            if (!getPieceShapeField(p, i, j)) continue;

            Pos fieldAPOS = FPOStoAPOS(p, i, j);
            if (fieldAPOS.x == apos.x && fieldAPOS.y == apos.y) return true;
        }
    }

    return false;
}



void strToLower(char* to, size_t toSize, char* from, size_t fromSize) {
    for (int i = 0; i < toSize && i < fromSize; i++) {
        to[i] = tolower(from[i]);
    }
}



void printHelpText() {
    clearterm();
    printf(
        "---------- HOW TO PLAY ----------\r\n"

        "\r\n----- TERMINOLOGY -----\r\n"

        "\r\n- Btn cursor     (Shortening of \"Button cursor\") Cursor in main menu,"
        "\r\n                 used to navigate between individual buttons."
        "\r\n- Input cursor   Cursor in main menu,"
        "\r\n                 used to navigate between individual characters/digits within an input field."

        "\r\n- Tick           One forced movement downward."
        "\r\n                     By default, this is 1 per second, becoming faster as the game goes on.\r\n"

        "\r\n- Lock-delay     Amount of time it takes for a falling piece to be frozen.\r\n"

        "\r\n----- MENU CONTROLS -----\r\n"

        "\r\n-    UP          If outside an input field, move button cursor up"
        "\r\n-  DOWN          If outside an input field, move button cursor down"
        "\r\n- RIGHT          If inside an input field, move input cursor right"
        "\r\n-  LEFT          If inside an input field, move input cursor left\r\n"

        "\r\n- ENTER          Select/deselect a button\r\n"

        "\r\n- CTRL+D         Alternative to delete characters from a text input field"
        "\r\n                 if backspace doesn't work\r\n"

        "\r\n----- GAMEPLAY CONTROLS -----\r\n"

        "\r\n-  DOWN/S        Move current piece down."
        "\r\n- RIGHT/D        Move current piece right."
        "\r\n-  LEFT/A        Move current piece left.\r\n"

        "\r\n- X/N/UP/W       Rotate 90 degrees clockwise (right)."
        "\r\n-    Y/Z/B       Rotate 90 degrees counter-clockwise (left).\r\n"

        "\r\n- C/M            Hold or swap to a held piece.\r\n"

        "\r\n                     If there is no held piece, the current piece is stored and the next one grabbed."
        "\r\n                     If there is a held piece, the current piece is swapped with the held piece."
        "\r\n                     In both cases, the position of the current piece is reset to the top of the arena.\r\n"

        "\r\n                     There can only be one swap/hold per round (until the current piece is frozen).\r\n"

        "\r\n----- OTHER CONTROLS -----\r\n"

        "\r\n-  SPACE         Pause/Resume the game if not in main menu."
        "\r\n- CTRL+R         Restart the game if not in main menu."
        "\r\n                     This does not reset anything input within the main menu."
        "\r\n- CTRL+H         Print this text."
        "\r\n                     Press again to hide this text."
        "\r\n- ESC            Go to main menu."
        "\r\n- CTRL+A         Clear the screen."
        "\r\n                     Depending on where this is pressed, the screen may or may not come back on its own."
        "\r\n                     If it doesn't, pressing CTRL+H and/or ESC will fix it,"
        "\r\n                     though the latter (ESC) should only be used outside of runs due to what it does.\r\n"

        "\r\n----- MECHANICS -----\r\n"

        "\r\n- Every tick, the current piece falls by 1 tile."
        "\r\n  When it reaches the ground,"
        "\r\n  it remains movable for the duration of the lock-delay before being frozen. When a piece is frozen,"
        "\r\n  a new one spawns in its place, which can be seen from the next-piece display.\r\n"

        "\r\n  If the piece is rotated into a position which is higher than the ground,"
        "\r\n  the piece won't be frozen until reaching the ground again and the lock-delay passing.\r\n"

        "\r\n- If a piece is frozen and it fills one or multiple lines, those lines are cleared,"
        "\r\n  and everything above them falls down by the number of lines that were cleared."
        "\r\n  Every 10 cleared lines, there is a level-up, which increases the speed at which a piece falls.\r\n"

        "\r\n- Every piece frozen is worth 50 score.\r\n"

        "\r\n  The amount of score gained from line clears is based on the formula:"
        "\r\n  (CLEARED ^ 2) * 100 * (1 + 0.075 * LEVEL),"
        "\r\n  where CLEARED is the number of lines that were just cleared, and LEVEL is the current level."
        "\r\n  The score is always rounded down to the nearest integer.\r\n"

        "\r\n  This means possible line-clears are worth the following:"
        "\r\n  1 Line:   100-> 107-> 115...;"
        "\r\n  2 Lines:  400-> 430-> 460...;"
        "\r\n  3 Lines:  900-> 967->1035...;"
        "\r\n  4 Lines: 1600->1720->1840....\r\n"

        "\r\n  The amount of score gained from level-ups is based on the formula:"
        "\r\n  200 * LEVEL, where LEVEL is the current level AFTER advancing.\r\n"

        "\r\n  This means the first few level-ups are worth the following:"
        "\r\n  0->1: 200;"
        "\r\n  1->2: 400;"
        "\r\n  2->3: 600.\r\n"

        "\r\n  All three of these sources of score can happen within a single tick.\r\n"

        "\r\n- If a piece spawns in and immediately overlaps any frozen piece, the game ends.\r\n"

        "\r\n----- LOGGING -----\r\n"

        "\r\n- If enabled, a statistics log is saved at the end of every run."
        "\r\n  The \"%s\" button sorts these logs based on the selected criterium,"
        "\r\n  saving it to a file named \"%s\"."
        "\r\n  Restarting does not create logs.\r\n"

        "\r\n  Internally, these logs are stored in \"%s\"."
        "\r\n  Attempting to edit this file will result in near-guaranteed data-loss,"
        "\r\n  as well as potential crashes.\r\n"

        "\r\n- The \"%s\" button will clear the entire history of logs.\r\n"

        "\r\n- There is no limit to how many logs there can be.",
        EXPORT_LOG_LBL,
        EXTRN_LOG_FNM,
        INTRN_LOG_FNM,
        CLR_LOG_LBL
    );
    fflush(stdout);
}



void resetterm() {
    #ifdef _WIN32
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE) ERR_NOFORMAT("ERROR: WINDOWS: INVALID HANDLE");

        DWORD mode = 0;
        if (GetConsoleMode(handle, &mode) == 0) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO GET MODE");

        mode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(handle, mode) == 0) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO SET MODE");

        SetConsoleCtrlHandler(HandlerRoutine, FALSE);

        timeEndPeriod(1); // Set minimum resolution back to default

        CONSOLE_CURSOR_INFO cursor;
        GetConsoleCursorInfo(handle, &cursor);
        cursor.bVisible = TRUE;
        SetConsoleCursorInfo(handle, &cursor);
    #else
        fputs("\x1b[?25h\r\n", stdout); // Show cursor
        fflush(stdout);

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orgterm);
    #endif
}

void enableraw() {
    #ifdef _WIN32
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE) ERR_NOFORMAT("ERROR: WINDOWS: INVALID HANDLE");

        DWORD mode = 0;
        if (GetConsoleMode(handle, &mode) == 0) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO GET MODE");

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(handle, mode) == 0) ERR_NOFORMAT("ERROR: WINDOWS: FAILED TO SET MODE");

        _setmode(_fileno(stdout), _O_BINARY);

        timeBeginPeriod(1); // Minimum resolution is set to 1ms

        CONSOLE_CURSOR_INFO cursor;
        GetConsoleCursorInfo(handle, &cursor);
        cursor.bVisible = FALSE;
        SetConsoleCursorInfo(handle, &cursor);
    #else
        struct termios term;

        tcgetattr(STDIN_FILENO, &term);
        tcgetattr(STDIN_FILENO, &orgterm);

        term.c_iflag &= ~(ICRNL | IXON);
        term.c_oflag &= ~(OPOST | ONLCR);
        term.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        term.c_cc[VMIN] = 0;
        term.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);

        if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) == -1) ERR_NOFORMAT("ERROR: POSIX: FAILED TO SET O_NONBLOCK");
    
        fputs("\x1b[?25l", stdout); // Hide cursor
        fflush(stdout);
    #endif

    clearterm();

    atexit(resetterm);
}

void clearterm() {
    fputs("\x1b[H\x1b[0J\x1b[3J", stdout);
    fflush(stdout);
}

static inline void flushstdin(uint8_t* c) {
    #ifdef _WIN32
        while (_kbhit()) getch();
    #else
        while (read(STDIN_FILENO, c, 1) == 1);
    #endif
}


#ifndef _WIN32
    void crash(int sig) {
        printf("\r\nProgram received signal %i.", sig);
        resetterm();
        raise(sig);
    }

    void setsigs() {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));

        sa.sa_handler = crash;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESETHAND;

        if (sigaction(SIGSEGV, &sa, NULL) == -1) goto fail;
        if (sigaction(SIGABRT, &sa, NULL) == -1) goto fail;
        if (sigaction(SIGILL,  &sa, NULL) == -1) goto fail;
        if (sigaction(SIGFPE,  &sa, NULL) == -1) goto fail;
        if (sigaction(SIGTERM, &sa, NULL) == -1) goto fail;
        if (sigaction(SIGINT,  &sa, NULL) == -1) goto fail;

        goto success;

        fail:
            ERR_NOFORMAT("FAILED TO ESTABLISH SIGNAL HANDLER(S)");
        success:
            return;
    }
#else
    BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
        resetterm();
        if (GenerateConsoleCtrlEvent(dwCtrlType, 0) == 0) return FALSE;
        else return TRUE;
    }

    void setWinHandler() {
        SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    }
#endif


// Creates and returns a vector representing the play-area (arena)
Vector* createArena() {
    Vector* ARENA = crtvec(ARENA_ROWS, DTYPE_VECTOR, NULL);

    for (int i = 0; i < ARENA_ROWS; i++) {
        Vector* row = crtvec(ARENA_COLS, DTYPE_NORMAL, NULL);

        for (int j = 0; j < ARENA_COLS; j++) {
            int8_t* field = malloc(sizeof(int8_t));
            *field = EMPTY_MRKR;
            vecpush(row, field);
        }

        vecpush(ARENA, row);
    }

    return ARENA;
}

void initPlayer(Player* player) {
    player->speed             = START_SPEED;
    player->framesPassed      = 0;

    player->currLinesCleared  = 0;
    player->linesCleared      = 0;
    player->level             = 0;
    player->startLevel        = 0;
    player->score             = 0;
    player->gameOver          = false;
    player->tetrisCount       = 0;

    player->bagLen            = 0;
    regenBag(player);
    player->nextPiece         = createPiece();
    initPiece(&player->nextPiece);
    useNextPiece(player);

    player->paused            = false;

    player->heldPiece         = createPiece();

    player->swappedThisRound  = false;

    player->currToBeFrozen    = false;
    player->lockDelayFrames   = 0;
    player->lockDelay         = lockDelayFormula(player);

    player->lnTimeout         = lnTimeoutFormula(player, player->level);

    player->helpTextOpen      = false;

    player->mainMenuOpen      = false;

    for (int i = 0; i < QUIT; i++) {
        player->buttons[i] = (Button){
            .chars    = (i < SORT_BY) ? crtvec(MAX_INPUT_LENS[i], DTYPE_NORMAL, NULL) : NULL, // Only STRT_LVL and NAME need input fields
            .cycle    = 0,
            .cycleLen = 0,
            .id       = i,
            .label    = BUTTON_LABELS[i],
            .onPress  = BUTTON_FUNCTIONS[i], // This is fine as the array will contain NULL for unused indices/buttons
            .leaveGap = (i == NAME - 1 || i == EXPORT_LOG - 1 || i == CLR_LOG - 1) ? true : false
        };

        switch (i + 1) { // Must add 1 as the enum is 1-based
            case   STRT_LVL: {
                player->buttons[i].valueType = BVTYPE_NUM;
                for (int j = 0; j < player->buttons[i].chars->max; j++) {
                    uint8_t* num = malloc(sizeof(uint8_t));
                            *num = 0;
                    vecpush(player->buttons[i].chars, num);
                }
                break;
            }
            case       NAME: player->buttons[i].valueType = BVTYPE_CHAR; break;
            case    SORT_BY: {
                player->buttons[i].valueType = BVTYPE_CYCLE;
                player->buttons[i].cycleLen  = SB_CYCLE_TETRIS_CNT + 1; // Must add 1 as the enum is 0-based
                player->buttons[i].cycle     = SB_CYCLE_SCORE;
                break;
            }
            case SORT_ORDER: {
                player->buttons[i].valueType = BVTYPE_CYCLE;
                player->buttons[i].cycleLen  = SOB_CYCLE_ASCENDING + 1; // Must add 1 as the enum is 0-based
                player->buttons[i].cycle     = SOB_CYCLE_DESCENDING;
                break;
            }
            case TOGGLE_LOG: {
                player->buttons[i].valueType = BVTYPE_CYCLE;
                player->buttons[i].cycleLen  = TLB_CYCLE_ON + 1; // Must add 1 as the enum is 0-based
                player->buttons[i].cycle     = TLB_CYCLE_ON;     // Automatically turn logging on
                break;
            }
            default: player->buttons[i].valueType = BVTYPE_NULL;
        }
    }

    player->buttonCursor     =  0;
    player->inputCursor      =  0;
    player->selected         = -1; // No selection is represented with -1

    for (int i = 0; i < sizeof(player->name) / sizeof(player->name[0]); i++) {
        player->name[i] = 0;
    }

    player->quit             = false;

    player->firstLevel       = true;

    player->log              = initLog();
    readLog(player->log);
    player->logOpen          = false;
    player->logUnsorted      = true;
}

void reset(Player* player, Vector* ARENA) {
    for (int i = 0; i < ARENA_ROWS; i++) {
        for (int j = 0; j < ARENA_COLS; j++) {
            *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j] = 0;
        }
    }

    /*
     * Skipped:
     * player->startLevel;
     * player->buttons;
     * player->buttonCursor;
     * player->inputCursor;
     * player->selected;
     * player->name;
     * player->quit;
     * player->log;
     * player->logUnsorted;
     */

    player->speed             = START_SPEED;
    player->framesPassed      = 0;

    player->currLinesCleared  = 0;
    player->linesCleared      = 0;
    player->level             = player->startLevel;
    player->score             = 0;
    player->gameOver          = false;

    player->bagLen            = 0;
    regenBag(player);
    player->nextPiece         = createPiece();
    initPiece(&player->nextPiece);
    useNextPiece(player);

    player->paused            = false;

    player->helpTextOpen      = false;

    player->mainMenuOpen      = false;

    player->heldPiece.MATR_SZ = MATR_SZ_FOUR;
    for (int i = 0; i < player->heldPiece.MATR_SZ; i++) {
        for (int j = 0; j < player->heldPiece.MATR_SZ; j++) {
            setPieceShapeField(&player->heldPiece, i, j, getGlobalShapeField(player->heldPiece.MATR_SZ, PIECE_NULL, i, j));
        }
    }

    player->heldPiece.pos     = (Pos){.x = 0, .y = 0};
    player->heldPiece.color   = EMPTY_MRKR;
    player->heldPiece.type    = PIECE_NULL;

    player->swappedThisRound  = false;

    player->currToBeFrozen    = false;

    player->lockDelayFrames   = 0;
    player->lockDelay         = lockDelayFormula(player);

    player->lnTimeout         = lnTimeoutFormula(player, player->level);

    player->firstLevel        = true;

    player->logOpen           = false;

    clearterm();
}

void freePlayer(Player* player) {
    for (int i = 0; i < sizeof(player->buttons) / sizeof(player->buttons[0]); i++) {
        freevec(player->buttons[i].chars);
    }
    freevec(player->log);
}

void quit(Player* player, Vector* ARENA) {
    freevec(ARENA);
    freePlayer(player);
}



void renderGame(Player* player, Vector* ARENA) {
    // Must add 1 to rows to include bottom bound;
    // Must add 3 to cols to include left and right bounds + CRLF ("\r\n");
    // Must add NXT_PC_XPOS_STRT + MATR_SZ_FOUR to cols so we fit the padding AND the max amount of columns needed to display the next/held piece.
    // We must multiply by 16 to account for writing two characters for each field, as well as escape codes for enabling and disabling colors.

    // We allocate on the heap to allow pointer arithmetic with the buffer
    size_t alloc = (
        ( ((ARENA_ROWS + 1) * (ARENA_COLS + (NXT_PC_XPOS_STRT + MATR_SZ_FOUR) + 3) * 16) + 1 ) *
        sizeof(char)
    );

    char* toPrint = malloc(alloc);
    char* toPrintBegin = toPrint; // Needed to access the beginning of the buffer after shifting the original pointer

    for (int i = 0; i < ARENA_ROWS; i++) {
        writeField(&toPrint, BOUND_MRKR, NULL);

        for (int j = 0; j < ARENA_COLS; j++) {
            if (APOSinpiece(&player->currPiece, (Pos){.x = j, .y = i} ) && *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j] != GAME_OVER_MRKR) {
                writeField(&toPrint, player->currPiece.color, NULL);
            } else {
                switch( *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j] ) {
                    case EMPTY_MRKR:

                    case CYAN_MRKR:
                    case YLLW_MRKR:
                    case MGNT_MRKR:
                    case GREEN_MRKR:
                    case RED_MRKR:
                    case BLUE_MRKR:
                    case GOLD_MRKR:

                    case LINE_MRKR:
                    case GAME_OVER_MRKR: {
                        // If we found a marker, it's enough to use the field as the marker
                        writeField(&toPrint, *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j], NULL);
                        break;
                    }
                    default: ERR_FORMAT("ERROR: INVALID MARKER IN ARENA: %i", *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j]);
                }
            }
        }

        writeField(&toPrint, BOUND_MRKR, NULL);

        for (int j = 0; j < NXT_PC_XPOS_STRT; j++) writeField(&toPrint, EMPTY_MRKR, NULL);

        // We shift the actual piece displays down by two lines to create an extra line of padding
        // between the labels and the pieces
        uint8_t dispYOffset = 2;

        if (!player->gameOver && i >= NXT_PC_YPOS_STRT + dispYOffset && i < NXT_PC_YPOS_STRT + dispYOffset + player->nextPiece.MATR_SZ) {
            // In the case of an O or J piece, we want to shift one column to the right,
            // as there is no left-padding in the matrix representation,
            // making it look shifted to the left.
            bool shouldOffsetRight = player->nextPiece.type == PIECE_O || player->nextPiece.type == PIECE_J;
            if (shouldOffsetRight) writeField(&toPrint, EMPTY_MRKR, NULL);

            for (int j = 0; j < player->nextPiece.MATR_SZ; j++) {
                uint8_t marker = getPieceShapeField(&player->nextPiece, i - (NXT_PC_YPOS_STRT + dispYOffset), j);
                switch (marker) {
                    case EMPTY_MRKR: writeField(&toPrint, EMPTY_MRKR, NULL);              break;
                    case          1: writeField(&toPrint, player->nextPiece.color, NULL); break;
                    default:         ERR_FORMAT("ERROR: INVALID MARKER IN NEXT PIECE: %i", marker);
                }
            }

            // If there was a next-piece with a larger footprint than the current one,
            // there'll be fields left over from the last render,
            // so we must overwrite them here.
            for (int j = 0; j < MATR_SZ_FOUR - player->nextPiece.MATR_SZ - ((shouldOffsetRight) ? 1 : 0); j++) writeField(&toPrint, EMPTY_MRKR, NULL);
        } else if (!player->gameOver && i >= HLD_PC_YPOS_STRT + dispYOffset && i < HLD_PC_YPOS_STRT + dispYOffset + player->heldPiece.MATR_SZ) {
            bool shouldOffsetRight = player->heldPiece.type == PIECE_O || player->heldPiece.type == PIECE_L;
            if (shouldOffsetRight) writeField(&toPrint, EMPTY_MRKR, NULL);

            for (int j = 0; j < player->heldPiece.MATR_SZ; j++) {
                uint8_t marker = getPieceShapeField(&player->heldPiece, i - (HLD_PC_YPOS_STRT + dispYOffset), j);
                switch (marker) {
                    case EMPTY_MRKR: writeField(&toPrint, EMPTY_MRKR, NULL);              break;
                    case          1: writeField(&toPrint, player->heldPiece.color, NULL); break;
                    default:         ERR_FORMAT("ERROR: INVALID MARKER IN HELD PIECE: %i", marker);
                }
            }

            // The footprint of the held-piece is constant, so we can omit empty padding.
        } else if (!player->gameOver && i == NXT_PC_YPOS_STRT) {
            writeField(&toPrint, TEXT_MRKR, "NE");
            writeField(&toPrint, TEXT_MRKR, "XT");
            writeField(&toPrint, TEXT_MRKR, ": ");
            writeField(&toPrint, EMPTY_MRKR, NULL);
        } else if (!player->gameOver && i == HLD_PC_YPOS_STRT) {
            writeField(&toPrint, TEXT_MRKR, "HE");
            writeField(&toPrint, TEXT_MRKR, "LD");
            writeField(&toPrint, TEXT_MRKR, ": ");
            writeField(&toPrint, EMPTY_MRKR, NULL);
        } else {
            // Fill the remaining columns with spaces if there's nothing to print
            for (int j = 0; j < MATR_SZ_FOUR; j++) writeField(&toPrint, EMPTY_MRKR, NULL);
        }

        writeField(&toPrint, TEXT_MRKR, "\r\n");
    }

    for (int j = 0; j < ARENA_COLS + 2; j++) {
        writeField(&toPrint, BOUND_MRKR, NULL);
    }

    writeField(&toPrint, TEXT_MRKR, "\r\n");

    *toPrint = '\0';



    fputs("\x1b[H", stdout);
    fputs(toPrintBegin, stdout);

    if (player->name[0]) {
        printf("\r\nNAME: \"%s\"\r\n", player->name);
    }

    if (!player->gameOver) {
        printf(
            "\r\nSCORE: %lu"
            "\r\nLINES: %u"
            "\r\nLEVEL: %u\r\n",
            player->score,
            player->linesCleared,
            player->level
        );
    }

    fflush(stdout);

    free(toPrintBegin);
}

void gameOver(Player* player, Vector* ARENA, struct timespec* animationTimeout) {
    clearterm();
    renderGame(player, ARENA);

    suspend(animationTimeout);

    struct timespec timeout = {
        .tv_sec  = 0,
        .tv_nsec = (int)5e7
    };

    for (int i = 0; i < ARENA_ROWS; i++) {
        for (int j = 0; j < ARENA_COLS; j++) {
            *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j] = GAME_OVER_MRKR;
        }

        suspend(&timeout);
        renderGame(player, ARENA);
    }

    suspend(animationTimeout);

    clearterm();
    renderGame(player, ARENA);

    printf(
        "\r\nGAME OVER!\r\n"

        "\r\nSCORE: %lu"
        "\r\nLINES CLEARED: %u"
        "\r\nLEVEL REACHED: %u\r\n",
        player->score,
        player->linesCleared,
        player->level
    );
    fflush(stdout);


    if (player->buttons[TOGGLE_LOG - 1].cycle) {
        LogRow* logRow = malloc(sizeof(LogRow));

        *logRow = (LogRow){
            .name        = strdup((player->name[0]) ? player->name : DFLT_NAME),
            .score       = player->score,
            .lines       = player->linesCleared,
            .level       = player->level,
            .startLevel  = player->startLevel,
            .tetrisCount = player->tetrisCount
        };

        vecpush(player->log, logRow);

        writeLog(player->log);
    }

    player->logUnsorted = true;
}

void levelUp(struct timespec* animationTimeout) {
    fputs("\r\nLEVEL UP!\r\n", stdout);
    fflush(stdout);
    suspend(animationTimeout);
    clearterm();
}



void togglePause(Player* player) {
    if (player->gameOver) return;

    player->paused = !player->paused;
    clearterm();
    if (player->paused) {
        fputs("PAUSED\r\n", stdout);
        fflush(stdout);
    } else {
        uint8_t c;
        flushstdin(&c);
    }
}

void toggleHelp(Player* player, Vector* ARENA) {
    player->helpTextOpen = !player->helpTextOpen;

    if (player->helpTextOpen) printHelpText();
    else {
        clearterm();

        if (player->paused) {
            togglePause(player); // Disable so we can enable it again after rerendering to restore the pause-text
            renderGame(player, ARENA);
            togglePause(player); // Enable to print pause-text
        }
        if (player->mainMenuOpen) {
            renderMenu(player);
        }
    }
}

void goToMenu(Player* player, Vector* ARENA) {
    reset(player, ARENA);
    player->mainMenuOpen = true;
    clearterm();
    renderMenu(player);
}



void regenBag(Player* player) {
    // We store the indices of the templates for piece shapes,
    // and we select a random one to push to the bag, removing the chosen index each time,
    // until there are no indices left.

    Vector* remaining = crtvec(PIECE_Z, DTYPE_NORMAL, NULL);

    for (int p = PIECE_I; p <= PIECE_Z; p++) {
        uint8_t* ind = malloc(sizeof(uint8_t));
                *ind = p;
        vecpush(remaining, ind);
    }

    while (remaining->len >= 1) {
        Piece* bagPiece = &player->bag[player->bagLen];
              *bagPiece = createPiece();

        #ifdef _WIN32
            uint32_t randInd;
            rand_s(&randInd);
            randInd %= remaining->len;
        #else
            uint32_t randInd = arc4random_uniform((uint32_t)remaining->len);
        #endif

        uint8_t shapeInd = *(uint8_t*)remaining->arr[randInd];

        bagPiece->type = shapeInd;

        switch (bagPiece->type) {
            case PIECE_NULL:
            case PIECE_I: bagPiece->MATR_SZ = MATR_SZ_FOUR;  break;
            case PIECE_O: bagPiece->MATR_SZ = MATR_SZ_TWO;   break;
            default:      bagPiece->MATR_SZ = MATR_SZ_THREE; break;
        }

        for (int i = 0; i < bagPiece->MATR_SZ; i++) {
            for (int j = 0; j < bagPiece->MATR_SZ; j++) {
                setPieceShapeField(bagPiece, i, j, getGlobalShapeField(bagPiece->MATR_SZ, bagPiece->type, i, j));
            }
        }

        if (bagPiece->MATR_SZ > MATR_SZ_FOUR || bagPiece->MATR_SZ < MATR_SZ_TWO) {
            printf("\r\nINVALID PIECE MATRIX SIZE: %i (Line: %i)\r\n", bagPiece->MATR_SZ, __LINE__);
            fflush(stdout);
            abort();
        }

        bagPiece->color = (MARKERS)bagPiece->type;
        bagPiece->pos = (Pos){
            .x = PIECE_STRT_XPOS(bagPiece->type),
            .y = 0
        };

        player->bagLen++;
        vecrmv(remaining, randInd);
    }

    freevec(remaining);
}

void useNextPiece(Player* player) {
    if (player->nextPiece.MATR_SZ != requiredPieceMatrixSize(&player->nextPiece)) abort();

    player->currPiece = player->nextPiece;
    player->nextPiece = player->bag[--player->bagLen];
    if (player->bagLen < 1) regenBag(player);
}

Piece createPiece() {
    Piece p = {0};
    p.MATR_SZ = MATR_SZ_FOUR;
    return p;
}

void initPiece(Piece* p) {
    #ifdef _WIN32
        rand_s(&p->type);
        p->type = (p->type % 7) + 1; // Must add 1 to skip PIECE_NULL
    #else
        p->type = arc4random_uniform((uint32_t)PIECE_Z) + 1; // Must add 1 to skip PIECE_NULL
    #endif

    p->pos = (Pos){
        .x = PIECE_STRT_XPOS(p->type),
        .y = 0
    };

    switch (p->type) {
        case PIECE_I:
        case PIECE_NULL: p->MATR_SZ = MATR_SZ_FOUR;  break;
        case PIECE_O:    p->MATR_SZ = MATR_SZ_TWO;   break;
        default:         p->MATR_SZ = MATR_SZ_THREE; break;
    }

    p->color = (MARKERS)p->type;

    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            setPieceShapeField(p, i, j, getGlobalShapeField(p->MATR_SZ, p->type, i, j));
        }
    }
}

void setPieceToNull(Piece* p) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            setPieceShapeField(p, i, j, 0);
        }
    }
}



bool pieceCollides(Piece* p, Vector* ARENA) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            if (!getPieceShapeField(p, i, j)) continue;

            Pos APOS = FPOStoAPOS(p, i, j);
            if ( APOS.x < 0 || APOS.x >= ARENA_COLS || APOS.y < 0 || APOS.y >= ARENA_ROWS ) return true;
            if ( *(int8_t*)( (Vector*)ARENA->arr[APOS.y] )->arr[APOS.x] ) return true;
        }
    }

    return false;
}



bool canMoveDown(Piece* p, Vector* ARENA) {
    p->pos.y++;
    bool collides = pieceCollides(p, ARENA);
    p->pos.y--;
    return !collides; // pieceCollides() returns true if there is collision, so we invert to check for no collision
}

bool canMoveRight(Piece* p, Vector* ARENA) {
    p->pos.x++;
    bool collides = pieceCollides(p, ARENA);
    p->pos.x--;
    return !collides;
}

bool canMoveLeft(Piece* p, Vector* ARENA) {
    p->pos.x--;
    bool collides = pieceCollides(p, ARENA);
    p->pos.x++;
    return !collides;
}

static inline void moveDown(Piece* p, Vector* ARENA) {
    if (canMoveDown(p, ARENA)) p->pos.y++;
}

static inline void moveRight(Piece* p, Vector* ARENA) {
    if (canMoveRight(p, ARENA)) p->pos.x++;
}

static inline void moveLeft(Piece* p, Vector* ARENA) {
    if (canMoveLeft(p, ARENA)) p->pos.x--;
}



void transpose(Piece* p) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            // The left half of the diagonal (from top-left to bottom-right) will already be swapped, so we skip it
            // We also skip 'j' being equal to 'i' as such fields are not affected
            if (j > i) {
                uint8_t temp = getPieceShapeField(p, i, j);
                setPieceShapeField(p, i, j, getPieceShapeField(p, j, i));
                setPieceShapeField(p, j, i, temp);
            }
        }
    }
}

void reverseRows(Piece* p) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < (int)(p->MATR_SZ / 2); j++) { // Must divide by 2 as going through the second half would reverse the operation
            // Must subtract 1 as the matrix size is 1-based
            int lastIndJ = p->MATR_SZ-1 - j;

            uint8_t temp = getPieceShapeField(p, i, j);
            setPieceShapeField(p, i, j, getPieceShapeField(p, i, lastIndJ));
            setPieceShapeField(p, i, lastIndJ, temp);
        }
    }
}

void rotCW(Piece* p) {
      transpose(p);
    reverseRows(p);
}

void rotCCW(Piece *p) {
    reverseRows(p);
      transpose(p);
}



bool wallKick(Piece* p, Vector* ARENA) {
    // We test different pushes (including no push at all),
    // and if any succeed the function found a valid spot and thus the rotation is valid.

    if (!pieceCollides(p, ARENA)) return true; // Don't test any kicks if the piece is already in a valid position

    Pos orgpos = p->pos;
    for (int i = 0; i < sizeof(WALL_KICKS) / sizeof(WALL_KICKS[0]); i++) {
        p->pos.x += WALL_KICKS[i].x;
        p->pos.y += WALL_KICKS[i].y;
        if (!pieceCollides(p, ARENA)) return true;
        p->pos = orgpos;
    }

    return false;
}



bool canRotateCW(Piece* p, Vector* ARENA) {
    Piece copy = *p;
    rotCW(&copy);
    return wallKick(&copy, ARENA);
}

bool canRotateCCW(Piece* p, Vector* ARENA) {
    Piece copy = *p;
    rotCCW(&copy);
    return wallKick(&copy, ARENA);
}

void rotateCW(Piece* p, Vector* ARENA) {
    // Rotating the O-piece doesn't do anything, so we skip it
    if (p->type == PIECE_O) return;

    if (canRotateCW(p, ARENA)) {
        rotCW(p);
        wallKick(p, ARENA);
    }
}

void rotateCCW(Piece* p, Vector* ARENA) {
    // Rotating the O-piece doesn't do anything, so we skip it
    if (p->type == PIECE_O) return;

    if (canRotateCCW(p, ARENA)) {
        rotCCW(p);
        wallKick(p, ARENA);
    }
}



void swapHeldPiece(Player* player) {
    if (player->swappedThisRound) return;

    Piece* heldPiece = &player->heldPiece;
    Piece* currPiece = &player->currPiece;


    if (player->heldPiece.type == PIECE_NULL) {
        for (int i = 0; i < currPiece->MATR_SZ; i++) {
            for (int j = 0; j < currPiece->MATR_SZ; j++) {
                setPieceShapeField(heldPiece, i, j, getPieceShapeField(currPiece, i, j));
            }
        }

        heldPiece->color = currPiece->color;
        heldPiece->type  = currPiece->type;

        useNextPiece(player);
    } else {
        Piece tempHeldPiece = *heldPiece;
        *heldPiece = createPiece();

        for (int i = 0; i < currPiece->MATR_SZ; i++) {
            for (int j = 0; j < currPiece->MATR_SZ; j++) {
                setPieceShapeField(heldPiece, i, j, getPieceShapeField(currPiece, i, j));
            }
        }

        currPiece->MATR_SZ = requiredPieceMatrixSize(&tempHeldPiece);

        for (int i = 0; i < currPiece->MATR_SZ; i++) {
            for (int j = 0; j < currPiece->MATR_SZ; j++) {
                setPieceShapeField(currPiece, i, j, getPieceShapeField(&tempHeldPiece, i, j));
            }
        }

        MARKERS tempColor = tempHeldPiece.color;
        heldPiece->color  = currPiece->color;
        currPiece->color  = tempColor;

        SHAPES tempType = tempHeldPiece.type;
        heldPiece->type = currPiece->type;
        currPiece->type = tempType;

        // Same logic as in createPiece()

        currPiece->pos = (Pos){
            .x = ARENA_COLS / 2 - ((currPiece->type == PIECE_O) ? 1 : 2),
            .y = 0
        };
    }

    player->swappedThisRound = true;
}



void engrainPiece(Piece* p, Vector* ARENA) {
    for (int i = 0; i < p->MATR_SZ; i++) {
        for (int j = 0; j < p->MATR_SZ; j++) {
            if (!getPieceShapeField(p, i, j)) continue;

            Pos APOS = FPOStoAPOS(p, i, j);
            *(int8_t*)( (Vector*)ARENA->arr[APOS.y] )->arr[APOS.x] = p->color;
        }
    }
}



bool markLines(Vector* ARENA) {
    bool foundLines = false;

    for (int i = 0; i < ARENA_ROWS; i++) {
        if ( !*(int8_t*)( (Vector*)ARENA->arr[i] )->arr[0] ) continue;

        int j = 0;
        for (; j < ARENA_COLS; j++) {
            if ( !*(int8_t*)( (Vector*)ARENA->arr[i] )->arr[j] ) break;
        }
        if (j >= ARENA_COLS) {
            for (int c = 0; c < ARENA_COLS; c++) {
                *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[c] = LINE_MRKR;
            }
            foundLines = true;
        }
    }

    return foundLines;
}

int clearLines(Vector* ARENA) {
    int8_t lines = 0;

    for (int i = 0; i < ARENA_ROWS; i++) {
        if ( *(int8_t*)( (Vector*)ARENA->arr[i] )->arr[0] == LINE_MRKR ) {
            Vector* row = crtvec(ARENA_COLS, DTYPE_NORMAL, NULL);
            for (int c = 0; c < ARENA_COLS; c++) {
                int8_t* field = malloc(sizeof(int8_t));
                       *field = EMPTY_MRKR;
                vecpush(row, field);
            }

                vecrmv(ARENA, i);
            vecinsrtsf(ARENA, 0, row);

            lines++;
        }
    }

    return lines;
}



void renderMenu(Player* player) {
    printf("\x1b[H%s\r\n\r\n", MENU_TITLE);

    for (int i = 0; i < QUIT; i++) {
        Button* btn = &player->buttons[i];

        if (i == player->buttonCursor && player->selected == -1) {
            printf("\x1b[48;5;15;38;5;16m%s\x1b[0m", btn->label);
        } else {
            fputs(btn->label, stdout);
        }

        uint8_t padding = MENU_PADDING - strlen(btn->label);
        printf("%*s", padding, "");

        switch (btn->valueType) {
            case BVTYPE_CHAR:
            case  BVTYPE_NUM: {
                for (int j = 0; j < btn->chars->max + 1; j++) {
                    bool highlightChar = (j == player->inputCursor && i == player->selected);

                    if (highlightChar) {
                        fputs("\x1b[48;5;15;38;5;16m", stdout);
                    }

                    char* format = NULL;

                    switch (btn->valueType) {
                        case BVTYPE_CHAR: format = "%c"; break;
                        case  BVTYPE_NUM: format = "%i"; break;
                    }

                    if (j < btn->chars->len && btn->chars->arr[j]) {
                        printf(format, *(uint8_t*)btn->chars->arr[j]);
                    } else {
                        fputc(' ', stdout);
                    }

                    if (highlightChar) {
                        fputs("\x1b[0m", stdout);
                    }
                }

                break;
            }
            case BVTYPE_CYCLE: {
                bool highLightCycle = (i == player->selected);

                if (highLightCycle) {
                    fputs("\x1b[48;5;15;38;5;16m", stdout);
                }

                switch (btn->id + 1) { // Must add 1 as ids are 0-based while the enum is 1-based
                    case    SORT_BY: fputs( SB_BTN_CYCLE_LBLS[btn->cycle], stdout); break;
                    case TOGGLE_LOG: fputs(TLB_BTN_CYCLE_LBLS[btn->cycle], stdout); break;
                    case SORT_ORDER: fputs(    SOB_CYCLE_LBLS[btn->cycle], stdout); break;
                }

                if (highLightCycle) {
                    fputs("\x1b[0m", stdout);
                }

                break;
            }
        }

        padding = 0;

        switch (btn->valueType) {
            case  BVTYPE_CHAR:
            case   BVTYPE_NUM: padding = MENU_PADDING - btn->chars->len; break;
            case BVTYPE_CYCLE: {
                switch (btn->id + 1) {
                    case    SORT_BY: padding = MENU_PADDING - strlen( SB_BTN_CYCLE_LBLS[btn->cycle]); break;
                    case TOGGLE_LOG: padding = MENU_PADDING - strlen(TLB_BTN_CYCLE_LBLS[btn->cycle]); break;
                    case SORT_ORDER: padding = MENU_PADDING - strlen(    SOB_CYCLE_LBLS[btn->cycle]); break;
                }
            }
        }

        printf("%*s\r\n", padding, "");

        if (btn->leaveGap) fputs("\r\n", stdout);
    }

    fflush(stdout);
}



static inline void markLogUnsorted(Player* player, Button* btn) {
    if (btn->id == SORT_BY - 1 || btn->id == SORT_ORDER - 1) {
        player->logUnsorted = true;
    }
}

void moveCursorUp(Player* player) {
    if (player->buttonCursor > 0 && player->selected == -1) {
        player->buttonCursor--;
        renderMenu(player);
    }
}

void moveCursorDown(Player* player) {
    // Must subtract 1 from QUIT as it is 1-based while the cursor is 0-based
    if (player->buttonCursor < QUIT - 1 && player->selected == -1) {
        player->buttonCursor++;
        renderMenu(player);
    }
}

void moveCursorLeft(Player* player) {
    if (player->selected == -1) return;

    Button* btn = &player->buttons[player->selected];

    if (btn->valueType == BVTYPE_CYCLE) {
        if (btn->cycle == 0) btn->cycle = btn->cycleLen - 1;
        else btn->cycle--;
    } else if (player->inputCursor > 0) {
        player->inputCursor--;
    }

    markLogUnsorted(player, btn);

    renderMenu(player);
}

void moveCursorRight(Player* player) {
    // Only buttons up to the sort-by button (inclusive) have input fields of some kind
    if (player->selected == -1) return;

    Vector* vec = player->buttons[player->selected].chars;
    Button* btn = &player->buttons[player->selected];

    uint8_t rightBound = 0;

    switch (btn->valueType) {
        case  BVTYPE_CHAR: rightBound = MIN(vec->len, vec->max); break;
        case   BVTYPE_NUM: {
            // If the input field is of a numeric type, we don't want the cursor to go past the length as no appending is needed
            rightBound = MIN(vec->len - 1, vec->max);
            break;
        }
        case BVTYPE_CYCLE: break; // Cyclic buttons don't use the input cursor, so we don't need a bound
    }

    if (btn->valueType == BVTYPE_CYCLE) {
        if (btn->cycle == btn->cycleLen - 1) btn->cycle = 0;
        else btn->cycle++;
    } else if (player->inputCursor < rightBound) {
        player->inputCursor++;
    }

    markLogUnsorted(player, btn);

    renderMenu(player);
}



void selectBtn(Player* player) {
    if (player->selected != -1) {
        player->selected    = -1;
        player->inputCursor =  0;
    }
    else player->selected = player->buttonCursor;
    renderMenu(player);
}



void writeCharToInput(Player* player, char c) {
    Vector* vec = player->buttons[player->selected].chars;
    if (vec->len == vec->max) return;

    char* ch = malloc(sizeof(char));
         *ch = c;
    vecinsrtsf(vec, player->inputCursor, ch);

    updateFromInput(player);
    moveCursorRight(player);

    // Moving the cursor usually rerenders the menu, but there are cases when it doesn't (e.g., cursor at input boundaries), so we do it here
    renderMenu(player);
}

void writeNumToInput(Player* player, uint8_t nc) {
    Vector* vec = player->buttons[player->selected].chars;
    
    *(uint8_t*)vec->arr[player->inputCursor] = nc;

    updateFromInput(player);
    renderMenu(player);
}

void removeCharFromInput(Player* player) {
    Vector* vec = player->buttons[player->selected].chars;
    if (vec->len == 0 || player->inputCursor < 1) return;

    vecrmv(vec, player->inputCursor - 1);

    updateFromInput(player);
    moveCursorLeft(player);

    // Moving the cursor usually rerenders the menu, but there are cases when it doesn't (e.g., cursor at input boundaries), so we do it here
    renderMenu(player);
}

uint32_t uint8VecToInt(Vector* vec) {
    uint32_t num = 0;
    for (int i = 0; i < vec->len; i++) {
        num = num * 10 + *(uint8_t*)vec->arr[i];
    }
    return num;
}

void capUint8Vec(Vector* vec, uint8_t cap) {
    for (int i = vec->len - 1; i >= 0; i--) {
        *(uint8_t*)vec->arr[i] = cap % 10;
        cap /= 10;
    }
}



void updateFromInput(Player* player) {
    for (int i = 0; i < sizeof(player->buttons) / sizeof(player->buttons[0]); i++) {
        Vector* vec = player->buttons[i].chars;
        switch (player->buttons[i].id + 1) {
            case STRT_LVL: {
                uint32_t num = uint8VecToInt(vec);
                if (num >= MAX_STARTING_LVL) {
                    num  = MAX_STARTING_LVL;
                    capUint8Vec(vec, num);
                }
                player->startLevel = num;
                break;
            }
            case NAME: {
                for (int j = 0; j < sizeof(player->name) / sizeof(player->name[0]); j++) {
                    // We must zero-out the name as otherwise only characters up to the Vector's length would be overridden,
                    // causing issues when deleting characters.
                    player->name[j] = '\0';
                }

                int j = 0;
                for (; vec->arr[j] != NULL && j < vec->max; j++) {
                    player->name[j] = *(uint8_t*)vec->arr[j];
                }
                player->name[j] = '\0';
                break;
            }
        }
    }
}



static inline void triggerLogSort(Player* player) {
    if (player->logUnsorted) {
        sortLogVec(
            player->log,
            player->buttons[SORT_BY - 1].cycle,
            (player->buttons[SORT_ORDER - 1].cycle == SOB_CYCLE_DESCENDING) ? true : false
        );
        player->logUnsorted = false;
    }
}

void startBtnPressed(void* player) {
    Player* _player = player;

    _player->mainMenuOpen = false;
    _player->level = _player->startLevel;
}

void helpBtnPressed(void* player) {
    ( (Player*)player )->helpTextOpen = true;
    clearterm();
    printHelpText();
}

void quitBtnPressed(void* player) {
    ( (Player*)player )->quit = true;
}

void viewLogBtnPressed(void* player) {
    Player* _player = player;

    triggerLogSort(_player);
    printLog(_player->log);

    _player->logOpen = true;
}

void exportLogBtnPressed(void* player) {
    Player* _player = player;

    triggerLogSort(_player);
    exportLog(_player->log);
}

void clearLogBtnPressed(void* player) {
    clearLog(( (Player*)player )->log);
    writeLog(( (Player*)player )->log);
}



void executeSelected(Player* player) {
    if (player->selected != -1 && player->buttons[player->selected].onPress) {
        player->buttons[player->selected].onPress(player);
        player->selected = -1;

        if (!player->helpTextOpen && !player->logOpen && player->mainMenuOpen) renderMenu(player);
    }
}



void upPressed(Player* player, Vector* ARENA) {
    if (!player->mainMenuOpen) {
        rotateCW(&player->currPiece, ARENA);
    } else {
        moveCursorUp(player);
    }
}

void downPressed(Player* player, Vector* ARENA) {
    if (!player->mainMenuOpen) {
        moveDown(&player->currPiece, ARENA);
    } else {
        moveCursorDown(player);
    }
}

void rightPressed(Player* player, Vector* ARENA) {
    if (!player->mainMenuOpen) {
        moveRight(&player->currPiece, ARENA);
    } else {
        moveCursorRight(player);
    }
}

void leftPressed(Player* player, Vector* ARENA) {
    if (!player->mainMenuOpen) {
        moveLeft(&player->currPiece, ARENA);
    } else {
        moveCursorLeft(player);
    }
}