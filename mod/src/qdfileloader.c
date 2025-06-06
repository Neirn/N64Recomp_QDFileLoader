#include "global.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "string.h"

// 64 MiB should be more than enough
// MM Recomp only has 512 MiB of RAM
#define QDFL_MAX_FILE_SIZE 67108864

// enum must be kept in sync with qdfileloader_api.h !!!
typedef enum {
    QDFL_STATUS_OK,
    QDFL_STATUS_ERR_UNKNOWN,
    QDFL_STATUS_ERR_PATH_NO_EXIST,
    QDFL_STATUS_ERR_FILE_NO_EXIST,
    QDFL_STATUS_ERR_DIR_NO_EXIST,
    QDFL_STATUS_EXPECTED_FILE,
    QDFL_STATUS_EXPECTED_DIR,
    QDFL_STATUS_ERR_FILE_TOO_LARGE,
    QDFL_STATUS_ERR_READ_FILE,
} QDFL_Status;

RECOMP_IMPORT(".", bool QDFL_N_isExist(const char *path));
RECOMP_IMPORT(".", bool QDFL_N_isFile(const char *path));
RECOMP_IMPORT(".", bool QDFL_N_isDirectory(const char *path));
RECOMP_IMPORT(".", u32 QDFL_N_getFileSize(const char *path));
RECOMP_IMPORT(".", bool QDFL_N_copyToBuffer(const char *path, u8 buffer[], u32 bufferSize));
RECOMP_IMPORT(".", u32 QDFL_N_getNumDirEntries(const char *dirPath));
RECOMP_IMPORT(".", u32 QDFL_N_getDirEntryNameLengthByIndex(const char *dirPath, u32 index));
RECOMP_IMPORT(".", bool QDFL_N_getDirEntryNameByIndex(const char *dirPath, u32 index, u8 *bufferPtr, u32 bufferSize));

void logStatus(const char *path, QDFL_Status s) {
    switch (s) {
        case QDFL_STATUS_OK:
            recomp_printf("QDFL_STATUS_OK: SimpleFileLoader operation completed successfully.");
            break;

        case QDFL_STATUS_ERR_UNKNOWN:
            recomp_printf("QDFL_STATUS_ERR_UNKNOWN: Unspecified error ocurred while processing %s.\n", path);
            break;

        case QDFL_STATUS_ERR_PATH_NO_EXIST:
            recomp_printf("QDFL_STATUS_ERR_PATH_NO_EXIST: Could not open file or folder at path %s\n", path);
            break;

        case QDFL_STATUS_ERR_FILE_NO_EXIST:
            recomp_printf("QDFL_STATUS_ERR_FILE_NO_EXIST: Could not find file at %s\n", path);
            break;

        case QDFL_STATUS_ERR_DIR_NO_EXIST:
            recomp_printf("QDFL_STATUS_ERR_DIR_NO_EXIST: Could not find dir at %s\n", path);
            break;

        case QDFL_STATUS_EXPECTED_FILE:
            recomp_printf("QDFL_STATUS_EXPECTED_FILE: Expected a file, but %s is not a file\n", path);
            break;

        case QDFL_STATUS_EXPECTED_DIR:
            recomp_printf("QDFL_STATUS_EXPECTED_DIR: Expected a directory, but %s is not a directory\n", path);
            break;

        case QDFL_STATUS_ERR_FILE_TOO_LARGE:
            recomp_printf("QDFL_STATUS_ERR_FILE_TOO_LARGE: %s is too large! Files are limited %.2f MiB or less.\n", path, QDFL_MAX_FILE_SIZE / 1024.0 / 1024.0);
            break;
        
        case QDFL_STATUS_ERR_READ_FILE:
            recomp_printf("QDFL_STATUS_ERR_READ_FILE: Error occurred while reading file %s.\n", path);
            break;

        default:
            recomp_printf("QDFL_STATUS_UNRECOGNIZED_CODE: Unimplemented QDFL_STATUS. Maybe something went wrong while processing %s, maybe not.\n", path);
            break;
    }
}

RECOMP_EXPORT int QDFL_isExist(const char *path) {
    return QDFL_N_isExist(path) ? 1 : 0;
}

RECOMP_EXPORT int QDFL_isFile(const char *path) {
    return QDFL_N_isExist(path) ? 1 : 0;
}

RECOMP_EXPORT int QDFL_isDirectory(const char *path) {
    return QDFL_N_isDirectory(path) ? 1 : 0;
}

