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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/systemInfo.h"

#include <string>
#include <vector>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {
#if defined(ARCH_OS_WINDOWS)
const char* knownDirPath = "c:\\Windows";
const char* knownFilePath = "c:\\Windows\\System32\\notepad.exe";
const char* knownFilePath2 = "c:\\.\\Windows\\.\\..\\Windows\\System32\\notepad.exe";
const char* knownNoSuchPath = "c:\\nosuch";
#elif defined(ARCH_OS_DARWIN)
const char* knownDirPath = "/private/etc";
const char* knownFilePath = "/private/etc/passwd";
const char* knownFilePath2 = "/./private/./etc/./../etc/passwd";
const char* knownNoSuchPath = "/nosuch";
#else
const char* knownDirPath = "/etc";
const char* knownFilePath = "/etc/passwd";
const char* knownFilePath2 = "/./etc/./../etc/passwd";
const char* knownNoSuchPath = "/nosuch";
#endif
const char* knownNoSuchRelPath = "nosuch";

bool testSymlinks = true;

// Wrap TfSymlink in code to check if we should do symlink tests.
bool
_Symlink(const std::string& src, const std::string& dst)
{
    if (testSymlinks) {
        if (!TfSymlink(src, dst)) {
            if (errno == EPERM) {
                testSymlinks = false;
                TF_WARN("Not testing symlinks");
                return true;
            }
            return false;
        }
    }
    return true;
}

} // anonymous namespace

static bool
TestTfRealPath()
{
    TF_AXIOM(TfRealPath("") == "");
    TF_AXIOM(TfRealPath("binary") == "");
    TF_AXIOM(TfRealPath(knownFilePath2) == knownFilePath);

    // No symlinks
    TF_AXIOM(TfIsDir("subdir/e") || TfMakeDirs("subdir/e"));
    TF_AXIOM(TfRealPath("subdir", true) == TfAbsPath("subdir"));

    // Create a nest of links for testing.
    if (testSymlinks) {
        TF_AXIOM(TfIsLink("b") || _Symlink("subdir", "b"));
        TF_AXIOM(TfIsLink("c") || _Symlink("b", "c"));
        TF_AXIOM(TfIsLink("d") || _Symlink("c", "d"));
        TF_AXIOM(TfIsLink("e") || _Symlink("missing", "e"));
        TF_AXIOM(TfIsLink("f") || _Symlink("e", "f"));
        TF_AXIOM(TfIsLink("g") || _Symlink("f", "g"));
    }

    if (testSymlinks) {
        // Leaf dir is symlink
        TF_AXIOM(TfRealPath("d", true) == TfAbsPath("subdir"));
        // Symlinks through to dir
        TF_AXIOM(TfRealPath("d/e", true) == TfAbsPath("subdir/e"));
        // Symlinks through to nonexistent dirs
        TF_AXIOM(TfRealPath("d/e/f/g/h", true) == TfAbsPath("subdir/e/f/g/h"));
        // Symlinks through to broken link
        TF_AXIOM(TfRealPath("g", true) == "");
    }

    // Empty
    TF_AXIOM(TfRealPath("", true) == "");
    // Nonexistent absolute
    TF_AXIOM(TfRealPath(knownNoSuchPath, true) == knownNoSuchPath);
    // Nonexistent relative
    TF_AXIOM(TfRealPath(knownNoSuchRelPath, true) ==
             TfAbsPath(knownNoSuchRelPath));

    if (testSymlinks) {
        string error;
        string::size_type split = TfFindLongestAccessiblePrefix("g", &error);
        TF_AXIOM(split == 0);
        TF_AXIOM(error == "encountered dangling symbolic link");
    }

    return true;
}

static bool
TestTfNormPath()
{
    TF_AXIOM(TfNormPath("") == ".");
    TF_AXIOM(TfNormPath(".") == ".");
    TF_AXIOM(TfNormPath("..") == "..");
    TF_AXIOM(TfNormPath("foobar/../barbaz") == "barbaz");
    TF_AXIOM(TfNormPath("/") == "/");
    TF_AXIOM(TfNormPath("//") == "//");
    TF_AXIOM(TfNormPath("///") == "/");
    TF_AXIOM(TfNormPath("///foo/.//bar//") == "/foo/bar");
    TF_AXIOM(TfNormPath("///foo/.//bar//.//..//.//baz") == "/foo/baz");
    TF_AXIOM(TfNormPath("///..//./foo/.//bar") == "/foo/bar");
    TF_AXIOM(TfNormPath("foo/bar/../../../../../../baz") == "../../../../baz");

    return true;
}

