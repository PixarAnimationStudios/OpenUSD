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
#include "pxr/pxr.h"
#include "pxr/usd/usd/zipFile.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/safeOutputFile.h"

#include <boost/crc.hpp>

#include <cstdint>
#include <ctime>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Metafunction that determines if a T instance can be read/written by simple
// bitwise copy.
//
// XXX: This could be std::is_trivially_copyable, but that isn't implemented in
//      older versions of gcc.
template <class T>
struct _IsBitwiseReadWrite {
    static const bool value =
        std::is_enum<T>::value ||
        std::is_arithmetic<T>::value ||
        std::is_trivial<T>::value;
};

struct _InputStream {
    _InputStream(const char* buffer, size_t size, size_t offset = 0)
        : _cur(buffer + offset)
        , _size(size)
        , _buffer(buffer)
    { }

    size_t RemainingSize() const
    {
        return _size - (_cur - _buffer);
    }

    template <class T>
    void Read(T* dest)
    {
        static_assert(_IsBitwiseReadWrite<T>::value, 
                      "Cannot memcpy to non-trivially-copyable type");
        memcpy(reinterpret_cast<char*>(dest), _cur, sizeof(T));
        _cur += sizeof(T);
    }

    inline void Read(void* dest, size_t nBytes)
    {
        memcpy(dest, _cur, nBytes);
        _cur += nBytes;
    }

    inline void Advance(size_t offset)
    {
        _cur += offset;
    }

    inline size_t Tell() const
    {
        return (_cur - _buffer);
    }

    inline const char* TellMemoryAddress() const
    {
        return _cur;
    }

private:
    const char* _cur;
    size_t _size;
    const char* _buffer;
};

struct _OutputStream 
{
    _OutputStream(FILE* f) : _f(f) { }

    template <class T>
    inline void Write(const T& value)
    {
        static_assert(_IsBitwiseReadWrite<T>::value, 
                      "Cannot fwrite non-trivially-copyable type");
        fwrite(&value, sizeof(T), 1, _f);
    }

    inline void Write(const char* buffer, size_t numBytes)
    {
        fwrite(buffer, /* size = */ 1, /* count = */ numBytes, _f);
    }

    inline long Tell() const
    {
        return ftell(_f);
    }

private:
    FILE* _f;
};

// ------------------------------------------------------------

// Local file header for each file in the zip archive.
//
// See section 4.3.7 in zip file specification for more details.
struct _LocalFileHeader
{
    static const uint32_t Signature = 0x04034b50;

    struct Fixed {
        uint32_t signature;
        uint16_t versionForExtract;
        uint16_t bits;
        uint16_t compressionMethod;
        uint16_t lastModTime;
        uint16_t lastModDate;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t uncompressedSize;
        uint16_t filenameLength;
        uint16_t extraFieldLength;
    };
    
    static const size_t FixedSize = 
        sizeof(uint32_t) * 4 + sizeof(uint16_t) * 7;
    static_assert(sizeof(Fixed) >= FixedSize, "");

    // Fixed-length header
    Fixed f;

    // NOTE: 
    // const char* values below do not point to null-terminated strings.
    // Use indicated memory ranges.

    // Filename in [filenameStart, filenameStart + f.filenameLength)
    const char* filenameStart;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    const char* extraFieldStart;
    // File data in [dataStart, dataStart + f.compressedSize)
    const char* dataStart;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }
};

// Read _LocalFileHeader from input stream. Returns an invalid _LocalFileHeader
// if an error occurs or the input stream is too small.
_LocalFileHeader
_ReadLocalFileHeader(_InputStream& src)
{
    // If the source does not have enough bytes to accommodate the
    // fixed-sized portion of the header, bail out so we don't try to
    // read off the end of the source.
    if (src.RemainingSize() < _LocalFileHeader::FixedSize) {
        return _LocalFileHeader();
    }

    _LocalFileHeader h;

    // If signature is not the expected value, reset the source back to
    // its original position and bail.
    src.Read(&h.f.signature);
    if (!h.IsValid()) {
        src.Advance(-sizeof(decltype(h.f.signature)));
        return _LocalFileHeader();
    }

    src.Read(&h.f.versionForExtract);
    src.Read(&h.f.bits);
    src.Read(&h.f.compressionMethod);
    src.Read(&h.f.lastModTime);
    src.Read(&h.f.lastModDate);
    src.Read(&h.f.crc32);
    src.Read(&h.f.compressedSize);
    src.Read(&h.f.uncompressedSize);
    src.Read(&h.f.filenameLength);
    src.Read(&h.f.extraFieldLength);

    if (src.RemainingSize() < h.f.filenameLength) {
        return _LocalFileHeader();
    }

    h.filenameStart = src.TellMemoryAddress();
    src.Advance(h.f.filenameLength);

    if (src.RemainingSize() < h.f.extraFieldLength) {
        return _LocalFileHeader();
    }

    h.extraFieldStart = src.TellMemoryAddress();
    src.Advance(h.f.extraFieldLength);

    if (src.RemainingSize() < h.f.compressedSize) {
        return _LocalFileHeader();
    }

    h.dataStart = src.TellMemoryAddress();
    src.Advance(h.f.compressedSize);

    return h;
}

