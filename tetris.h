#ifndef TETRIS_H
#define TETRIS_H

#define ARENA_ROWS       20 // Height
#define ARENA_COLS       10 // Width

#define BOUND_STR      "##" // String to draw at the bounds of the board
#define EMPTY_STR      "  " // String to draw in place of empty fields in arena
#define BLOCK_STR      "[]" // String to draw in place of frozen pieces, or the current piece
#define LINE_CLEAR_STR "==" // String to draw in place of lines marked for clearing
#define GAME_OVER_STR  "**" // String to draw in place of fields marked at the end of the game

#define NXT_PC_YPOS_STRT  4 // The next-piece display starts at row index 4 (row 5)
#define NXT_PC_XPOS_STRT  3 // The next-piece display starts after 3 empty columns past the arena bounds
#define HLD_PC_YPOS_STRT 12 // The held-piece display starts at row index 12 (row 13)
#define HLD_PC_XPOS_STRT  3 // The held-piece display starts after 3 empty columns past the arena bounds

#define FPS              60 // 60 Frames per second
#define START_SPEED      60 // The current piece moves down by 1 tile over a second in the beginning
#define MIN_SPEED         2 // Max speed that can be reached (lower is faster)
#define ANIMATION_TIMEOUT 5 // 0.5s
#define LEVEL_THRESHOLD  10 // Number of lines to clear to reach next level

#define MIN_LOCK_DELAY    8 // Least amount of frames lock-delay can take

#define BASE_ANIM_TIMEOUT 5 // 0.5s, used for player

#define MAX_LVL_INPUT_LEN 2 // How many digits the input starting level can be
#define MAX_NME_INPUT_LEN 8 // How many characters the input name can be

#define MAX_STARTING_LVL 29

#define MENU_PADDING     14 // How many columns a button or its input field should push text to the right

// Appears at top of main menu
#define VERSION_STR       "V1.6.4"
#define MENU_TITLE        ("CHOMPTRIS " VERSION_STR)

// Labels for the buttons in main menu
#define STRT_LVL_LBL      "LEVEL"
#define NAME_LBL          "NAME"

#define SORT_BY_LBL       "SORT BY"
#define SORT_ORDER_LBL    "SORT ORDER"
#define TOGGLE_LOG_LBL    "TOGGLE LOG"
#define VIEW_LOG_LBL      "VIEW LOG"
#define EXPORT_LOG_LBL    "EXPORT LOG"

#define CLR_LOG_LBL       "CLEAR LOG"

#define STRT_LBL          "START"
#define HELP_LBL          "HELP"
#define QUIT_LBL          "QUIT"



#define CLR_LOG_CONF_MSG  "Clear log? (Press ENTER to confirm, ESC to abort)"



#define SB_BTN_SCR_LBL    "SCORE"
#define SB_BTN_LN_LBL     "LINES"
#define SB_BTN_LVL_LBL    "LEVEL"
#define SB_BTN_STRT_LBL   "START"
#define SB_BTN_TETR_LBL   "TETRISES"

#define SOB_DESCEND_LBL   "DESCENDING"
#define SOB_ASCEND_LBL    "ASCENDING"

#define BTN_ON_LBL        "ON"
#define BTN_OFF_LBL       "OFF"



#ifdef _WIN32
    #define _CRT_RAND_S
#endif

#include "vector/vector.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/signal.h>
#endif



typedef struct Pos {
    int8_t x;
    int8_t y;
} Pos;



typedef enum SHAPES {
    PIECE_NULL = 0,
    PIECE_I,
    PIECE_O,
    PIECE_T,
    PIECE_J,
    PIECE_L,
    PIECE_S,
    PIECE_Z
} SHAPES;

typedef enum MARKERS {
    EMPTY_MRKR = 0, // Used to represent empty fields

    CYAN_MRKR,      // Used to represent fields containing cyan blocks
    YLLW_MRKR,      // Used to represent fields containing yellow blocks
    MGNT_MRKR,      // Used to represent fields containing magenta blocks
    GREEN_MRKR,     // Used to represent fields containing green blocks
    RED_MRKR,       // Used to represent fields containing red blocks
    BLUE_MRKR,      // Used to represent fields containing blue blocksA
    GOLD_MRKR,      // Used to represent fields containing gold blocks

    LINE_MRKR,      // Used to represent fields that were marked as filled lines
    GAME_OVER_MRKR, // Used to represent fields at the end of the game

    BOUND_MRKR,     // Though not used within the arena, this tells writeField() to write bounding characters

    TEXT_MRKR       // Though not used within the arena, this tells writeField() to write characters from text
} MARKERS;

typedef enum SHP_MATR_SZS {
    MATR_SZ_TWO = 2,
    MATR_SZ_THREE,
    MATR_SZ_FOUR,
} SHP_MATR_SZS;

