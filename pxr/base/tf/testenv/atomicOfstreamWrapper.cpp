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
/// \file test/atomicOfstreamWrapper.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"

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

static void
TestErrorCases()
{
    // Empty file path.
    TF_AXIOM(!TfAtomicOfstreamWrapper("").Open());
    // Can't create destination directory.
    TF_AXIOM(!TfAtomicOfstreamWrapper("/var/run/a/testTf_file_").Open());
    // Insufficient permission to create destination file.
    TF_AXIOM(!TfAtomicOfstreamWrapper("/var/run/testTf_file_").Open());
    // Unwritable file.
    TF_AXIOM(!TfAtomicOfstreamWrapper("/etc/passwd").Open());
    // wrapper not open.
    TF_AXIOM(!TfAtomicOfstreamWrapper("").Commit());
    TF_AXIOM(!TfAtomicOfstreamWrapper("").Cancel());

    {
        TfAtomicOfstreamWrapper wrapper("");
        TF_AXIOM(!wrapper.GetStream().is_open());
        TF_AXIOM(wrapper.GetStream().good());
        wrapper.GetStream() << "Into the bit bucket..." << endl;
        TF_AXIOM(!wrapper.GetStream().good());
    }
}

static void
TestCommitToNewFile()
{
    ArchUnlinkFile("testTf_NewFileCommit.txt");
    TfAtomicOfstreamWrapper wrapper("testTf_NewFileCommit.txt");
    TF_AXIOM(wrapper.Open());

    // Destination file doesn't exist yet.
    TF_AXIOM(!TfIsFile("testTf_NewFileCommit.txt"));

    // Temporary file exists.
    TF_AXIOM(Tf_CountFileMatches("testTf_NewFileCommit.*") == 1);

    // Write content to the stream.
    wrapper.GetStream() << "New Content" << endl;
    TF_AXIOM(wrapper.GetStream().good());

    // Commit.
    TF_AXIOM(wrapper.Commit());
    TF_AXIOM(!wrapper.GetStream().is_open());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountFileMatches("testTf_NewFileCommit.*") == 1);

    // Verify destination file content.
    ifstream ifs("testTf_NewFileCommit.txt");
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

static void
TestCommitToExistingFile()
{
    {
        ofstream ofs("testTf_ExFileCommit.txt");
        TF_AXIOM(ofs.good());
        ofs << "Existing content" << endl;
        ofs.close();
    }

    TF_AXIOM(TfIsFile("testTf_ExFileCommit.txt"));
    TfAtomicOfstreamWrapper wrapper("testTf_ExFileCommit.txt");
    TF_AXIOM(wrapper.Open());

    // Temporary file exists.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileCommit.*") == 2);

    // Write content to the stream.
    wrapper.GetStream() << "New Content" << endl;
    TF_AXIOM(wrapper.GetStream().good());

    // Commit.
    TF_AXIOM(wrapper.Commit());
    TF_AXIOM(!wrapper.GetStream().is_open());

    // Temporary file is gone.
    TF_AXIOM(Tf_CountFileMatches("testTf_ExFileCommit.*") == 1);

    // Verify destination file content.
    ifstream ifs("testTf_ExFileCommit.txt");
    string newContent;
    getline(ifs, newContent);
    TF_AXIOM(newContent == "New Content");
}

static void
TestCommitSymlink()
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
    TfAtomicOfstreamWrapper wrapper("testTf_Symlink.txt");
    TF_AXIOM(wrapper.Open());

    // Temporary file created in the real path.
    TF_AXIOM(Tf_CountFileMatches("a/b/c/d/testTf_File.*") == 2);

    // Write content to the stream.
    wrapper.GetStream() << "New Content" << endl;
    TF_AXIOM(wrapper.GetStream().good());

    // Commit the wrapper.
    TF_AXIOM(wrapper.Commit());
    TF_AXIOM(!wrapper.GetStream().is_open());

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
TestCancel()
{
    ArchUnlinkFile("testTf_Cancel.txt");
    TfAtomicOfstreamWrapper wrapper("testTf_Cancel.txt");
    TF_AXIOM(wrapper.Open());

    // Destination file not there yet.
    TF_AXIOM(!TfIsFile("testTf_Cancel.txt"));

    // Temporary file exists.
    TF_AXIOM(Tf_CountFileMatches("testTf_Cancel.*") == 1);

    // Cancel.
    TF_AXIOM(wrapper.Cancel());

    // Destination and temporary files are not on disk.
    TF_AXIOM(!TfIsFile("testTf_Cancel.txt"));
    TF_AXIOM(Tf_CountFileMatches("testTf_Cancel.*") == 0);
}

static void
TestAutoCancel()
{
    {
        ArchUnlinkFile("testTf_AutoCancel.txt");
        TfAtomicOfstreamWrapper wrapper("testTf_AutoCancel.txt");
        TF_AXIOM(wrapper.Open());

        // Destination file not there yet.
        TF_AXIOM(!TfIsFile("testTf_AutoCancel.txt"));

        // Temporary file exists.
        TF_AXIOM(Tf_CountFileMatches("testTf_AutoCancel.*") == 1);

        // wrapper going out of scope without a commit.
    }

    // Destination and temporary files are not on disk.
    TF_AXIOM(!TfIsFile("testTf_AutoCancel.txt"));
    TF_AXIOM(Tf_CountFileMatches("testTf_AutoCancel.*") == 0);
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
        TfAtomicOfstreamWrapper wrapper("testTf_NewFilePerm.txt");
        TF_AXIOM(wrapper.Open());
        TF_AXIOM(wrapper.Commit());

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

        TfAtomicOfstreamWrapper wrapper("testTf_ExistingFilePerm.txt");
        TF_AXIOM(wrapper.Open());
        wrapper.GetStream() << "testTf_ExistingFilePerm.txt" << endl;
        TF_AXIOM(wrapper.Commit());

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

static bool
Test_TfAtomicOfstreamWrapper()
{
    TestErrorCases();
    TestCommitToNewFile();
    TestCommitToExistingFile();
#if !defined(ARCH_OS_WINDOWS)
    // Windows has issues with the create symlink privilege.
    TestCommitSymlink();
#endif
    TestCancel();
    TestAutoCancel();
    TestFilePermissions();

    return true;
}

TF_ADD_REGTEST(TfAtomicOfstreamWrapper);
