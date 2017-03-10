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
#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/arch/pragmas.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(ARCH_OS_WINDOWS)
#include <unistd.h>
#else
#define S_IRWXU 0
typedef int mode_t;
#endif

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>

ARCH_PRAGMA_DEPRECATED_POSIX_NAME

using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using boost::assign::list_of;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct _DirInfo {
    _DirInfo(
        string const& dirpath,
        vector<string> const& dirnames,
        vector<string> const& filenames)
        : dirpath(dirpath), dirnames(dirnames), filenames(filenames)
    { }

    string dirpath;
    vector<string> dirnames;
    vector<string> filenames;
};

TF_MAKE_STATIC_DATA(vector<_DirInfo>, _SetupData) {
    // Test directory structure:
    //   (<dirpath>, [<dirnames>], [<filenames>])
    *_SetupData = list_of<_DirInfo>
        ("a",
            list_of<string>("b"),
            list_of<string>("one")("two")("aardvark"))
        ("a/b",
            list_of<string>("c"),
            list_of<string>("three")("four")("banana"))
        ("a/b/c",
            list_of<string>("d"),
            list_of<string>("five")("six")("cat"))
        ("a/b/c/d",
            list_of<string>("e"),
            list_of<string>("seven")("eight")("dog"))
        ("a/b/c/d/e",
            list_of<string>("f"),
            list_of<string>
                ("nine")("ten")("elephant")("Eskimo")("Fortune")("Garbage"))
        ("a/b/c/d/e/f",
            list_of<string>("g")("h")("i"),
            list_of<string>("eleven")("twelve")("fish"))
        ("a/b/c/d/e/f/g",
            vector<string>(),
            list_of<string>("thirteen")("fourteen")("gator"))
        ("a/b/c/d/e/f/h",
            vector<string>(),
            list_of<string>("fifteen")("sixteen")("hippo"))
        ("a/b/c/d/e/f/i",
            vector<string>(),
            list_of<string>("seventeen")("eighteen")("igloo"))
        .convert_to_container< vector<_DirInfo> >();
        ;
}

} // End of anonymous namespace.

static bool
Setup()
{
    string topDir = (*_SetupData)[0].dirpath;
    if (TfIsDir(topDir))
        TfRmTree(topDir);
    else if (TfPathExists(topDir))
        ArchUnlinkFile(topDir.c_str());

    TF_FOR_ALL(i, *_SetupData) {
        if (!(TfIsDir(i->dirpath) || TfMakeDirs(i->dirpath)))
            TF_FATAL_ERROR("Failed to create directory '%s'",
                i->dirpath.c_str());

        TF_FOR_ALL(d, i->dirnames) {
            string dirPath = TfStringCatPaths(i->dirpath, *d);
            if (!(TfIsDir(dirPath) || TfMakeDirs(dirPath)))
                TF_FATAL_ERROR("Failed to create directory '%s'",
                    dirPath.c_str());
        }

        TF_FOR_ALL(f, i->filenames) {
            string filePath = TfStringCatPaths(i->dirpath, *f);
            if (!(TfIsFile(filePath) || TfTouchFile(filePath)))
                TF_FATAL_ERROR("Failed to create file '%s'",
                    filePath.c_str());
        }
    }

    // Create a symlink cycle for TfWalkDirs to avoid.
    TF_AXIOM(TfSymlink("../../../b", "a/b/c/d/cycle_to_b"));

    // Create a symlink to the top-level directory.
    ArchUnlinkFile("link_to_a");
    TF_AXIOM(TfSymlink("a", "link_to_a"));

    return true;
}

static bool
TestTfPathExists()
{
    cout << "Testing TfPathExists" << endl;

    TF_AXIOM(TfPathExists("/tmp"));
    TF_AXIOM(!TfPathExists("no/such/path"));
    TF_AXIOM(!TfPathExists(""));

    ArchUnlinkFile("link-to-file");
    TfSymlink("/no/such/file", "link-to-file");
    TF_AXIOM(TfPathExists("link-to-file"));
    TF_AXIOM(!TfPathExists("link-to-file", true));

    return true;
}

