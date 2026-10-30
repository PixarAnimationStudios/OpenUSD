//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/zipFile.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
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

TF_DEFINE_ENV_SETTING(SDF_MAX_ZIP32_ENTRY_SIZE, -1,
"When writing SdfZipFiles, if the length of a single entry is greater than or "
"equal to this size the 64 bit zip extensions will be used in the archive. A "
"negative value indicates that the maximum allowable size for 32bit zip files, "
"which is UINT32_MAX, should be used. "
"This setting is usually only modified for testing purposes. "
"The default value is -1");

TF_DEFINE_ENV_SETTING(SDF_MAX_ZIPFILE_ENTRY_SIZE, -1,
"When writing SdfZipFiles, ensure that the length of a single entry does not "
"exceed this size.  If a file to be added exceeds this size, an error will "
"be generated. A negative value indicates that the maximum allowable size, "
"which is UINT64_MAX, should be used. "
"This setting is usually only modified for testing purposes. "
"The default value is -1");

TF_DEFINE_ENV_SETTING(SDF_MAX_ZIPFILE_SIZE, -1,
"When writing SdfZipFiles, ensure that the length of the archive does not "
"exceed this size.  If adding a file or saving the archive would push the size "
"over this value an error will be generated. A negative value indicates that "
"the maximum allowable size, which is INT64_MAX, should be used. "
"This setting is usually only modified for testing purposes. "
"The default value is -1");

TF_DEFINE_ENV_SETTING(SDF_MAX_ZIP32_ZIPFILE_SIZE, -1,
"When writing SdfZipFiles, if the length of the archive is greater than or "
"equal to this size, the 64 bit zip extensions will be used. A negative value "
"indicates that the maximum allowable size for 32bit zip files, which is "
"UINT32_MAX, should be used. "
"This setting is usually only modified for testing purposes. "
"The default value is -1");

TF_DEFINE_ENV_SETTING(SDF_MAX_ZIP32_ENTRY_COUNT, -1,
"When writing SdfZipFiles, if the number of entries in the archive is greater "
"than or equal to this count, the 64 bit zip extensions will be used in the "
"archive. A negative value or a value greater than the maximum allowable "
"count for 32bit zip files, which is UINT16_MAX, indicates that UINT16_MAX "
"should be used. "
"This setting is usually only modified for testing purposes. "
"The default value is -1");

static inline size_t Sdf_GetMaxZip32EntrySize() {
    const int envVal = TfGetEnvSetting(SDF_MAX_ZIP32_ENTRY_SIZE);
    return envVal < 0 ? std::numeric_limits<uint32_t>::max() : envVal;
}

static inline uint64_t Sdf_GetMaxZip32ZipFileSize() {
    const int envVal = TfGetEnvSetting(SDF_MAX_ZIP32_ZIPFILE_SIZE);
    return envVal < 0 ? std::numeric_limits<uint32_t>::max() : envVal;
}

static inline uint64_t Sdf_GetMaxZip32EntryCount() {
    const int envVal = TfGetEnvSetting(SDF_MAX_ZIP32_ENTRY_COUNT);
    const uint64_t maxVal = std::numeric_limits<uint16_t>::max();
    return (envVal < 0 || static_cast<uint64_t>(envVal) > maxVal) ?
        maxVal : envVal;
}

static inline size_t Sdf_GetMaxZipFileEntrySize() {
    const int envVal = TfGetEnvSetting(SDF_MAX_ZIPFILE_ENTRY_SIZE);
    return envVal < 0 ? std::numeric_limits<uint64_t>::max() : envVal;
}

// This is signed to match the type returned by _OutputStream::Tell() (i.e.
// ftello / _ftelli64)
static inline int64_t Sdf_GetMaxZipFileSize() {
    const int envVal = TfGetEnvSetting(SDF_MAX_ZIPFILE_SIZE);
    return envVal < 0 ? std::numeric_limits<int64_t>::max() : envVal;
}

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
        _CheckLimit();
    }

    inline void Write(const char* buffer, size_t numBytes)
    {
        fwrite(buffer, /* size = */ 1, /* count = */ numBytes, _f);
        _CheckLimit();
    }

    inline int64_t Tell() const
    {
        #if defined(ARCH_OS_WINDOWS)
            return _ftelli64(_f);
        #else
            return ftello(_f);
        #endif
    }

