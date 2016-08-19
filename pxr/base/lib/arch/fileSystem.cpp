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
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/arch/vsnprintf.h"

#include <atomic>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <memory>
#include <sstream>

#include <alloca.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>

#if defined (ARCH_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#endif // ARCH_OS_WINDOWS

using std::string;
using std::set;

#if defined (ARCH_OS_WINDOWS)
static inline HANDLE _FileToWinHANDLE(FILE *file) {
    return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
}
#endif // ARCH_OS_WINDOWS

bool
ArchStatIsWritable(const struct stat *st)
{
#if defined(ARCH_OS_LINUX) || defined (ARCH_OS_DARWIN)
    if (st) {
        return (st->st_mode & S_IWOTH) or
            ((getegid() == st->st_gid) and (st->st_mode & S_IWGRP)) or
            ((geteuid() == st->st_uid) and (st->st_mode & S_IWUSR))
            ;
    }
    return false;
#else
#error Unknown system architecture.
#endif
}

double
ArchGetModificationTime(const struct stat& st)
{
#if defined(ARCH_OS_LINUX)
    return st.st_mtim.tv_sec + 1e-9*st.st_mtim.tv_nsec;
#elif defined(ARCH_OS_DARWIN)
    return st.st_mtimespec.tv_sec + 1e-9*st.st_mtimespec.tv_nsec;
#else
#error Unknown system architecture
#endif
}

double
ArchGetAccessTime(const struct stat& st)
{
#if defined(ARCH_OS_LINUX)
    return st.st_atim.tv_sec + 1e-9*st.st_atim.tv_nsec;
#elif defined(ARCH_OS_DARWIN)
    return st.st_atimespec.tv_sec + 1e-9*st.st_atimespec.tv_nsec;
#else
#error Unknown system architecture
#endif
}

double
ArchGetStatusChangeTime(const struct stat& st)
{
#if defined(ARCH_OS_LINUX)
    return st.st_ctim.tv_sec + 1e-9*st.st_ctim.tv_nsec;
#elif defined(ARCH_OS_DARWIN)
    return st.st_ctimespec.tv_sec + 1e-9*st.st_ctimespec.tv_nsec;
#else
#error Unknown system architecture
#endif
}

namespace {
struct _Fcloser {
    inline void operator()(FILE *f) const { if (f) { fclose(f); } }
};
} // anon

using _UniqueFILE = std::unique_ptr<FILE, _Fcloser>;

int64_t
ArchGetFileLength(FILE *file)
{
    if (!file)
        return -1;
#if defined (ARCH_OS_LINUX) || defined (ARCH_OS_DARWIN)
    struct stat buf;
    return fstat(fileno(file), &buf) < 0 ? -1 :
        static_cast<int64_t>(buf.st_size);
#elif defined (ARCH_OS_WINDOWS)
    LARGE_INTEGER sz;
    return GetFileSizeEx(_FileToWinHANDLE(file), &sz) ?
        static_cast<int64_t>(sz.QuadPart) : -1;
#else
#error Unknown system architecture
#endif
}

int64_t
ArchGetFileLength(const char* fileName)
{
#if defined (ARCH_OS_LINUX) || defined (ARCH_OS_DARWIN)
    struct stat buf;
    return stat(fileName, &buf) < 0 ? -1 : static_cast<int64_t>(buf.st_size);
#elif defined (ARCH_OS_WINDOWS)
    return ArchGetFileLength(_UniqueFILE(fopen(fileName, "rb")).get());
#else
#error Unknown system architecture
#endif
}

string
ArchMakeTmpFileName(const string& prefix, const string& suffix)
{
    string  tmpDir = ArchGetTmpDir();

    static std::atomic<int> nCalls(1);
    const int n = nCalls++;

    if (n == 1)
	return ArchStringPrintf("%s/%s.%d%s", tmpDir.c_str(), prefix.c_str(),
				getpid(), suffix.c_str());
    else
	return ArchStringPrintf("%s/%s.%d.%d%s", tmpDir.c_str(), prefix.c_str(),
				getpid(), n, suffix.c_str());
}

int
ArchMakeTmpFile(const std::string& prefix, std::string* pathname)
{
    return ArchMakeTmpFile(ArchGetTmpDir(), prefix, pathname);
}

