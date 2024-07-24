//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/zipFile.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/safeOutputFile.h"

#include <cstdint>
#include <ctime>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>

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

    inline void Seek(size_t offset)
    {
        _cur = _buffer + offset;
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
        uint32_t signature = 0;
        uint16_t versionForExtract = 0;
        uint16_t bits = 0;
        uint16_t compressionMethod = 0;
        uint16_t lastModTime = 0;
        uint16_t lastModDate = 0;
        uint32_t crc32 = 0;
        uint32_t compressedSize = 0;
        uint32_t uncompressedSize = 0;
        uint16_t filenameLength = 0;
        uint16_t extraFieldLength = 0;
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
    const char* filenameStart = nullptr;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    const char* extraFieldStart = nullptr;
    // File data in [dataStart, dataStart + f.compressedSize)
    const char* dataStart = nullptr;

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
    const size_t signaturePos = src.Tell();
    src.Read(&h.f.signature);
    if (!h.IsValid()) {
        src.Seek(signaturePos);
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
        uint32_t signature = 0;
        uint16_t versionMadeBy = 0;
        uint16_t versionForExtract = 0;
        uint16_t bits = 0;
        uint16_t compressionMethod = 0;
        uint16_t lastModTime = 0;
        uint16_t lastModDate = 0;
        uint32_t crc32 = 0;
        uint32_t compressedSize = 0;
        uint32_t uncompressedSize = 0;
        uint16_t filenameLength = 0;
        uint16_t extraFieldLength = 0;
        uint16_t commentLength = 0;
        uint16_t diskNumberStart = 0;
        uint16_t internalAttrs = 0;
        uint32_t externalAttrs = 0;
        uint32_t localHeaderOffset = 0;
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
    const char* filenameStart = nullptr;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    const char* extraFieldStart = nullptr;
    // Comment in [commentStart, commentStart + f.commentLength)
    const char* commentStart = nullptr;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }
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
        uint32_t signature = 0;
        uint16_t diskNumber = 0;
        uint16_t diskNumberForCentralDir = 0;
        uint16_t numCentralDirEntriesOnDisk = 0;
        uint16_t numCentralDirEntries = 0;
        uint32_t centralDirLength = 0;
        uint32_t centralDirOffset = 0;
        uint16_t commentLength = 0;
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
    const char* commentStart = nullptr;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }
};

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
    uint16_t headerId = 0;
    uint16_t dataSize = 0;
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
    std::shared_ptr<const char> storage;

    // Cached mapping of filename to buffer
    std::unordered_map<std::string, Iterator> _cachedPaths;
    // Iterator to start on when adding to the cached mapping
    std::unique_ptr<Iterator> _cachedPathIt;
    // UsdZipFile::begin is called often, so might as well cache it too
    std::unique_ptr<Iterator> _cachedBeginIt;
    // Use a single mutex as there doesn't look to be much contention from
    // calling through via UsdZipFile::begin() and UsdZipFile::Find()
    std::shared_timed_mutex _rwMutex;

    void
    _SetupIterators()
    {
        _cachedBeginIt.reset(new Iterator(this));
        _cachedPathIt.reset(new Iterator(*_cachedBeginIt));
    }

public:
    // This is the same as storage.get(), but saved separately to simplify
    // code so they don't have to call storage.get() all the time.
    const char* buffer;
    size_t size;

    _Impl(std::shared_ptr<const char>&& buffer_, size_t size_)
        : storage(std::move(buffer_))
        , buffer(storage.get())
        , size(size_)
    { }

    Iterator
    CachedBegin()
    {
        {
            std::shared_lock<std::shared_timed_mutex> l(_rwMutex);
            if (_cachedBeginIt) {
                return *_cachedBeginIt;
            }
        }
        std::unique_lock<std::shared_timed_mutex> l(_rwMutex);
        _SetupIterators();
        return *_cachedBeginIt;
    }

    Iterator
    Find(const std::string& path)
    {
        const Iterator vend;
        // ReadOnly lock to lookup if this item has already been found
        {
            std::shared_lock<std::shared_timed_mutex> l(_rwMutex);
            auto cached = _cachedPaths.find(path);
            if (cached != _cachedPaths.end()) {
                return cached->second;
            }
            // Early exit if _cachedPathIt exists and is at end
            if (_cachedPathIt && *_cachedPathIt == vend) {
                return vend;
            }
        }
        // Simplest implementation to lock and iterate linearly until found,
        // filling in cache along the way was chosen as all other
        // 'more complicated' attempts with less contention/blocking didn't
        // add much savings in performance/time.

        // WriteLock for linear traversal saving into cache
        {
            std::unique_lock<std::shared_timed_mutex> l(_rwMutex);
            if (!_cachedPathIt) {
                _SetupIterators();
            }
            for (Iterator& itr = *_cachedPathIt; itr != vend;) {
                auto added = _cachedPaths.emplace(*itr, itr).first;
                ++itr;
                if (path == added->first) {
                    return added->second;
                }
            }
        }

        // ReadOnly lock in case a different thread cached _cachedPaths[path]
        {
            std::shared_lock<std::shared_timed_mutex> l(_rwMutex);
            auto cached = _cachedPaths.find(path);
            if (cached != _cachedPaths.end()) {
                return cached->second;
            }
        }

        return vend;
    }
};

