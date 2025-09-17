//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SAFE_OUTPUT_FILE_H
#define PXR_BASE_TF_SAFE_OUTPUT_FILE_H

/// \file tf/safeOutputFile.h
/// Safe file writer with FILE * interface.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <cstdio>
#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfSafeOutputFile
///
/// Opens a file for output, either for update "r+" or to completely replace
/// "w+".  In the case of complete replacement, create a sibling temporary file
/// to write to instead.  When writing is complete, rename the temporary file
/// over the target file.  This provides some safety to other processes reading
/// the existing file (at least on unix-like OSs).  They will continue to see
/// the existing contents of the old file.  If we overwrote the file itself,
/// then those other processes would see undefined, possibly partially updated
/// content.
class TfSafeOutputFile
{
    TfSafeOutputFile(TfSafeOutputFile const &) = delete;
    TfSafeOutputFile &operator=(TfSafeOutputFile const &) = delete;
public:
    TfSafeOutputFile() = default;

    TfSafeOutputFile(TfSafeOutputFile &&other)
        : _file(other._file)
        , _targetFileName(std::move(other._targetFileName))
        , _tempFileName(std::move(other._tempFileName))
        { other._file = nullptr; }

    TfSafeOutputFile &operator=(TfSafeOutputFile &&other) {
        _file = other._file;
        _targetFileName = std::move(other._targetFileName);
        _tempFileName = std::move(other._tempFileName);
        other._file = nullptr;
        return *this;
    }

    /// Destructor invokes Close().
    TF_API ~TfSafeOutputFile();

    /// Open \p fileName for update ("r+").
    TF_API static TfSafeOutputFile Update(std::string const &fileName);

    /// Arrange for \p fileName to be replaced.  Create a sibling temporary file
    /// and open that for writing.  When Close() is called (or the destructor is
    /// run) close the temporary file and rename it over \p fileName.
    TF_API static TfSafeOutputFile Replace(std::string const &fileName);

    /// Close the file.  If the file was opened with Replace(), rename the
    /// temporary file over the target file to replace it.
    TF_API void Close();

    /// Close the file.  If the file was opened with Replace(), the temporary
    /// file is removed and not renamed over the target file.  It is an error
    /// to call this for files opened for Update.
    TF_API void Discard();

    /// Return the opened FILE *.
    FILE *Get() const { return _file; }
    
    /// If the underlying file was opened by Update(), return it.  The caller
    /// takes responsibility for closing the file later.  It is an error to call
    /// this for files opened for Replace.
    TF_API FILE *ReleaseUpdatedFile();

    /// Return true if this TfSafeOutputFile was created by a call to Update(),
    /// false otherwise.
    TF_API bool IsOpenForUpdate() const;

private:
    FILE *_file = nullptr;
    std::string _targetFileName;
    std::string _tempFileName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SAFE_OUTPUT_FILE_H
