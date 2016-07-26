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
///
/// \file Tf/AtomicOfstreamWrapper.h

#ifndef TF_ATOMIC_OSTREAM_WRAPPER_H
#define TF_ATOMIC_OSTREAM_WRAPPER_H

#include <boost/noncopyable.hpp>
#include <fstream>
#include <string>
#include <vector>

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
class TfAtomicOfstreamWrapper : boost::noncopyable
{
public:
    /// Constructor.
    explicit TfAtomicOfstreamWrapper(const std::string& filePath);

    /// Destructor. Calls Cancel().
    ~TfAtomicOfstreamWrapper();

    /// Opens the temporary file for writing. If the destination directory
    /// does not exist, it is created. If the destination directory exists but
    /// is unwritable, the destination directory cannot be created, or the
    /// temporary file cannot be opened for writing in the destination
    /// directory, this method returns false and \p reason is set to the
    /// reason for failure.
    bool Open(std::string* reason = 0);

    /// Synchronizes the temporary file contents to disk, and renames the
    /// temporary file into the file path passed to Open. If the file path
    /// passed to the constructor names an existing file, the file, the file
    /// is atomically replaced with the temporary file. If the rename fails,
    /// false is returned and \p reason is set to the reason for failure.
    bool Commit(std::string* reason = 0);

    /// Closes the temporary file and removes it from disk, if it exists.
    bool Cancel(std::string* reason = 0);

    /// Returns the stream. If this is called before a call to Open, the
    /// returned file stream is not yet initialized. If called after Commit or
    /// Cancel, the returned file stream is closed.
    std::ofstream& GetStream() { return _stream; }

private:
    std::string _filePath;
    std::string _tmpFilePath;
    std::ofstream _stream;
};

#endif // TF_ATOMIC_OSTREAM_WRAPPER_H
