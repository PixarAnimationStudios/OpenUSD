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
#ifndef ARCH_FILESYSTEM_H
#define ARCH_FILESYSTEM_H

#include "pxr/base/arch/defines.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <set>

#if defined(ARCH_OS_LINUX)
#include <sys/statfs.h>
#elif defined(ARCH_OS_DARWIN)
#include <sys/mount.h>
#endif

/*!
 * \file fileSystem.h
 * \brief Architecture dependent file system access
 * \ingroup group_arch_SystemFunctions
 */

/*!
 * \brief This enum is used to specify a comparison operator for
 * \c ArchStatCompare().
 * \ingroup group_arch_SystemFunctions
 */

enum ArchStatComparisonOp {
    ARCH_STAT_MTIME_EQUAL,	/*!< Modification times are equal */
    ARCH_STAT_MTIME_LESS,	/*!< Modification time for \c stat1 is less */
    ARCH_STAT_SAME_FILE		/*!< Both refer to same file */
};
    
/*!
 * \brief Compares two \c stat structures.
 * \ingroup group_arch_SystemFunctions
 *
 * Compares two \c stat structures with a given comparison
 * operation, returning non-zero if the operation is true with respect
 * to \p stat1 and \p stat2.
 */

int ArchStatCompare(enum ArchStatComparisonOp op,
		    const struct stat *stat1,
		    const struct stat *stat2);

/*!
 * \brief Return the length of a file in bytes.
 * \ingroup group_arch_SystemFunctions
 *
 * Returns -1 if the file cannot be opened/read.
 */
int ArchGetFileLength(const char* fileName);

/*!
 * \brief Returns true if the data in \c stat struct \p st indicates that the
 * target file or directory is writable.
 *
 * This returns true if the struct pointer is valid, and the stat indicates
 * the target is writable by the effective user, effective group, or all
 * users.
 */
bool ArchStatIsWritable(const struct stat *st);

/*!
 * \brief Returns the modification time (mtime) in seconds from the stat struct.
 * \ingroup group_arch_SystemFunctions
 *
 * This function returns the modification time with as much precision as is
 * available in the stat structure for the current platform.
 */
double ArchGetModificationTime(const struct stat& st);

/*!
 * \brief Returns the access time (atime) in seconds from the stat struct.
 * \ingroup group_arch_SystemFunctions
 *
 * This function returns the access time with as much precision as is
 * available in the stat structure for the current platform.
 */
double ArchGetAccessTime(const struct stat& st);

/*!
 * \brief Returns the status change time (ctime) in seconds from the stat struct.
 * \ingroup group_arch_SystemFunctions
 *
 * This function returns the status change time with as much precision as is
 * available in the stat structure for the current platform.
 */
double ArchGetStatusChangeTime(const struct stat& st);

/*!
 * \brief Return the path to a temporary directory for this platform.
 *
 * The returned temporary directory will be a location that will normally
 * be cleaned out on a reboot. This is /var/tmp on Linux machines (for
 * legacy reasons), but /tmp on Darwin machines (/var/tmp on Darwin is
 * specified as a location where files are kept between system reboots -
 * see "man hier"). The returned string will not have a trailing slash.
 * This routine is threadsafe and will not perform any memory allocations.
 *
 */
const char *ArchGetTmpDir();

/*!
 * \brief Make a temporary file name, in a system-determined
 * temporary directory.
 *
 * The result returned has the form TMPDIR/prefix.pid[.n]suffix
 * where TMPDIR is a system-determined temporary directory (typically
 * /tmp or /usr/tmp), pid is the process id of the process, and
 * the optional .n records the number of times this function has been called
 * by a process (and is ommited the first time this function is called).
 *
 * The call is threadsafe.
 *
 * This call opens a security hole because of the race between choosing
 * the name and opening the file.  This call should be avoided in favor
 * of ArchMakeTmpFile().
 */

std::string ArchMakeTmpFileName(const std::string& prefix,
    	    	    	    	const std::string& suffix = std::string());

/*!
 * \brief Create a temporary file, in a system-determined
 * temporary directory.
 *
 * The result returned has the form TMPDIR/prefix.XXXXXX where
 * TMPDIR is a system-determined temporary directory (typically
 * /tmp or /usr/tmp) and XXXXXX is a unique suffix.  Returns the
 * file descriptor of the new file and, if pathname isn't NULL,
 * returns the full path to the file in pathname.  Returns -1 on
 * failure and errno is set.
 *
 * The call is threadsafe.
 */
int ArchMakeTmpFile(const std::string& prefix, std::string* pathname = 0);

/*!
 * \brief Create a temporary file, in a given temporary directory.
 *
 * The result returned has the form TMPDIR/prefix.XXXXXX where
 * TMPDIR is the given temporary directory and XXXXXX is a unique
 * suffix.  Returns the file descriptor of the new file and, if
 * pathname isn't NULL, returns the full path to the file in
 * pathname.  Returns -1 on failure and errno is set.
 *
 * The call is threadsafe.
 */
int ArchMakeTmpFile(const std::string& tmpdir,
                    const std::string& prefix, std::string* pathname = 0);

/*!
 * \brief Create a temporary sub-direcrory, in a given temporary directory.
 *
 * The result returned has the form TMPDIR/prefix.XXXXXX/ where
 * TMPDIR is the given temporary directory and XXXXXX is a unique
 * suffix.  Returns the the full path to the subdir in
 * pathname.  Returns empty string on failure and errno is set.
 *
 * The call is threadsafe.
 */
std::string ArchMakeTmpSubdir(const std::string& tmpdir,
                              const std::string& prefix);

/*!
 * \brief Return all automounted directories.
 *
 * Returns a set of all directories that are automount points for the host.
 */
std::set<std::string> ArchGetAutomountDirectories();

#endif // ARCH_FILESYSTEM_H