typedef struct Piece {
    Pos     pos; // X/Y positions relative to top-left corner of the piece
    uint8_t shape4[4][4];
    uint8_t shape3[3][3];
    uint8_t shape2[2][2];
    SHP_MATR_SZS MATR_SZ;
    SHAPES  type;
    MARKERS color;
} Piece;



typedef enum BUTTONS {
    STRT_LVL = 1,
    NAME,

    SORT_BY,
    SORT_ORDER,
    TOGGLE_LOG,
    VIEW_LOG,
    EXPORT_LOG,

    CLR_LOG,

    STRT,
    HELP,
    QUIT
} BUTTONS;

typedef enum BUTTON_VAL_TYPES {
    BVTYPE_NULL = 0,
    BVTYPE_CHAR,
    BVTYPE_NUM,
    BVTYPE_CYCLE
} BUTTON_VAL_TYPES;

typedef enum SORT_BTN_CYCLES {
    SB_CYCLE_SCORE = 0,
    SB_CYCLE_LINES,
    SB_CYCLE_LEVEL,
    SB_CYCLE_STRT_LVL,
    SB_CYCLE_TETRIS_CNT
} SORT_BTN_CYCLES;

typedef enum SORT_ORD_BTN_CYCLES {
    SOB_CYCLE_DESCENDING = 0,
    SOB_CYCLE_ASCENDING,
} SORT_ORD_BTN_CYCLES;

typedef enum TOGGLE_LOG_BTN_CYCLES {
    TLB_CYCLE_OFF = 0,
    TLB_CYCLE_ON,
} TOGGLE_LOG_BTN_CYCLES;

typedef struct Button {
    Vector*     chars;             // Representation of the input-field
    uint8_t     cycle;             // Used for buttons of type BVTYPE_CYCLE
    uint8_t     cycleLen;          // How many cycles there are
    uint8_t     id;
    const char* label;
    uint8_t     valueType;
    bool        leaveGap;          // If this is true, there will be an extra newline inserted after rendering the button's label
    void        (*onPress)(void*); // Must use void* as the Player type is undefined here
} Button;



typedef struct Player {
    Piece    currPiece;
    Piece    nextPiece;
    Piece    heldPiece;
    bool     gameOver;
    uint64_t score;
    uint8_t  level;
    uint8_t  startLevel;       // Input starting level via main menu
    uint32_t linesCleared;
    uint16_t currLinesCleared; // Local to current level
    uint16_t tetrisCount;

    uint8_t  framesPassed;     // How many frames have elapsed since last second
    uint8_t  speed;            // Frame threshold for game updates

    Piece    bag[PIECE_Z];     // Container to put randomly generated piece types in
    uint8_t  bagLen;           // Tracker to determine whether bag needs to be regenerated

    bool     paused;

    bool     swappedThisRound; // Flag to mark whether the player has swapped between the current and held piece this round

    bool     currToBeFrozen;   // Flag to mark whether the current piece is to be frozen
    uint8_t  lockDelayFrames;  // How many frames have elapsed since the current piece was marked to be frozen
    uint8_t  lockDelay;        // How many frames must pass before the current piece is frozen

    struct timespec lnTimeout; // How long a line clear should freeze the game in tenths of seconds

    bool     helpTextOpen;     // Whether the player is currently looking at the help text

    bool     mainMenuOpen;     // Whether the player is currently in the main menu
    Button   buttons[QUIT];
    uint8_t  buttonCursor;     // Which button the player is currently "hovering" over
    uint8_t  inputCursor;      // Which character the player is currently on within an input
    int8_t   selected;         // Which button the player currently has selected

    char     name[MAX_NME_INPUT_LEN + 1]; // Must add 1 for '\0'

    bool     quit;

    bool     firstLevel;       // If this is true, the threshold for the first level up is shifted

    Vector*  log;
    bool     logOpen;          // Whether the player is currently looking at the highscore logs
    bool     logUnsorted;      // Whether there have been any modifications to the log since it was last sorted

    bool     confirmationOpen; // Whether the player is currently looking at the confirmation screen of a button
} Player;



#define ERR_FORMAT(format, ...) do {                                                   \
    clearterm();                                                                       \
    fprintf(stderr, "\r\n" format "\r\n(%s:%i)\r\n", __VA_ARGS__, __FILE__, __LINE__); \
    fflush(stderr);                                                                    \
    abort();                                                                           \
} while (0)
#define ERR_NOFORMAT(msg) do {                                         \
    clearterm();                                                       \
    fprintf(stderr, "\r\n" msg "\r\n(%s:%i)\r\n", __FILE__, __LINE__); \
    fflush(stderr);                                                    \
    abort();                                                           \
} while (0)

#define MIN(n1, n2) (n1 < n2 ? n1 : n2)
#define MAX(n1, n2) (n1 > n2 ? n1 : n2)

static inline void readInput(Player* player, Vector* ARENA, uint8_t* c);

