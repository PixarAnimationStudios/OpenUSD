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
#include <filesystem>

PXR_NAMESPACE_USING_DIRECTIVE

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

#ifdef ARCH_OS_WINDOWS

namespace {

std::string _CreateLongWindowsPath(bool dir, bool dotted) {
    std::string p = ArchGetTmpDir();
    for (size_t i = 0; i < 15; ++i) {
        p += "\\abcdefghijklmnopqrs";
    }
    if (dotted) {
        p += "\\.\\..\\abcdefghijklmnopqrs";
    }
    if (!dir) {
        p += "\\foo.bar";
    }
    ARCH_AXIOM(p.size() > ARCH_PATH_MAX);
    return p;
}

// slightly awkward way of creating and deleting a long path without having to
// introduce recursive deletion etc.
std::string _CreatePhysicalLongPathDirectory() {
    static const std::string tmpDirPart0("UsdArchTestLongPaths");
    static const std::string tmpDirPart1(150, 'a');
    static const std::string tmpDirPart2(150, 'b');
    std::string tmpDir(ArchGetTmpDir());
    tmpDir = ArchMakeTmpSubdir(tmpDir, tmpDirPart0);
    tmpDir = ArchMakeTmpSubdir(tmpDir, tmpDirPart1);
    tmpDir = ArchMakeTmpSubdir(tmpDir, tmpDirPart2);
    ARCH_AXIOM(tmpDir.size() > ARCH_PATH_MAX);
    return tmpDir;
}

// implying structure from above
void _RemoveLongPathDirectory(const std::string& longTmpDir) {
    ARCH_AXIOM(ArchRmDir(longTmpDir.c_str()) == 0);

    std::string::size_type lastSep = longTmpDir.find_last_of('\\');
    ARCH_AXIOM(lastSep != std::string::npos);
    ARCH_AXIOM(ArchRmDir(longTmpDir.substr(0, lastSep).c_str()) == 0);

    lastSep = longTmpDir.find_last_of('\\', lastSep-1);
    ARCH_AXIOM(lastSep != std::string::npos);
    ARCH_AXIOM(ArchRmDir(longTmpDir.substr(0, lastSep).c_str()) == 0);
}

} // namespace