static bool
TestTfIsDir()
{
    cout << "Testing TfIsDir" << endl;

    TF_AXIOM(TfIsDir("/etc"));
    TF_AXIOM(!TfIsDir("/etc/passwd"));
    TF_AXIOM(!TfIsDir(""));

    ArchUnlinkFile("link-to-dir");
    TfSymlink("/etc", "link-to-dir");
    TF_AXIOM(!TfIsDir("link-to-dir"));
    TF_AXIOM(TfIsDir("link-to-dir", true));

    return true;
}

static bool
TestTfIsFile()
{
    cout << "Testing TfIsFile" << endl;

    TF_AXIOM(TfIsFile("/etc/passwd"));
    TF_AXIOM(!TfIsFile("/etc"));
    TF_AXIOM(!TfIsFile(""));

    ArchUnlinkFile("link-to-file");
    TfSymlink("/etc/passwd", "link-to-file");
    TF_AXIOM(!TfIsFile("link-to-file"));
    TF_AXIOM(TfIsFile("link-to-file", true));

    return true;
}

static bool
TestTfIsWritable()
{
    cout << "Testing TfIsWritable" << endl;

    TF_AXIOM(TfIsWritable("/tmp"));
    TF_AXIOM(!TfIsWritable(""));
    TF_AXIOM(!TfIsWritable("/etc"));
    TF_AXIOM(!TfIsWritable("/etc/passwd"));

    TfTouchFile("testTfIsWritable.txt");
    TF_AXIOM(TfIsWritable("testTfIsWritable.txt"));
    (void) ArchUnlinkFile("testTfIsWritable.txt");

    return true;
}

static bool
TestTfIsDirEmpty()
{
    cout << "Testing TfIsDirEmpty" << endl;

    TF_AXIOM(!TfIsDirEmpty("/etc/passwd"));
    TF_AXIOM(!TfIsDirEmpty("/etc"));
    TF_AXIOM(TfIsDir("empty") || TfMakeDirs("empty"));
    TF_AXIOM(TfIsDirEmpty("empty"));
    (void) ArchRmDir("empty");
    return true;
}

static bool
TestTfSymlink()
{
    cout << "Testing TfSymlink/TfIsLink" << endl;

    (void) ArchUnlinkFile("test-symlink");

    TF_AXIOM(!TfIsLink("/no/such/file"));
    TF_AXIOM(!TfIsLink("/etc/passwd"));
    TF_AXIOM(!TfIsLink(""));
    TF_AXIOM(TfSymlink("/etc/passwd", "test-symlink"));
    TF_AXIOM(TfIsLink("test-symlink"));
    TF_AXIOM(TfReadLink("test-symlink") == "/etc/passwd");

    (void) ArchUnlinkFile("test-symlink");

    return true;
}

static bool
TestTfDeleteFile()
{
    cout << "Testing TfDeleteFile" << endl;

    string t_file = "delete-test-file";
    TF_AXIOM(TfTouchFile(t_file));
    TF_AXIOM(TfDeleteFile(t_file));

    // Can't delete (file doesn't exist)
    cerr << "=== BEGIN EXPECTED ERROR ===" << endl;
    TfErrorMark m;
    TF_AXIOM(!TfDeleteFile(t_file));
    m.Clear();
    cerr << "=== END EXPECTED ERROR ===" << endl;

    return true;
}