RECOMP_EXPORT QDFL_Status QDFL_getFileSize(const char *path, unsigned long *out) {
    if (!QDFL_isExist(path)) {
        logStatus(path, QDFL_STATUS_ERR_PATH_NO_EXIST);
        return QDFL_STATUS_ERR_PATH_NO_EXIST;
    }

    if (!QDFL_isFile(path)) {
        logStatus(path, QDFL_STATUS_EXPECTED_FILE);
        return QDFL_STATUS_EXPECTED_FILE;
    }

    *out = QDFL_N_getFileSize(path);

    return QDFL_STATUS_OK;
}

RECOMP_EXPORT QDFL_Status QDFL_loadFile(const char *path, void **out) {
    u32 fileSize;
    QDFL_Status status = QDFL_getFileSize(path, &fileSize);

    if (status != QDFL_STATUS_OK) {
        logStatus(path, status);
        return status;
    }

    u8 *file = NULL;

    file = recomp_alloc(fileSize);

    if (QDFL_N_copyToBuffer(path, file, fileSize)) {
        *out = file;

        return QDFL_STATUS_OK;
    }
    else {
        *out = NULL;

        return QDFL_STATUS_ERR_READ_FILE;
    }
}

RECOMP_EXPORT QDFL_Status QDFL_getNumDirEntries(const char *dirPath, unsigned long *out) {
    if (!QDFL_isExist(dirPath)) {
        logStatus(dirPath, QDFL_STATUS_ERR_PATH_NO_EXIST);
        return QDFL_STATUS_ERR_PATH_NO_EXIST;
    }

    if (!QDFL_isDirectory(dirPath)) {
        logStatus(dirPath, QDFL_STATUS_EXPECTED_DIR);
        return QDFL_STATUS_EXPECTED_DIR;
    }

    *out = QDFL_N_getNumDirEntries(dirPath);

    return QDFL_STATUS_OK;
}

RECOMP_EXPORT QDFL_Status QDFL_getDirEntryNameByIndex(const char *dirPath, unsigned long index, char **out) {
    if (!QDFL_isExist(dirPath)) {
        logStatus(dirPath, QDFL_STATUS_ERR_PATH_NO_EXIST);
        return QDFL_STATUS_ERR_PATH_NO_EXIST;
    }

    if (!QDFL_isDirectory(dirPath)) {
        logStatus(dirPath, QDFL_STATUS_EXPECTED_DIR);
        return QDFL_STATUS_EXPECTED_DIR;
    }

    size_t entryLength = QDFL_N_getDirEntryNameLengthByIndex(dirPath, index) + 1;
    char *entryName = NULL;

    if (entryLength > 0) {
        entryName = recomp_alloc(entryLength * sizeof(char));
        if (!QDFL_N_getDirEntryNameByIndex(dirPath, index, (u8 *)entryName, entryLength)) {
            logStatus(dirPath, QDFL_STATUS_ERR_PATH_NO_EXIST);
            return QDFL_STATUS_ERR_PATH_NO_EXIST;
        }
    }

    *out = entryName;
    return QDFL_STATUS_OK;
}

typedef struct {
    char *str;
    size_t length;
} StringInfo;

RECOMP_EXPORT char *QDFL_getCombinedPath(unsigned long count, ...) {

    char *combinedPath = NULL;

    size_t finalPathLen = 0;

    StringInfo *pathStrings = recomp_alloc(count * sizeof(StringInfo));

    va_list args;
    va_start(args, count);

    for (u32 i = 0; i < count; ++i) {
        StringInfo *strInfo = &pathStrings[i];
        strInfo->str = NULL;
        strInfo->length = 0;

        strInfo->str = va_arg(args, char *);

        strInfo->length = strlen(strInfo->str);

        finalPathLen += strInfo->length;
    }

    va_end(args);

    // make room for dir seperators
    finalPathLen += count - 1;

    // room for null byte
    ++finalPathLen;

    combinedPath = recomp_alloc(finalPathLen * sizeof(char));
    char *pos = combinedPath;

    for (size_t i = 0; i < count; ++i) {
        StringInfo *strInfo = &pathStrings[i];

        for (size_t j = 0; j < strInfo->length; ++j) {
            *pos = strInfo->str[j];
            ++pos;
        }

        // add directory seperators between path names
        if (i < count - 1) {
            // Windows/Mac/Linux all support forward slash dir seperator
            // so there's no need to handle back slash
            pos[0] = '/';
            ++pos;
        }
    }

    combinedPath[finalPathLen - 1] = '\0';

    recomp_free(pathStrings);

    return combinedPath;
}