private:
    enum _WriteError
    {
        MaxArchiveSizeExceeded,
    };

    inline void _CheckLimit() const {
        if (Tell() > Sdf_GetMaxZipFileSize()) {
            TF_ERROR(MaxArchiveSizeExceeded, "Max Archive Size Exceeded");
        }
    }

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

    static const uint32_t Zip64Sentinel = 0xFFFFFFFF;
    static const uint16_t Zip64ExtraFieldHeaderId = 0x0001;
    static const uint16_t Zip64ExtraFieldDataSize = sizeof(uint64_t) * 2;
    static const size_t Zip64ExtraFieldSize =
        sizeof(uint16_t) * 2 + Zip64ExtraFieldDataSize;

    // Fixed-length header
    Fixed f;

    // 64-bit sizes of the file data. These are always populated. If the
    // 32-bit sizes in the fixed-length header is set to Zip64Sentinel, these
    // values will be stored in the Zip64 extended information extra field.
    uint64_t compressedSize64 = 0;
    uint64_t uncompressedSize64 = 0;

    // NOTE: 
    // const char* values below do not point to null-terminated strings.
    // Use indicated memory ranges.

    // Filename in [filenameStart, filenameStart + f.filenameLength)
    const char* filenameStart = nullptr;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    // When reading a header, extraFieldStart points at the beginning of the
    // entire extra field
    // When writing a header that HasZip64ExtraField(), extraFieldStart points 
    // just past the Zip64 extra field.
    const char* extraFieldStart = nullptr;
    // File data in [dataStart, dataStart + compressedSize64)
    const char* dataStart = nullptr;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }

    // Return true if this header requires a Zip64 extended information
    // extra field.
    bool HasZip64ExtraField() const
    {
        return f.compressedSize == Zip64Sentinel ||
            f.uncompressedSize == Zip64Sentinel;
    }
};