// Write given _LocalFileHeader to given output stream.
void
_WriteLocalFileHeader(_OutputStream& out, const _LocalFileHeader& h)
{
    out.Write(h.f.signature);
    out.Write(h.f.versionForExtract);
    out.Write(h.f.bits);
    out.Write(h.f.compressionMethod);
    out.Write(h.f.lastModTime);
    out.Write(h.f.lastModDate);
    out.Write(h.f.crc32);
    out.Write(h.f.compressedSize);
    out.Write(h.f.uncompressedSize);
    out.Write(h.f.filenameLength);
    out.Write(h.f.extraFieldLength);
    out.Write(h.filenameStart, h.f.filenameLength);
    out.Write(h.extraFieldStart, h.f.extraFieldLength);
    out.Write(h.dataStart, h.f.compressedSize);
}

// ------------------------------------------------------------

// Central directory header for each file in the zip archive. These headers
// are stored after the data for the last file.
//
// See section 4.3.12 in zip file specification for more details.
struct _CentralDirectoryHeader
{
    static const uint32_t Signature = 0x02014b50;

    struct Fixed {
        uint32_t signature;
        uint16_t versionMadeBy;
        uint16_t versionForExtract;
        uint16_t bits;
        uint16_t compressionMethod;
        uint16_t lastModTime;
        uint16_t lastModDate;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t uncompressedSize;
        uint16_t filenameLength;
        uint16_t extraFieldLength;
        uint16_t commentLength;
        uint16_t diskNumberStart;
        uint16_t internalAttrs;
        uint32_t externalAttrs;
        uint32_t localHeaderOffset;
    };

    static const size_t FixedSize = 
        sizeof(uint32_t) * 6 + sizeof(uint16_t) * 11;
    static_assert(sizeof(Fixed) >= FixedSize, "");

    // Fixed-length header
    Fixed f;

    // NOTE: 
    // const char* values below do not point to null-terminated strings.
    // Use indicated memory ranges.

    // Filename in [filenameStart, filenameStart + f.filenameLength)
    const char* filenameStart;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    const char* extraFieldStart;
    // Comment in [commentStart, commentStart + f.commentLength)
    const char* commentStart;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }
};

// Read _CentralDirectoryHeader from input stream. Returns an invalid
// _CentralDirectoryHeader if an error occurs of the input stream is too small.
_CentralDirectoryHeader
_ReadCentralDirectoryHeader(_InputStream& src)
{
    // If the source does not have enough bytes to accommodate the
    // fixed-sized portion of the header, bail out so we don't try to
    // read off the end of the source.
    if (src.RemainingSize() < _CentralDirectoryHeader::FixedSize) {
        return _CentralDirectoryHeader();
    }

    _CentralDirectoryHeader h;
    src.Read(&h.f.signature);

    // If signature is not the expected value, reset the source back to
    // its original position and bail.
    if (!h.IsValid()) {
        src.Advance(-sizeof(decltype(h.f.signature)));
        return _CentralDirectoryHeader();
    }

    src.Read(&h.f.versionMadeBy);
    src.Read(&h.f.versionForExtract);
    src.Read(&h.f.bits);
    src.Read(&h.f.compressionMethod);
    src.Read(&h.f.lastModTime);
    src.Read(&h.f.lastModDate);
    src.Read(&h.f.crc32);
    src.Read(&h.f.compressedSize);
    src.Read(&h.f.uncompressedSize);
    src.Read(&h.f.filenameLength);
    src.Read(&h.f.extraFieldLength);
    src.Read(&h.f.commentLength);
    src.Read(&h.f.diskNumberStart);
    src.Read(&h.f.internalAttrs);
    src.Read(&h.f.externalAttrs);
    src.Read(&h.f.localHeaderOffset);

    if (src.RemainingSize() < h.f.filenameLength) {
        return _CentralDirectoryHeader();
    }

    h.filenameStart = src.TellMemoryAddress();
    src.Advance(h.f.filenameLength);

    if (src.RemainingSize() < h.f.extraFieldLength) {
        return _CentralDirectoryHeader();
    }

    h.extraFieldStart = src.TellMemoryAddress();
    src.Advance(h.f.extraFieldLength);

    if (src.RemainingSize() < h.f.commentLength) {
        return _CentralDirectoryHeader();
    }

    h.commentStart = src.TellMemoryAddress();
    src.Advance(h.f.commentLength);

    return h;
};