static bool
TestTfMakeDir()
{
    cout << "Testing TfMakeDir" << endl;

    // Default permissions
    if (TfIsDir("test-directory-1"))
        (void) ArchRmDir("test-directory-1");

    mode_t oldMask = umask(2);
    TF_AXIOM(TfMakeDir("test-directory-1"));
    umask(oldMask);

    struct stat stbuf;
    TF_AXIOM(stat("test-directory-1", &stbuf) != -1);
    TF_AXIOM(S_ISDIR(stbuf.st_mode));
#if !defined(ARCH_OS_WINDOWS)
    TF_AXIOM((stbuf.st_mode & ~S_IFMT) ==
                (S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH));
#endif
    (void) ArchRmDir("test-directory-1");

    // Non-default permissions
    if (TfIsDir("test-directory-2"))
        (void) ArchRmDir("test-directory-2");

    TF_AXIOM(TfMakeDir("test-directory-2", S_IRWXU));
    TF_AXIOM(stat("test-directory-2", &stbuf) != -1);
    TF_AXIOM(S_ISDIR(stbuf.st_mode));
    TF_AXIOM((stbuf.st_mode & ~S_IFMT) == S_IRWXU);

    (void) ArchRmDir("test-directory-2");

    // Parent directories don't exist
    TF_AXIOM(!TfMakeDir("parents/do/not/exist"));

    return true;
}

static bool
TestTfMakeDirs()
{
    cout << "Testing TfMakeDirs" << endl;

    if (TfIsDir("testTfMakeDirs-1"))
        TfRmTree("testTfMakeDirs-1");

    cout << "+ relative path" << endl;
    TF_AXIOM(TfMakeDirs("testTfMakeDirs-1/b/c/d/e/f"));

    // No slashes
    if (TfIsDir("testTfMakeDirs-2"))
        TfRmTree("testTfMakeDirs-2");

    cout << "+ no slashes" << endl;
    TF_AXIOM(TfMakeDirs("testTfMakeDirs-2"));

    // Just a slash
    cout << "+ only a slash" << endl;
    TF_AXIOM(!TfMakeDirs("/"));

    // Begins with a dot
    if (TfIsDir("testTfMakeDirs-3"))
        TfRmTree("testTfMakeDirs-3");

    cout << "+ begins with a dot" << endl;
    TF_AXIOM(TfMakeDirs("./testTfMakeDirs-3/bar/baz"));

    // Partial path already exists
    cout << "+ partial path already exists" << endl;
    TF_AXIOM(TfMakeDirs("testTfMakeDirs-3/bar/baz/leaf"));

    // Whole path already exists
    cout << "+ whole path already exists" << endl;
    TF_AXIOM(!TfMakeDirs("testTfMakeDirs-3/bar/baz/leaf"));

    // Dots in path
    if (TfIsDir("testTfMakeDirs-4"))
        TfRmTree("testTfMakeDirs-4");

    cout << "+ dots in path" << endl;
    struct stat st;
    TF_AXIOM(TfMakeDirs("testTfMakeDirs-4/bar/./../baz"));
    TF_AXIOM(stat("testTfMakeDirs-4/baz", &st) != -1);
    TF_AXIOM(S_ISDIR(st.st_mode));

    // Non-directory in path
    if (TfIsDir("testTfMakeDirs-5"))
        TfRmTree("testTfMakeDirs-5");

    cout << "+ non-directory in path" << endl;
    TF_AXIOM(TfMakeDirs("testTfMakeDirs-5/bar"));
    TF_AXIOM(TfTouchFile("testTfMakeDirs-5/bar/a"));
    cerr << "=== BEGIN EXPECTED ERROR ===" << endl;
    TfErrorMark m;
    TF_AXIOM(!TfMakeDirs("./testTfMakeDirs-5/bar/a/b/c/d"));
    m.Clear();
    cerr << "=== END EXPECTED ERROR ===" << endl;

    return true;
}

struct Tf_WalkLogger
{
    Tf_WalkLogger(
        std::ostream* ostr,
        const string& stopPath = string())
        : _ostr(ostr), _stopPath(stopPath)
    { }

    void SetStopPath(const string& stopPath) {
        _stopPath = stopPath;
    }

