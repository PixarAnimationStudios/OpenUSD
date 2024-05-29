//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_ATOMIC_OFSTREAM_WRAPPER_H
#define PXR_BASE_TF_ATOMIC_OFSTREAM_WRAPPER_H

/// \file tf/atomicOfstreamWrapper.h
/// Atomic file writer with ofstream interface.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <fstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfAtomicOfstreamWrapper
///
/// A class that wraps a file output stream, providing improved tolerance for
/// write failures. The wrapper opens an output file stream to a temporary
/// file on the same file system as the desired destination file, and if no
/// errors occur while writing the temporary file, it can be renamed
/// atomically to the destination file name. In this way, write failures are
/// encountered while writing the temporary file content, rather than while
/// writing the destination file. This ensures that, if the destination
/// existed prior to writing, it is left untouched in the event of a write
/// failure, and if the destination did not exist, a partial file is not
/// written.
///
/// \section cppcode_AtomicOfstreamWrapper Example
/// \code
/// // Create a new wrapper with the destination file path.
/// TfAtomicOfstreamWrapper wrapper("/home/user/realFile.txt");
/// 
/// // Open the wrapped stream.
/// string reason;
/// if (not wrapper.Open(&reason)) {
///     TF_RUNTIME_ERROR(reason);
/// }
///
/// // Write content to the wrapped stream.
/// bool ok = WriteContentToStream(wrapper.GetStream());
///
/// if (ok) {
///     // No errors encountered, rename the temporary file to the real name.
///     string reason;
///     if (not wrapper.Commit(&reason)) {
///         TF_RUNTIME_ERROR(reason);
///     }
/// }
///
/// // If wrapper goes out of scope without being Commit()ed, Cancel() is
/// // called, and the temporary file is removed.
/// \endcode
///
class TfAtomicOfstreamWrapper
{
    TfAtomicOfstreamWrapper(const TfAtomicOfstreamWrapper&) = delete;
    TfAtomicOfstreamWrapper& operator=(
        const TfAtomicOfstreamWrapper&) = delete;
public:
    /// Constructor.
    TF_API explicit TfAtomicOfstreamWrapper(const std::string& filePath);

    /// Destructor. Calls Cancel().
    TF_API ~TfAtomicOfstreamWrapper();

    /// Opens the temporary file for writing. If the destination directory
    /// does not exist, it is created. If the destination directory exists but
    /// is unwritable, the destination directory cannot be created, or the
    /// temporary file cannot be opened for writing in the destination
    /// directory, this method returns false and \p reason is set to the
    /// reason for failure.
    TF_API bool Open(std::string* reason = 0);

    /// Synchronizes the temporary file contents to disk, and renames the
    /// temporary file into the file path passed to Open. If the file path
    /// passed to the constructor names an existing file, the file, the file
    /// is atomically replaced with the temporary file. If the rename fails,
    /// false is returned and \p reason is set to the reason for failure.
    TF_API bool Commit(std::string* reason = 0);

    /// Closes the temporary file and removes it from disk, if it exists.
    TF_API bool Cancel(std::string* reason = 0);

    /// Returns the stream. If this is called before a call to Open, the
    /// returned file stream is not yet initialized. If called after Commit or
    /// Cancel, the returned file stream is closed.
    std::ofstream& GetStream() { return _stream; }

private:
    std::string _filePath;
    std::string _tmpFilePath;
    std::ofstream _stream;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_ATOMIC_OFSTREAM_WRAPPER_H