namespace {
std::string
_AbsPathFilter(const std::string& path)
{
#if defined(ARCH_OS_WINDOWS)
    // Strip driver specifier and convert backslashes to forward slashes.
    return TfStringReplace(path.substr(2), "\\", "/");
#else
    // Return path as-is.
    return path;
#endif
}
}

static bool
TestTfAbsPath()
{
    TF_AXIOM(TfAbsPath("") == "");
    TF_AXIOM(TfAbsPath("foo") != "foo");
    TF_AXIOM(_AbsPathFilter(TfAbsPath("/foo/bar")) == "/foo/bar");
    TF_AXIOM(_AbsPathFilter(TfAbsPath("/foo/bar/../baz")) == "/foo/baz");

    return true;
}

static bool
TestTfReadLink()
{
    TF_AXIOM(TfReadLink("") == "");

    if (testSymlinks) {
        ArchUnlinkFile("test-link");
        if (!_Symlink(knownDirPath, "test-link")) {
            TF_RUNTIME_ERROR("failed to create test link: %s",
                                ArchStrerror(errno).c_str());
            return false;
        }

        TF_AXIOM(TfReadLink("test-link") == knownDirPath);
        TF_AXIOM(TfReadLink(knownDirPath) == "");
        ArchUnlinkFile("test-link");
    }

    return true;
}

namespace {
std::string
_GlobFilter(const std::string& lhs, const std::string& rhs)
{
#if defined(ARCH_OS_WINDOWS)
    // Join and convert backslashes to forward slashes.
    return TfStringReplace(lhs + rhs, "\\", "/");
#else
    // Simply join the paths.
    return lhs + rhs;
#endif
}
}

static bool
TestTfGlob()
{
    vector<string> empty;
    TF_AXIOM(TfGlob(empty).empty());

    TF_AXIOM(TfGlob("").empty());

    vector<string> dir_a = TfGlob(knownDirPath);
    TF_AXIOM(dir_a.size() == 1);
    TF_AXIOM(dir_a.at(0) == _GlobFilter(knownDirPath, "/"));

    vector<string> dir_b = TfGlob(_GlobFilter(knownDirPath, "/"));
    TF_AXIOM(dir_b.size() == 1);
    TF_AXIOM(dir_b.at(0) == _GlobFilter(knownDirPath, "/"));

    vector<string> dir_c = TfGlob(_GlobFilter(knownDirPath, "/*"));
    TF_AXIOM(dir_c.size() > 1);

    vector<string> dir_d = TfGlob(_GlobFilter(knownDirPath, "/_no_such_file"));
    TF_AXIOM(dir_d.size() == 1);
    TF_AXIOM(dir_d.at(0) == _GlobFilter(knownDirPath, "/_no_such_file"));

    vector<string> dir_e = TfGlob(_GlobFilter(knownNoSuchPath, "*"));
    TF_AXIOM(dir_e.size() == 1);
    TF_AXIOM(dir_e.at(0) == _GlobFilter(knownNoSuchPath, "*"));

    vector<string> dir_f = TfGlob("//depot/...");
    TF_AXIOM(dir_f.size() == 1);
    TF_AXIOM(dir_f.at(0) == "//depot/...");

    vector<string> paths;
    paths.push_back(knownDirPath);
    vector<string> result = TfGlob(paths);
    TF_AXIOM(result.size() == 1);

    return true;
}

static bool
TestTfGetExtension()
{
    string emptyPath = "";
    string dotFile = ".foo";
    string dotFileWithPath = "/bar/baz/.foo";
    string directoryPath = "/bar/baz";
    string normalFilePath = "/bar/baz/foo.py";
    string dotDirectoryFilePath = "/bar.foo/baz.py";
    string clipFilePath = "/bar/baz/foo.bar.py";
    string hiddenFileWithExtension = "/foo/.bar.py";

    TF_AXIOM(TfGetExtension(emptyPath) == emptyPath);
    TF_AXIOM(TfGetExtension(dotFile) == emptyPath);
    TF_AXIOM(TfGetExtension(dotFileWithPath) == emptyPath);
    TF_AXIOM(TfGetExtension(directoryPath) == emptyPath);
    TF_AXIOM(TfGetExtension(dotDirectoryFilePath) == "py");
    TF_AXIOM(TfGetExtension(normalFilePath) == "py");
    TF_AXIOM(TfGetExtension(clipFilePath) == "py");
    TF_AXIOM(TfGetExtension(hiddenFileWithExtension) == "py");

    return true;
}

static bool
Test_TfPathUtils()
{
    return TestTfRealPath() &&
           TestTfNormPath() &&
           TestTfAbsPath() &&
           TestTfReadLink() &&
           TestTfGetExtension() &&
           TestTfGlob()
           ;
}

TF_ADD_REGTEST(TfPathUtils);
