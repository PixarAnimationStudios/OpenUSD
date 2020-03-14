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
///
/// \file test/safeOutputFile.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/safeOutputFile.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/regTest.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

static size_t
Tf_CountFileMatches(const string& pattern)
{
    // Temporary file exists.
    vector<string> matches = TfGlob(pattern, 0);
    cout << "TfGlob('" << pattern << "') => " << matches << endl;
    return matches.size();
}

template <class Fn>
static void
CheckFail(Fn const &fn) {
    TfErrorMark m;
    TfSafeOutputFile outf = fn();
    TF_AXIOM(!m.IsClean());
    m.Clear();
}

static void
TestErrorCases()
{
    TfSafeOutputFile outf;
    TfErrorMark m;
    
    // Empty file path.
    CheckFail([]() { return TfSafeOutputFile::Update(""); });
    CheckFail([]() { return TfSafeOutputFile::Replace(""); });
    
    // Can't create destination directory.
    CheckFail([]() {
            return TfSafeOutputFile::Update("/var/run/a/testTf_file_"); });
    CheckFail([]() {
            return TfSafeOutputFile::Replace("/var/run/a/testTf_file_"); });

    // Insufficient permission to create destination file.
    CheckFail([]() {
            return TfSafeOutputFile::Update("/var/run/testTf_file_"); });
    CheckFail([]() {
            return TfSafeOutputFile::Replace("/var/run/testTf_file_"); });

    // Unwritable file.
    CheckFail([]() { return TfSafeOutputFile::Update("/etc/passwd"); });
    CheckFail([]() { return TfSafeOutputFile::Replace("/etc/passwd"); });
}

static void
TestReplaceNewFile()
{
    ArchUnlinkFile("testTf_NewFileCommit.txt");
    auto outf = TfSafeOutputFile::Replace("testTf_NewFileCommit.txt");
    TF_AXIOM(outf.Get());

    // Destination file doesn't exist yet.
    TF_AXIOM(!TfIsFile("testTf_NewFileCommit.txt"));

    // Temporary file exists.
    TF_AXIOM(Tf_CountFileMatches("testTf_NewFileCommit.*") == 1);

    // Write content to the stream.
    fprintf(outf.Get(), "New Content\n");

    // Commit.
    outf.Close();
    TF_AXIOM(!outf.Get());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountFileMatches("testTf_NewFileCommit.*") == 1);

    // Verify destination file content.
    ifstream ifs("testTf_NewFileCommit.txt");
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

