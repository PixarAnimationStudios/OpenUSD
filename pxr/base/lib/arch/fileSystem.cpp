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
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/errno.h"
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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(ARCH_OS_WINDOWS)
#include <functional>
#include <io.h>
#include <process.h>
#include <Windows.h>
#include <WinIoCtl.h>
#else
#include <alloca.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <unistd.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

using std::string;
using std::set;

#if defined (ARCH_OS_WINDOWS)
namespace {
static inline HANDLE _FileToWinHANDLE(FILE *file)
{
    return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
}
}
#endif // ARCH_OS_WINDOWS

FILE* ArchOpenFile(char const* fileName, char const* mode)
{
    return fopen(fileName, mode);
}

#if defined(ARCH_OS_WINDOWS)
int ArchRmDir(const char* path)
{
    return RemoveDirectory(path) ? 0 : -1;
}
#endif

bool
ArchStatIsWritable(const ArchStatType *st)
{
#if defined(ARCH_OS_LINUX) || defined (ARCH_OS_DARWIN)
    if (st) {
        return (st->st_mode & S_IWOTH) || 
            ((getegid() == st->st_gid) && (st->st_mode & S_IWGRP)) ||
            ((geteuid() == st->st_uid) && (st->st_mode & S_IWUSR))
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

bool
ArchGetModificationTime(const char* pathname, double* time)
{
    ArchStatType st;
#if defined(ARCH_OS_WINDOWS)
    if (_stat64(pathname, &st) == 0)
#else
    if (stat(pathname, &st) == 0)
#endif
    {
        *time = ArchGetModificationTime(st);
        return true;
    }
    return false;
}

double
ArchGetModificationTime(const ArchStatType& st)
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

bool 
ArchGetStatMode(const char *pathname, int *mode)
{
    ArchStatType st;
#if defined(ARCH_OS_WINDOWS)
    if (__stat64(pathname, &st) == 0) {
#else
    if (stat(pathname, &st) == 0) {
#endif
        *mode = st.st_mode;
        return true;
    }
    return false;
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

#if defined (ARCH_OS_WINDOWS)

namespace {
int64_t
_GetFileLength(HANDLE handle)
{
    LARGE_INTEGER sz;
    return GetFileSizeEx(handle, &sz) ? static_cast<int64_t>(sz.QuadPart) : -1;
}
} // anonymous namespace

#endif

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
    return _GetFileLength(_FileToWinHANDLE(file));
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
    // Open a handle with 0 as the desired access and full sharing.
    // This opens the file even if exclusively locked.
    HANDLE handle =
        CreateFile(fileName, 0,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle) {
        const auto result = _GetFileLength(handle);
        CloseHandle(handle);
        return result;
    }
    return -1;
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

#if defined (ARCH_OS_WINDOWS)

namespace {
std::string
MakeUnique(
    const std::string& sTemplate,
    std::function<bool(const char* name)> func,
    int maxRetry = 1000)
{
    static const bool init = (srand(GetTickCount()), true);

    // Copy template to a writable buffer.
    const auto length = sTemplate.size();
    char* cTemplate = reinterpret_cast<char*>(alloca(length + 1));
    strcpy(cTemplate, sTemplate.c_str());

    // Fill template with random characters from table.
    const char* table = "abcdefghijklmnopqrstuvwxyz123456";
    std::string::size_type offset = length - 6;
    int retry = 0;
    do {
        unsigned int x = (static_cast<unsigned int>(rand()) << 15) + rand();
        cTemplate[offset + 0] = table[(x >> 25) & 31];
        cTemplate[offset + 1] = table[(x >> 20) & 31];
        cTemplate[offset + 2] = table[(x >> 15) & 31];
        cTemplate[offset + 3] = table[(x >> 10) & 31];
        cTemplate[offset + 4] = table[(x >>  5) & 31];
        cTemplate[offset + 5] = table[(x      ) & 31];

        // Invoke callback and if successful return the path.  Otherwise
        // repeat with a different random name for up to maxRetry times.
        if (func(cTemplate)) {
            return cTemplate;
        }
    } while (++retry < maxRetry);

    return std::string();
}

}

#endif

int
ArchMakeTmpFile(const std::string& tmpdir,
                const std::string& prefix, std::string* pathname)
{
    // Format the template.
    std::string sTemplate =
        ArchStringPrintf("%s/%s.XXXXXX", tmpdir.c_str(), prefix.c_str());

#if defined(ARCH_OS_WINDOWS)
    int fd = -1;
    auto cTemplate =
        MakeUnique(sTemplate, [&fd](const char* name){
            _sopen_s(&fd, name, _O_CREAT | _O_EXCL | _O_RDWR,
                     _SH_DENYNO, _S_IREAD | _S_IWRITE);
            return fd != -1;
        });
#else
    // Copy template to a writable buffer.
    char* cTemplate = reinterpret_cast<char*>(alloca(sTemplate.size() + 1));
    strcpy(cTemplate, sTemplate.c_str());

    // Open the file.
    int fd = mkstemp(cTemplate);
    if (fd != -1) {
        // Make sure file is readable by group.  mkstemp created the
        // file with 0600 permissions.  We want 0640.
        //
        fchmod(fd, 0640);
    }
#endif

    if (fd != -1) {
        // Save the path.
        if (pathname) {
            *pathname = cTemplate;
        }
    }

    return fd;
}

std::string
ArchMakeTmpSubdir(const std::string& tmpdir,
                  const std::string& prefix)
{
    std::string retstr;

    // Format the template.
    std::string sTemplate =
        ArchStringPrintf("%s/%s.XXXXXX", tmpdir.c_str(), prefix.c_str());

#if defined(ARCH_OS_WINDOWS)
    retstr =
        MakeUnique(sTemplate, [](const char* name){
            return CreateDirectory(name, NULL) != FALSE;
        });
#else
    // Copy template to a writable buffer.
    char* cTemplate = reinterpret_cast<char*>(alloca(sTemplate.size() + 1));
    strncpy(cTemplate, sTemplate.c_str(), sTemplate.size() + 1);

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
    const std::string tmpdir = ArchGetEnv("TMPDIR");
    if (!tmpdir.empty()) {
        // This function is not exposed in the header; it is only used during
        // Arch_InitConfig. If this is called more than once when TMPDIR is
        // set, the following call will leak a string.
        _TmpDir = strdup(tmpdir.c_str());
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
Arch_MapFileImpl(FILE *file, std::string *errMsg)
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
    auto m = mmap(nullptr, length,
                  isConst ? PROT_READ : PROT_READ | PROT_WRITE,
                  MAP_PRIVATE, fileno(file), 0);
    Mapping ret(m == MAP_FAILED ? nullptr : static_cast<PtrType>(m),
                Arch_Unmapper(length));
    if (!ret && errMsg) {
        int err = errno;
        if (err == EINVAL) {
            *errMsg = "bad arguments to mmap()";
        } else if (err == EMFILE || err == ENOMEM) {
            *errMsg = "system limit on mapped regions exceeded, "
                "or out of memory";
        } else {
            *errMsg = ArchStrerror();
        }
    }
    return ret;
#endif
}

ArchConstFileMapping
ArchMapFileReadOnly(FILE *file, std::string *errMsg)
{
    return Arch_MapFileImpl<ArchConstFileMapping>(file, errMsg);
}

ArchMutableFileMapping
ArchMapFileReadWrite(FILE *file, std::string *errMsg)
{
    return Arch_MapFileImpl<ArchMutableFileMapping>(file, errMsg);
}

ARCH_API
void ArchMemAdvise(void const *addr, size_t len, ArchMemAdvice adv)
{
#if defined(ARCH_OS_WINDOWS)
    // No windows implementation yet.  Look at
    // PrefetchVirtualMemory()/OfferVirtualMemory() in future.
#else // assume POSIX
    // Have to adjust addr to be page-size aligned.
    static size_t mask = ~(static_cast<size_t>(sysconf(_SC_PAGESIZE)) - 1);
    uintptr_t addrInt = reinterpret_cast<uintptr_t>(addr);
    uintptr_t alignedAddrInt = addrInt & mask;

    // This must follow ArchMemAdvice exactly.
    int adviceMap[] = {
        /* ArchMemAdviceNormal       = */ POSIX_MADV_NORMAL,
        /* ArchMemAdviceWillNeed     = */ POSIX_MADV_WILLNEED,
        /* ArchMemAdviceDontNeed     = */ POSIX_MADV_DONTNEED,
        /* ArchMemAdviceRandomAccess = */ POSIX_MADV_RANDOM
    };

    int rval = posix_madvise(reinterpret_cast<void *>(alignedAddrInt),
                             len + (addrInt - alignedAddrInt), adviceMap[adv]);
    if (rval != 0) {
        fprintf(stderr, "failed call to posix_madvise(%zd, %zd)"
                "ret=%d, errno=%d '%s'\n",
                alignedAddrInt, len + (addrInt-alignedAddrInt),
                rval, errno, ArchStrerror().c_str());
    }
#endif
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
    if (ARCH_LIKELY(nread == signedCount || nread == 0))
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
        if (ARCH_LIKELY(nread == signedCount || nread == 0))
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

#if defined(ARCH_OS_WINDOWS)

static inline DWORD ArchModeToAccess(int mode)
{
    switch (mode) {
    case X_OK: return FILE_GENERIC_EXECUTE;
    case W_OK: return FILE_GENERIC_WRITE;
    case R_OK: return FILE_GENERIC_READ;
    default:   return FILE_ALL_ACCESS;
    }
}

static int Arch_FileAccessError()
{
    switch (GetLastError()) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        // No such file.
        errno = ENOENT;
        return -1;

    case ERROR_FILENAME_EXCED_RANGE:
        // path too long
        errno = ENAMETOOLONG;
        return -1;

    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_BAD_NETPATH:
        // Invalid path.
        errno = ENOTDIR;
        return -1;

    case ERROR_INVALID_DRIVE:
    case ERROR_NOT_READY:
        // Media is missing.
        errno = EIO;
        return -1;

    case ERROR_INVALID_PARAMETER:
        errno = EINVAL;
        return -1;

    default:
        // Unexpected error.  Fall through to report access denied.

    case ERROR_ACCESS_DENIED:
        errno = EACCES;
        return -1;
    }
}

int ArchFileAccess(const char* path, int mode)
{
    // Simple existence check is handled specially.
    if (mode == F_OK) {
        return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
                ? 0 : Arch_FileAccessError();
    }

    const SECURITY_INFORMATION securityInfo = OWNER_SECURITY_INFORMATION |
                                              GROUP_SECURITY_INFORMATION |
                                              DACL_SECURITY_INFORMATION;

    // Get the SECURITY_DESCRIPTOR size.
    DWORD length = 0;
    if (!GetFileSecurity(path, securityInfo, NULL, 0, &length)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return Arch_FileAccessError();
        }
    }

    // Get the SECURITY_DESCRIPTOR.
    std::unique_ptr<unsigned char[]> buffer(new unsigned char[length]);
    PSECURITY_DESCRIPTOR security = (PSECURITY_DESCRIPTOR)buffer.get();
    if (!GetFileSecurity(path, securityInfo, security, length, &length)) {
        return Arch_FileAccessError();
    }


    HANDLE token;
    DWORD desiredAccess = TOKEN_IMPERSONATE | TOKEN_QUERY |
                          TOKEN_DUPLICATE | STANDARD_RIGHTS_READ;
    if (!OpenThreadToken(GetCurrentThread(), desiredAccess, TRUE, &token))
    {
        if (!OpenProcessToken(GetCurrentProcess(), desiredAccess, &token))
        {
            CloseHandle(token);
            errno = EACCES;
            return -1;
        }
    }

    bool result = false;
    HANDLE duplicateToken;
    if (DuplicateToken(token, SecurityImpersonation, &duplicateToken))
    {
        PRIVILEGE_SET privileges = {0};
        DWORD grantedAccess = 0;
        DWORD privilegesLength = sizeof(privileges);
        BOOL accessStatus = FALSE;

        GENERIC_MAPPING mapping;
        mapping.GenericRead = FILE_GENERIC_READ;
        mapping.GenericWrite = FILE_GENERIC_WRITE;
        mapping.GenericExecute = FILE_GENERIC_EXECUTE;
        mapping.GenericAll = FILE_ALL_ACCESS;

        DWORD accessMask = ArchModeToAccess(mode);
        MapGenericMask(&accessMask, &mapping);

        if (AccessCheck(security,
                        duplicateToken,
                        accessMask,
                        &mapping,
                        &privileges,
                        &privilegesLength,
                        &grantedAccess,
                        &accessStatus))
        {
            if (accessStatus) {
                result = true;
            }
            else {
                errno = EACCES;
            }
        }
        CloseHandle(duplicateToken);
    }
    CloseHandle(token);

    return result ? 0 : -1;
}

// https://msdn.microsoft.com/en-us/library/windows/hardware/ff552012.aspx

#define MAX_REPARSE_DATA_SIZE  (16 * 1024)

typedef struct _REPARSE_DATA_BUFFER {
    ULONG   ReparseTag;
    USHORT  ReparseDataLength;
    USHORT  Reserved;
    union {
        struct {
            USHORT  SubstituteNameOffset;
            USHORT  SubstituteNameLength;
            USHORT  PrintNameOffset;
            USHORT  PrintNameLength;
            ULONG   Flags;
            WCHAR   PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT  SubstituteNameOffset;
            USHORT  SubstituteNameLength;
            USHORT  PrintNameOffset;
            USHORT  PrintNameLength;
            WCHAR   PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

std::string ArchReadLink(const char* path)
{
    HANDLE handle = ::CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT |
        FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return std::string();

    std::unique_ptr<unsigned char[]> buffer(new
                               unsigned char[MAX_REPARSE_DATA_SIZE]);
    REPARSE_DATA_BUFFER* reparse = (REPARSE_DATA_BUFFER*)buffer.get();

    if (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                         MAX_REPARSE_DATA_SIZE, NULL, NULL)) {
        CloseHandle(handle);
        return std::string();
    }
    CloseHandle(handle);

    if (IsReparseTagMicrosoft(reparse->ReparseTag)) {
        if (reparse->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
            const size_t length =
                reparse->SymbolicLinkReparseBuffer.PrintNameLength /
                                                                sizeof(WCHAR);
            std::unique_ptr<WCHAR[]> reparsePath(new WCHAR[length + 1]);
            wcsncpy(reparsePath.get(),
              &reparse->SymbolicLinkReparseBuffer.PathBuffer[
              reparse->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)
              ], length);

            reparsePath.get()[length] = 0;

            // Convert wide-char to narrow char
            std::wstring ws(reparsePath.get());
            string str(ws.begin(), ws.end());

            return str;
        }
    }

    return std::string();
}

#else

std::string
ArchReadLink(const char* path)
{
    if (!path || !path[0]) {
        return std::string();
    }

    // Make a buffer for the link on the heap.
    // Explicit deleter to work around libc++ bug.
    // See https://llvm.org/bugs/show_bug.cgi?id=18350.
    ssize_t size = ARCH_PATH_MAX;
    std::unique_ptr<char, std::default_delete<char[]> > buffer;

    // Read the link.
    while (true) {
        // Allocate the buffer.
        buffer.reset(new char[size]);
        if (!buffer) {
            // Not enough memory.
            return std::string();
        }

        // Read the link.
        const ssize_t n = readlink(path, buffer.get(), size);
        if (n == -1) {
            // We can't read the link.
            return std::string();
        }
        else if (n >= size) {
            // We don't have enough space.  Find out how much space we need.
            struct stat sb;
            if (lstat(path, &sb) == 0) {
                size = sb.st_size + 1;
            }
            else {
                size *= 2;
            }
        }
        else {
            // Success.  readlink() doesn't NUL terminate.
            buffer.get()[n] = '\0';
            return std::string(buffer.get());
        }
    }
}

#endif

ARCH_API
void ArchFileAdvise(
    FILE *file, int64_t offset, size_t count, ArchFileAdvice adv)
{
#if defined(ARCH_OS_WINDOWS)
    // No windows implementation yet.  Not clear what's equivalent.
#elif defined(ARCH_OS_DARWIN)
    // No OSX implementation; posix_fadvise does not exist on that platform.
#else // assume POSIX
    // This must follow ArchFileAdvice exactly.
    int adviceMap[] = {
        /* ArchFileAdviceNormal       = */ POSIX_FADV_NORMAL,
        /* ArchFileAdviceWillNeed     = */ POSIX_FADV_WILLNEED,
        /* ArchFileAdviceDontNeed     = */ POSIX_FADV_DONTNEED,
        /* ArchFileAdviceRandomAccess = */ POSIX_FADV_RANDOM
    };
    int rval = posix_fadvise(fileno(file), offset, static_cast<off_t>(count),
                             adviceMap[adv]);
    if (rval != 0) {
        fprintf(stderr, "failed call to posix_fadvise(%d, %zd, %zd)"
                "ret=%d, errno=%d '%s'\n",
                fileno(file), offset, static_cast<off_t>(count),
                rval, errno, ArchStrerror().c_str());
    }
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