// Write given _CentralDirectoryHeader to given output stream.
void
_WriteCentralDirectoryHeader(
    _OutputStream& out, const _CentralDirectoryHeader& h)
{
    out.Write(h.f.signature);
    out.Write(h.f.versionMadeBy);
    out.Write(h.f.versionForExtract);
    out.Write(h.f.bits);
    out.Write(h.f.compressionMethod);
    out.Write(h.f.lastModTime);
    out.Write(h.f.lastModDate);
    out.Write(h.f.crc32);
    out.Write(h.f.compressedSize);
    out.Write(h.f.uncompressedSize);
    out.Write(h.f.filenameLength);
    out.Write(h.f.extraFieldLength);
    out.Write(h.f.commentLength);
    out.Write(h.f.diskNumberStart);
    out.Write(h.f.internalAttrs);
    out.Write(h.f.externalAttrs);
    out.Write(h.f.localHeaderOffset);
    out.Write(h.filenameStart, h.f.filenameLength);
    out.Write(h.extraFieldStart, h.f.extraFieldLength);
    out.Write(h.commentStart, h.f.commentLength);
}

// ------------------------------------------------------------

// End of central directory record for zip archive. This header is stored
// after the last central directory header.
struct _EndOfCentralDirectoryRecord
{
    static const uint32_t Signature = 0x06054b50;

    struct Fixed {
        uint32_t signature;
        uint16_t diskNumber;
        uint16_t diskNumberForCentralDir;
        uint16_t numCentralDirEntriesOnDisk;
        uint16_t numCentralDirEntries;
        uint32_t centralDirLength;
        uint32_t centralDirOffset;
        uint16_t commentLength;
    };

    static const size_t FixedSize = 
        sizeof(uint32_t) * 3 + sizeof(uint16_t) * 5;
    static_assert(sizeof(Fixed) >= FixedSize, "");

    // Fixed-length header
    Fixed f;

    // NOTE: 
    // const char* values below do not point to null-terminated strings.
    // Use indicated memory ranges.

    // Comment in [commentStart, commentStart + f.commentLength)
    const char* commentStart;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }
};

// Read _EndOfCentralDirectoryRecord from input stream. Returns an invalid 
// _EndOfCentralDirectoryRecord if an error occurs or the input stream is too 
// small.
_EndOfCentralDirectoryRecord
_ReadEndOfCentralDirectoryRecord(_InputStream& src)
{
    // If the source does not have enough bytes to accommodate the
    // fixed-sized portion of the record, bail out so we don't try to
    // read off the end of the source.
    if (src.RemainingSize() < _EndOfCentralDirectoryRecord::FixedSize) {
        return _EndOfCentralDirectoryRecord();
    }

    _EndOfCentralDirectoryRecord r;

    // If signature is not the expected value, reset the source back to
    // its original position and bail.
    src.Read(&r.f.signature);
    if (!r.IsValid()) {
        src.Advance(-sizeof(decltype(r.f.signature)));
        return _EndOfCentralDirectoryRecord();
    }

    src.Read(&r.f.signature);
    src.Read(&r.f.diskNumber);
    src.Read(&r.f.diskNumberForCentralDir);
    src.Read(&r.f.numCentralDirEntriesOnDisk);
    src.Read(&r.f.numCentralDirEntries);
    src.Read(&r.f.centralDirLength);
    src.Read(&r.f.centralDirOffset);
    src.Read(&r.f.commentLength);

    if (src.RemainingSize() < r.f.commentLength) {
        return _EndOfCentralDirectoryRecord();
    }

    r.commentStart = src.TellMemoryAddress();
    src.Advance(r.f.commentLength);

    return r;
}

