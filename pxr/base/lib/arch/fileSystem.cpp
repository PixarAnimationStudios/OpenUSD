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
#include "pxr/base/arch/env.h"
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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(ARCH_OS_WINDOWS)
#include <io.h>
#include <stdio.h>
#include <process.h>
#else
#include <sys/mman.h>
#include <alloca.h>
#include <sys/file.h>
#include <unistd.h>
#endif
#include <atomic>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sstream>
#include <mutex>

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

FILE* ArchOpenFile(char const* fileName, char const* mode)
{
    FILE* stream = nullptr;
#if defined(ARCH_OS_WINDOWS)
	fopen_s(&stream, fileName, mode);
#else
	stream = fopen(fileName, mode);
#endif
	return stream;
}

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
int
ArchGetFilesystemStats(const char *path, struct statfs *buf)
{
    return statfs(path, buf) != -1;
}
#endif

int
ArchStatCompare(enum ArchStatComparisonOp op,
		const struct stat *stat1,
		const struct stat *stat2)
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN) || defined(ARCH_OS_WINDOWS)
    switch (op) {
      case ARCH_STAT_MTIME_EQUAL:
	return (stat1->st_mtime == stat2->st_mtime);
      case ARCH_STAT_MTIME_LESS:
	return (stat1->st_mtime < stat2->st_mtime);
      case ARCH_STAT_SAME_FILE:
	return (stat1->st_dev == stat2->st_dev &&
		stat1->st_ino == stat2->st_ino);
      default:
        // CODE_COVERAGE_OFF
        ARCH_ERROR("Unknown comparison operator");
	return 0;
        // CODE_COVERAGE_ON
    }
#else
#error Unknown system architecture.
#endif
}

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
#elif defined(ARCH_OS_WINDOWS)
	if (st) {
		return (st->st_mode & _S_IWRITE) ? true : false;
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
#elif defined(ARCH_OS_WINDOWS)
	// NB: this may need adjusting
	return static_cast<double>(st.st_mtime);
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
#elif defined(ARCH_OS_WINDOWS)
	// NB: this may need adjusting
	return static_cast<double>(st.st_atime);
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
#elif defined(ARCH_OS_WINDOWS)
	// NB: this may need adjusting
	return static_cast<double>(st.st_mtime);
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
    return ArchGetFileLength(_UniqueFILE(ArchOpenFile(fileName, "rb")).get());
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
#if defined(ARCH_OS_WINDOWS)
	int pid = _getpid();
#else
    int pid = getpid();
#endif

    if (n == 1)
	return ArchStringPrintf("%s/%s.%d%s", tmpDir.c_str(), prefix.c_str(),
				pid, suffix.c_str());
    else
	return ArchStringPrintf("%s/%s.%d.%d%s", tmpDir.c_str(), prefix.c_str(),
				pid, n, suffix.c_str());
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
#if defined(ARCH_OS_WINDOWS)
	char filename[ARCH_PATH_MAX];
	UINT ret = ::GetTempFileName(tmpdir.c_str(), prefix.c_str(), 0, filename);
	if (ret == 0)
	{
		ARCH_ERROR("Call to GetTempFileName failed.");
		return -1;
	}

	// Attempt to create the file
	HANDLE fileHandle = ::CreateFile(filename,
			GENERIC_WRITE,          // open for write
			0,                      // not for sharing
			NULL,                   // default security
			CREATE_ALWAYS,          // overwrite existing
			FILE_ATTRIBUTE_NORMAL,  //normal file
			NULL);                  // no template

	if (fileHandle == INVALID_HANDLE_VALUE) {
		ARCH_ERROR("Call to CreateFile failed.");
		return -1;
	}
	
	// Close the file
	::CloseHandle(fileHandle);

	if (pathname)
	{
		*pathname = filename;
	}

	int fd = 0;
	_sopen_s(&fd, filename, _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    return fd;
#else
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
#endif
}

std::string
ArchMakeTmpSubdir(const std::string& tmpdir,
                  const std::string& prefix)
{
	std::string retstr;

    // Format the template.
    std::string sTemplate =
        ArchStringPrintf("%s/%s.XXXXXX", tmpdir.c_str(), prefix.c_str());

    // Copy template to a writable buffer.
    char* cTemplate = reinterpret_cast<char*>(alloca(sTemplate.size() + 1));

#if defined(ARCH_OS_WINDOWS)
    strcpy_s(cTemplate, sTemplate.size() + 1, sTemplate.c_str());
	if (::CreateDirectory(sTemplate.c_str(), NULL))
	{
		retstr = sTemplate;
	}
#else
    strcpy(cTemplate, sTemplate.size() + 1, sTemplate.c_str());
    // Open the tmpdir.
    char *tmpSubdir = mkdtemp(cTemplate);

    if (tmpSubdir) {
        // mkdtemp creates the directory with 0700 permissions.  We
        // want 0750.
        chmod(tmpSubdir, 0750);

        retstr = tmpSubdir;
    }
#endif
    return retstr;
}

static const char* _TmpDir = 0;

ARCH_HIDDEN
void
Arch_InitTmpDir()
{
#if defined(ARCH_OS_WINDOWS)
	char tmpPath[MAX_PATH];

	// On Windows, let GetTempPath use the standard env vars, not our own.
	int sizeOfPath = GetTempPath(MAX_PATH - 1, tmpPath);
	if (sizeOfPath > MAX_PATH || sizeOfPath == 0) {
		ARCH_ERROR("Call to GetTempPath failed.");
		_TmpDir = ".";
		return;
	}

	// Strip the trailing slash
	tmpPath[sizeOfPath-1] = 0;
	_TmpDir = _strdup(tmpPath);
#else
    if (const char* tmpdir = ArchGetEnv("TMPDIR")) {
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
#endif
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