    bool operator()(
        const string& dirpath,
        vector<string>* dirnames,
        const vector<string> & filenames_)
    {
        *_ostr << "('" << dirpath << "', ";

        // Sort names so the test log is stable.
        vector<string> filenames(filenames_);
        std::sort(dirnames->begin(), dirnames->end());
        std::sort(filenames.begin(), filenames.end());

        if (!dirnames->empty())
            *_ostr << "['" << TfStringJoin(*dirnames, "', '") << "'], ";
        else
            *_ostr << "[], ";

        if (!filenames.empty())
            *_ostr << "['" << TfStringJoin(filenames, "', '") << "'])" << endl;
        else
            *_ostr << "[])" << endl;

        return dirpath != _stopPath;
    }
    
private:
    std::ostream* _ostr;
    string _stopPath;
};

struct Tf_WalkErrorHandler {
    Tf_WalkErrorHandler(size_t* errors) : _errors(errors) {
        *_errors = 0;
    }
    void operator()(const string& path, const string& message) {
        TF_RUNTIME_ERROR("%s: %s", path.c_str(), message.c_str());
        ++(*_errors);
    }
private:
    size_t* _errors;
};

static bool
TestTfWalkDirs()
{
    cout << "Testing TfWalkDirs" << endl;

    // We need the directory structure created in the test setup.  If
    // something (e.g. TestTfRmTree) removes the directory before this test is
    // run, the following test will fail.
    TF_AXIOM(TfIsDir("a"));

    std::ofstream logstr("TestTfWalkDirs-log.txt");
    if (!logstr) {
        cerr << "Failed to open TestTfWalkDirs-log.txt" << endl;
        return false;
    }

    Tf_WalkLogger logger(&logstr);
    size_t errorCount = 0;

    logstr << "+ top down walk" << endl;
    TfWalkDirs("a", logger,
        /* topDown */ true,
        Tf_WalkErrorHandler(&errorCount));
    TF_AXIOM(errorCount == 0);

    logstr << "+ top down walk from symlink root" << endl;
    TfWalkDirs("link_to_a", logger,
        /* topDown */ true,
        Tf_WalkErrorHandler(&errorCount));
    TF_AXIOM(errorCount == 0);

    logstr << "+ top down walk from root with followLinks=true" << endl;
    TfWalkDirs("a", logger,
        /* topDown */ true,
        Tf_WalkErrorHandler(&errorCount),
        /* followLinks */ true);
    TF_AXIOM(errorCount == 0);

    logstr << "+ bottom up walk" << endl;
    TfWalkDirs("a", logger,
        /* topDown */ false,
        Tf_WalkErrorHandler(&errorCount));
    TF_AXIOM(errorCount == 0);

    logger.SetStopPath("a/b/c/d");

    logstr << "+ top down, stop at a/b/c/d" << endl;
    TfWalkDirs("a", logger,
        /* topDown */ true,
        Tf_WalkErrorHandler(&errorCount));
    TF_AXIOM(errorCount == 0);

    logstr << "+ bottom up, stop at a/b/c/d" << endl;
    TfWalkDirs("a", logger,
        /* topDown */ false,
        Tf_WalkErrorHandler(&errorCount));
    TF_AXIOM(errorCount == 0);

    return true;
}

static bool
TestTfListDir()
{
    cout << "Testing TfListDir" << endl;

    TF_AXIOM("listing a non-existent path" &&
             TfListDir("nosuchpath").empty());
    TF_AXIOM("listing a file" &&
             TfListDir("/etc/passwd").empty());

    // The success of the following result size checks depend on how many
    // files/directories/etc. are created by the sample hierarchy script.

    cout << "+ non-recursive listing" << endl;
    {
        vector<string> result = TfListDir("a");
        TF_AXIOM("listing the sample directory works" &&
                 !result.empty());

        cout << "entries = " << result.size() << endl;
        for (vector<string>::const_iterator it = result.begin();
             it != result.end(); ++it)
            cout << *it << endl;

        TF_AXIOM("check the number of entries returned" &&
                 result.size() == 4);
    }

    cout << "+ recursive listing" << endl;
    {
        vector<string> result = TfListDir("a", true);
        TF_AXIOM("listing the sample directory recursively works" &&
                 !result.empty());

        cout << "entries = " << result.size() << endl;
        for (vector<string>::const_iterator it = result.begin();
             it != result.end(); ++it)
            cout << *it << endl;

        TF_AXIOM("check the number of entries returned" &&
                 result.size() == 39);
    }

    return true;
}