UsdZipFile
UsdZipFile::Open(const std::string& filePath)
{
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(filePath));
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
    return _impl ? _impl->Find(path) : end();
}

UsdZipFile::Iterator 
UsdZipFile::begin() const
{
    return _impl ? _impl->CachedBegin() : end();
}

UsdZipFile::Iterator 
UsdZipFile::end() const
{
    return Iterator();
}

class UsdZipFile::Iterator::_IteratorData
{
public:
    const _Impl* impl = nullptr;
    size_t offset = 0;
    _LocalFileHeader fileHeader;
    size_t nextHeaderOffset = 0;
};

UsdZipFile::Iterator::Iterator() = default;

UsdZipFile::Iterator::Iterator(const _Impl* impl, size_t offset)
{
    _InputStream src(impl->buffer, impl->size, offset);
    _LocalFileHeader fileHeader = _ReadLocalFileHeader(src);
    if (fileHeader.IsValid()) {
        _data.reset(new _IteratorData);
        _data->impl = impl;
        _data->offset = offset;
        _data->fileHeader = fileHeader;
        _data->nextHeaderOffset = src.Tell();
    }
}

UsdZipFile::Iterator::Iterator(const Iterator& rhs)
    : _data(rhs._data ? new _IteratorData(*rhs._data) : nullptr)
{
}

UsdZipFile::Iterator::Iterator(Iterator&& rhs) = default;

UsdZipFile::Iterator::~Iterator() = default;

UsdZipFile::Iterator&
UsdZipFile::Iterator::operator=(const Iterator& rhs)
{
    Iterator rhsCopy(rhs);
    *this = std::move(rhsCopy);
    return *this;
}

UsdZipFile::Iterator&
UsdZipFile::Iterator::operator=(Iterator&& rhs) = default;

UsdZipFile::Iterator::reference 
UsdZipFile::Iterator::operator*() const
{
    if (_data) {
        const _LocalFileHeader& h = _data->fileHeader;
        return std::string(h.filenameStart, h.f.filenameLength);
    }
    return std::string();
}

UsdZipFile::Iterator::pointer
UsdZipFile::Iterator::operator->() const
{
    return _ArrowProxy(this->operator*());
}

UsdZipFile::Iterator& 
UsdZipFile::Iterator::operator++()
{
    // See if we can read a header at the next header offset.
    // If not, we've hit the end.
    if (_data) {
        _InputStream src(
            _data->impl->buffer, _data->impl->size, _data->nextHeaderOffset);
        _LocalFileHeader nextHeader = _ReadLocalFileHeader(src);
        if (nextHeader.IsValid()) {
            _data->offset = _data->nextHeaderOffset;
            _data->fileHeader = nextHeader;
            _data->nextHeaderOffset = src.Tell();
        }
        else {
            *this = Iterator();
        }
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
    if (!_data && !rhs._data) {
        return true;
    }

    return _data && rhs._data
        && _data->impl == rhs._data->impl
        && _data->offset == rhs._data->offset;
}
    
bool 
UsdZipFile::Iterator::operator!=(const Iterator& rhs) const
{
    return !(*this == rhs);
}

const char*
UsdZipFile::Iterator::GetFile() const
{
    return _data ? _data->fileHeader.dataStart : nullptr;
}

UsdZipFile::FileInfo 
UsdZipFile::Iterator::GetFileInfo() const
{
    FileInfo f;
    if (_data) {
        const _LocalFileHeader& h = _data->fileHeader;
        f.dataOffset = h.dataStart - _data->impl->buffer;
        f.size = h.f.compressedSize;
        f.uncompressedSize = h.f.uncompressedSize;
        f.crc = h.f.crc32;
        f.compressionMethod = h.f.compressionMethod;
        f.encrypted = h.f.bits & 0x1; // Per 4.4.4, bit 0 is set if encrypted
    }
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
// The implementation for this function is based on the one found in
// the stbiw__crc32 function of the stb_image_write library.
uint32_t
_Crc32(const ArchConstFileMapping& file)
{
    static unsigned int crc_table[256] =
    {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0eDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    const unsigned char *buffer = 
        reinterpret_cast<const unsigned char *>(file.get());
    const size_t bufferLength = ArchGetFileMappingLength(file);
    unsigned int crc = ~0u;

    for (size_t i = 0; i < bufferLength; ++i) {
        crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
    }
    
    return ~crc;
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
