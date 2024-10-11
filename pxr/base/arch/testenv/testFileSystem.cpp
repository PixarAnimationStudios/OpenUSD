//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/pragmas.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

ARCH_PRAGMA_DEPRECATED_POSIX_NAME

static bool
TestArchNormPath()
{
    ARCH_AXIOM(ArchNormPath("") == ".");
    ARCH_AXIOM(ArchNormPath(".") == ".");
    ARCH_AXIOM(ArchNormPath("..") == "..");
    ARCH_AXIOM(ArchNormPath("foobar/../barbaz") == "barbaz");
    ARCH_AXIOM(ArchNormPath("/") == "/");
    ARCH_AXIOM(ArchNormPath("//") == "//");
    ARCH_AXIOM(ArchNormPath("///") == "/");
    ARCH_AXIOM(ArchNormPath("///foo/.//bar//") == "/foo/bar");
    ARCH_AXIOM(ArchNormPath("///foo/.//bar//.//..//.//baz") == "/foo/baz");
    ARCH_AXIOM(ArchNormPath("///..//./foo/.//bar") == "/foo/bar");
    ARCH_AXIOM(ArchNormPath(
            "foo/bar/../../../../../../baz") == "../../../../baz");

#if defined(ARCH_OS_WINDOWS)
    ARCH_AXIOM(ArchNormPath("C:\\foo\\bar") == "C:/foo/bar");
    ARCH_AXIOM(ArchNormPath("C:foo\\bar") == "C:foo/bar");
    ARCH_AXIOM(ArchNormPath("c:\\foo\\bar") == "c:/foo/bar");
    ARCH_AXIOM(ArchNormPath("c:foo\\bar") == "c:foo/bar");
    ARCH_AXIOM(ArchNormPath(
            "C:\\foo\\bar", /* stripDriveSpecifier = */ true) == "/foo/bar");
    ARCH_AXIOM(ArchNormPath(
            "C:foo\\bar", /* stripDriveSpecifier = */ true) == "foo/bar");
#endif

    return true;
}

namespace {
std::string
_AbsPathFilter(const std::string& path)
{
#if defined(ARCH_OS_WINDOWS)
    // Strip drive specifier and convert backslashes to forward slashes.
    std::string result = path.substr(2);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
#else
    // Return path as-is.
    return path;
#endif
}

std::string
_DosDevicePathFilter(const std::string& path)
{
#if defined(ARCH_OS_WINDOWS)
    // Strip prefix \\?\ or \\.\ and convert backslashes to forward slashes.
    std::string result = path;
    if (result.length() > 4)
    {
        std::string prefix = result.substr(0, 4);
        if (prefix == "\\\\?\\" || prefix == "\\\\.\\")
        {
            result = result.substr(4);
        }
    }

    return ArchNormPath(result);
#else
    // Return path as-is.
    return path;
#endif
}
}

static bool
TestArchAbsPath()
{
    ARCH_AXIOM(ArchAbsPath("") == "");
    ARCH_AXIOM(ArchAbsPath("foo") != "foo");
    ARCH_AXIOM(_AbsPathFilter(ArchAbsPath("/foo/bar")) == "/foo/bar");
    ARCH_AXIOM(_AbsPathFilter(ArchAbsPath("/foo/bar/../baz")) == "/foo/baz");

    return true;
}

int main()
{
    std::string firstName = ArchMakeTmpFileName("archFS");
    FILE *firstFile;

    char const * const testContent = "text in a file";

    // Open a file, check that its length is 0, write to it, close it, and then
    // check that its length is now the number of characters written.
    ARCH_AXIOM((firstFile = ArchOpenFile(firstName.c_str(), "wb")) != NULL);
    fflush(firstFile);
    ARCH_AXIOM(ArchGetFileLength(firstName.c_str()) == 0);
    fputs(testContent, firstFile);
    fclose(firstFile);
    ARCH_AXIOM(ArchGetFileLength(firstName.c_str()) == strlen(testContent));

    // Open a file, check that the file path from FILE* handle is matched.
    ARCH_AXIOM((firstFile = ArchOpenFile(firstName.c_str(), "rb")) != NULL);
    std::string filePath = ArchGetFileName(firstFile);
    ARCH_AXIOM(_DosDevicePathFilter(filePath) == _DosDevicePathFilter(firstName));
    fclose(firstFile);

    // Map the file and assert the bytes are what we expect they are.
    ARCH_AXIOM((firstFile = ArchOpenFile(firstName.c_str(), "rb")) != NULL);
    ArchConstFileMapping cfm = ArchMapFileReadOnly(firstFile);
    fclose(firstFile);
    ARCH_AXIOM(cfm);
    ARCH_AXIOM(memcmp(testContent, cfm.get(), strlen(testContent)) == 0);
    cfm.reset();

    // Try again with a mutable mapping.
    ARCH_AXIOM((firstFile = ArchOpenFile(firstName.c_str(), "rb")) != NULL);
    ArchMutableFileMapping mfm = ArchMapFileReadWrite(firstFile);
    fclose(firstFile);
    ARCH_AXIOM(mfm);
    ARCH_AXIOM(memcmp(testContent, mfm.get(), strlen(testContent)) == 0);
    // Check that we can successfully mutate.
    mfm.get()[0] = 'T'; mfm.get()[2] = 's';
    ARCH_AXIOM(memcmp("Test", mfm.get(), strlen("Test")) == 0);
    mfm.reset();
    ArchUnlinkFile(firstName.c_str());

    // Test ArchPWrite and ArchPRead.
    int64_t len = strlen(testContent);
    ARCH_AXIOM((firstFile = ArchOpenFile(firstName.c_str(), "w+b")) != NULL);
    ARCH_AXIOM(ArchPWrite(firstFile, testContent, len, 0) == len);
    std::unique_ptr<char[]> buf(new char[len]);
    ARCH_AXIOM(ArchPRead(firstFile, buf.get(), len, 0) == len);
    ARCH_AXIOM(memcmp(testContent, buf.get(), len) == 0);
    char const * const newText = "overwritten in a file";
    ARCH_AXIOM(ArchPWrite(firstFile, newText, strlen(newText),
                      5/*index of 'in a file'*/) == strlen(newText));
    std::unique_ptr<char[]> buf2(new char[strlen("written in a")]);
    ARCH_AXIOM(ArchPRead(firstFile, buf2.get(), strlen("written in a"),
                     9/*index of 'written in a'*/) == strlen("written in a"));
    ARCH_AXIOM(memcmp("written in a", buf2.get(), strlen("written in a")) == 0);

    // create and remove a tmp subdir
    std::string retpath;
    retpath = ArchMakeTmpSubdir(ArchGetTmpDir(), "myprefix");
    ARCH_AXIOM (retpath != "");
    ArchRmDir(retpath.c_str());

    // Test other utilities
    TestArchNormPath();
    TestArchAbsPath();

    return 0;
}