// Write given _EndOfCentralDirectoryRecord to given output stream.
void
_WriteEndOfCentralDirectoryRecord(
    _OutputStream& out, const _EndOfCentralDirectoryRecord& r)
{
    out.Write(r.f.signature);
    out.Write(r.f.diskNumber);
    out.Write(r.f.diskNumberForCentralDir);
    out.Write(r.f.numCentralDirEntriesOnDisk);
    out.Write(r.f.numCentralDirEntries);
    out.Write(r.f.centralDirLength);
    out.Write(r.f.centralDirOffset);
    out.Write(r.f.commentLength);
    out.Write(r.commentStart, r.f.commentLength);
}

// ------------------------------------------------------------

// Per usdz specifications, file data must be aligned to 64 byte boundaries.
// UsdZipFileWriter adds padding bytes to the 'extra' extensible data field
// described in section 4.5 of the zip specification to achieve this. This
// is complicated by the requirement that each entry in the 'extra' field
// be preceded by a 4 byte header.

struct _ExtraFieldHeader
{
    uint16_t headerId;
    uint16_t dataSize;
};

constexpr size_t _HeaderSize = sizeof(uint16_t) * 2;
constexpr size_t _DataAlignment = 64;

// Maximum size of buffer needed for padding bytes.
constexpr size_t _PaddingBufferSize = _HeaderSize + _DataAlignment;

// Compute the number of padding bytes (including header) needed to align
// data at the given offset to the required alignment.
uint16_t
_ComputeExtraFieldPaddingSize(size_t offset)
{
    uint16_t requiredPadding = _DataAlignment - (offset % _DataAlignment);
    if (requiredPadding == _DataAlignment) {
        requiredPadding = 0;
    }
    else if (requiredPadding < _HeaderSize) {
        // If the amount of padding needed is too small to contain the header,
        // bump the size up while maintaining the required alignment. 
        requiredPadding += _DataAlignment;
    }
    return requiredPadding;
}

// Fill the given extraFieldBuffer to accommodate the specified number of
// padding bytes. For convenience, returns extraFieldBuffer.
const char*
_PrepareExtraFieldPadding(
    char (&extraFieldBuffer)[_PaddingBufferSize],
    uint16_t numPaddingBytes)
{
    if (numPaddingBytes == 0) {
        return nullptr;
    }

    TF_VERIFY(numPaddingBytes >= _HeaderSize);
    TF_VERIFY(numPaddingBytes <= sizeof(extraFieldBuffer));

    _ExtraFieldHeader header;
    header.headerId = 0x1986; // Arbitrarily chosen, unreserved ID.
    header.dataSize = numPaddingBytes - _HeaderSize;
    
    memcpy(extraFieldBuffer, &header.headerId, sizeof(header.headerId)) ;
    memcpy(extraFieldBuffer + sizeof(header.headerId), 
        &header.dataSize, sizeof(header.dataSize));

    return extraFieldBuffer;
}

} // end anonymous namespace

// ------------------------------------------------------------

class UsdZipFile::_Impl
{
public:
    _Impl(std::shared_ptr<const char>&& buffer_, size_t size_)
        : storage(std::move(buffer_))
        , buffer(storage.get())
        , size(size_)
    { }

    std::shared_ptr<const char> storage;

    // This is the same as storage.get(), but saved separately to simplify
    // code so they don't have to call storage.get() all the time.
    const char* buffer;
    size_t size;
};

UsdZipFile
UsdZipFile::Open(const std::string& filePath)
{
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(filePath);
    if (!asset) {
        return UsdZipFile();
    }

    return Open(asset);
}

