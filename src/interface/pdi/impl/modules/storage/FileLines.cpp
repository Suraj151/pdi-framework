/******************************** File Lines **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 22nd Oct 2026
******************************************************************************/

#include <config/Config.h>
#include "FileLines.h"

/**
 * Finds the string in a file, storing the offsets of every nth match up to
 * maxindices; how many, or an error code.
 */
int FileLines::findInFile(iFileSystemInterface &fs, const char* path, const char* findStr, pdiutil::vector<uint32_t> *findindices, int maxindices, int everynthindice, int64_t offset, CallBackVoidArgFn yield){

    int foundcount = 0;
    if(findindices) findindices->clear();

    if( nullptr != findStr ){

        int filesize = fs.getFileSize(path);
        if(filesize < 0) return filesize;

        int bytesReadOrErr = 0;
        int64_t endoffset = (offset < 0) ? -offset : -1;
        uint64_t offsetindex = (offset < 0) ? 0 : offset;
        int findstrlen = strlen(findStr);
        bool didmatchfound = false;
        uint64_t chunksizetoread = (endoffset < 0 || endoffset > 250) ? 250 : (uint64_t)endoffset;
        if(maxindices > 0 && everynthindice > 1){
            maxindices = maxindices / everynthindice;
        }

        int nextPushAt = everynthindice;
        if(offset < 0 && everynthindice > 1){
            int foundmatches = findInFile(fs, path, findStr, nullptr, -1, 1, offset, yield);
            nextPushAt = (foundmatches % everynthindice) + 1;
        }

        do{

            bytesReadOrErr = fs.readFile(path, chunksizetoread, [&](char *data, uint32_t size) -> bool {
                return true;
            }, offsetindex, findStr, &didmatchfound);

            if( bytesReadOrErr < 0 ){
                return bytesReadOrErr;
            }

            offsetindex += bytesReadOrErr + 1;
            if( didmatchfound ){
                didmatchfound = false;
                foundcount++;
                if(foundcount == nextPushAt){
                    if(findindices){
                        if(offset < 0 && maxindices > 0 && (int)findindices->size() >= maxindices){
                            findindices->erase(findindices->begin());
                        }
                        findindices->push_back(offsetindex - findstrlen);
                    }
                    nextPushAt += everynthindice;
                }
            }

            if( yield ){
                yield();
            }

            if( endoffset > 0 ){

                if( offsetindex > endoffset ){
                    break;
                }

                if( (offsetindex + chunksizetoread) > endoffset ){
                    chunksizetoread = endoffset - offsetindex + 1;
                }
            }
        }while( offsetindex < filesize && ( maxindices == -1 || offset < 0 || (findindices ? findindices->size() : foundcount) < maxindices  ) );
    }else{
        return PDI_ERR_NULL_PTR;
    }

    return foundcount;
}

/**
 * The offset at which a line number starts in a file, or a negative error code.
 */
int64_t FileLines::getOffsetFromLineNumber(iFileSystemInterface &fs, const char* path, int linenumber, CallBackVoidArgFn yield){

    int64_t offset = 0;

    if( linenumber < 0 ){

        uint32_t totalnumberoflines = 0;
        int iStatus = fs.readFile(path, 250, [&](char *data, uint32_t size)->bool{
            for(uint32_t i = 0; i < size; i++){
                if(data[i] == '\n') totalnumberoflines++;
            }
            return true;
        });
        if( iStatus < 0 ) return iStatus;

        int64_t filesize = fs.getFileSize(path);
        if( filesize > 0 ){
            char lastChar = '\n';
            fs.readFile(path, 1, [&](char *data, uint32_t size) -> bool {
                if(size > 0) lastChar = data[0];
                return true;
            }, (uint64_t)(filesize - 1));
            if( lastChar != '\n' ){
                totalnumberoflines++;
            }
        }

        linenumber = totalnumberoflines + linenumber;
        if( linenumber < 0 ) return PDI_ERR_RANGE;
    }

    if( linenumber > 0 ){

        int newlinesFound = 0;

        int iStatus = fs.readFile(path, 250, [&](char *data, uint32_t size)->bool{

            for(uint32_t i = 0; i < size; i++){

                if(data[i] == '\n') {

                    newlinesFound++;

                    if(newlinesFound >= linenumber){

                        offset += i + 1;
                        return false;
                    }
                }
            }

            offset += size;

            if(yield){
                yield();
            }

            return true;
        });

        if( iStatus < 0 ) return iStatus;
    }

    return offset;
}

/**
 * The line number holding an offset in a file, or a negative error code.
 */
int64_t FileLines::getLineNumberFromOffset(iFileSystemInterface &fs, const char* path, int64_t offset, CallBackVoidArgFn yield){

    int64_t linenumber = 0;
    int64_t filesize = fs.getFileSize(path);
    if( filesize < 0 ){
        return filesize;
    }

    if( offset < 0 ){
        offset = filesize + offset;
    }

    if( offset > 0 ){

        int64_t offsetcount = 0;
        int iStatus = fs.readFile(path, 250, [&](char *data, uint32_t size)->bool{

            for(uint32_t i = 0; i < size; i++){

                if(offset <= (offsetcount + i)){
                    return false;
                }

                if(data[i] == '\n') {
                    linenumber++;
                }
            }

            offsetcount += size;

            if(yield){
                yield();
            }

            return true;
        });
        if( iStatus < 0 ) return iStatus;
    }

    return linenumber;
}

