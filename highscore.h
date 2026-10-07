#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdint.h>
#include "tetris.h"
#define AR ARENA_ROWS // Suppresses unused header warning (the header includes the vector library, which is why we include it here)
#include <stdbool.h>

#define DFLT_NAME "None"

#define INTRN_LOG_FNM "hs.txt"
#define EXTRN_LOG_FNM "highscores.txt"



typedef enum {
    SRT_TYPE_SCORE = 0,
    SRT_TYPE_LINES,
    SRT_TYPE_LEVEL,
    SRT_TYPE_STRTLVL,
    SRT_TYPE_TETRCNT
} SORT_TYPES;

typedef struct {
    const char* name;
    uint64_t    score;
    uint32_t    lines;
    uint8_t     level;
    uint8_t     startLevel;
    uint16_t    tetrisCount;
} LogRow;



HVector* initLog();

void  writeLog(HVector* log);
void exportLog(HVector* log);
void   readLog(HVector* log);

void sortLogVec(HVector* log, SORT_TYPES sortType, bool descending);

void printLog(HVector* log);

void clearLog(HVector* log);
void clearLogFile();

#endif