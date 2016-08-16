//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/nap.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
crash(int sig) {
        printf("crashed!\n");
        exit(sig);
}

int main()
{
    (void) signal(SIGABRT,crash);

    std::string firstName = ArchMakeTmpFileName("archFS");
    FILE *firstFile;

    char const * const testContent = "text in a file";

    // Open a file, check that its length is 0, write to it, close it, and then
    // check that its length is now the number of characters written.
    assert((firstFile = fopen(firstName.c_str(), "wb")) != NULL);
    fflush(firstFile);
    assert(ArchGetFileLength(firstName.c_str()) == 0);
    fputs(testContent, firstFile);
    fclose(firstFile);
    assert(ArchGetFileLength(firstName.c_str()) == strlen(testContent));

    // Map the file and assert the bytes are what we expect they are.
    assert((firstFile = fopen(firstName.c_str(), "rb")) != NULL);
    ArchConstFileMapping cfm = ArchMapFileReadOnly(firstFile);
    fclose(firstFile);
    assert(cfm);
    assert(memcmp(testContent, cfm.get(), strlen(testContent)) == 0);
    cfm.reset();

    // Try again with a mutable mapping.
    assert((firstFile = fopen(firstName.c_str(), "rb")) != NULL);
    ArchMutableFileMapping mfm = ArchMapFileReadWrite(firstFile);
    fclose(firstFile);
    assert(mfm);
    assert(memcmp(testContent, mfm.get(), strlen(testContent)) == 0);
    // Check that we can successfully mutate.
    mfm.get()[0] = 'T'; mfm.get()[2] = 's';
    assert(memcmp("Test", mfm.get(), strlen("Test")) == 0);
    mfm.reset();
    unlink(firstName.c_str());

    // Test ArchPWrite and ArchPRead.
    size_t len = strlen(testContent);
    assert((firstFile = fopen(firstName.c_str(), "w+b")) != NULL);
    assert(ArchPWrite(firstFile, testContent, len, 0) == len);
    std::unique_ptr<char[]> buf(new char[len]);
    assert(ArchPRead(firstFile, buf.get(), len, 0) == len);
    assert(memcmp(testContent, buf.get(), len) == 0);
    char const * const newText = "overwritten in a file";
    assert(ArchPWrite(firstFile, newText, strlen(newText),
                      5/*index of 'in a file'*/) == strlen(newText));
    std::unique_ptr<char[]> buf2(new char[strlen("written in a")]);
    assert(ArchPRead(firstFile, buf2.get(), strlen("written in a"),
                     9/*index of 'written in a'*/) == strlen("written in a"));
    assert(memcmp("written in a", buf2.get(), strlen("written in a")) == 0);

    // create and remove a tmp subdir
    std::string retpath;
    retpath = ArchMakeTmpSubdir(ArchGetTmpDir(), "myprefix");
    assert (retpath != "");
    rmdir(retpath.c_str());

    return 0;
}