static void
TestReplaceExistingFile()
{
    {
        ofstream ofs("testTf_ExFileCommit.txt");
        TF_AXIOM(ofs.good());
        ofs << "Existing content" << endl;
        ofs.close();
    }

    TF_AXIOM(TfIsFile("testTf_ExFileCommit.txt"));
    auto outf = TfSafeOutputFile::Replace("testTf_ExFileCommit.txt");
    TF_AXIOM(outf.Get());

    // Temporary file exists.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileCommit.*") == 2);

    // Write content to the stream.
    fprintf(outf.Get(), "New Content\n");

    // Commit.
    outf.Close();
    TF_AXIOM(!outf.Get());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileCommit.*") == 1);

    // Verify destination file content.
    ifstream ifs("testTf_ExFileCommit.txt");
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

static void
TestUpdateExistingFile()
{
    {
        ofstream ofs("testTf_ExFileUpdate.txt");
        TF_AXIOM(ofs.good());
        ofs << "Existing content" << endl;
        ofs.close();
    }

    TF_AXIOM(TfIsFile("testTf_ExFileUpdate.txt"));
    auto outf = TfSafeOutputFile::Update("testTf_ExFileUpdate.txt");
    TF_AXIOM(outf.Get());

    // Temporary file does not exist.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileUpdate.*") == 1);

    // Write content to the stream.
    fprintf(outf.Get(), "New Content\n");

    TF_AXIOM(outf.IsOpenForUpdate());
    
    // Commit.
    outf.Close();
    TF_AXIOM(!outf.Get());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileUpdate.*") == 1);

    // Verify destination file content.
    ifstream ifs("testTf_ExFileUpdate.txt");
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

static void
TestReplaceSymlink()
{
    // Create destination directory.
    if (!TfIsDir("a/b/c/d")) {
        TF_AXIOM(TfMakeDirs("a/b/c/d"));
    }

    // Create destination file.
    string filePath = TfAbsPath("a/b/c/d/testTf_File.txt");
    {
        ArchUnlinkFile(filePath.c_str());
        ofstream ofs(filePath.c_str());
        TF_AXIOM(ofs.good());
        ofs << "Existing Content" << endl;
        ofs.close();
    }

    // Create a symlink to the destination file.
    TF_AXIOM(TfIsFile(filePath.c_str()));
    ArchUnlinkFile("testTf_Symlink.txt");
    TF_AXIOM(TfSymlink(filePath, "testTf_Symlink.txt"));
    TF_AXIOM(TfIsLink("testTf_Symlink.txt"));

    // Create a wrapper.
    auto outf = TfSafeOutputFile::Replace("testTf_Symlink.txt");
    TF_AXIOM(outf.Get());

    // Temporary file created in the real path.
    TF_AXIOM(Tf_CountFileMatches("a/b/c/d/testTf_File.*") == 2);

    // Write content to the stream.
    fprintf(outf.Get(), "New Content\n");

    // Commit the wrapper.
    outf.Close();
    TF_AXIOM(!outf.Get());

    // Temporary file is removed.
    TF_AXIOM(Tf_CountFileMatches("a/b/c/d/testTf_File.*") == 1);

    // Verify destination file content.
    ifstream ifs(filePath.c_str());
    string newContent;
    getline(ifs, newContent);
    fprintf(stderr, "newContent = '%s'\n", newContent.c_str());
    TF_AXIOM(newContent == "New Content");
}

static void
TestFilePermissions()
{
    // Set the umask for this duration of this test to a predictable value.
#if !defined(ARCH_OS_WINDOWS)
    const mode_t testUmask = 00002;
    const mode_t mask = umask(testUmask);
#endif

    {
        ArchUnlinkFile("testTf_NewFilePerm.txt");
        auto outf = TfSafeOutputFile::Replace("testTf_NewFilePerm.txt");
        TF_AXIOM(outf.Get());
        outf.Close();

#if !defined(ARCH_OS_WINDOWS)
        struct stat st;
        TF_AXIOM(stat("testTf_NewFilePerm.txt", &st) != -1);
        mode_t perms = st.st_mode & ACCESSPERMS;
        fprintf(stderr, "testTf_NewFilePerm: fileMode = 0%03o\n", perms);
        TF_AXIOM(perms == (DEFFILEMODE - testUmask));
#endif
    }

    {
        ArchUnlinkFile("testTf_ExistingFilePerm.txt");
#if !defined(ARCH_OS_WINDOWS)
        int fd = open("testTf_ExistingFilePerm.txt", O_CREAT, S_IRUSR|S_IWUSR);
        struct stat est;
        TF_AXIOM(fstat(fd, &est) != -1);
        TF_AXIOM((est.st_mode & ACCESSPERMS) == (S_IRUSR|S_IWUSR));
        close(fd);
#endif

        auto outf = TfSafeOutputFile::Replace("testTf_ExistingFilePerm.txt");
        TF_AXIOM(outf.Get());
        fprintf(outf.Get(), "testTf_ExistingFilePerm.txt\n");
        outf.Close();

#if !defined(ARCH_OS_WINDOWS)
        struct stat st;
        TF_AXIOM(stat("testTf_ExistingFilePerm.txt", &st) != -1);
        mode_t fileMode = st.st_mode & ACCESSPERMS;
        fprintf(stderr, "testTf_ExistingFilePerm: fileMode = 0%03o\n", fileMode);
        TF_AXIOM(!(fileMode & (S_IRGRP|S_IWGRP)));
#endif
    }

#if !defined(ARCH_OS_WINDOWS)
    // Restore umask to whatever it was.
    umask(mask);
#endif
}

static void
TestDiscard()
{
    {
        ofstream ofs("testTf_Discard.txt");
        TF_AXIOM(ofs.good());
        ofs << "Existing content" << endl;
        ofs.close();
    }

    TF_AXIOM(TfIsFile("testTf_Discard.txt"));
    
    // Calling Discard on file opened for Update is an error.
    {
        TfErrorMark m;

        auto outf = TfSafeOutputFile::Update("testTf_Discard.txt");
        outf.Discard();

        TF_AXIOM(!m.IsClean());
        m.Clear();
    }

    // Verify that new content written will not overwrite existing
    // content if Discard is called on TfSafeOutputFile opened for
    // Replace.
    {
        auto outf = TfSafeOutputFile::Replace("testTf_Discard.txt");
        fprintf(outf.Get(), "New Content");
        outf.Discard();

        TF_AXIOM(!outf.Get());

        ifstream ifs("testTf_Discard.txt");
        string content;
        getline(ifs, content);
        fprintf(stderr, "content = '%s'\n", content.c_str());
        TF_AXIOM(content == "Existing content");
    }
    
    // Verify that new file won't be written if Discard is called on
    // TfSafeOutputFile opened for Replace.
    {
        auto outf = TfSafeOutputFile::Replace("testTf_Discard_New.txt");
        fprintf(outf.Get(), "New Content");
        outf.Discard();

        TF_AXIOM(!outf.Get());
        TF_AXIOM(!TfIsFile("testTf_Discard_New.txt"));
    }
}

static bool
Test_TfSafeOutputFile()
{
    TestErrorCases();
    TestReplaceNewFile();
    TestReplaceExistingFile();
    TestUpdateExistingFile();
#if !defined(ARCH_OS_WINDOWS)
    // Windows has issues with the create symlink privilege.
    TestReplaceSymlink();
#endif
    TestFilePermissions();
    TestDiscard();

    return true;
}

TF_ADD_REGTEST(TfSafeOutputFile);