int
ArchMakeTmpFile(const std::string& tmpdir,
                const std::string& prefix, std::string* pathname)
{
    // Format the template.
    std::string sTemplate =
        ArchStringPrintf("%s/%s.XXXXXX", tmpdir.c_str(), prefix.c_str());

    // Copy template to a writable buffer.
    char* cTemplate = reinterpret_cast<char*>(alloca(sTemplate.size() + 1));
    strcpy(cTemplate, sTemplate.c_str());
    // Open the file.
    int fd = mkstemp(cTemplate);

    if (fd != -1) {
        // Save the path.
        if (pathname) {
            *pathname = cTemplate;
        }

        // Make sure file is readable by group.  mkstemp created the
        // file with 0600 permissions.  We want 0640.
        //
        fchmod(fd, 0640);
    }

    return fd;
}

std::string
ArchMakeTmpSubdir(const std::string& tmpdir,
                  const std::string& prefix)
{
    // Format the template.
    std::string sTemplate =
        ArchStringPrintf("%s/%s.XXXXXX", tmpdir.c_str(), prefix.c_str());

    // Copy template to a writable buffer.
    char* cTemplate = reinterpret_cast<char*>(alloca(sTemplate.size() + 1));
    strcpy(cTemplate, sTemplate.c_str());

    std::string retstr;

    // Open the tmpdir.
    char *tmpSubdir = mkdtemp(cTemplate);

    if (tmpSubdir) {
        // mkdtemp creates the directory with 0700 permissions.  We
        // want 0750.
        chmod(tmpSubdir, 0750);

        retstr = tmpSubdir;
    }

    return retstr;
}

static const char* _TmpDir = 0;

ARCH_HIDDEN
void
Arch_InitTmpDir()
{
    if (char* tmpdir = getenv("TMPDIR")) {
        // This function is not exposed in the header; it is only used during
        // Arch_InitConfig. If this is called more than once when TMPDIR is
        // set, the following call will leak a string.
        _TmpDir = strdup(tmpdir);
    } else {
#if defined(ARCH_OS_DARWIN)
        _TmpDir = "/tmp";
#else
        _TmpDir = "/var/tmp";
#endif
    }
}

const char *
ArchGetTmpDir()
{
    return _TmpDir;
}

set<string>
ArchGetAutomountDirectories()
{
    set<string> result;

#if !defined(ARCH_OS_LINUX)
    ARCH_ERROR("unimplemented function");
#else
    if (FILE *in = fopen("/proc/mounts","r")) {
	char linebuffer[1024];
	
	while (fgets(linebuffer, 1024, in)) {
	    char name[1024], dir[1024], type[1024], opts[1024];
        if (sscanf(linebuffer, "%s %s %s %s", name, dir, type, opts) == 4 &&
            strcmp(type, "autofs") == 0) {

            // Omit mounts with the 'direct' option set.
            bool direct = false;

            char* saveptr;
            char* token = strtok_r(opts, ",", &saveptr);
            while (token) {
                if (strcmp(token, "direct") == 0) {
                    direct = true;
                    break;
                }
                token = strtok_r(NULL, ",", &saveptr);
            }

            if (not direct)
                result.insert(dir);
	    }
	}

	fclose(in);
    }
    else {
        ARCH_ERROR("Cannot open /proc/mounts");
    }
#endif
    
    return result;
}

void
Arch_Unmapper::operator()(char const *mapStart) const
{
    void *ptr = static_cast<void *>(const_cast<char *>(mapStart));
    if (!ptr)
        return;
#if defined(ARCH_OS_WINDOWS)
    UnmapViewOfFile(ptr);
#else // assume POSIX
    munmap(ptr, _length);
#endif
}

void
Arch_Unmapper::operator()(char *mapStart) const
{
    (*this)(static_cast<char const *>(mapStart));
}

template <class Mapping>
static inline Mapping
Arch_MapFileImpl(FILE *file)
{
    using PtrType = typename Mapping::pointer;
    constexpr bool isConst =
        std::is_const<typename Mapping::element_type>::value;

    auto length = ArchGetFileLength(file);
    if (length < 0)
        return Mapping();

#if defined(ARCH_OS_WINDOWS)
    uint64_t unsignedLength = length;
    DWORD maxSizeHigh = static_cast<DWORD>(unsignedLength >> 32);
    DWORD maxSizeLow = static_cast<DWORD>(unsignedLength);
    HANDLE hFileMap = CreateFileMapping(
        _FileToWinHANDLE(file), NULL,
        PAGE_READONLY /* allow read-only or copy-on-write */,
        maxSizeHigh, maxSizeLow, NULL);
    if (hFileMap == NULL)
        return Mapping();
    auto ptr = static_cast<PtrType>(
        MapViewOfFile(hFileMap, isConst ? FILE_MAP_READ : FILE_MAP_COPY,
                      /*offsetHigh=*/ 0, /*offsetLow=*/0, unsignedLength));
    // Close the mapping handle, and return the view pointer.
    CloseHandle(hFileMap);
    return Mapping(ptr, Arch_Unmapper());
#else // Assume POSIX
    return Mapping(
        static_cast<PtrType>(
            mmap(nullptr, length, isConst ? PROT_READ : PROT_READ | PROT_WRITE,
                 MAP_PRIVATE, fileno(file), 0)), Arch_Unmapper(length));
#endif
}

