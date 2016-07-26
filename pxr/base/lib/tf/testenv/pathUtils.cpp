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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/systemInfo.h"

#include <string>
#include <vector>
#include <errno.h>
#include <unistd.h>

using namespace std;

static bool
TestTfRealPath()
{
    TF_AXIOM(TfRealPath("") == "");
    TF_AXIOM(TfRealPath("binary") == "");
    TF_AXIOM(TfRealPath("/./etc/./../etc/passwd") == "/etc/passwd");

    // Create a nest of links for testing.
    TF_AXIOM(TfIsDir("subdir/e") or TfMakeDirs("subdir/e"));
    TF_AXIOM(TfIsLink("b") or TfSymlink("subdir", "b"));
    TF_AXIOM(TfIsLink("c") or TfSymlink("b", "c"));
    TF_AXIOM(TfIsLink("d") or TfSymlink("c", "d"));
    TF_AXIOM(TfIsLink("e") or TfSymlink("missing", "e"));
    TF_AXIOM(TfIsLink("f") or TfSymlink("e", "f"));
    TF_AXIOM(TfIsLink("g") or TfSymlink("f", "g"));

    // No symlinks
    TF_AXIOM(TfRealPath("subdir", true) == TfAbsPath("subdir"));
    // Leaf dir is symlink
    TF_AXIOM(TfRealPath("d", true) == TfAbsPath("subdir"));
    // Symlinks through to dir
    TF_AXIOM(TfRealPath("d/e", true) == TfAbsPath("subdir/e"));
    // Symlinks through to nonexistent dirs
    TF_AXIOM(TfRealPath("d/e/f/g/h", true) == TfAbsPath("subdir/e/f/g/h"));
    // Symlinks through to broken link
    TF_AXIOM(TfRealPath("g", true) == "");
    // Empty
    TF_AXIOM(TfRealPath("", true) == "");
    // Nonexistent absolute
    TF_AXIOM(TfRealPath("/nosuch", true) == "/nosuch");
    // Nonexistent relative
    TF_AXIOM(TfRealPath("nosuch", true) == TfAbsPath("nosuch"));

    string error;
    string::size_type split = TfFindLongestAccessiblePrefix("g", &error);
    TF_AXIOM(split == 0);
    TF_AXIOM(error == "encountered dangling symbolic link");

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

static bool
TestTfAbsPath()
{
    TF_AXIOM(TfAbsPath("") == "");
    TF_AXIOM(TfAbsPath("foo") != "foo");
    TF_AXIOM(TfAbsPath("/foo/bar") == "/foo/bar");
    TF_AXIOM(TfAbsPath("/foo/bar/../baz") == "/foo/baz");

    return true;
}

static bool
TestTfReadLink()
{
    TF_AXIOM(TfReadLink("") == "");

    unlink("test-link");
    if (symlink("/etc/passwd", "test-link") == -1) {
        TF_RUNTIME_ERROR("failed to create test link: %s", strerror(errno));
        return false;
    }

    TF_AXIOM(TfReadLink("test-link") == "/etc/passwd");
    TF_AXIOM(TfReadLink("/usr") == "");
    unlink("test-link");

    return true;
}

static bool
TestTfGlob()
{
    vector<string> empty;
    TF_AXIOM(TfGlob(empty).empty());

    TF_AXIOM(TfGlob("").empty());

    vector<string> dir_a = TfGlob("/etc/pam.d");
    TF_AXIOM(dir_a.size() == 1);
    TF_AXIOM(dir_a.at(0) == "/etc/pam.d/");

    vector<string> dir_b = TfGlob("/etc/pam.d/");
    TF_AXIOM(dir_b.size() == 1);
    TF_AXIOM(dir_b.at(0) == "/etc/pam.d/");

    vector<string> dir_c = TfGlob("/etc/pam.d/*");
    TF_AXIOM(dir_c.size() > 1);

    vector<string> dir_d = TfGlob("/etc/pam.d/_no_such_config");
    TF_AXIOM(dir_d.size() == 1);
    TF_AXIOM(dir_d.at(0) == "/etc/pam.d/_no_such_config");

    vector<string> dir_e = TfGlob("/ZAXXON*");
    TF_AXIOM(dir_e.size() == 1);
    TF_AXIOM(dir_e.at(0) == "/ZAXXON*");

    vector<string> dir_f = TfGlob("//depot/...");
    TF_AXIOM(dir_f.size() == 1);
    TF_AXIOM(dir_f.at(0) == "//depot/...");

    vector<string> paths;
    paths.push_back("/etc/pam.d");
    paths.push_back("/etc/init.d");
    vector<string> result = TfGlob(paths);
    TF_AXIOM(result.size() > 1);

    return true;
}

static bool
Test_TfPathUtils()
{
    return TestTfRealPath() &&
           TestTfNormPath() &&
           TestTfAbsPath() &&
           TestTfReadLink() &&
           TestTfGlob()
           ;
}

TF_ADD_REGTEST(TfPathUtils);
