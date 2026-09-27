#include "tetris.h"
#include "highscore.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>



#define TEMP_FNM      "./temp.txt"
#define COL_SEPARATOR '\x04'
#define COL_SEP_STR   "\x04"

#define SCORE_WIDTH   10 // How many digits the score must take up. The number is padded with zeros if it's too small
#define LINES_WIDTH    6 // How many digits the lines must take up
#define LEVEL_WIDTH    3 // How many digits the level must take up
#define STRT_LVL_WIDTH 3 // How many digits the starting level must take up
#define TETR_CNT_WIDTH 5 // How many digits the number of tetrises must take up



static const uint8_t XOR_KEY[] = {
    0b10100101,
    0b00100001,
    0b10101001,
    0b01000100,
    0b11010010,
    0b10110001,
    0b01100100,
    0b11001010,
};



static void freeLogRow(void *logRow);

static uint32_t strToUInt(const char *str);

static void deepcopyRow(LogRow* restrict to, const LogRow* restrict from);



static void freeLogRow(void* logRow) {
    if (!logRow) return;

    free((char*)( (LogRow*)(logRow) )->name);
    free(logRow);
}



static uint32_t strToUInt(const char* str) {
    uint32_t num = 0;
    uint8_t  len = strlen(str);

    for (int i = 0; i < len && str[i] != '.'; i++) {
        if ( i == 0 && (str[i] == '+' || str[i] == '-') ) continue;
        else if (str[i] < '0' || str[i] > '9') break;

        num += (str[i] - '0') * pow(10, len-1 - i);
    }

    return num;
}



static void deepcopyRow(LogRow* restrict to, const LogRow* restrict from) {
    *to = (LogRow){
        .name        = strdup(from->name),
        .score       = from->score,
        .lines       = from->lines,
        .level       = from->level,
        .startLevel  = from->startLevel,
        .tetrisCount = from->tetrisCount
    };
}



static void tokenizeLine(char* restrict seg, size_t segSize, const char* restrict line, int* linePos) {
    int segPos = 0;
    for (
        ;
        segPos < segSize                &&
        line[*linePos] != '\0'          &&
        line[*linePos] != COL_SEPARATOR &&
        line[*linePos] != '\r'          &&
        line[*linePos] != '\n';
        segPos++, (*linePos)++
    ) {
        seg[segPos] = line[*linePos];
    }
    seg[segPos] = '\0';

    (*linePos)++; // Must shift the index forward to skip the delimiter
}



Vector* initLog() {
    Vector* log = crtvec(0, DTYPE_STRUCT, freeLogRow);
    return log;
}



