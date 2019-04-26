//
// Copyright 2018 Pixar
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
#ifndef USD_ZIPFILE_H
#define USD_ZIPFILE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;

/// \class UsdZipFile
///
/// Class for reading a zip file. This class is primarily intended to support
/// the .usdz file format. It is not a general-purpose zip reader, as it does
/// not implement the full zip file specification. In particular:
///
/// - This class does not natively support decompressing data from a zip 
///   archive. Clients may access the data exactly as stored in the file and
///   perform their own decompression if desired.
///
/// - This class does not rely on the central directory in order to read the
///   contents of the file. This allows it to operate on partial zip archives.
///   However, this also means it may handle certain zip files incorrectly.
///   For example, if a file was deleted from a zip archive by just removing 
///   its central directory header, that file will still be found by this 
///   class.
///
class UsdZipFile
{
private:
    class _Impl;

public:
    /// Opens the zip archive at \p filePath. 
    /// Returns invalid object on error.
    USD_API
    static UsdZipFile Open(const std::string& filePath);

    /// Opens the zip archive \p asset.
    /// Returns invalid object on error.
    USD_API
    static UsdZipFile Open(const std::shared_ptr<ArAsset>& asset);

    /// Create an invalid UsdZipFile object.
    USD_API
    UsdZipFile();

    USD_API
    ~UsdZipFile();

    /// Return true if this object is valid, false otherwise.
    USD_API
    explicit operator bool() const { return static_cast<bool>(_impl); }

    /// \class FileInfo
    /// Information for a file in the zip archive.
    class FileInfo
    {
    public:
        /// Offset of the beginning of this file's data from the start of
        /// the zip archive.
        size_t dataOffset = 0;

        /// Size of this file as stored in the zip archive. If this file is
        /// compressed, this is its compressed size. Otherwise, this is the
        /// same as the uncompressed size.
        size_t size = 0;

        /// Uncompressed size of this file. This may not be the same as the
        /// size of the file as stored in the zip archive.
        size_t uncompressedSize = 0;

        /// Compression method for this file. See section 4.4.5 of the zip
        /// file specification for valid values. In particular, a value of 0
        /// means this file is stored with no compression.
        uint16_t compressionMethod = 0;

        /// Whether or not this file is encrypted.
        bool encrypted = false;
    };

    /// \class Iterator
    /// Iterator for traversing and inspecting the contents of the zip archive.
    class Iterator 
    {
    public:
        USD_API
        Iterator();

        // Proxy type for operator->(), needed since this iterator's value
        // is generated on the fly.
        class _ArrowProxy 
        {
        public:
            explicit _ArrowProxy(const std::string& s) : _s(s) { }
            const std::string* operator->() const { return &_s; }
        private:
            std::string _s;
        };

        using difference_type = std::ptrdiff_t;
        using value_type = std::string;
        using pointer = _ArrowProxy;
        using reference = std::string;
        using iterator_category = std::forward_iterator_tag;

        USD_API
        Iterator& operator++();
        USD_API
        Iterator operator++(int);

        USD_API
        bool operator==(const Iterator& rhs) const;
        USD_API
        bool operator!=(const Iterator& rhs) const;

        /// Returns filename of the current file in the zip archive.
        USD_API
        reference operator*() const;

        /// Returns filename of the current file in the zip archive.
        USD_API
        pointer operator->() const;

        /// Returns pointer to the beginning of the current file in the
        /// zip archive. The contents of the current file span the range
        /// [GetFile(), GetFile() + GetFileInfo().size). 
        ///
        /// Note that this points to the raw data stored in the zip archive;
        /// no decompression or other transformation is applied.
        USD_API
        const char* GetFile() const;

        /// Returns FileInfo object containing information about the
        /// current file.
        USD_API
        FileInfo GetFileInfo() const;

    private:
        friend class UsdZipFile;
        Iterator(const _Impl* impl);

        const _Impl* _impl;
        size_t _offset;
    };

    /// Returns iterator pointing to the first file in the zip archive.
    USD_API
    Iterator begin() const;

    /// Returns iterator pointing to the first file in the zip archive.
    Iterator cbegin() const { return begin(); }

    /// Returns end iterator for this zip archive.
    USD_API
    Iterator end() const;

    /// Returns end iterator for this zip archive.
    Iterator cend() const { return end(); }

    /// Returns iterator to the file with the given \p path in this zip
    /// archive, or end() if no such file exists.
    USD_API
    Iterator Find(const std::string& path) const;

    /// Print out listing of contents of this zip archive to stdout.
    /// For diagnostic purposes only.
    USD_API
    void DumpContents() const;

private:
    UsdZipFile(std::shared_ptr<_Impl>&& impl);

    std::shared_ptr<_Impl> _impl;
};

/// \class UsdZipFileWriter
///
/// Class for writing a zip file. This class is primarily intended to support
/// the .usdz file format. It is not a general-purpose zip writer, as it does
/// not implement the full zip file specification. However, all files written
/// by this class should be valid zip files and readable by external zip
/// libraries and utilities.
///
class UsdZipFileWriter
{
public:
    /// Create a new file writer with \p filePath as the destination file path
    /// where the zip archive will be written. The zip file will not be written
    /// to \p filePath until the writer is destroyed or Save() is called.
    ///
    /// Returns an invalid object on error.
    USD_API
    static UsdZipFileWriter CreateNew(const std::string& filePath);

    /// Create an invalid UsdZipFileWriter object.
    USD_API
    UsdZipFileWriter();

    /// Calls Save()
    USD_API
    ~UsdZipFileWriter();

    UsdZipFileWriter(const UsdZipFileWriter&) = delete;
    UsdZipFileWriter& operator=(const UsdZipFileWriter&) = delete;

    UsdZipFileWriter(UsdZipFileWriter&& rhs);
    UsdZipFileWriter& operator=(UsdZipFileWriter&& rhs);

    /// Returns true if this is a valid object, false otherwise.
    USD_API
    explicit operator bool() const { return static_cast<bool>(_impl); }

    /// Adds the file at \p filePath to the zip archive with no compression
    /// applied. If \p filePathInArchive is non-empty, the file will be
    /// added at that path in the archive. Otherwise, it will be added
    /// at \p filePath.
    ///
    /// Returns the file path used to identify the file in the zip archive
    /// on success. This path conforms to the zip file specification and may
    /// not be the same as \p filePath or \p filePathInArchive. Returns an 
    /// empty string on failure.
    USD_API
    std::string AddFile(const std::string& filePath,
                        const std::string& filePathInArchive = std::string());

    /// Finalizes the zip archive and saves it to the destination file path.
    /// Once saved, the file writer is invalid and may not be reused.
    /// Returns true on success, false otherwise.
    USD_API
    bool Save();

    /// Discards the zip archive so that it is not saved to the destination
    /// file path. Once discarded, the file writer is invalid and may not be 
    /// reused.
    USD_API
    void Discard();

private:
    class _Impl;
    UsdZipFileWriter(std::unique_ptr<_Impl>&& impl);

    std::unique_ptr<_Impl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_ZIPFILE_H