ArchConstFileMapping
ArchMapFileReadOnly(FILE *file)
{
    return Arch_MapFileImpl<ArchConstFileMapping>(file);
}

ArchMutableFileMapping
ArchMapFileReadWrite(FILE *file)
{
    return Arch_MapFileImpl<ArchMutableFileMapping>(file);
}

int64_t
ArchPRead(FILE *file, void *buffer, size_t count, int64_t offset)
{
    if (count == 0)
        return 0;

#if defined(ARCH_OS_WINDOWS)
    HANDLE hFile = _FileToWinHANDLE(file);

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));

    uint64_t uoffset = offset;
    overlapped.OffsetHigh = static_cast<DWORD>(uoffset >> 32);
    overlapped.Offset = static_cast<DWORD>(uoffset);

    DWORD numRead = 0;
    if (ReadFile(hFile, buffer, static_cast<DWORD>(count),
                 &numRead, &overlapped)) {
        return numRead;
    }
    return -1;
#else // assume POSIX
    // Read and check if all got read (most common case).
    int fd = fileno(file);
    // Convert to signed so we can compare the result of pread with count
    // without the compiler complaining.  This conversion is implementation
    // defined if count is larger than what's representable by int64_t, and
    // POSIX pread also specifies that this case is implementation defined.  We
    // follow suit.
    int64_t signedCount = static_cast<int64_t>(count);
    int64_t nread = pread(fd, buffer, signedCount, offset);
    if (ARCH_LIKELY(nread == signedCount or nread == 0))
        return nread;

    // Track a total and retry until we read everything or hit EOF or an error.
    int64_t total = std::max<int64_t>(nread, 0);
    while (nread != -1 || (nread == -1 && errno == EINTR)) {
        // Update bookkeeping and retry.
        if (nread > 0) {
            total += nread;
            signedCount -= nread;
            offset += nread;
            buffer = static_cast<char *>(buffer) + nread;
        }
        nread = pread(fd, buffer, signedCount, offset);
        if (ARCH_LIKELY(nread == signedCount or nread == 0))
            return total + nread;
    }
    
    // Error case.
    return -1;
#endif
}

int64_t
ArchPWrite(FILE *file, void const *bytes, size_t count, int64_t offset)
{
    if (offset < 0)
        return -1;
    
#if defined(ARCH_OS_WINDOWS)
    HANDLE hFile = _FileToWinHANDLE(file);

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));

    uint64_t uoffset = offset;
    overlapped.OffsetHigh = static_cast<DWORD>(uoffset >> 32);
    overlapped.Offset = static_cast<DWORD>(uoffset);

    DWORD numWritten = 0;
    if (WriteFile(hFile, bytes, static_cast<DWORD>(count),
                  &numWritten, &overlapped)) {
        return numWritten;
    }
    return -1;
#else // assume POSIX
    // It's claimed that correct, modern POSIX will never return 0 for (p)write
    // unless count is zero.  It will either be the case that some bytes were
    // written, or we get an error return.

    // Write and check if all got written (most common case).
    int fd = fileno(file);
    // Convert to signed so we can compare the result of pwrite with count
    // without the compiler complaining.  This conversion is implementation
    // defined if count is larger than what's representable by int64_t, and
    // POSIX pwrite also specifies that this case is implementation defined.  We
    // follow suit.
    int64_t signedCount = static_cast<int64_t>(count);
    int64_t nwritten = pwrite(fd, bytes, signedCount, offset);
    if (ARCH_LIKELY(nwritten == signedCount))
        return nwritten;

    // Track a total and retry until we write everything or hit an error.
    int64_t total = std::max<int64_t>(nwritten, 0);
    while (nwritten != -1) {
        // Update bookkeeping and retry.
        total += nwritten;
        signedCount -= nwritten;
        offset += nwritten;
        bytes = static_cast<char const *>(bytes) + nwritten;
        nwritten = pwrite(fd, bytes, signedCount, offset);
        if (ARCH_LIKELY(nwritten == signedCount))
            return total + nwritten;
    }

    // Error case.
    return -1;
#endif
}

