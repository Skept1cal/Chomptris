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

#define LINE_NUM_PAD   7 // How many columns the line numbering when exporting and printing the logs must shift the rest of the row



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

static uint64_t strToUInt(const char *str);

static void tokenizeLine(char* restrict seg, size_t segSize, const char* restrict line, int* linePos);

static void formatLogRow(char* to, size_t toSize, Vector* log, int ind);

static void xorStr(char* str, int* keyPos);



static void freeLogRow(void* logRow) {
    if (!logRow) return;

    free((char*)( (LogRow*)(logRow) )->name);
    free(logRow);
}



static uint64_t strToUInt(const char* str) {
    uint64_t num = 0;
    uint8_t  len = strlen(str);

    for (int i = 0; i < len && str[i] != '.'; i++) {
        if ( i == 0 && (str[i] == '+' || str[i] == '-') ) continue;
        else if (str[i] < '0' || str[i] > '9') break;

        num = num * 10 + str[i] - '0';
    }

    return num;
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



static void formatLogRow(char* to, size_t toSize, Vector* log, int ind) {
    LogRow* row = log->arr[ind];

    uint8_t padding = LINE_NUM_PAD - (uint8_t)log10(ind + 1);

    snprintf(
        to,
        toSize,
        "%i.%*sName: %*s; Score: %0*lu; Lines: %0*i; Level: %0*i; Starting level: %0*i; Tetrises: %0*i%s",
        ind + 1,
        padding, "",
        MAX_NME_INPUT_LEN, row->name,
              SCORE_WIDTH, row->score,
              LINES_WIDTH, row->lines,
              LEVEL_WIDTH, row->level,
           STRT_LVL_WIDTH, row->startLevel,
           TETR_CNT_WIDTH, row->tetrisCount,
        (ind < log->len - 1) ? "\r\n" : ""
    );
}



static void xorStr(char* str, int* keyPos) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] ^= XOR_KEY[(*keyPos)++];
        if (*keyPos == sizeof(XOR_KEY) / sizeof(*XOR_KEY)) *keyPos = 0;
    }
}



Vector* initLog() {
    Vector* log = crtvec(0, DTYPE_STRUCT, freeLogRow);
    return log;
}



void writeLog(Vector* log) {
    // We first save a temporary copy of the current data,
    // then overwrite the original file with new data,
    // to prevent potential losses.

    FILE* f;

    if ( (f = fopen(INTRN_LOG_FNM, "rb")) ) {
        fseek(f, 0L, SEEK_END);
        long bytes = ftell(f);
        if (bytes > 0L) {
            FILE* tmp = fopen(TEMP_FNM, "wb");
            if (!tmp) {
                fclose(f);
                return;
            }

            rewind(f);
            
            uint8_t* tempBuff = calloc(bytes, sizeof(uint8_t));
            if (!tempBuff) {
                fclose(tmp);
                fclose(f);
                return;
            }
             fread(tempBuff, sizeof(uint8_t), bytes,   f);
            fwrite(tempBuff, sizeof(uint8_t), bytes, tmp);

            fclose(tmp);
            free(tempBuff);
        }

        fclose(f);
    }
    
    if ( !(f = fopen(INTRN_LOG_FNM, "wb")) ) return;

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

        xorStr(rowBuff, &keyPos);

        fwrite(rowBuff, sizeof(*rowBuff), len, f);
    }

    fclose(f);
    remove(TEMP_FNM);
}

void exportLog(Vector* log) {
    FILE *f = fopen(EXTRN_LOG_FNM, "wb");
    if (!f) return;

    for (int i = 0; i < log->len; i++) {
        size_t alloc = 256 * sizeof(char);
        char*  to    = malloc(alloc);

        formatLogRow(to, alloc, log, i);
        fputs(to, f);
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
    int keyPos = 0;
    xorStr((char*)txt, &keyPos);

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



void sortLogVec(Vector* log, SORT_TYPES sortType, bool descending) {
    for (int i = 0; i < log->len; i++) {
        if (!log->arr[i]) continue;

        LogRow*  highestLowest    = log->arr[i];
        uint32_t highestLowestInd = i;
        for (int j = i; j < log->len; j++) {
            if (!log->arr[j]) continue;

            uint64_t currVal;
            uint64_t compVal;

            LogRow* currRow = log->arr[j];

            switch (sortType) {
                case SRT_TYPE_SCORE:   currVal = currRow->score;       compVal = highestLowest->score;       break;
                case SRT_TYPE_LINES:   currVal = currRow->lines;       compVal = highestLowest->lines;       break;
                case SRT_TYPE_LEVEL:   currVal = currRow->level;       compVal = highestLowest->level;       break;
                case SRT_TYPE_STRTLVL: currVal = currRow->startLevel;  compVal = highestLowest->startLevel;  break;
                case SRT_TYPE_TETRCNT: currVal = currRow->tetrisCount; compVal = highestLowest->tetrisCount; break;
            }

            if ((descending) ? (currVal >= compVal) : (currVal <= compVal)) {
                highestLowest = currRow;
                highestLowestInd = j;
            }
        }

        LogRow* temp = log->arr[i];
        log->arr[i] = highestLowest;
        log->arr[highestLowestInd] = temp;
    }
}



void printLog(Vector* log) {
    clearterm();

    for (int i = 0; i < log->len; i++) {
        size_t alloc = 256 * sizeof(char);
        char*  to    = malloc(alloc);

        formatLogRow(to, alloc, log, i);
        fputs(to, stdout);
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