static inline bool strmtch(const char* str1, const char* str2);
static inline void writeField(char** buff, MARKERS marker, const char* str);

static inline uint8_t lockDelayFormula(Player* player);
static inline struct timespec lnTimeoutFormula(Player* player, uint8_t currLevel);
static inline uint8_t speedFormula(uint8_t currLevel);
static inline uint64_t pieceFreezeFormula();
static inline uint64_t lineClearFormula(double lines, uint8_t currLevel);
static inline uint64_t levelUpFormula(uint8_t levels);

static inline uint8_t getGlobalShapeField(SHP_MATR_SZS MATR_SZ, int p, int i, int j);
static inline void setPieceShapeField(Piece* p, int i, int j, int field);
static inline uint8_t getPieceShapeField(Piece* p, int i, int j);
static inline SHP_MATR_SZS requiredPieceMatrixSize(Piece* p);

static inline void suspend(struct timespec* timeout);

static inline Pos FPOStoAPOS(Piece* p, int row, int col);
static inline Pos APOStoFPOS(Piece* p, int row, int col);

void strToLower(char* to, size_t toSize, char* from, size_t fromSize);

void printHelpText();

void  resetterm();
void  enableraw();
void  clearterm();
static inline void flushstdin(uint8_t* c);

#ifndef _WIN32
    void crash(int sig);
    void setsigs();
#else
    BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
    void setWinHandler();
#endif

Vector* createArena();
void     initPlayer(Player* player);
void          reset(Player* player, Vector* ARENA);
void     freePlayer(Player* player);
void           quit(Player* player, Vector* ARENA);

void renderGame(Player* player, Vector* ARENA);
void   gameOver(Player* player, Vector* ARENA, struct timespec* animationTimeout);
void    levelUp(struct timespec* animationTimeout);

void togglePause(Player* player);
void  toggleHelp(Player* player, Vector* ARENA);
void    goToMenu(Player* player, Vector* ARENA);

// O and J pieces don't have left-padding in their matrix representations
#define PIECE_LEFT_PADDING(type) (type != PIECE_O && type != PIECE_J)

// We must subtract at least 1 as ARENA_COLS / 2 is 1-based, while coordinates are 0-based.
// For pieces with left-padding, there will be a blank space at the left of their shape matrices, requiring the subtraction of 2 instead of 1.
#define PIECE_STRT_XPOS(type) ( ARENA_COLS / 2 - ((PIECE_LEFT_PADDING(type)) ? 2 : 1) )

void       regenBag(Player* player);
void   useNextPiece(Player* player);
Piece   createPiece();
void      initPiece(Piece* p);
void setPieceToNull(Piece* p);

bool pieceCollides(Piece* p, Vector* ARENA);

bool  canMoveDown(Piece* p, Vector* ARENA);
bool canMoveRight(Piece* p, Vector* ARENA);
bool  canMoveLeft(Piece* p, Vector* ARENA);
static inline void  moveDown(Piece* p, Vector* ARENA);
static inline void moveRight(Piece* p, Vector* ARENA);
static inline void  moveLeft(Piece* p, Vector* ARENA);

void   transpose(Piece *p);
void reverseRows(Piece *p);
void       rotCW(Piece *p);
void      rotCCW(Piece *p);

bool wallKick(Piece *p, Vector *ARENA);

bool  canRotateCW(Piece* p, Vector* ARENA);
bool canRotateCCW(Piece* p, Vector* ARENA);
void     rotateCW(Piece* p, Vector* ARENA);
void    rotateCCW(Piece* p, Vector* ARENA);

void swapHeldPiece(Player* player);

void engrainPiece(Piece* p, Vector* ARENA);

bool markLines(Vector* ARENA);
int clearLines(Vector* ARENA);

void renderMenu(Player* player);

static inline void markLogUnsorted(Player* player, Button* btn);
void    moveCursorUp(Player* player);
void  moveCursorDown(Player* player);
void  moveCursorLeft(Player* player);
void moveCursorRight(Player* player);

void selectBtn(Player* player);

void    writeCharToInput(Player* player,    char  c);
void     writeNumToInput(Player* player, uint8_t nc);
void removeCharFromInput(Player* player);

uint32_t uint8VecToInt(Vector* vec);
void       capUint8Vec(Vector* vec, uint8_t cap);

void updateFromInput(Player* player);

static inline void triggerLogSort(Player* player);
void     startBtnPressed(void* player);
void      helpBtnPressed(void* player);
void      quitBtnPressed(void* player);
void   viewLogBtnPressed(void* player);
void exportLogBtnPressed(void* player);
void  clearLogBtnPressed(void* player);

void executeSelected(Player* player);

void    upPressed(Player* player, Vector* ARENA);
void  downPressed(Player* player, Vector* ARENA);
void rightPressed(Player* player, Vector* ARENA);
void  leftPressed(Player* player, Vector* ARENA);

#endif