// Finds and reads the 64-bit sizes for the given _LocalFileHeader from the
// Zip64 extended information extra field.
bool
_ReadZip64ExtraField(_LocalFileHeader& h)
{
    _InputStream src(h.extraFieldStart, h.f.extraFieldLength);

    while (src.RemainingSize() >= sizeof(uint16_t) * 2) {
        uint16_t headerId = 0;
        uint16_t dataSize = 0;
        src.Read(&headerId);
        src.Read(&dataSize);

        if (src.RemainingSize() < dataSize) {
            return false;
        }

        if (headerId != _LocalFileHeader::Zip64ExtraFieldHeaderId) {
            src.Advance(dataSize);
            continue;
        }

        if (dataSize < _LocalFileHeader::Zip64ExtraFieldDataSize) {
            return false;
        }

        src.Read(&h.uncompressedSize64);
        src.Read(&h.compressedSize64);
        return true;
    }

    return false;
}

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

    h.compressedSize64 = h.f.compressedSize;
    h.uncompressedSize64 = h.f.uncompressedSize;
    if (h.HasZip64ExtraField() && !_ReadZip64ExtraField(h)) {
        TF_WARN("Missing or invalid Zip64 extended information extra field "
                "in local file header for '%s'",
                std::string(h.filenameStart, h.f.filenameLength).c_str());
        return _LocalFileHeader();
    }

    if (src.RemainingSize() < h.compressedSize64) {
        return _LocalFileHeader();
    }

    h.dataStart = src.TellMemoryAddress();
    src.Advance(h.compressedSize64);

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

    // f.extraFieldLength is the combined length of all extra field blocks
    // but extraFieldStart only points at the alignment padding. The Zip64
    // extra field is written here from the 64-bit sizes, so subtract its size
    // to get the number of bytes remaining to be written.
    uint16_t remainingExtraFieldLength = h.f.extraFieldLength;
    if (h.HasZip64ExtraField()) {
        TF_VERIFY(remainingExtraFieldLength >=
            _LocalFileHeader::Zip64ExtraFieldSize);
        const uint16_t headerId = _LocalFileHeader::Zip64ExtraFieldHeaderId;
        const uint16_t dataSize = _LocalFileHeader::Zip64ExtraFieldDataSize;
        out.Write(headerId);
        out.Write(dataSize);
        out.Write(h.uncompressedSize64);
        out.Write(h.compressedSize64);
        remainingExtraFieldLength -= _LocalFileHeader::Zip64ExtraFieldSize;
    }

    out.Write(h.extraFieldStart, remainingExtraFieldLength);
    out.Write(h.dataStart, h.compressedSize64);
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

    // 64-bit sizes of the file data and offset of the local file header.
    uint64_t compressedSize64 = 0;
    uint64_t uncompressedSize64 = 0;
    uint64_t localHeaderOffset64 = 0;

    // The Zip64 extended information extra field always stores all three
    // 64-bit values above.
    static const uint16_t Zip64ExtraFieldDataSize = sizeof(uint64_t) * 3;
    static const size_t Zip64ExtraFieldSize =
        sizeof(uint16_t) * 2 + Zip64ExtraFieldDataSize;

    // NOTE:
    // const char* values below do not point to null-terminated strings.
    // Use indicated memory ranges.

    // Filename in [filenameStart, filenameStart + f.filenameLength)
    const char* filenameStart = nullptr;
    // Extra data in [extraFieldStart, extraFieldStart + f.extraFieldLength)
    // When writing a header that HasZip64ExtraField(), the Zip64 extra field
    // is written from the 64-bit values above, followed by the remaining
    // (f.extraFieldLength - Zip64ExtraFieldSize) bytes in
    // [extraFieldStart, ...).
    const char* extraFieldStart = nullptr;
    // Comment in [commentStart, commentStart + f.commentLength)
    const char* commentStart = nullptr;

    // Return true if the required signature is stored in this header.
    bool IsValid() const
    {
        return f.signature == Signature;
    }

    bool HasZip64ExtraField() const
    {
        return f.compressedSize == _LocalFileHeader::Zip64Sentinel ||
            f.uncompressedSize == _LocalFileHeader::Zip64Sentinel ||
            f.localHeaderOffset == _LocalFileHeader::Zip64Sentinel;
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

    // f.extraFieldLength is the combined length of all extra field blocks
    // but extraFieldStart only points at the blocks that follow the Zip64 extra
    // field (e.g. alignment padding). The Zip64 extra field is written here
    // from the 64-bit values, so subtract its size to get the number of bytes
    // remaining to be written from extraFieldStart.
    uint16_t remainingExtraFieldLength = h.f.extraFieldLength;
    if (h.HasZip64ExtraField()) {
        TF_VERIFY(remainingExtraFieldLength >=
            _CentralDirectoryHeader::Zip64ExtraFieldSize);
        const uint16_t headerId = _LocalFileHeader::Zip64ExtraFieldHeaderId;
        const uint16_t dataSize =
            _CentralDirectoryHeader::Zip64ExtraFieldDataSize;
        out.Write(headerId);
        out.Write(dataSize);
        out.Write(h.uncompressedSize64);
        out.Write(h.compressedSize64);
        out.Write(h.localHeaderOffset64);
        remainingExtraFieldLength -=
            _CentralDirectoryHeader::Zip64ExtraFieldSize;
    }

    out.Write(h.extraFieldStart, remainingExtraFieldLength);
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

    static const uint16_t Zip64Sentinel16 = 0xFFFF;
    static const uint32_t Zip64Sentinel32 = 0xFFFFFFFF;

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

// Informs the reader where it can find the Zip64 End of central directory
// record (4.3.15)
struct _Zip64EndOfCentralDirectoryLocator
{
    static const uint32_t Signature = 0x07064b50;

    uint32_t signature = Signature;
    uint32_t centralDirStartDisk = 0;
    uint64_t endOfCentralDirRecordOffset = 0;
    uint32_t numberOfDisks = 0;

};

void
_WriteZip64EndOfCentralDirectoryLocator(
    _OutputStream& out, const _Zip64EndOfCentralDirectoryLocator& l)
{
    out.Write(l.signature);
    out.Write(l.centralDirStartDisk);
    out.Write(l.endOfCentralDirRecordOffset);
    out.Write(l.numberOfDisks);
}

// End of central directory record for zip 64 archives. This header is stored
// before Zip64 end of central directory locator (4.3.14)
struct _Zip64EndOfCentralDirectoryRecord
{
    static const uint32_t Signature = 0x06064b50;

    struct Fixed {
        uint32_t signature = Signature;
        uint64_t recordSize = 0;

        uint16_t versionMadeBy = 0;
        uint16_t versionNeededToExtract = 0;
        uint32_t diskNumber = 0;
        uint32_t centralDirStartDisk = 0;
        uint64_t currentDiskCentralDirectoryEntryCount = 0;
        uint64_t centralDirectoryEntryCount = 0;
        uint64_t centralDirectorySize = 0;
        uint64_t centralDirectoryOffset = 0;
    };

    // Note that sizeof(Fixed) includes alignment padding between members,
    // so the on-disk size of the fixed-length record is computed explicitly.
    static const size_t FixedSize =
        sizeof(uint32_t) * 3 + sizeof(uint16_t) * 2 + sizeof(uint64_t) * 5;
    static_assert(sizeof(Fixed) >= FixedSize, "");

    Fixed f;
    const char* extensibleDataSector = nullptr;
    size_t extensibleDataSectorSize = 0;

    // Note: 4.3.14.1 states that the size field should not include the 12 bytes
    // occupied by the signature and record size fields.
    inline uint64_t CalculateRecordSize() const {
        return FixedSize + extensibleDataSectorSize -
            (sizeof(f.signature) + sizeof(f.recordSize));
    }
};

void
_WriteZip64EndOfCentralDirectoryRecord(
    _OutputStream& out, const _Zip64EndOfCentralDirectoryRecord& r)
{
    out.Write(r.f.signature);
    out.Write(r.f.recordSize);
    out.Write(r.f.versionMadeBy);
    out.Write(r.f.versionNeededToExtract);
    out.Write(r.f.diskNumber);
    out.Write(r.f.centralDirStartDisk);
    out.Write(r.f.currentDiskCentralDirectoryEntryCount);
    out.Write(r.f.centralDirectoryEntryCount);
    out.Write(r.f.centralDirectorySize);
    out.Write(r.f.centralDirectoryOffset);
    out.Write(r.extensibleDataSector, r.extensibleDataSectorSize);
}

// ------------------------------------------------------------

// Per usdz specifications, file data must be aligned to 64 byte boundaries.
// SdfZipFileWriter adds padding bytes to the 'extra' extensible data field
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

class SdfZipFile::_Impl
{
    std::shared_ptr<const char> storage;

    // Cached mapping of filename to buffer
    std::unordered_map<std::string, Iterator> _cachedPaths;
    // Iterator to start on when adding to the cached mapping
    std::unique_ptr<Iterator> _cachedPathIt;
    // SdfZipFile::begin is called often, so might as well cache it too
    std::unique_ptr<Iterator> _cachedBeginIt;
    // Use a single mutex as there doesn't look to be much contention from
    // calling through via SdfZipFile::begin() and SdfZipFile::Find()
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

SdfZipFile
SdfZipFile::Open(const std::string& filePath)
{
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(filePath));
    if (!asset) {
        return SdfZipFile();
    }

    return Open(asset);
}

SdfZipFile
SdfZipFile::Open(const std::shared_ptr<ArAsset>& asset)
{
    if (!asset) {
        TF_CODING_ERROR("Invalid asset");
        return SdfZipFile();
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (!buffer) {
        TF_RUNTIME_ERROR("Could not retrieve buffer from asset");
        return SdfZipFile();
    }

    return SdfZipFile(std::shared_ptr<_Impl>(
        new _Impl(std::move(buffer), asset->GetSize())));
}

SdfZipFile::SdfZipFile(std::shared_ptr<_Impl>&& impl)
    : _impl(std::move(impl))
{
}

SdfZipFile::SdfZipFile()
{
}

SdfZipFile::~SdfZipFile()
{
}

void 
SdfZipFile::DumpContents() const
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

SdfZipFile::Iterator
SdfZipFile::Find(const std::string& path) const
{
    return _impl ? _impl->Find(path) : end();
}

SdfZipFile::Iterator 
SdfZipFile::begin() const
{
    return _impl ? _impl->CachedBegin() : end();
}

SdfZipFile::Iterator 
SdfZipFile::end() const
{
    return Iterator();
}

class SdfZipFile::Iterator::_IteratorData
{
public:
    const _Impl* impl = nullptr;
    size_t offset = 0;
    _LocalFileHeader fileHeader;
    size_t nextHeaderOffset = 0;
};

SdfZipFile::Iterator::Iterator() = default;

SdfZipFile::Iterator::Iterator(const _Impl* impl, size_t offset)
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

SdfZipFile::Iterator::Iterator(const Iterator& rhs)
    : _data(rhs._data ? new _IteratorData(*rhs._data) : nullptr)
{
}

SdfZipFile::Iterator::Iterator(Iterator&& rhs) = default;

SdfZipFile::Iterator::~Iterator() = default;

SdfZipFile::Iterator&
SdfZipFile::Iterator::operator=(const Iterator& rhs)
{
    Iterator rhsCopy(rhs);
    *this = std::move(rhsCopy);
    return *this;
}

SdfZipFile::Iterator&
SdfZipFile::Iterator::operator=(Iterator&& rhs) = default;

SdfZipFile::Iterator::reference 
SdfZipFile::Iterator::operator*() const
{
    if (_data) {
        const _LocalFileHeader& h = _data->fileHeader;
        return std::string(h.filenameStart, h.f.filenameLength);
    }
    return std::string();
}

SdfZipFile::Iterator::pointer
SdfZipFile::Iterator::operator->() const
{
    return _ArrowProxy(this->operator*());
}

SdfZipFile::Iterator& 
SdfZipFile::Iterator::operator++()
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

SdfZipFile::Iterator
SdfZipFile::Iterator::operator++(int)
{
    Iterator it(*this);
    ++*this;
    return it;
}

bool 
SdfZipFile::Iterator::operator==(const Iterator& rhs) const
{
    if (!_data && !rhs._data) {
        return true;
    }

    return _data && rhs._data
        && _data->impl == rhs._data->impl
        && _data->offset == rhs._data->offset;
}
    
bool 
SdfZipFile::Iterator::operator!=(const Iterator& rhs) const
{
    return !(*this == rhs);
}

const char*
SdfZipFile::Iterator::GetFile() const
{
    return _data ? _data->fileHeader.dataStart : nullptr;
}

SdfZipFile::FileInfo 
SdfZipFile::Iterator::GetFileInfo() const
{
    FileInfo f;
    if (_data) {
        const _LocalFileHeader& h = _data->fileHeader;
        f.dataOffset = h.dataStart - _data->impl->buffer;
        f.size = h.compressedSize64;
        f.uncompressedSize = h.uncompressedSize64;
        f.crc = h.f.crc32;
        f.compressionMethod = h.f.compressionMethod;
        f.encrypted = h.f.bits & 0x1; // Per 4.4.4, bit 0 is set if encrypted
        f.versionNeededToExtract = h.f.versionForExtract;
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

class SdfZipFileWriter::_Impl
{
public:
    _Impl(TfSafeOutputFile&& out) 
        : outputFile(std::move(out)) 
    { 
    }

    TfSafeOutputFile outputFile;

    // this will be true if the Zip64 Extensions are used in this archive,
    // either due to size, or file count.
    bool isZip64 = false;

    // Record for each file added to the zip file.
    struct _Record {
        // File path in zip file
        std::string path;
        // Fixed portion of local file header
        _LocalFileHeader::Fixed localHeader;
        // 64-bit sizes of the file data
        uint64_t compressedSize64 = 0;
        uint64_t uncompressedSize64 = 0;
        // Offset from beginning of zip file to start of local file header
        uint64_t localHeaderOffset = 0;
        // Size of the alignment padding in the local file header's extra
        // field, mirrored in the central directory header's extra field
        uint16_t paddingSize = 0;
    };
    std::vector<_Record> addedFiles;
};

SdfZipFileWriter
SdfZipFileWriter::CreateNew(const std::string& filePath)
{
    TfErrorMark mark;
    TfSafeOutputFile outputFile = TfSafeOutputFile::Replace(filePath);
    if (!mark.IsClean()) {
        return SdfZipFileWriter();
    }

    return SdfZipFileWriter(std::unique_ptr<_Impl>(
        new _Impl(std::move(outputFile))));
}

SdfZipFileWriter::SdfZipFileWriter()
{
}

SdfZipFileWriter::SdfZipFileWriter(std::unique_ptr<_Impl>&& impl)
    : _impl(std::move(impl))
{
}

SdfZipFileWriter::SdfZipFileWriter(SdfZipFileWriter&& rhs)
    : _impl(std::move(rhs._impl))
{
}

SdfZipFileWriter& 
SdfZipFileWriter::operator=(SdfZipFileWriter&& rhs)
{
    if (this != &rhs) {
        _impl = std::move(rhs._impl);
    }
    return *this;
}

SdfZipFileWriter::~SdfZipFileWriter()
{
    if (_impl) {
        Save();
    }
}

static inline bool 
Sdf_CheckZipFileWriteOperation(const TfErrorMark& mark) {
    // The zip file writing code and associated mark that this function
    // is meant to validate should only generate MaxArchiveSizeExceeded errors.
    // Therefore we do not interrogate individual errors contained in the mark.
    if (!mark.IsClean()) {
        mark.Clear();
        TF_RUNTIME_ERROR(
            "Zipfile is larger than maximum allowed size (%" PRId64 ")",
                Sdf_GetMaxZipFileSize()
        );

        return false;
    }

    return true;
}

std::string
SdfZipFileWriter::AddFile(
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
                return r.path == zipFilePath;
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

    // Trigger an error early if the file size is larger than the configured max
    // entry size (see SDF_MAX_ZIPFILE_ENTRY_SIZE).
    if (ArchGetFileMappingLength(mapping) > Sdf_GetMaxZipFileEntrySize()) {
        TF_RUNTIME_ERROR(
            "'%s' exceeds maximum allowable single file size (%zu > %zu)", 
                filePath.c_str(), ArchGetFileMappingLength(mapping),
                Sdf_GetMaxZipFileEntrySize());
        return std::string();
    }

    // Set up local file header
    _LocalFileHeader h;
    h.f.signature = _LocalFileHeader::Signature;
    h.compressedSize64 = ArchGetFileMappingLength(mapping);
    h.uncompressedSize64 = ArchGetFileMappingLength(mapping);

    const uint64_t offset = static_cast<uint64_t>(outStream.Tell());

    if ((ArchGetFileMappingLength(mapping) >= Sdf_GetMaxZip32EntrySize())) {
        _impl->isZip64 = true;
        h.f.versionForExtract = 45; // Zip64 extension support required
        h.f.compressedSize = _LocalFileHeader::Zip64Sentinel;
        h.f.uncompressedSize = _LocalFileHeader::Zip64Sentinel;
    } else {
        h.f.versionForExtract = 10; // Default value
        h.f.compressedSize = h.compressedSize64;
        h.f.uncompressedSize = h.uncompressedSize64;
    }

    if (offset >= Sdf_GetMaxZip32ZipFileSize()) {
        _impl->isZip64 = true;
        h.f.versionForExtract = 45;
    }

    h.f.bits = 0;
    h.f.compressionMethod = 0; // No compression
    std::tie(h.f.lastModTime, h.f.lastModDate) = _ModTimeAndDate(filePath);
    h.f.crc32 = _Crc32(mapping);

    h.f.filenameLength = zipFilePath.length();
    
    // The Zip64 extra field, if present, is written before the padding so
    // it must be accounted for when computing the padding needed to align
    // the file data.
    const size_t zip64ExtraFieldSize = h.HasZip64ExtraField() ?
        _LocalFileHeader::Zip64ExtraFieldSize : 0;

    const size_t dataOffset =
        offset + _LocalFileHeader::FixedSize + h.f.filenameLength +
        zip64ExtraFieldSize;
    const uint16_t paddingSize = _ComputeExtraFieldPaddingSize(dataOffset);
    h.f.extraFieldLength = zip64ExtraFieldSize + paddingSize;

    h.filenameStart = zipFilePath.data();

    char extraFieldBuffer[_PaddingBufferSize] = { 0 };
    h.extraFieldStart = _PrepareExtraFieldPadding(
        extraFieldBuffer, paddingSize);

    h.dataStart = mapping.get();

    TfErrorMark mark;
    _WriteLocalFileHeader(outStream, h);
    if (!Sdf_CheckZipFileWriteOperation(mark)) {
        return {};
    }

    _Impl::_Record record;
    record.path = zipFilePath;
    record.localHeader = h.f;
    record.compressedSize64 = h.compressedSize64;
    record.uncompressedSize64 = h.uncompressedSize64;
    record.localHeaderOffset = offset;
    record.paddingSize = paddingSize;
    _impl->addedFiles.push_back(std::move(record));

    return zipFilePath;
}

bool 
SdfZipFileWriter::Save()
{
    if (!_impl) {
        TF_CODING_ERROR("File is not open for writing");
        return false;
    }

    TfErrorMark mark;
    _OutputStream outStream(_impl->outputFile.Get());

    // Write central directory headers for each file added to the zip archive.
    const int64_t centralDirectoryStart = outStream.Tell();

    for (const _Impl::_Record& record : _impl->addedFiles) {
        const _LocalFileHeader::Fixed& localHeader = record.localHeader;

        _CentralDirectoryHeader h;
        h.f.signature = _CentralDirectoryHeader::Signature;
        h.f.versionMadeBy = 0;
        h.f.versionForExtract = localHeader.versionForExtract;
        h.f.bits = localHeader.bits;
        h.f.compressionMethod = localHeader.compressionMethod;
        h.f.lastModTime = localHeader.lastModTime;
        h.f.lastModDate = localHeader.lastModDate;
        h.f.crc32 = localHeader.crc32;
        h.f.filenameLength = localHeader.filenameLength;
        h.f.commentLength = 0;
        h.f.diskNumberStart = 0;
        h.f.internalAttrs = 0;
        h.f.externalAttrs = 0;
        h.compressedSize64 = record.compressedSize64;
        h.uncompressedSize64 = record.uncompressedSize64;
        h.localHeaderOffset64 = record.localHeaderOffset;

        // A Zip64 extra field is needed if the file is too large for the
        // 32-bit size fields or its local file header starts beyond the
        // range of the 32-bit offset field. The extra field always stores
        // all three 64-bit values, so all three 32-bit fields are set to
        // Zip64Sentinel. Note that versionForExtract and isZip64 were
        // already set in AddFile().
        const bool needsZip64ExtraField =
            localHeader.compressedSize == _LocalFileHeader::Zip64Sentinel ||
            record.localHeaderOffset >= Sdf_GetMaxZip32ZipFileSize();
        if (needsZip64ExtraField) {
            h.f.compressedSize = _LocalFileHeader::Zip64Sentinel;
            h.f.uncompressedSize = _LocalFileHeader::Zip64Sentinel;
            h.f.localHeaderOffset = _LocalFileHeader::Zip64Sentinel;
        } else {
            h.f.compressedSize = localHeader.compressedSize;
            h.f.uncompressedSize = localHeader.uncompressedSize;
            h.f.localHeaderOffset = record.localHeaderOffset;
        }

        h.filenameStart = record.path.data();

        // The Zip64 extra field (if needed) is followed by padding of the
        // same size as in the local file header. This padding is not needed
        // for alignment, but mirroring it keeps the central directory
        // consistent with the local file header.
        h.f.extraFieldLength =
            (h.HasZip64ExtraField() ?
                _CentralDirectoryHeader::Zip64ExtraFieldSize : 0) +
            record.paddingSize;

        char extraFieldBuffer[_PaddingBufferSize] = { 0 };
        h.extraFieldStart = _PrepareExtraFieldPadding(
            extraFieldBuffer, record.paddingSize);

        h.commentStart = nullptr;

        _WriteCentralDirectoryHeader(outStream, h);
        if (!Sdf_CheckZipFileWriteOperation(mark)) {
            _impl->outputFile.Close();
            _impl.reset();

            return false;
        }
    }

    const int64_t centralDirectoryEnd = outStream.Tell();

    const uint64_t numEntries = _impl->addedFiles.size();
    const uint64_t centralDirectoryLength =
        static_cast<uint64_t>(centralDirectoryEnd - centralDirectoryStart);
    const uint64_t centralDirectoryOffset =
        static_cast<uint64_t>(centralDirectoryStart);

    const bool entriesOverflow = numEntries >= Sdf_GetMaxZip32EntryCount();
    const bool lengthOverflow =
        centralDirectoryLength >= Sdf_GetMaxZip32ZipFileSize();
    const bool offsetOverflow =
        centralDirectoryOffset >= Sdf_GetMaxZip32ZipFileSize();

    if (entriesOverflow || lengthOverflow || offsetOverflow) {
        _impl->isZip64 = true;
    }

    if (_impl->isZip64) {
        const uint64_t zip64RecordOffset =
            static_cast<uint64_t>(centralDirectoryEnd);

        _Zip64EndOfCentralDirectoryRecord zip64Record;
        zip64Record.f.signature = _Zip64EndOfCentralDirectoryRecord::Signature;
        zip64Record.f.recordSize = zip64Record.CalculateRecordSize();
        zip64Record.f.versionMadeBy = 0;
        zip64Record.f.versionNeededToExtract = 45;
        zip64Record.f.diskNumber = 0;
        zip64Record.f.centralDirStartDisk = 0;
        zip64Record.f.currentDiskCentralDirectoryEntryCount = numEntries;
        zip64Record.f.centralDirectoryEntryCount = numEntries;
        zip64Record.f.centralDirectorySize = centralDirectoryLength;
        zip64Record.f.centralDirectoryOffset = centralDirectoryOffset;

        _WriteZip64EndOfCentralDirectoryRecord(outStream, zip64Record);

        _Zip64EndOfCentralDirectoryLocator zip64Locator;
        zip64Locator.signature = _Zip64EndOfCentralDirectoryLocator::Signature;
        zip64Locator.centralDirStartDisk = 0;
        zip64Locator.endOfCentralDirRecordOffset = zip64RecordOffset;
        zip64Locator.numberOfDisks = 1;

        _WriteZip64EndOfCentralDirectoryLocator(outStream, zip64Locator);
    }

    // Write the end of central directory record.
    {
        _EndOfCentralDirectoryRecord r;
        r.f.signature = _EndOfCentralDirectoryRecord::Signature;
        r.f.diskNumber = 0;
        r.f.diskNumberForCentralDir = 0;
        r.f.numCentralDirEntriesOnDisk = entriesOverflow ?
            _EndOfCentralDirectoryRecord::Zip64Sentinel16 : numEntries;
        r.f.numCentralDirEntries = entriesOverflow ?
            _EndOfCentralDirectoryRecord::Zip64Sentinel16 : numEntries;
        r.f.centralDirLength = lengthOverflow ?
            _EndOfCentralDirectoryRecord::Zip64Sentinel32 :
            centralDirectoryLength;
        r.f.centralDirOffset = offsetOverflow ?
            _EndOfCentralDirectoryRecord::Zip64Sentinel32 :
            centralDirectoryOffset;
        r.f.commentLength = 0;
        r.commentStart = nullptr;

        _WriteEndOfCentralDirectoryRecord(outStream, r);
    }

    _impl->outputFile.Close();
    _impl.reset();

    return Sdf_CheckZipFileWriteOperation(mark);
}

void 
SdfZipFileWriter::Discard()
{
    if (!_impl) {
        TF_CODING_ERROR("File is not open for writing");
        return;
    }

    _impl->outputFile.Discard();
    _impl.reset();
}

PXR_NAMESPACE_CLOSE_SCOPE
