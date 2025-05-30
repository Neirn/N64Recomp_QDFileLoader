#include "helpers.hpp"
#include "mod_recomp.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool copyToRdramBuffer(uint8_t *rdram, const std::string &dirPath, PTR(char) bufferPtr, uint32_t bufferSize) {
    auto path = fs::weakly_canonical(dirPath);

    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    auto fileSize = fs::file_size(path);

    if (bufferSize < fileSize) {
        return false;
    }

    std::ifstream file(dirPath, std::ios::binary);

    if (file.bad()) {
        return false;
    }

    std::istreambuf_iterator<char> fileStart{file}, fileEnd;

    std::vector<char> fileBytes(fileStart, fileEnd);

    // unlikely scenario where file increased between getting size and reading it fully
    auto realFileSize = fileBytes.size();

    if (realFileSize > bufferSize) {
        return false;
    }

    for (size_t i = 0; i < realFileSize; i++) {
        MEM_B(bufferPtr, i) = fileBytes[i];
    }

    return true;
}

std::vector<std::string> getSortedDirEntryRelativeNames(fs::path dirPath) {
    std::vector<std::string> v;

    if (fs::is_directory(dirPath)) {
        for (const auto &entry : fs::recursive_directory_iterator(dirPath)) {
            v.push_back(fs::relative(entry.path(), dirPath).u8string());
        }
    }

    std::sort(v.begin(), v.end());

    return v;
}

extern "C" {

DLLEXPORT uint32_t recomp_api_version = 1;

DLLEXPORT void QDFL_N_isExist(uint8_t *rdram, recomp_context *ctx) {
    _return(ctx, fs::exists(fs::weakly_canonical(_arg_string<0>(rdram, ctx))));
}

DLLEXPORT void QDFL_N_isFile(uint8_t *rdram, recomp_context *ctx) {
    _return(ctx, std::filesystem::is_regular_file(fs::weakly_canonical(_arg_string<0>(rdram, ctx))));
}

DLLEXPORT void QDFL_N_isDirectory(uint8_t *rdram, recomp_context *ctx) {
    _return(ctx, fs::is_directory(fs::weakly_canonical(_arg_string<0>(rdram, ctx))));
}

DLLEXPORT void QDFL_N_getFileSize(uint8_t *rdram, recomp_context *ctx) {
    auto path = fs::weakly_canonical(_arg_string<0>(rdram, ctx));

    if (!fs::is_regular_file(path)) {
        _return(ctx, 0);
        return;
    }

    _return(ctx, U32(fs::file_size(path)));
}

DLLEXPORT void QDFL_N_copyToBuffer(uint8_t *rdram, recomp_context *ctx) {
    auto path = fs::weakly_canonical(_arg_string<0>(rdram, ctx));
    auto bufferPtr = _arg<1, PTR(char)>(rdram, ctx);
    auto bufferSize = _arg<2, uint32_t>(rdram, ctx);

    _return(ctx, copyToRdramBuffer(rdram, _arg_string<0>(rdram, ctx), bufferPtr, bufferSize));
}

DLLEXPORT void QDFL_N_getDirEntryNameLengthByIndex(uint8_t *rdram, recomp_context *ctx) {
    auto directoryPath = fs::weakly_canonical(_arg_string<0>(rdram, ctx));
    auto targetIndex = _arg<1, uint32_t>(rdram, ctx);

    if (!fs::is_directory(directoryPath)) {
        _return<uint32_t>(ctx, 0);
        return;
    }

    auto entryNames = getSortedDirEntryRelativeNames(directoryPath);

    if (targetIndex < entryNames.size()) {
        _return<uint32_t>(ctx, entryNames[targetIndex].length());
        return;
    }

    _return<uint32_t>(ctx, 0);
}

DLLEXPORT void QDFL_N_getDirEntryNameByIndex(uint8_t *rdram, recomp_context *ctx) {
    auto directoryPath = fs::weakly_canonical(_arg_string<0>(rdram, ctx));
    auto targetIndex = _arg<1, uint32_t>(rdram, ctx);
    auto bufferPtr = _arg<2, PTR(char)>(rdram, ctx);
    auto bufferSize = _arg<3, uint32_t>(rdram, ctx);

    if (!fs::is_directory(directoryPath)) {
        _return(ctx, false);
        return;
    }

    auto entryNames = getSortedDirEntryRelativeNames(directoryPath);

    if (targetIndex < entryNames.size()) {
        std::string &entryName = entryNames[targetIndex];

        // get the smaller of the space value and make room for null byte
        uint32_t maxLen = (entryName.size() < bufferSize ? entryName.size() : bufferSize);

        for (size_t i = 0; i < maxLen; i++) {
            MEM_B(bufferPtr, i) = entryName[i];
        }

        size_t terminatorIndex;
        if (maxLen < bufferSize) {
            terminatorIndex = maxLen;
        } else {
            terminatorIndex = bufferSize - 1;
        }

        MEM_B(bufferPtr, terminatorIndex) = '\0';
        _return<bool>(ctx, true);
        return;
    }

    _return<bool>(ctx, false);
}

DLLEXPORT void QDFL_N_getNumDirEntries(uint8_t *rdram, recomp_context *ctx) {
    auto directoryPath = fs::weakly_canonical(_arg_string<0>(rdram, ctx));

    if (!fs::is_directory(directoryPath)) {
        _return<uint32_t>(ctx, 0);
        return;
    }

    fs::recursive_directory_iterator start(directoryPath), end;

    _return<uint32_t>(ctx, std::distance(start, end));
}
}