UsdZipFile
UsdZipFile::Open(const std::shared_ptr<ArAsset>& asset)
{
    if (!asset) {
        TF_CODING_ERROR("Invalid asset");
        return UsdZipFile();
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (!buffer) {
        TF_RUNTIME_ERROR("Could not retrieve buffer from asset");
        return UsdZipFile();
    }

    return UsdZipFile(std::shared_ptr<_Impl>(
        new _Impl(std::move(buffer), asset->GetSize())));
}

UsdZipFile::UsdZipFile(std::shared_ptr<_Impl>&& impl)
    : _impl(std::move(impl))
{
}

UsdZipFile::UsdZipFile()
{
}

UsdZipFile::~UsdZipFile()
{
}

void 
UsdZipFile::DumpContents() const
{
    printf("    Offset\t      Comp\t    Uncomp\tName\n");
    printf("    ------\t      ----\t    ------\t----\n");

    size_t n = 0;
    for (auto it = begin(), e = end(); it != e; ++it, ++n) {
        const FileInfo info = it.GetFileInfo();
        printf("%10zu\t%10zu\t%10zu\t%s\n", 
            info.dataOffset, info.size, info.uncompressedSize, 
            it->c_str());
    }

    printf("----------\n");
    printf("%zu files total\n", n);
}

UsdZipFile::Iterator
UsdZipFile::Find(const std::string& path) const
{
    return std::find(begin(), end(), path);
}

UsdZipFile::Iterator 
UsdZipFile::begin() const
{
    return Iterator(_impl.get());
}

UsdZipFile::Iterator 
UsdZipFile::end() const
{
    return Iterator();
}

UsdZipFile::Iterator::Iterator()
    : _impl(nullptr)
    , _offset(0)
{
}

UsdZipFile::Iterator::Iterator(const _Impl* impl)
    : _impl(impl)
    , _offset(0)
{
    _InputStream src(_impl->buffer, _impl->size, _offset);
    if (!_ReadLocalFileHeader(src).IsValid()) {
        *this = Iterator();
    }
}

UsdZipFile::Iterator::reference 
UsdZipFile::Iterator::operator*() const
{
    _InputStream src(_impl->buffer, _impl->size, _offset);
    _LocalFileHeader h = _ReadLocalFileHeader(src);
    return std::string(h.filenameStart, h.f.filenameLength);
}

UsdZipFile::Iterator::pointer
UsdZipFile::Iterator::operator->() const
{
    return _ArrowProxy(this->operator*());
}

UsdZipFile::Iterator& 
UsdZipFile::Iterator::operator++()
{
    // Advance past the file the iterator is currently pointing
    // to, then check if we can read the next file header. If not,
    // we've hit the end.
    _InputStream src(_impl->buffer, _impl->size, _offset);
    _ReadLocalFileHeader(src);

    size_t newOffset = src.Tell();
    const _LocalFileHeader newHeader = _ReadLocalFileHeader(src);
    if (newHeader.IsValid()) {
        _offset = newOffset;
    }
    else {
        *this = Iterator();
    }
    return *this;
}

UsdZipFile::Iterator
UsdZipFile::Iterator::operator++(int)
{
    Iterator it(*this);
    ++*this;
    return it;
}

bool 
UsdZipFile::Iterator::operator==(const Iterator& rhs) const
{
    return _impl == rhs._impl && _offset == rhs._offset;
}
    
bool 
UsdZipFile::Iterator::operator!=(const Iterator& rhs) const
{
    return !(*this == rhs);
}

const char*
UsdZipFile::Iterator::GetFile() const
{
    _InputStream src(_impl->buffer, _impl->size, _offset);
    const _LocalFileHeader h = _ReadLocalFileHeader(src);
    return h.dataStart;
}

UsdZipFile::FileInfo 
UsdZipFile::Iterator::GetFileInfo() const
{
    _InputStream src(_impl->buffer, _impl->size, _offset);
    const _LocalFileHeader h = _ReadLocalFileHeader(src);

    FileInfo f;
    f.dataOffset = h.dataStart - _impl->buffer;
    f.size = h.f.compressedSize;
    f.uncompressedSize = h.f.uncompressedSize;
    f.compressionMethod = h.f.compressionMethod;
    return f;
}

// ------------------------------------------------------------

namespace
{
// Compute last modified date and time for given file in MS-DOS format.
std::pair<uint16_t, uint16_t>
_ModTimeAndDate(const std::string& filename)
{
    double mtime = 0;
    ArchGetModificationTime(filename.c_str(), &mtime);

    const std::time_t t = static_cast<std::time_t>(mtime);
    const std::tm* localTime = std::localtime(&t);

    // MS-DOS time encoding is a 16-bit value where:
    // - bits 0-4: second divided by 2
    // - bits 5-10: minute (0-59)
    // - bits 11-15: hour (0-23)
    uint16_t modTime = 
        static_cast<uint16_t>(localTime->tm_hour) << 11 |
        static_cast<uint16_t>(localTime->tm_min) << 5 |
        static_cast<uint16_t>(localTime->tm_sec / 2);

    // MS-DOS date encoding is a 16-bit value where:
    // - bits 0-4: day of the month (1-31)
    // - bits 5-8: month (1-12)
    // - bits 9-15: year offset from 1980
    uint16_t modDate = 
        static_cast<uint16_t>(localTime->tm_year - 80) << 9 |
        static_cast<uint16_t>(localTime->tm_mon + 1) << 5 |
        static_cast<uint16_t>(localTime->tm_mday);

    return std::make_pair(modTime, modDate);
}

// Compute CRC32 checksum for given file per zip specification
uint32_t
_Crc32(const ArchConstFileMapping& file)
{
    boost::crc_32_type result;
    result.process_block(
        file.get(), file.get() + ArchGetFileMappingLength(file));
    return result.checksum();
}

// Sanitize the given path to conform to zip file specifications:
//
//   4.4.17.1 The name of the file, with optional relative path.
//   The path stored MUST not contain a drive or
//   device letter, or a leading slash.  All slashes
//   MUST be forward slashes '/' as opposed to
//   backwards slashes '\' for compatibility with Amiga
//   and UNIX file systems etc.  If input came from standard
//   input, there is no file name field.  
std::string
_ZipFilePath(const std::string& filePath)
{
    // TfNormPath will flip all backslashes to forward slashes and
    // strip drive letters.
    std::string result = 
        TfNormPath(filePath, /* stripDriveSpecifier = */ true);

    // Strip off any initial slashes.
    result = TfStringTrimLeft(result, "/");
    return result;
}

} // end anonymous namespace

class UsdZipFileWriter::_Impl
{
public:
    _Impl(TfSafeOutputFile&& out) 
        : outputFile(std::move(out)) 
    { 
    }

    TfSafeOutputFile outputFile;

    // Record for each file added to the zip file:
    //  - File path in zip file
    //  - Fixed portion of local file header
    //  - Offset from beginning of zip file to start of local file header
    using _Record = 
        std::tuple<std::string, _LocalFileHeader::Fixed, uint32_t>;
    std::vector<_Record> addedFiles;
};

UsdZipFileWriter
UsdZipFileWriter::CreateNew(const std::string& filePath)
{
    TfErrorMark mark;
    TfSafeOutputFile outputFile = TfSafeOutputFile::Replace(filePath);
    if (!mark.IsClean()) {
        return UsdZipFileWriter();
    }

    return UsdZipFileWriter(std::unique_ptr<_Impl>(
        new _Impl(std::move(outputFile))));
}

UsdZipFileWriter::UsdZipFileWriter()
{
}

UsdZipFileWriter::UsdZipFileWriter(std::unique_ptr<_Impl>&& impl)
    : _impl(std::move(impl))
{
}

UsdZipFileWriter::UsdZipFileWriter(UsdZipFileWriter&& rhs)
    : _impl(std::move(rhs._impl))
{
}

UsdZipFileWriter& 
UsdZipFileWriter::operator=(UsdZipFileWriter&& rhs)
{
    if (this != &rhs) {
        _impl = std::move(rhs._impl);
    }
    return *this;
}

UsdZipFileWriter::~UsdZipFileWriter()
{
    if (_impl) {
        Save();
    }
}

std::string
UsdZipFileWriter::AddFile(
    const std::string& filePath,
    const std::string& filePathInArchiveIn)
{
    if (!_impl) {
        TF_CODING_ERROR("File is not open for writing");
        return std::string();
    }

    const std::string& filePathInArchive = 
        filePathInArchiveIn.empty() ? filePath : filePathInArchiveIn;

    // Conform the file path we're writing into the archive to make sure
    // it follows zip file specifications.
    const std::string zipFilePath = _ZipFilePath(filePathInArchive);

    // Check if this file has already been written to this zip archive; if so, 
    // just skip it.
    if (std::find_if(
            _impl->addedFiles.begin(), _impl->addedFiles.end(),
            [&zipFilePath](const _Impl::_Record& r) { 
                return std::get<0>(r) == zipFilePath; 
            }) != _impl->addedFiles.end()) {
        return zipFilePath;
    }

    _OutputStream outStream(_impl->outputFile.Get());

    std::string err;
    ArchConstFileMapping mapping = ArchMapFileReadOnly(filePath, &err);
    if (!mapping) {
        TF_RUNTIME_ERROR(
            "Failed to map '%s': %s", filePath.c_str(), err.c_str());
        return std::string();
    }

    // Set up local file header
    _LocalFileHeader h;
    h.f.signature = _LocalFileHeader::Signature;
    h.f.versionForExtract = 10; // Default value
    h.f.bits = 0;
    h.f.compressionMethod = 0; // No compression
    std::tie(h.f.lastModTime, h.f.lastModDate) = _ModTimeAndDate(filePath);
    h.f.crc32 = _Crc32(mapping);
    h.f.compressedSize = ArchGetFileMappingLength(mapping);
    h.f.uncompressedSize = ArchGetFileMappingLength(mapping);
    h.f.filenameLength = zipFilePath.length();
    
    const uint32_t offset = outStream.Tell();
    const size_t dataOffset = 
        offset + _LocalFileHeader::FixedSize + h.f.filenameLength;
    h.f.extraFieldLength = _ComputeExtraFieldPaddingSize(dataOffset);

    h.filenameStart = zipFilePath.data();

    char extraFieldBuffer[_PaddingBufferSize] = { 0 };
    h.extraFieldStart = _PrepareExtraFieldPadding(
        extraFieldBuffer, h.f.extraFieldLength);

    h.dataStart = mapping.get();

    _WriteLocalFileHeader(outStream, h);
    _impl->addedFiles.emplace_back(zipFilePath, h.f, offset);

    return zipFilePath;
}

bool 
UsdZipFileWriter::Save()
{
    if (!_impl) {
        TF_CODING_ERROR("File is not open for writing");
        return false;
    }

    _OutputStream outStream(_impl->outputFile.Get());

    // Write central directory headers for each file added to the zip archive.
    const long centralDirectoryStart = outStream.Tell();

    for (const _Impl::_Record& record : _impl->addedFiles) {
        const std::string& fileToZip = std::get<0>(record);
        const _LocalFileHeader::Fixed& localHeader = std::get<1>(record);
        uint32_t offset = std::get<2>(record);

        _CentralDirectoryHeader h;
        h.f.signature = _CentralDirectoryHeader::Signature;
        h.f.versionMadeBy = 0;
        h.f.versionForExtract = localHeader.versionForExtract;
        h.f.bits = localHeader.bits;
        h.f.compressionMethod = localHeader.compressionMethod;
        h.f.lastModTime = localHeader.lastModTime;
        h.f.lastModDate = localHeader.lastModDate;
        h.f.crc32 = localHeader.crc32;
        h.f.compressedSize = localHeader.compressedSize;
        h.f.uncompressedSize = localHeader.uncompressedSize;
        h.f.filenameLength = localHeader.filenameLength;
        h.f.extraFieldLength = localHeader.extraFieldLength;
        h.f.commentLength = 0;
        h.f.diskNumberStart = 0;
        h.f.internalAttrs = 0;
        h.f.externalAttrs = 0;
        h.f.localHeaderOffset = offset;
        h.filenameStart = fileToZip.data();

        char extraFieldBuffer[_PaddingBufferSize] = { 0 };
        h.extraFieldStart = _PrepareExtraFieldPadding(
            extraFieldBuffer, h.f.extraFieldLength);

        h.commentStart = nullptr;

        _WriteCentralDirectoryHeader(outStream, h);
    }

    const long centralDirectoryEnd = outStream.Tell();

    // Write the end of central directory record.
    {
        _EndOfCentralDirectoryRecord r;
        r.f.signature = _EndOfCentralDirectoryRecord::Signature;
        r.f.diskNumber = 0;
        r.f.diskNumberForCentralDir = 0;
        r.f.numCentralDirEntriesOnDisk = _impl->addedFiles.size();
        r.f.numCentralDirEntries = _impl->addedFiles.size();
        r.f.centralDirLength = (centralDirectoryEnd - centralDirectoryStart);
        r.f.centralDirOffset = centralDirectoryStart;
        r.f.commentLength = 0;
        r.commentStart = nullptr;

        _WriteEndOfCentralDirectoryRecord(outStream, r);
    }

    _impl->outputFile.Close();
    _impl.reset();

    return true;
}

void 
UsdZipFileWriter::Discard()
{
    if (!_impl) {
        TF_CODING_ERROR("File is not open for writing");
        return;
    }

    _impl->outputFile.Discard();
    _impl.reset();
}

PXR_NAMESPACE_CLOSE_SCOPE