/**
 * Counts the lines in a file, storing up to maxlinenumbers line offsets; how many,
 * or a negative error code.
 */
int FileLines::getLineNumbersInFile(iFileSystemInterface &fs, const char* path, pdiutil::vector<uint32_t> &linenumberindices, int maxlinenumbers, int linenumberoffset, CallBackVoidArgFn yield){

    int64_t offset = getOffsetFromLineNumber(fs, path, linenumberoffset, nullptr);

    if( offset < 0 ){
        offset = 0;
    }

    int status = findInFile(fs, path, "\n", &linenumberindices, maxlinenumbers, 1, offset, yield);

    if( status < 0 ){
        return status;
    }

    if( linenumberindices.size() > 0 ){

        int64_t filesize = fs.getFileSize(path);
        linenumberindices.insert(linenumberindices.begin(), (uint32_t)offset);

        for (size_t i = 1; i < linenumberindices.size(); i++){
            linenumberindices[i] = linenumberindices[i] + 1;
        }

        if( (linenumberindices.back() + 1) > (uint32_t)filesize ){
            linenumberindices.pop_back();
        }
    }

    return linenumberindices.size();
}

/**
 * Reads a line of a file, negative numbers counting from the end, matched to
 * pattern when given; the bytes read.
 */
int FileLines::readLineInFile(iFileSystemInterface &fs, const char* path, int32_t linenumber, pdiutil::string &linedata, const char* pattern, CallBackVoidArgFn yield){

    linedata.clear();

    int64_t filesize = fs.getFileSize(path);
    if(filesize < 0) return (int)filesize;

    int64_t lineoffset = -1;

    if(pattern == nullptr){

        lineoffset = getOffsetFromLineNumber(fs, path, linenumber, yield);
        if(lineoffset < 0) return (int)lineoffset;
        if(lineoffset >= filesize) return PDI_ERR_NOT_FOUND;

    }else{

        const int CAP = (linenumber > 0) ? linenumber : ((linenumber < 0) ? -linenumber : 1);

        pdiutil::vector<uint32_t> patternindices;
        int rc = findInFile(fs, path, pattern, &patternindices, CAP, CAP,
                            (linenumber < 0) ? -filesize : 0, yield);
        if(rc < 0) return rc;
        if(patternindices.size() == 0) return PDI_ERR_NOT_FOUND;

        int64_t anchorLine = getLineNumberFromOffset(fs, path, patternindices[0], yield);
        if(anchorLine < 0) return (int)anchorLine;

        const int WINDOW = 50;
        int windowStartLine = (int)anchorLine - WINDOW;
        if(windowStartLine < 0) windowStartLine = 0;

        pdiutil::vector<uint32_t> linenumbersvec;
        rc = getLineNumbersInFile(fs, path, linenumbersvec, 2 * WINDOW + 1, windowStartLine, yield);
        if(rc < 0) return rc;
        if(linenumbersvec.size() == 0) return PDI_ERR_NOT_FOUND;

        pdiutil::vector<uint32_t> winpatternindices;
        rc = findInFile(fs, path, pattern, &winpatternindices, -1, 1, (int64_t)linenumbersvec[0], yield);
        if(rc < 0) return rc;

        pdiutil::vector<uint32_t> filtered;
        for(size_t i = 0; i < linenumbersvec.size(); i++){
            uint32_t lineStart = linenumbersvec[i];
            uint32_t lineEnd = (i+1 < linenumbersvec.size()) ? linenumbersvec[i+1] : (uint32_t)filesize;
            for(size_t j = 0; j < winpatternindices.size(); j++){
                if(winpatternindices[j] >= lineStart && winpatternindices[j] < lineEnd){
                    filtered.push_back(lineStart);
                    break;
                }
            }
        }

        if(filtered.size() == 0) return PDI_ERR_NOT_FOUND;

        int32_t idx = (linenumber < 0) ? ((int32_t)filtered.size() + linenumber) : linenumber;
        if(idx < 0 || (size_t)idx >= filtered.size()) return PDI_ERR_NOT_FOUND;

        lineoffset = (int64_t)filtered[idx];
        if(lineoffset >= filesize) return PDI_ERR_NOT_FOUND;
    }

    int bytesReadedOrError = fs.readFile(path, 250, [&](char *data, uint32_t size) -> bool {
        linedata += pdiutil::string(data, size);
        return true;
    }, (uint64_t)lineoffset, "\n");

    if( !linedata.empty() && linedata.back() == '\r' ){
        linedata.pop_back();
        bytesReadedOrError--;
    }

    return bytesReadedOrError;
}
