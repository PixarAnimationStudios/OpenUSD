//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Atomically rename \p srcFileName over \p dstFileName, assuming they are
// sibling files on the same filesystem.  Set \p error and return false in case
// of an error, otherwise return true.
bool
Tf_AtomicRenameFileOver(std::string const &srcFileName,
                        std::string const &dstFileName,
                        std::string *error);

// Attempt to create a temporary sibling file of \p fileName.  If succesful
// return the realpath of \p fileName in \p realFileName, the created temporary
// file name in \p tempFileName, and its open file descriptor.  In case of an
// error, set \p error and return -1.
int
Tf_CreateSiblingTempFile(std::string fileName,
                         std::string *realFileName,
                         std::string *tempFileName,
                         std::string *error);

PXR_NAMESPACE_CLOSE_SCOPE