void writeLog(Vector* log) {
    // We first save a temporary copy of the current data,
    // then overwrite the original file with new data,
    // to prevent potential losses.

    FILE* f = fopen(INTRN_LOG_FNM, "rb");

    if (!f) {
        f = fopen(INTRN_LOG_FNM, "wb");
        fclose(f);
        return;
    }

    FILE* tmp = fopen(TEMP_FNM, "wb");
    f = fopen(INTRN_LOG_FNM, "rb");

    if (!tmp || !f) {
        if (tmp) {
            fclose(tmp);
            remove(TEMP_FNM);
        }
        if (f) fclose(f);
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t bytes = ftell(f);
    rewind(f);

    uint8_t* tempBuff = malloc(bytes);

     fread(tempBuff, sizeof(uint8_t), bytes, f);
    fwrite(tempBuff, sizeof(uint8_t), bytes, tmp);

    fclose(tmp);
    fclose(f);



    f = fopen(INTRN_LOG_FNM, "wb");
    if (!f) return;

    char rowBuff[128] = {0};
    for (int i = 0, keyPos = 0; i < log->len; i++) {
        LogRow* row = log->arr[i];

        snprintf(
            rowBuff,
            sizeof(rowBuff),
            "%s%c%lu%c%i%c%i%c%i%c%i%c%s",
            (row->name[0]) ? row->name : DFLT_NAME, COL_SEPARATOR,
            row->score,       COL_SEPARATOR,
            row->lines,       COL_SEPARATOR,
            row->level,       COL_SEPARATOR,
            row->startLevel,  COL_SEPARATOR,
            row->tetrisCount, COL_SEPARATOR,
            (i < log->len - 1) ? "\r\n" : ""
        );

        size_t len = strlen(rowBuff);

        for (int j = 0; rowBuff[j] != '\0'; j++) {
            rowBuff[j] ^= XOR_KEY[keyPos++];
            if (keyPos == sizeof(XOR_KEY) / sizeof(*XOR_KEY)) keyPos = 0;
        }

        fwrite(rowBuff, sizeof(*rowBuff), len, f);
    }

    fclose(f);
    remove(TEMP_FNM);
}

void exportLog(Vector* log) {
    FILE *f = fopen(EXTRN_LOG_FNM, "wb");
    if (!f) return;

    for (int i = 0; i < log->len; i++) {
        LogRow* row = log->arr[i];

        fprintf(
            f,
            "%i.   Name: %*s; Score: %0*lu; Lines: %0*i; Level: %0*i; Starting level: %0*i; Tetrises: %0*i%s",
            i + 1,
            MAX_NME_INPUT_LEN, row->name,
                  SCORE_WIDTH, row->score,
                  LINES_WIDTH, row->lines,
                  LEVEL_WIDTH, row->level,
               STRT_LVL_WIDTH, row->startLevel,
               TETR_CNT_WIDTH, row->tetrisCount,
            (i < log->len - 1) ? "\r\n" : ""
        );
    }

    fclose(f);
}

void readLog(Vector* log) {
    // We first decrypt the data, then transfer it to a temporary file.
    // This allows the usage of fgets() due to the presence of newlines now separating each log.

    clearLog(log);

    FILE* f = fopen(INTRN_LOG_FNM, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    size_t bytes = ftell(f);
    rewind(f);

    uint8_t txt[bytes];

    fread(txt, sizeof(*txt), bytes, f);
    for (int i = 0, keyPos = 0; i < bytes; i++) {
        txt[i] ^= XOR_KEY[keyPos++];
        if (keyPos == sizeof(XOR_KEY) / sizeof(*XOR_KEY)) keyPos = 0;
    }

    fclose(f);

    FILE* tmp = fopen(TEMP_FNM, "wb");
    if (!tmp) return;

    fwrite(txt, sizeof(*txt), bytes, tmp);
    fclose(tmp);



    tmp = fopen(TEMP_FNM, "rb");
    if (!tmp) return;

    char  seg[32]  = {0};
    char line[128] = {0};

    while (fgets(line, sizeof(line), tmp)) {
        LogRow* row = malloc(sizeof(LogRow));
               *row = (LogRow){0};

        int linePos = 0;
        size_t segSize = sizeof(seg) / sizeof(seg[0]);

        tokenizeLine(seg, segSize, line, &linePos);
        row->name = strdup( (seg[0]) ? seg : DFLT_NAME );
        memset(seg, 0, segSize);

        tokenizeLine(seg, segSize, line, &linePos);
        row->score = strToUInt(seg);
        memset(seg, 0, segSize);

        tokenizeLine(seg, segSize, line, &linePos);
        row->lines = strToUInt(seg);
        memset(seg, 0, segSize);

        tokenizeLine(seg, segSize, line, &linePos);
        row->level = strToUInt(seg);
        memset(seg, 0, segSize);

        tokenizeLine(seg, segSize, line, &linePos);
        row->startLevel = strToUInt(seg);
        memset(seg, 0, segSize);

        tokenizeLine(seg, segSize, line, &linePos);
        row->tetrisCount = strToUInt(seg);

        vecpush(log, row);
    }

    fclose(tmp);
    remove(TEMP_FNM);
}



void sortLogVec(Vector** log, SORT_TYPES sortType) {
    Vector* sorted = initLog();

    while ((*log)->len > 0) {
        LogRow* highest = malloc(sizeof(LogRow));
               *highest = (LogRow){0};

        uint32_t highestInd = 0;
        
        for (int i = 0; i < (*log)->len; i++) {
            if (!(*log)->arr[i]) continue;

            uint64_t currVal;
            uint64_t compVal;

            LogRow* currRow = (*log)->arr[i];

            switch (sortType) {
                case SRT_TYPE_SCORE:   currVal = currRow->score;       compVal = highest->score;       break;
                case SRT_TYPE_LINES:   currVal = currRow->lines;       compVal = highest->lines;       break;
                case SRT_TYPE_LEVEL:   currVal = currRow->level;       compVal = highest->level;       break;
                case SRT_TYPE_STRTLVL: currVal = currRow->startLevel;  compVal = highest->startLevel;  break;
                case SRT_TYPE_TETRCNT: currVal = currRow->tetrisCount; compVal = highest->tetrisCount; break;
            }

            if (currVal >= compVal) {
                // We must free the name because 'highest' can be copied to multiple times and we'd leave behind the previously
                // duplicated name pointers, causing a leak
                free((void*)highest->name);
                deepcopyRow(highest, currRow);
                highestInd = i;
            }
        }

        vecpush(sorted, highest);
        vecrmv(*log, highestInd);
    }

    freevec(*log);
    *log = sorted;
}



void printLog(Vector* log) {
    clearterm();

    for (int i = 0; i < log->len; i++) {
        LogRow* row = log->arr[i];

        printf(
            "%i.\x1b[3CName: %*s; Score: %0*lu; Lines: %0*i; Level: %0*i; Starting level: %0*i; Tetrises: %0*i\r\n",
            i + 1,
            MAX_NME_INPUT_LEN, row->name,
                  SCORE_WIDTH, row->score,
                  LINES_WIDTH, row->lines,
                  LEVEL_WIDTH, row->level,
               STRT_LVL_WIDTH, row->startLevel,
               TETR_CNT_WIDTH, row->tetrisCount
        );
    }

    fflush(stdout);
}



void clearLog(Vector* log) {
    for (int i = 0, vecLen = log->len; i < vecLen; i++) {
        vecpop(log);
    }
}

void clearLogFile() {
    FILE* f = fopen(INTRN_LOG_FNM, "rb");
    if (!f) return;
    fclose(f);
    f = fopen(INTRN_LOG_FNM, "wb");
    if (f) fclose(f);
}