static bool TestLongPaths()
{
    const std::string longFilePathDotted = _CreateLongWindowsPath(false, true);
    const std::string longFilePath = _CreateLongWindowsPath(false, false);
    const std::string longFilePathForwardSlash = [&longFilePath]() {
        std::string t = longFilePath;
        std::replace(t.begin(), t.end(), '\\', '/');
        return t;
    }();

    {
        const std::string actual = ArchNormPath(longFilePathDotted, false);
        const std::string expected = longFilePathForwardSlash;
        ARCH_AXIOM(actual == expected);
    }
    {
        const std::string actual = ArchAbsPath(longFilePathDotted);
        const std::string expected = longFilePath;
        ARCH_AXIOM(actual == expected);
    }

    // A path that is long before normalization but short after.  This exercises
    // _MakeWinPath's post-normalization length recheck -- the result should not
    // get a \\?\ prefix or any other mangling.
    {
        std::string pathological = "C:\\x";
        while (pathological.size() < ARCH_PATH_MAX) {
            pathological += "\\..\\x";
        }
        ARCH_AXIOM(pathological.size() >= ARCH_PATH_MAX);
        ARCH_AXIOM(ArchNormPath(pathological) == "C:/x");
        ARCH_AXIOM(ArchAbsPath(pathological) == "C:\\x");
    }

    // Physical long-path operations: open, write, stat, access, length.
    {
        std::string longTmpDir = _CreatePhysicalLongPathDirectory();
        const std::string longTmpFilePath = longTmpDir + '\\' + "foo.bar";

        FILE *file;
        ARCH_AXIOM((file = ArchOpenFile(longTmpFilePath.c_str(), "wb")) != NULL);
        std::fprintf(file, "%s", "hello");
        fclose(file);

        ARCH_AXIOM(ArchFileAccess(longTmpFilePath.c_str(), F_OK) == 0);
        ARCH_AXIOM(ArchFileAccess(longTmpFilePath.c_str(), R_OK) == 0);
        ARCH_AXIOM(ArchFileAccess(longTmpFilePath.c_str(), W_OK) == 0);
        ARCH_AXIOM(ArchGetFileLength(longTmpFilePath.c_str()) == 5);

        double mtime = 0;
        ARCH_AXIOM(ArchGetModificationTime(longTmpFilePath.c_str(), &mtime));
        ARCH_AXIOM(mtime > 0);

        int mode = 0;
        ARCH_AXIOM(ArchGetStatMode(longTmpFilePath.c_str(), &mode));
        ARCH_AXIOM(mode != 0);

        // Access through a path with .. components -- verifies that
        // _MakeWinPath resolves these via GetFullPathNameW before adding the
        // long-path prefix.
        const std::string dottedPath = longTmpDir + "\\x\\..\\foo.bar";
        ARCH_AXIOM(dottedPath.size() > longTmpFilePath.size());
        ARCH_AXIOM(ArchGetFileLength(dottedPath.c_str()) == 5 /*hello*/);

        ARCH_AXIOM(ArchUnlinkFile(longTmpFilePath.c_str()) == 0);
        _RemoveLongPathDirectory(longTmpDir);
    }

    // TouchFile with long path.
    {
        std::string longTmpDir = _CreatePhysicalLongPathDirectory();
        const std::string longTmpFilePath = longTmpDir + '\\' + "foo.bar";
        ARCH_AXIOM(ArchTouchFile(longTmpFilePath, true));
        ARCH_AXIOM(ArchUnlinkFile(longTmpFilePath.c_str()) == 0);
        _RemoveLongPathDirectory(longTmpDir);
    }

    // MakeTmpFile with long base directory.
    {
        std::string longTmpDir = _CreatePhysicalLongPathDirectory();
        std::string longTmpFilePath;
        int tmpFileHandle =
            ArchMakeTmpFile(longTmpDir, "foo", &longTmpFilePath);
        ARCH_AXIOM(tmpFileHandle != -1);
        ArchCloseFile(tmpFileHandle);
        ARCH_AXIOM(ArchUnlinkFile(longTmpFilePath.c_str()) == 0);
        _RemoveLongPathDirectory(longTmpDir);
    }

    // MakeTmpSubdir with long base directory.
    {
        std::string longTmpDir = _CreatePhysicalLongPathDirectory();
        std::string subdir = ArchMakeTmpSubdir(longTmpDir, "sub");
        ARCH_AXIOM(!subdir.empty());
        ARCH_AXIOM(ArchRmDir(subdir.c_str()) == 0);
        _RemoveLongPathDirectory(longTmpDir);
    }

    return true;
}

#endif

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
#if defined(ARCH_OS_WINDOWS)
    ARCH_AXIOM(std::filesystem::equivalent(ArchWindowsUtf8ToUtf16(filePath),
                   ArchWindowsUtf8ToUtf16(firstName)));
#else
    ARCH_AXIOM(std::filesystem::equivalent(filePath, firstName));
#endif
    fclose(firstFile);
    
    // Test utf-8 path
    std::string secondName = ArchMakeTmpFileName("测试");
    ARCH_AXIOM((firstFile = ArchOpenFile(secondName.c_str(), "w")) != NULL);
    filePath = ArchGetFileName(firstFile);
    ARCH_AXIOM(std::filesystem::equivalent(std::filesystem::u8path(filePath),
               std::filesystem::u8path(secondName)));
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
    ARCH_AXIOM(ArchUnlinkFile(firstName.c_str()) == 0);

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
    fclose(firstFile);
    ARCH_AXIOM(ArchUnlinkFile(firstName.c_str()) == 0);

    // create and remove a tmp subdir
    std::string retpath;
    retpath = ArchMakeTmpSubdir(ArchGetTmpDir(), "myprefix");
    ARCH_AXIOM (retpath != "");
    ArchRmDir(retpath.c_str());

    // Test other utilities
    TestArchNormPath();
    TestArchAbsPath();

#ifdef ARCH_OS_WINDOWS
    TestLongPaths();
#endif

    return 0;
}
