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
#include "pxr/base/arch/vsnprintf.h"
#include <sys/types.h>
#include <sys/stat.h>
#if defined(ARCH_OS_WINDOWS)
#include <io.h>
#include <stdio.h>
#include <process.h>
#else
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

using std::string;
using std::set;

// Currently this is only used by platforms that don't have pread.
static std::mutex _fileSystemMutex;

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

int
ArchGetFileLength(const char* fileName)
{
    struct stat buf;
    return stat(fileName, &buf) < 0 ? -1 : int(buf.st_size);
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

_off_t ArchPositionRead(int fd, void *buf, size_t count, off_t offset)
{
#if defined(ARCH_OS_WINDOWS)
	std::lock_guard<std::mutex> lock(_fileSystemMutex);
    off_t current_offset;
    off_t rc;

    current_offset = _lseek(fd, 0, SEEK_CUR);

	if (_lseek(fd, offset, SEEK_SET) != offset)
	{
		return -1;
	}
	
    rc = _read(fd, buf, static_cast<unsigned int>(count));

    if (current_offset != _lseek(fd, current_offset, SEEK_SET))
        return -1;
    return rc;
#else
	return pread(fd, buf, count, offset);
#endif
}

_off_t ArchPositionWrite(int fd, const void *buf, size_t count, off_t offset)
{
#if defined(ARCH_OS_WINDOWS)
	std::lock_guard<std::mutex> lock(_fileSystemMutex);
    off_t current_offset;
    off_t rc;

    current_offset = _lseek(fd, 0, SEEK_CUR);

	if (_lseek(fd, offset, SEEK_SET) != offset)
	{
		return -1;
	}
    rc = _write(fd, buf, static_cast<unsigned int>(count));

    if (current_offset != _lseek(fd, current_offset, SEEK_SET))
        return -1;
    return rc;
#else
	return pwrite(fd, buf, count, offset);
#endif
}