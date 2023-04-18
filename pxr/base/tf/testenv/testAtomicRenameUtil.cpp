//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/safeOutputFile.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    constexpr int WRONG_NUMBER_ARGS = 2;
    constexpr float MAX_WAIT_FOR_FILE_SECONDS = 10;
    constexpr float WAIT_FOR_SLEEP_SECONDS = .1;
    constexpr auto FINAL_EXT = ".final";

    // We're testing using TfSafeOutputFile::Replace, which first writes to
    // a temporary file, then moves it to the final file.
    // In our tests, our final files end with a ".final" filename suffix.
    // This checks for the existence of the temp files, by finding matches
    // that DON'T have the ".final" suffix
    static size_t
    Tf_CountTempFileMatches(const string& pattern)
    {
        std::vector<string> matches = TfGlob(pattern, 0);

        // Count matches that don't end with ".final"
        return std::count_if(
            matches.begin(), matches.end(),
            [](const string& x) {
                return !TfStringEndsWith(x, FINAL_EXT);
            }
        );
    }

} // end annonymous namespace

/// Tries to run a TfSafeOutputFile::Replace
///
/// If a non-empty waitForFile is provided, then it will pause after the temp files
/// are created, but before the file move is made, until the waitForFile exists.
///
/// This provides a means of communication for our external testing program, so
/// it can run arbitrary code at this point, then create the waitForFile to
/// signal that this process should proceed with the file move.
static void
RunSafeOutputFileReplace(const string& fileBaseName, const string& waitForFile)
{
    // We want to test Tf_AtomicRenameFileOver, but that's not exposed publicly, so we test
    // TfSafeOutputFile::Replace, which uses it

    TfErrorMark tfErrors;

    string fileFinalName = fileBaseName + FINAL_EXT;
    string fileTempPattern = fileBaseName + ".*";
    auto outf = TfSafeOutputFile::Replace(fileFinalName);
    TF_AXIOM(outf.Get());
    TF_AXIOM(tfErrors.IsClean());

    // Temporary file exists.
    TF_AXIOM(Tf_CountTempFileMatches(fileTempPattern) == 1);

    // Write content to the stream.
    fprintf(outf.Get(), "New Content\n");

    // If a waitForFile was given, pause until that file exists
    if (!waitForFile.empty()) {
        auto start = std::chrono::steady_clock::now();
        auto maxTime = start + std::chrono::duration<double>(MAX_WAIT_FOR_FILE_SECONDS);
        auto sleepTime = std::chrono::duration<double>(WAIT_FOR_SLEEP_SECONDS);
        while(!TfPathExists(waitForFile)) {
            TF_AXIOM(std::chrono::steady_clock::now() < maxTime);
            std::this_thread::sleep_for(sleepTime);
        }
    }

    // Commit.
    outf.Close();
    TF_AXIOM(!outf.Get());
    TF_AXIOM(tfErrors.IsClean());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountTempFileMatches(fileTempPattern) == 0);

    // Verify destination file content.
    std::ifstream ifs(fileFinalName);
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        string progName(argv[0]);
        std::cerr << "Usage: " << progName << " FILE_BASE_NAME [WAIT_FOR_FILE]" << std::endl;
        return WRONG_NUMBER_ARGS;
    }
    string waitForFile;
    if (argc > 2) {
        waitForFile = argv[2];
    }
    RunSafeOutputFileReplace(argv[1], waitForFile);
    return 0;
}