static void
_TestTfRmTreeOnError(string const& dirpath,
                     string const& message,
                     string const& expDirPath)
{
    cout << "+ checking that ("
         << dirpath << " == " << expDirPath << ")"
         << endl;

    TF_AXIOM(dirpath == expDirPath);
}

static bool
TestTfRmTree()
{
    cout << "Testing TfRmTree" << endl;

    cout << "+ no such directory, ignore errors" << endl;
    TfErrorMark m;
    TfRmTree("nosuchdirectory", TfWalkIgnoreErrorHandler);
    TF_AXIOM(m.IsClean());

    cout << "+ no such directory, raise errors" << endl;
    cerr << "=== BEGIN EXPECTED ERROR ===" << endl;
    TfRmTree("nosuchdirectory");
    cerr << "=== END EXPECTED ERROR ===" << endl;
    TF_AXIOM(!m.IsClean());
    m.Clear();

    cout << "+ no such directory, handle errors" << endl;
    TfRmTree("nosuchdirectory",
             boost::bind(_TestTfRmTreeOnError, _1, _2, "nosuchdirectory"));

    // We need the directory structure created in the test setup, as we're
    // about to remove it.  Any tests that require the test structure to exist
    // must be run before this test.
    TF_AXIOM(TfIsDir("a"));

    cout << "+ removing a typical directory structure" << endl;
    TfRmTree("a");
    TF_AXIOM(!TfIsDir("a"));

    return true;
}

static bool
TestTfTouchFile()
{
    cout << "Testing TfTouchFile" << endl;

    string fileName("test-touchfile");
    (void) ArchUnlinkFile(fileName.c_str());

    // Touch non-existent file, create = false -> fail...
    TF_AXIOM(!TfTouchFile(fileName, false));
    TF_AXIOM(!TfIsFile(fileName));

    // Touch non-existent file, create = true -> succeed
    TF_AXIOM(TfTouchFile(fileName, true));
    TF_AXIOM(TfIsFile(fileName));

    // Get modification time of created file
    struct stat st;
    TF_AXIOM(stat(fileName.c_str(), &st) != -1);
    time_t oldmTime = st.st_mtime;

    // Wait a moment, so that mod time differs...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Touch again
    TF_AXIOM(TfTouchFile(fileName, false));
    TF_AXIOM(TfIsFile(fileName));

    // Get mod time, should be less than the last mod time...
    // Get modification time of created file
    // (can't test access time, since it might be disabled)
    struct stat st2;
    TF_AXIOM(stat(fileName.c_str(), &st2) != -1);
    time_t newmTime = st2.st_mtime;

    TF_AXIOM(newmTime > oldmTime);

    (void) ArchUnlinkFile(fileName.c_str());

    return true;
}

static bool
Test_TfFileUtils()
{
    return Setup() &&
           TestTfPathExists() &&
           TestTfIsDir() &&
           TestTfIsFile() &&
           TestTfIsWritable() &&
           TestTfIsDirEmpty() &&
           TestTfSymlink() &&
           TestTfDeleteFile() &&
           TestTfMakeDir() &&
           TestTfMakeDirs() &&
           TestTfWalkDirs() &&
           TestTfListDir() &&
           TestTfRmTree() &&
           TestTfTouchFile()
           ;
}

TF_ADD_REGTEST(TfFileUtils);
