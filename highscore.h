#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdint.h>
#include "vector/vector.h"

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



Vector* initLog();

void  writeLog(Vector* log);
void exportLog(Vector* log);
void   readLog(Vector* log);

void sortLogVec(Vector** log, SORT_TYPES sortType);

void printLog(Vector* log);

void clearLog(Vector* log);
void clearLogFile();

#endif