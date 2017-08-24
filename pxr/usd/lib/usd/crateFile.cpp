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
#include "pxr/pxr.h"
#include "crateFile.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/regex.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <tbb/concurrent_queue.h>

#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

static int PAGESIZE = ArchGetPageSize();

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<Usd_CrateFile::TimeSamples>();
}

#define DEFAULT_NEW_VERSION "0.0.1"
TF_DEFINE_ENV_SETTING(
    USD_WRITE_NEW_USDC_FILES_AS_VERSION, DEFAULT_NEW_VERSION,
    "When writing new Usd Crate files, write them as this version.  "
    "This must have the same major version as the software and have less or "
    "equal minor and patch versions.  This is only for new files; saving "
    "edits to an existing file preserves its version.");

TF_DEFINE_ENV_SETTING(
    USDC_MMAP_PREFETCH_KB, 0,
    "If set to a nonzero value, attempt to disable the OS's prefetching "
    "behavior for memory-mapped files and instead do simple aligned block "
    "fetches of the given size instead.  If necessary the setting value is "
    "rounded up to the next whole multiple of the system's page size "
    "(typically 4 KB).");

static int _GetMMapPrefetchKB()
{
    auto getKB = []() {
        int setting = TfGetEnvSetting(USDC_MMAP_PREFETCH_KB);
        int kb = ((setting * 1024 + PAGESIZE - 1) & ~(PAGESIZE - 1)) / 1024;
        if (setting != kb) {
            fprintf(stderr, "Rounded USDC_MMAP_PREFETCH_KB value %d to %d",
                    setting, kb);
        }
        return kb;
    };
    static int kb = getKB();
    return kb;
}

// Write nbytes bytes to fd at pos.
static inline ssize_t
WriteToFd(FILE *file, void const *bytes, int64_t nbytes, int64_t pos) {
    int64_t nwritten = ArchPWrite(file, bytes, nbytes, pos);
    if (ARCH_UNLIKELY(nwritten < 0)) {
        TF_RUNTIME_ERROR("Failed writing usdc data: %s",
                         ArchStrerror().c_str());
        nwritten = 0;
    }
    return nwritten;
}

namespace Usd_CrateFile
{
// Metafunction that determines if a T instance can be read/written by simple
// bitwise copy.
template <class T>
struct _IsBitwiseReadWrite {
    static const bool value =
        std::is_enum<T>::value ||
        std::is_arithmetic<T>::value ||
        std::is_same<T, GfHalf>::value ||
        std::is_trivial<T>::value ||
        GfIsGfVec<T>::value ||
        GfIsGfMatrix<T>::value ||
        GfIsGfQuat<T>::value ||
        std::is_base_of<Index, T>::value;
};
} // Usd_CrateFile

namespace {

// We use type char and a deleter for char[] instead of just using
// type char[] due to a (now fixed) bug in libc++ in LLVM.  See
// https://llvm.org/bugs/show_bug.cgi?id=18350.
typedef std::unique_ptr<char, std::default_delete<char[]> > RawDataPtr;

using namespace Usd_CrateFile;

// To add a new section, add a name here and add that name to _KnownSections
// below, then add handling for it in _Write and _ReadStructuralSections.
constexpr _SectionName _TokensSectionName = "TOKENS";
constexpr _SectionName _StringsSectionName = "STRINGS";
constexpr _SectionName _FieldsSectionName = "FIELDS";
constexpr _SectionName _FieldSetsSectionName = "FIELDSETS";
constexpr _SectionName _PathsSectionName = "PATHS";
constexpr _SectionName _SpecsSectionName = "SPECS";

constexpr _SectionName _KnownSections[] = {
    _TokensSectionName, _StringsSectionName, _FieldsSectionName,
    _FieldSetsSectionName, _PathsSectionName, _SpecsSectionName
};

template <class T>
constexpr bool _IsInlinedType() {
    using std::is_same;
    return is_same<T, string>::value ||
        is_same<T, TfToken>::value ||
        is_same<T, SdfPath>::value ||
        is_same<T, SdfAssetPath>::value ||
        (sizeof(T) <= sizeof(uint32_t) && _IsBitwiseReadWrite<T>::value);
}

template <class T>
constexpr Usd_CrateFile::TypeEnum TypeEnumFor();
#define xx(ENUMNAME, _unused1, CPPTYPE, _unused2)               \
    template <>                                                 \
    constexpr TypeEnum TypeEnumFor<CPPTYPE>() {                 \
        return TypeEnum::ENUMNAME;                              \
    }
#include "crateDataTypes.h"
#undef xx

template <class T> struct ValueTypeTraits {};
// Generate value type traits providing enum value, array support, and whether
// or not the value may be inlined.
#define xx(ENUMNAME, _unused, CPPTYPE, SUPPORTSARRAY)                          \
    template <> struct ValueTypeTraits<CPPTYPE> {                              \
        static constexpr bool supportsArray = SUPPORTSARRAY;                   \
        static constexpr bool isInlined = _IsInlinedType<CPPTYPE>();           \
    };
#include "crateDataTypes.h"
#undef xx

template <class T>
static constexpr ValueRep ValueRepFor(uint64_t payload = 0) {
    return ValueRep(TypeEnumFor<T>(),
                    ValueTypeTraits<T>::isInlined, /*isArray=*/false, payload);
}

template <class T>
static constexpr ValueRep ValueRepForArray(uint64_t payload = 0) {
    return ValueRep(TypeEnumFor<T>(),
                    /*isInlined=*/false, /*isArray=*/true, payload);
}

} // anon


namespace Usd_CrateFile {

// XXX: These checks ensure VtValue can hold ValueRep in the lightest
// possible way -- WBN not to rely on intenral knowledge of that.
static_assert(boost::has_trivial_constructor<ValueRep>::value, "");
static_assert(boost::has_trivial_copy<ValueRep>::value, "");
static_assert(boost::has_trivial_assign<ValueRep>::value, "");
static_assert(boost::has_trivial_destructor<ValueRep>::value, "");

using namespace Usd_CrateValueInliners;

using std::make_pair;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

// Version history:
// 0.2.0: Added support for prepend and append fields of SdfListOp.
// 0.1.0: Fixed structure layout issue encountered in Windows port.
//        See _PathItemHeader_0_0_1.
// 0.0.1: Initial release.
constexpr uint8_t USDC_MAJOR = 0;
constexpr uint8_t USDC_MINOR = 2;
constexpr uint8_t USDC_PATCH = 0;

struct CrateFile::Version
{
    // Not named 'major' since that's a macro name conflict on POSIXes.
    uint8_t majver, minver, patchver;

    constexpr Version() : Version(0,0,0) {}
    constexpr Version(uint8_t majver, uint8_t minver, uint8_t patchver)
        : majver(majver), minver(minver), patchver(patchver) {}

    explicit Version(CrateFile::_BootStrap const &boot)
        : Version(boot.version[0], boot.version[1], boot.version[2]) {}
    
    static Version FromString(char const *str) {
        uint32_t maj, min, pat;
        if (sscanf(str, "%u.%u.%u", &maj, &min, &pat) != 3 ||
            maj > 255 || min > 255 || pat > 255) {
            return Version();
        }
        return Version(maj, min, pat);
    }

    constexpr uint32_t AsInt() const {
        return static_cast<uint32_t>(majver) << 16 |
            static_cast<uint32_t>(minver) << 8 |
            static_cast<uint32_t>(patchver);
    }

    std::string AsString() const {
        return TfStringPrintf("%d.%d.%d", majver, minver, patchver);
    }

    bool IsValid() const { return AsInt() != 0; }

    // Return true if fileVer has the same major version as this, and has a
    // lesser or same minor version.  Patch version irrelevant, since the
    // versioning scheme specifies that patch level changes are
    // forward-compatible.
    bool CanRead(Version const &fileVer) const {
        return fileVer.majver == majver && fileVer.minver <= minver;
    }

    // Return true if fileVer has the same major version as this, and has a
    // lesser minor version, or has the same minor version and a lesser or equal
    // patch version.
    bool CanWrite(Version const &fileVer) const {
        return fileVer.majver == majver &&
            (fileVer.minver < minver ||
             (fileVer.minver == minver && fileVer.patchver <= patchver));
    }        
    
#define LOGIC_OP(op)                                                    \
    constexpr bool operator op(Version const &other) const {            \
        return AsInt() op other.AsInt();                                \
    }
    LOGIC_OP(==); LOGIC_OP(!=);
    LOGIC_OP(<);  LOGIC_OP(>);
    LOGIC_OP(<=); LOGIC_OP(>=);
#undef LOGIC_OP
};

constexpr CrateFile::Version
_SoftwareVersion { USDC_MAJOR, USDC_MINOR, USDC_PATCH };

static CrateFile::Version
_GetVersionForNewlyCreatedFiles() {
    // Read the env setting and try to parse a version.  If that fails to
    // give a version this software is capable of writing, fall back to the
    // _SoftwareVersion.
    string setting = TfGetEnvSetting(USD_WRITE_NEW_USDC_FILES_AS_VERSION);
    auto ver = CrateFile::Version::FromString(setting.c_str());
    if (!ver.IsValid() || !_SoftwareVersion.CanWrite(ver)) {
        TF_WARN("Invalid value '%s' for USD_WRITE_NEW_USDC_FILES_AS_VERSION - "
                "falling back to default '%s'",
                setting.c_str(), DEFAULT_NEW_VERSION);
        ver = CrateFile::Version::FromString(DEFAULT_NEW_VERSION);
    }
    return ver;
}

static CrateFile::Version
GetVersionForNewlyCreatedFiles() {
    static CrateFile::Version ver = _GetVersionForNewlyCreatedFiles();
    return ver;
}

constexpr char const *USDC_IDENT = "PXR-USDC"; // 8 chars.

struct _PathItemHeader_0_0_1 {
    _PathItemHeader_0_0_1() {}
    _PathItemHeader_0_0_1(PathIndex pi, TokenIndex ti, uint8_t bs)
        : index(pi), elementTokenIndex(ti), bits(bs) {}

    // Deriving _BitwiseReadWrite and having members PathIndex and TokenIndex
    // that derive _BitwiseReadWrite caused gcc on linux and mac to leave 4
    // bytes at the head of this structure, making the whole thing 16 bytes,
    // with the members starting at offset 4.  This was revealed in the Windows
    // port since MSVC made this struct 12 bytes, as intended.  To fix this we
    // have two versions of the struct.  Version 0.0.1 files read/write this
    // structure.  Version 0.1.0 and newer read/write the new one.
    uint32_t _unused_padding_;

    PathIndex index;
    TokenIndex elementTokenIndex;
    uint8_t bits;
};
template <>
struct _IsBitwiseReadWrite<_PathItemHeader_0_0_1> : std::true_type {};

struct _PathItemHeader {
    _PathItemHeader() {}
    _PathItemHeader(PathIndex pi, TokenIndex ti, uint8_t bs)
        : index(pi), elementTokenIndex(ti), bits(bs) {}
    static const uint8_t HasChildBit = 1 << 0;
    static const uint8_t HasSiblingBit = 1 << 1;
    static const uint8_t IsPrimPropertyPathBit = 1 << 2;
    PathIndex index;
    TokenIndex elementTokenIndex;
    uint8_t bits;
};
template <>
struct _IsBitwiseReadWrite<_PathItemHeader> : std::true_type {};

struct _ListOpHeader {
    enum _Bits { IsExplicitBit = 1 << 0,
                 HasExplicitItemsBit = 1 << 1,
                 HasAddedItemsBit = 1 << 2,
                 HasDeletedItemsBit = 1 << 3,
                 HasOrderedItemsBit = 1 << 4,
                 HasPrependedItemsBit = 1 << 5,
                 HasAppendedItemsBit = 1 << 6 };

    _ListOpHeader() : bits(0) {}

    template <class T>
    explicit _ListOpHeader(SdfListOp<T> const &op) : bits(0) {
        bits |= op.IsExplicit() ? IsExplicitBit : 0;
        bits |= op.GetExplicitItems().size() ? HasExplicitItemsBit : 0;
        bits |= op.GetAddedItems().size() ? HasAddedItemsBit : 0;
        bits |= op.GetPrependedItems().size() ? HasPrependedItemsBit : 0;
        bits |= op.GetAppendedItems().size() ? HasAppendedItemsBit : 0;
        bits |= op.GetDeletedItems().size() ? HasDeletedItemsBit : 0;
        bits |= op.GetOrderedItems().size() ? HasOrderedItemsBit : 0;
    }

    bool IsExplicit() const { return bits & IsExplicitBit; }

    bool HasExplicitItems() const { return bits & HasExplicitItemsBit; }
    bool HasAddedItems() const { return bits & HasAddedItemsBit; }
    bool HasPrependedItems() const { return bits & HasPrependedItemsBit; }
    bool HasAppendedItems() const { return bits & HasAppendedItemsBit; }
    bool HasDeletedItems() const { return bits & HasDeletedItemsBit; }
    bool HasOrderedItems() const { return bits & HasOrderedItemsBit; }

    uint8_t bits;
};
template <> struct _IsBitwiseReadWrite<_ListOpHeader> : std::true_type {};

struct _MmapStream {
    
    explicit _MmapStream(ArchConstFileMapping const &mapStart,
                         char *debugPageMap)
        : _cur(mapStart.get())
        , _mapStart(mapStart.get())
        , _length(ArchGetFileMappingLength(mapStart))
        , _debugPageMap(debugPageMap)
        , _prefetchKB(_GetMMapPrefetchKB()) {}

    _MmapStream &DisablePrefetch() {
        _prefetchKB = 0;
        return *this;
    }
    
    inline void Read(void *dest, size_t nBytes) {
        if (ARCH_UNLIKELY(_debugPageMap)) {
            int64_t pageStart = (_cur - _mapStart) / PAGESIZE;
            int64_t pageEnd = ((_cur + nBytes - 1 - _mapStart) / PAGESIZE) + 1;
            memset(_debugPageMap + pageStart, 1, pageEnd-pageStart);
        }

        if (_prefetchKB) {
            // Custom aligned chunk "prefetch".
            const auto chunkBytes = _prefetchKB * 1024;
            auto firstChunk = (_cur-_mapStart) / chunkBytes;
            auto lastChunk = ((_cur-_mapStart) + nBytes) / chunkBytes;
            
            char const *beginAddr = _mapStart + firstChunk * chunkBytes;
            char const *endAddr =
                _mapStart + std::min(_length, (lastChunk + 1) * chunkBytes);
            
            ArchMemAdvise(reinterpret_cast<void *>(
                              const_cast<char *>(beginAddr)),
                          endAddr-beginAddr, ArchMemAdviceWillNeed);
        }

        memcpy(dest, _cur, nBytes);
        
        _cur += nBytes;
    }
    inline int64_t Tell() const { return _cur - _mapStart; }
    inline void Seek(int64_t offset) { _cur = _mapStart + offset; }
    inline void Prefetch(int64_t offset, int64_t size) {
        ArchMemAdvise(_mapStart + offset, size, ArchMemAdviceWillNeed);
    }

private:
    char const *_cur;
    char const *_mapStart;
    size_t _length;
    char *_debugPageMap;
    int _prefetchKB;
};

struct _PreadStream {
    explicit _PreadStream(FILE *file) : _cur(0), _file(file) {}
    inline void Read(void *dest, size_t nBytes) {
        _cur += ArchPRead(_file, dest, nBytes, _cur);
    }
    inline int64_t Tell() const { return _cur; }
    inline void Seek(int64_t offset) { _cur = offset; }
    inline void Prefetch(int64_t offset, int64_t size) {
        ArchFileAdvise(_file, offset, size, ArchFileAdviceWillNeed);
    }

private:
    int64_t _cur;
    FILE *_file;
};

////////////////////////////////////////////////////////////////////////
// _TableOfContents
CrateFile::_Section const *
CrateFile::_TableOfContents::GetSection(_SectionName name) const
{
    for (auto const &sec: sections) {
        if (name == sec.name)
            return &sec;
    }
    TF_RUNTIME_ERROR("Crate file missing %s section", name.c_str());
    return nullptr;
}

int64_t
CrateFile::_TableOfContents::GetMinimumSectionStart() const
{
    auto theMin = std::min_element(
        sections.begin(), sections.end(),
        [](_Section const &l, _Section const &r) { return l.start < r.start; });

    return theMin == sections.end() ? sizeof(_BootStrap) : theMin->start;
}

////////////////////////////////////////////////////////////////////////
// _BufferedOutput
class CrateFile::_BufferedOutput
{
public:
    // Current buffer size is 512k.
    static const size_t BufferCap = 512*1024;

    // Helper move-only buffer object -- memory + valid size.
    struct _Buffer {
        _Buffer() = default;
        _Buffer(_Buffer const &) = delete;
        _Buffer &operator=(_Buffer const &) = delete;
        _Buffer(_Buffer &&) = default;
        _Buffer &operator=(_Buffer &&) = default;

        RawDataPtr bytes { new char[BufferCap] };
        int64_t size = 0;
    };

    explicit _BufferedOutput(FILE *file)
        : _filePos(0)
        , _file(file)
        , _bufferPos(0)
        , _writeTask(
            _dispatcher, std::bind(&_BufferedOutput::_DoWrites, this)) {
        // Create NumBuffers buffers.  One is _buffer, the remainder live in
        // _freeBuffers.
        constexpr const int NumBuffers = 8;
        for (int i = 1; i != NumBuffers; ++i) {
            _freeBuffers.push(_Buffer());
        }
    }

    inline FILE *GetFile() const { return _file; }

    inline void Flush() {
        _FlushBuffer();
        _dispatcher.Wait();
    }

    inline void Write(void const *bytes, int64_t nBytes) {
        // Write and flush as needed.
        while (nBytes) {
            int64_t available = BufferCap - (_filePos - _bufferPos);
            int64_t numToWrite = std::min(available, nBytes);
            
            _WriteToBuffer(bytes, numToWrite);
            
            bytes = static_cast<char const *>(bytes) + numToWrite;
            nBytes -= numToWrite;

            if (numToWrite == available)
                _FlushBuffer();
        }
    }

    inline int64_t Tell() const { return _filePos; }

    inline void Seek(int64_t offset) {
        // If the seek lands in a valid buffer region, then just adjust the
        // _filePos.  Otherwise _FlushBuffer() and reset.
        if (offset >= _bufferPos && offset <= (_bufferPos + _buffer.size)) {
            _filePos = offset;
        }
        else {
            _FlushBuffer();
            _bufferPos = _filePos = offset;
        }
    }

private:
    inline void _FlushBuffer() {
        if (_buffer.size) {
            // Queue a write of _buffer bytes to the file at _bufferPos.  Set
            // _bufferPos to be _filePos.
            _QueueWrite(std::move(_buffer), _bufferPos);
            // Get a new _buffer.  May have to wait if all are pending writes.
            while (!_freeBuffers.try_pop(_buffer))
                _dispatcher.Wait();
        }
        // Adjust the buffer to start at the write head.
        _bufferPos = _filePos;
    }

    inline void _WriteToBuffer(void const *bytes, int64_t nBytes) {
        // Fill the buffer, update its size and update the write head. Client
        // guarantees no overrun.
        int64_t writeStart = (_filePos - _bufferPos);
        if (writeStart + nBytes > _buffer.size) {
            _buffer.size = writeStart + nBytes;
        }
        void *bufPtr = static_cast<void *>(_buffer.bytes.get() + writeStart);
        memcpy(bufPtr, bytes, nBytes);
        _filePos += nBytes;
    }
    
    // Move-only write operation for the writer task to process.
    struct _WriteOp {
        _WriteOp() = default;
        _WriteOp(_WriteOp const &) = delete;
        _WriteOp(_WriteOp &&) = default;
        _WriteOp &operator=(_WriteOp &&) = default;
        _WriteOp(_Buffer &&buf, int64_t pos) : buf(std::move(buf)), pos(pos) {}
        _Buffer buf;
        int64_t pos = 0;
    };

    inline int64_t _QueueWrite(_Buffer &&buf, int64_t pos) {
        // Arrange to write the buffered data.  Enqueue the op and wake the
        // writer task.
        int64_t sz = static_cast<int64_t>(buf.size);
        _writeQueue.push(_WriteOp(std::move(buf), pos));
        _writeTask.Wake();
        return sz;
    }

    void _DoWrites() {
        // This is the writer task.  It just pops off ops and writes them, then
        // moves the buffer to the free list.
        _WriteOp op;
        while (_writeQueue.try_pop(op)) {
            // Write the bytes.
            WriteToFd(_file, op.buf.bytes.get(), op.buf.size, op.pos);
            // Add the buffer back to _freeBuffers for reuse.
            op.buf.size = 0;
            _freeBuffers.push(std::move(op.buf));
        }
    }
    
    // Write head in the file.  Always inside the buffer region.
    int64_t _filePos;
    FILE *_file;

    // Start of current buffer is at this file offset.
    int64_t _bufferPos;
    _Buffer _buffer;

    // Queue of free buffer objects.
    tbb::concurrent_queue<_Buffer> _freeBuffers;
    // Queue of pending write operations.
    tbb::concurrent_queue<_WriteOp> _writeQueue;

    WorkArenaDispatcher _dispatcher;
    WorkSingularTask _writeTask;
};

////////////////////////////////////////////////////////////////////////
// _PackingContext
struct CrateFile::_PackingContext
{
    _PackingContext() = delete;
    _PackingContext(_PackingContext const &) = delete;
    _PackingContext &operator=(_PackingContext const &) = delete;

    _PackingContext(CrateFile *crate, FILE *file, std::string const &fileName) 
        : fileName(fileName)
        , writeVersion(crate->_fileName.empty() ?
                       GetVersionForNewlyCreatedFiles() :
                       Version(crate->_boot))
        , bufferedOutput(file) {
        
        // Populate this context with everything we need from \p crate in order
        // to do deduplication, etc.
        WorkArenaDispatcher wd;

        // Read in any unknown sections so we can rewrite them later.
        wd.Run([this, crate]() {
                for (auto const &sec: crate->_toc.sections) {
                    if (!_IsKnownSection(sec.name)) {
                        unknownSections.emplace_back(
                            sec.name, _ReadSectionBytes(sec, crate), sec.size);
                    }
                }
            });

        // Ensure that pathToPathIndex is correctly populated.
        wd.Run([this, crate]() {
                for (size_t i = 0; i != crate->_paths.size(); ++i)
                    pathToPathIndex[crate->_paths[i]] = PathIndex(i);
            });

        // Ensure that fieldToFieldIndex is correctly populated.
        wd.Run([this, crate]() {
                for (size_t i = 0; i != crate->_fields.size(); ++i)
                    fieldToFieldIndex[crate->_fields[i]] = FieldIndex(i);
            });
        
        // Ensure that fieldsToFieldSetIndex is correctly populated.
        auto const &fsets = crate->_fieldSets;
        wd.Run([this, &fsets]() {
                vector<FieldIndex> fieldIndexes;
                for (auto fsBegin = fsets.begin(),
                         fsEnd = find(fsBegin, fsets.end(), FieldIndex());
                     fsBegin != fsets.end();
                     fsBegin = fsEnd + 1,
                         fsEnd = find(fsBegin, fsets.end(), FieldIndex())) {
                    fieldIndexes.assign(fsBegin, fsEnd);
                    fieldsToFieldSetIndex[fieldIndexes] =
                        FieldSetIndex(fsBegin - fsets.begin());
                }
            });

        // Ensure that tokenToTokenIndex is correctly populated.
        wd.Run([this, crate]() {
                for (size_t i = 0; i != crate->_tokens.size(); ++i)
                    tokenToTokenIndex[crate->_tokens[i]] = TokenIndex(i);
            });

        // Ensure that stringToStringIndex is correctly populated.
        wd.Run([this, crate]() {
                for (size_t i = 0; i != crate->_strings.size(); ++i)
                    stringToStringIndex[
                        crate->GetString(StringIndex(i))] = StringIndex(i);
            });

        // Set file pos to start of the structural sections in the current TOC.
        bufferedOutput.Seek(crate->_toc.GetMinimumSectionStart());
        wd.Wait();
    }

    inline FILE *GetFile() const { return bufferedOutput.GetFile(); }

    // Inform the writer that the output stream requires the given version
    // (or newer) to be read back.  This allows the writer to start with
    // a conservative version assumption and promote to newer versions
    // only as required by the data stream contents.
    bool _RequestWriteVersionUpgrade(Version ver, std::string reason) {
        if (!writeVersion.CanRead(ver)) {
            TF_WARN("Upgrading crate file from version %s to %s because: %s",
                    writeVersion.AsString().c_str(), ver.AsString().c_str(),
                    reason.c_str());
            writeVersion = ver;
        }
        // For now, this always returns true, indicating success.  In
        // the future, we anticipate a mechanism to confirm the upgrade
        // is desired -- in which case this could return true or false.
        return true;
    }

    // Read the bytes of some unknown section into memory so we can rewrite them
    // out later (to preserve it).
    RawDataPtr
    _ReadSectionBytes(_Section const &sec, CrateFile *crate) const {
        RawDataPtr result(new char[sec.size]);
        crate->_ReadRawBytes(sec.start, sec.size, result.get());
        return result;
    }

    // Deduplication tables.
    unordered_map<TfToken, TokenIndex, _Hasher> tokenToTokenIndex;
    unordered_map<string, StringIndex, _Hasher> stringToStringIndex;
    unordered_map<SdfPath, PathIndex, SdfPath::Hash> pathToPathIndex;
    unordered_map<Field, FieldIndex, _Hasher> fieldToFieldIndex;
    
    // A mapping from a group of fields to their starting index in _fieldSets.
    unordered_map<vector<FieldIndex>,
                  FieldSetIndex, _Hasher> fieldsToFieldSetIndex;
    
    // Unknown sections we're moving to the new structural area.
    vector<tuple<string, RawDataPtr, size_t>> unknownSections;

    // Filename we're writing to.
    std::string fileName;
    // Version we're writing.
    Version writeVersion;
    // BufferedOutput helper.
    _BufferedOutput bufferedOutput;

};

/////////////////////////////////////////////////////////////////////////
// Readers
class CrateFile::_ReaderBase
{
public:
    _ReaderBase(CrateFile const *crate) : crate(crate) {}

    template <class T>
    T GetUninlinedValue(uint32_t x, T *) const {
        static_assert(sizeof(T) <= sizeof(x), "");
        T r;
        memcpy(&r, &x, sizeof(r));
        return r;
    }

    string const & GetUninlinedValue(uint32_t i, string *) const {
        return crate->GetString(StringIndex(i));
    }

    TfToken const &GetUninlinedValue(uint32_t i, TfToken *) const {
        return crate->GetToken(TokenIndex(i));
    }

    SdfPath const &GetUninlinedValue(uint32_t i, SdfPath *) const {
        return crate->GetPath(PathIndex(i));
    }

    SdfAssetPath GetUninlinedValue(uint32_t i, SdfAssetPath *) const {
        return SdfAssetPath(crate->GetToken(TokenIndex(i)));
    }

    CrateFile const *crate;
};

template <class ByteStream>
class CrateFile::_Reader : public _ReaderBase
{
    void _RecursiveRead() {
        auto start = src.Tell();
        auto offset = Read<int64_t>();
        src.Seek(start + offset);
    }

    void _RecursiveReadAndPrefetch() {
        auto start = src.Tell();
        auto offset = Read<int64_t>();
        src.Prefetch(start, offset);
        src.Seek(start + offset);
    }

public:
    _Reader(CrateFile const *crate, ByteStream &src)
        : _ReaderBase(crate)
        , src(src) {}

    template <class T>
    static typename std::enable_if<_IsBitwiseReadWrite<T>::value, T>::type
    StaticRead(ByteStream &src, T *) {
        T bits;
        src.Read(&bits, sizeof(bits));
        return bits;
    }

    void Prefetch(int64_t offset, int64_t size) { src.Prefetch(offset, size); }

    void Seek(uint64_t offset) { src.Seek(offset); }

    // Map helper.
    template <class Map>
    Map ReadMap() {
        Map map;
        auto sz = Read<uint64_t>();
        while (sz--) {
            // Do not combine the following into one statement.  It must be
            // separate because the two modifications to 'src' must be correctly
            // sequenced.
            auto key = Read<typename Map::key_type>();
            map[key] = Read<typename Map::mapped_type>();
        }
        return map;
    }

    ////////////////////////////////////////////////////////////////////////
    // Base template for Read.  It dispatches to the overloads that take a
    // dummy pointer argument to allow overloading/enable_if.
    template <class T>
    inline T Read() {
        return this->Read(static_cast<T *>(nullptr));
    }

    // read bitwise.
    template <class T>
    typename std::enable_if<_IsBitwiseReadWrite<T>::value, T>::type
    Read(T *) { return StaticRead(src, static_cast<T *>(nullptr)); }

    _TableOfContents Read(_TableOfContents *) {
        _TableOfContents ret;
        ret.sections = Read<decltype(ret.sections)>();
        return ret;
    }
    string Read(string *) { return crate->GetString(Read<StringIndex>()); }
    TfToken Read(TfToken *) { return crate->GetToken(Read<TokenIndex>()); }
    SdfPath Read(SdfPath *) { return crate->GetPath(Read<PathIndex>()); }
    VtDictionary Read(VtDictionary *) { return ReadMap<VtDictionary>(); }
    SdfAssetPath Read(SdfAssetPath *) {
        return SdfAssetPath(Read<string>());
    }
    SdfUnregisteredValue Read(SdfUnregisteredValue *) {
        VtValue val = Read<VtValue>();
        if (val.IsHolding<string>())
            return SdfUnregisteredValue(val.UncheckedGet<string>());
        if (val.IsHolding<VtDictionary>())
            return SdfUnregisteredValue(val.UncheckedGet<VtDictionary>());
        if (val.IsHolding<SdfUnregisteredValueListOp>())
            return SdfUnregisteredValue(
                val.UncheckedGet<SdfUnregisteredValueListOp>());
        TF_CODING_ERROR("SdfUnregisteredValue in crate file contains invalid "
                        "type '%s' = '%s'; expected string, VtDictionary or "
                        "SdfUnregisteredValueListOp; returning empty",
                        val.GetTypeName().c_str(), TfStringify(val).c_str());
        return SdfUnregisteredValue();
    }
    SdfVariantSelectionMap Read(SdfVariantSelectionMap *) {
        return ReadMap<SdfVariantSelectionMap>();
    }
    SdfLayerOffset Read(SdfLayerOffset *) {
        // Do not combine the following into one statement.  It must be separate
        // because the two modifications to 'src' must be correctly sequenced.
        auto offset = Read<double>();
        auto scale = Read<double>();
        return SdfLayerOffset(offset, scale);
    }
    SdfReference Read(SdfReference *) {
        // Do not combine the following into one statement.  It must be separate
        // because the two modifications to 'src' must be correctly sequenced.
        auto assetPath = Read<std::string>();
        auto primPath = Read<SdfPath>();
        auto layerOffset = Read<SdfLayerOffset>();
        auto customData = Read<VtDictionary>();
        return SdfReference(std::move(assetPath), std::move(primPath),
                            std::move(layerOffset), std::move(customData));
    }
    SdfPayload Read(SdfPayload *) {
        // Do not combine the following into one statement.  It must be separate
        // because the two modifications to 'src' must be correctly sequenced.
        auto assetPath = Read<string>();
        auto primPath = Read<SdfPath>();
        return SdfPayload(assetPath, primPath);
    }
    template <class T>
    SdfListOp<T> Read(SdfListOp<T> *) {
        SdfListOp<T> listOp;
        auto h = Read<_ListOpHeader>();
        if (h.IsExplicit()) { listOp.ClearAndMakeExplicit(); }
        if (h.HasExplicitItems()) {
            listOp.SetExplicitItems(Read<vector<T>>()); }
        if (h.HasAddedItems()) { listOp.SetAddedItems(Read<vector<T>>()); }
        if (h.HasPrependedItems()) {
            listOp.SetPrependedItems(Read<vector<T>>()); }
        if (h.HasAppendedItems()) {
            listOp.SetAppendedItems(Read<vector<T>>()); }
        if (h.HasDeletedItems()) { listOp.SetDeletedItems(Read<vector<T>>()); }
        if (h.HasOrderedItems()) { listOp.SetOrderedItems(Read<vector<T>>()); }
        return listOp;
    }
    VtValue Read(VtValue *) {
        _RecursiveReadAndPrefetch();
        auto rep = Read<ValueRep>();
        return crate->UnpackValue(rep);
    }

    TimeSamples Read(TimeSamples *) {

        TimeSamples ret;

        // Reconstitute a rep for this very location in the file to be retained
        // in the TimeSamples result.
        ret.valueRep = ValueRepFor<TimeSamples>(src.Tell());

        _RecursiveRead();
        auto timesRep = Read<ValueRep>();

        // Deduplicate times in-memory by ValueRep.
        // Optimistically take the read lock and see if we already have times.
        tbb::spin_rw_mutex::scoped_lock
            lock(crate->_sharedTimesMutex, /*write=*/false);
        auto sharedTimesIter = crate->_sharedTimes.find(timesRep);
        if (sharedTimesIter != crate->_sharedTimes.end()) {
            // Yes, reuse existing times.
            ret.times = sharedTimesIter->second;
        } else {
            // The lock upgrade here may or may not be atomic.  This means
            // someone else may have populated the table while we were
            // upgrading.
            lock.upgrade_to_writer();
            auto iresult =
                crate->_sharedTimes.emplace(timesRep, Usd_EmptySharedTag);
            if (iresult.second) {
                // We get to do the population.
                auto sharedTimes = TimeSamples::SharedTimes();
                crate->_UnpackValue(timesRep, &sharedTimes.GetMutable());
                iresult.first->second.swap(sharedTimes);
            }
            ret.times = iresult.first->second;
        }
        lock.release();

        _RecursiveRead();

        // Store the offset to the value reps in the file.  The values are
        // encoded as a uint64_t size followed by contiguous reps.  So we jump
        // over that uint64_t and store the start of the reps.  Then we seek
        // forward past the reps to continue.
        auto numValues = Read<uint64_t>();
        ret.valuesFileOffset = src.Tell();

        // Now move past the reps to continue.
        src.Seek(ret.valuesFileOffset + numValues * sizeof(ValueRep));

        return ret;
    }

    template <class T>
    vector<T> Read(vector<T> *) {
        auto sz = Read<uint64_t>();
        vector<T> vec(sz);
        ReadContiguous(vec.data(), sz);
        return vec;
    }

    template <class T>
    typename std::enable_if<_IsBitwiseReadWrite<T>::value>::type
    ReadContiguous(T *values, size_t sz) {
        src.Read(static_cast<void *>(values), sz * sizeof(*values));
    }

    template <class T>
    typename std::enable_if<!_IsBitwiseReadWrite<T>::value>::type
    ReadContiguous(T *values, size_t sz) {
        std::for_each(values, values + sz, [this](T &v) { v = Read<T>(); });
    }

    ByteStream src;
};

template <class ByteStream>
CrateFile::_Reader<ByteStream>
CrateFile::_MakeReader(ByteStream src) const
{
    return _Reader<ByteStream>(this, src);
}

/////////////////////////////////////////////////////////////////////////
// Writers
class CrateFile::_Writer
{
public:
    explicit _Writer(CrateFile *crate)
        : crate(crate)
        , sink(&crate->_packCtx->bufferedOutput) {}

    // Recursive write helper.  We use these when writing values if we may
    // invoke _PackValue() recursively.  Since _PackValue() may or may not write
    // to the file, we need to account for jumping over that written nested
    // data, and this function automates that.
    template <class Fn>
    void _RecursiveWrite(Fn const &fn) {
        // Reserve space for a forward offset to where the primary data will
        // live.
        int64_t offsetLoc = Tell();
        WriteAs<int64_t>(0);
        // Invoke the writing function, which may write arbitrary data.
        fn();
        // Now that we know where the primary data will end up, seek back and
        // write the offset value, then seek forward again.
        int64_t end = Tell();
        Seek(offsetLoc);
        WriteAs<int64_t>(end - offsetLoc);
        Seek(end);
    }

public:

    int64_t Tell() const { return sink->Tell(); }
    void Seek(int64_t offset) { sink->Seek(offset); }
    void Flush() { sink->Flush(); }

    template <class T>
    uint32_t GetInlinedValue(T x) {
        uint32_t r = 0;
        static_assert(sizeof(x) <= sizeof(r), "");
        memcpy(&r, &x, sizeof(x));
        return r;
    }

    uint32_t GetInlinedValue(string const &s) {
        return crate->_AddString(s).value;
    }

    uint32_t GetInlinedValue(TfToken const &t) {
        return crate->_AddToken(t).value;
    }

    uint32_t GetInlinedValue(SdfPath const &p) {
        return crate->_AddPath(p).value;
    }

    uint32_t GetInlinedValue(SdfAssetPath const &p) {
        return crate->_AddToken(TfToken(p.GetAssetPath())).value;
    }

    ////////////////////////////////////////////////////////////////////////
    // Basic Write
    template <class T>
    typename std::enable_if<_IsBitwiseReadWrite<T>::value>::type
    Write(T const &bits) { sink->Write(&bits, sizeof(bits)); }

    template <class U, class T>
    void WriteAs(T const &obj) { return Write(static_cast<U>(obj)); }

    // Map helper.
    template <class Map>
    void WriteMap(Map const &map) {
        WriteAs<uint64_t>(map.size());
        for (auto const &kv: map) {
            Write(kv.first);
            Write(kv.second);
        }
    }

    void Write(_TableOfContents const &toc) { Write(toc.sections); }
    void Write(std::string const &str) { Write(crate->_AddString(str)); }
    void Write(TfToken const &tok) { Write(crate->_AddToken(tok)); }
    void Write(SdfPath const &path) { Write(crate->_AddPath(path)); }
    void Write(VtDictionary const &dict) { WriteMap(dict); }
    void Write(SdfAssetPath const &ap) { Write(ap.GetAssetPath()); }
    void Write(SdfUnregisteredValue const &urv) { Write(urv.GetValue()); }
    void Write(SdfVariantSelectionMap const &vsmap) { WriteMap(vsmap); }
    void Write(SdfLayerOffset const &layerOffset) {
        Write(layerOffset.GetOffset());
        Write(layerOffset.GetScale());
    }
    void Write(SdfReference const &ref) {
        Write(ref.GetAssetPath());
        Write(ref.GetPrimPath());
        Write(ref.GetLayerOffset());
        Write(ref.GetCustomData());
    }
    void Write(SdfPayload const &ref) {
        Write(ref.GetAssetPath());
        Write(ref.GetPrimPath());
    }
    template <class T>
    void Write(SdfListOp<T> const &listOp) {
        _ListOpHeader h(listOp);
        if (h.HasPrependedItems() || h.HasAppendedItems()) {
            crate->_packCtx->_RequestWriteVersionUpgrade(
                Version(0, 2, 0),
                "A SdfListOp value using a prepended or appended value "
                "was detected, which requires crate version 0.2.0.");
        }
        Write(h);
        if (h.HasExplicitItems()) { Write(listOp.GetExplicitItems()); }
        if (h.HasAddedItems()) { Write(listOp.GetAddedItems()); }
        if (h.HasPrependedItems()) { Write(listOp.GetPrependedItems()); }
        if (h.HasAppendedItems()) { Write(listOp.GetAppendedItems()); }
        if (h.HasDeletedItems()) { Write(listOp.GetDeletedItems()); }
        if (h.HasOrderedItems()) { Write(listOp.GetOrderedItems()); }
    }
    void Write(VtValue const &val) {
        ValueRep rep;
        _RecursiveWrite(
            [this, &val, &rep]() { rep = crate->_PackValue(val); });
        Write(rep);
    }

    void Write(TimeSamples const &samples) {
        // Pack the times to deduplicate.
        ValueRep timesRep;
        _RecursiveWrite([this, &timesRep, &samples]() {
                timesRep = crate->_PackValue(samples.times.Get());
            });
        Write(timesRep);

        // Pack the individual elements, to deduplicate them.
        vector<ValueRep> reps(samples.values.size());
        _RecursiveWrite([this, &reps, &samples]() {
                transform(samples.values.begin(), samples.values.end(),
                          reps.begin(),
                          [this](VtValue const &val) {
                              return crate->_PackValue(val);
                          });
            });

        // Write size and contiguous reps.
        WriteAs<uint64_t>(reps.size());
        WriteContiguous(reps.data(), reps.size());
    }

    template <class T>
    void Write(vector<T> const &vec) {
        WriteAs<uint64_t>(vec.size());
        WriteContiguous(vec.data(), vec.size());
    }

    template <class T>
    typename std::enable_if<_IsBitwiseReadWrite<T>::value>::type
    WriteContiguous(T const *values, size_t sz) {
        sink->Write(values, sizeof(*values) * sz);
    }

    template <class T>
    typename std::enable_if<!_IsBitwiseReadWrite<T>::value>::type
    WriteContiguous(T const *values, size_t sz) {
        std::for_each(values, values + sz, [this](T const &v) { Write(v); });
    }

    CrateFile *crate;
    _BufferedOutput *sink;
};


////////////////////////////////////////////////////////////////////////
// ValueHandler class hierarchy.  See comment for _ValueHandler itself for more
// information.

struct CrateFile::_ValueHandlerBase {
    // Base Clear() does nothing.
    void Clear() {}
};

// Scalar handler for non-inlined types -- does deduplication.
template <class T, class Enable>
struct CrateFile::_ScalarValueHandlerBase : _ValueHandlerBase
{
    inline ValueRep Pack(_Writer writer, T const &val) {
        // See if we can inline the value -- we might be able to if there's some
        // encoding that can exactly represent it in 4 bytes.
        uint32_t ival;
        if (_EncodeInline(val, &ival)) {
            auto ret = ValueRepFor<T>(ival);
            ret.SetIsInlined();
            return ret;
        }

        // Otherwise dedup and/or write...
        if (!_valueDedup) {
            _valueDedup.reset(
                new typename decltype(_valueDedup)::element_type);
        }

        auto iresult = _valueDedup->emplace(val, ValueRep());
        ValueRep &target = iresult.first->second;
        if (iresult.second) {
            // Not yet present.  Invoke the write function.
            target = ValueRepFor<T>(writer.Tell());
            writer.Write(val);
        }
        return target;
    }
    template <class Reader>
    inline void Unpack(Reader reader, ValueRep rep, T *out) const {
        // If the value is inlined, just decode it.
        if (rep.IsInlined()) {
            uint32_t tmp = (rep.GetPayload() &
                            ((1ull << (sizeof(uint32_t) * 8))-1));
            _DecodeInline(out, tmp);
            return;
        }
        // Otherwise we have to read it from the file.
        reader.Seek(rep.GetPayload());
        *out = reader.template Read<T>();
    }
    void Clear() {
        _valueDedup.reset();
    }
    std::unique_ptr<std::unordered_map<T, ValueRep, _Hasher>> _valueDedup;
};

// Scalar handler for inlined types -- no deduplication.
template <class T>
struct CrateFile::_ScalarValueHandlerBase<
    T, typename std::enable_if<ValueTypeTraits<T>::isInlined>::type>
: _ValueHandlerBase
{
    inline ValueRep Pack(_Writer writer, T val) {
        // Inline it into the rep.
        return ValueRepFor<T>(writer.GetInlinedValue(val));
    }
    template <class Reader>
    inline void Unpack(Reader reader, ValueRep rep, T *out) const {
        // Value is directly in payload data.
        uint32_t tmp =
            (rep.GetPayload() & ((1ull << (sizeof(uint32_t) * 8))-1));
        *out = reader.GetUninlinedValue(tmp, static_cast<T *>(nullptr));
    }
};

// Array handler for types that don't support arrays.
template <class T, class Enable>
struct CrateFile::_ArrayValueHandlerBase : _ScalarValueHandlerBase<T>
{
    ValueRep PackVtValue(_Writer w, VtValue const &v) {
        return this->Pack(w, v.UncheckedGet<T>());
    }

    template <class Reader>
    void UnpackVtValue(Reader r, ValueRep rep, VtValue *out) {
        T obj;
        this->Unpack(r, rep, &obj);
        out->Swap(obj);
    }
};

// Array handler for types that support arrays -- does deduplication.
template <class T>
struct CrateFile::_ArrayValueHandlerBase<
    T, typename std::enable_if<ValueTypeTraits<T>::supportsArray>::type>
: _ScalarValueHandlerBase<T>
{
    ValueRep PackArray(_Writer w, VtArray<T> const &array) {
        auto result = ValueRepForArray<T>(0);

        // If this is an empty array we inline it.
        if (array.empty())
            return result;

        if (!_arrayDedup) {
            _arrayDedup.reset(
                new typename decltype(_arrayDedup)::element_type);
        }

        auto iresult = _arrayDedup->emplace(array, result);
        ValueRep &target = iresult.first->second;
        if (iresult.second) {
            // Not yet present.
            target.SetPayload(w.Tell());
            w.WriteAs<uint32_t>(1);
            w.WriteAs<uint32_t>(array.size());
            w.WriteContiguous(array.data(), array.size());
        }
        return target;
    }

    template <class Reader>
    void UnpackArray(Reader reader, ValueRep rep, VtArray<T> *out) const {
        // If payload is 0, it's an empty array.
        if (rep.GetPayload() == 0) {
            *out = VtArray<T>();
            return;
        }
        reader.Seek(rep.GetPayload());
        // Read and discard shape size.
        reader.template Read<uint32_t>();
        out->resize(reader.template Read<uint32_t>());
        reader.ReadContiguous(out->data(), out->size());
    }

    ValueRep PackVtValue(_Writer w, VtValue const &v) {
        return v.IsArrayValued() ?
            this->PackArray(w, v.UncheckedGet<VtArray<T>>()) :
            this->Pack(w, v.UncheckedGet<T>());
    }

    template <class Reader>
    void UnpackVtValue(Reader r, ValueRep rep, VtValue *out) {
        if (rep.IsArray()) {
            VtArray<T> array;
            this->UnpackArray(r, rep, &array);
            out->Swap(array);
        } else {
            T obj;
            this->Unpack(r, rep, &obj);
            out->Swap(obj);
        }
    }

    void Clear() {
        // Invoke base implementation to clear scalar table.
        _ScalarValueHandlerBase<T>::Clear();
        _arrayDedup.reset();
    }
    
    std::unique_ptr<
        std::unordered_map<VtArray<T>, ValueRep, _Hasher>> _arrayDedup;
};

// _ValueHandler derives _ArrayValueHandlerBase, which in turn derives
// _ScalarValueHandlerBase.  Those templates are specialized to handle types
// that support or do not support arrays and types that are inlined or not
// inlined.
template <class T>
struct CrateFile::_ValueHandler : public _ArrayValueHandlerBase<T> {};


////////////////////////////////////////////////////////////////////////
// CrateFile

/*static*/ bool
CrateFile::CanRead(string const &fileName) {
    // Create a unique_ptr with a functor that fclose()s for a deleter.
    _UniqueFILE in(ArchOpenFile(fileName.c_str(), "rb"));

    if (!in)
        return false;

    // Mark the entire file as random access to avoid prefetch.
    int64_t fileLength = ArchGetFileLength(in.get());
    ArchFileAdvise(in.get(), 0, fileLength, ArchFileAdviceRandomAccess);

    TfErrorMark m;
    _ReadBootStrap(_PreadStream(in.get()), fileLength);

    // Clear any issued errors again to avoid propagation, and return true if
    // there were no errors issued.
    return !m.Clear();
}

/* static */
std::unique_ptr<CrateFile>
CrateFile::CreateNew()
{
    bool useMmap = !TfGetenvBool("USDC_USE_PREAD", false);
    return std::unique_ptr<CrateFile>(new CrateFile(useMmap));
}

/* static */
ArchConstFileMapping
CrateFile::_MmapFile(char const *fileName, FILE *file)
{
    ArchConstFileMapping map = ArchMapFileReadOnly(file);
    if (!map)
        TF_RUNTIME_ERROR("Couldn't map file '%s'", fileName);
    return map;
}

/* static */
std::unique_ptr<CrateFile>
CrateFile::Open(string const &fileName)
{
    TfAutoMallocTag tag2("Usd_CrateFile::CrateFile::Open");

    std::unique_ptr<CrateFile> result;

    // Create a unique_ptr with a functor that fclose()s for a deleter.
    _UniqueFILE inputFile(ArchOpenFile(fileName.c_str(), "rb"));

    if (!inputFile) {
        TF_RUNTIME_ERROR("Failed to open file '%s'", fileName.c_str());
        return result;
    }

    if (!TfGetenvBool("USDC_USE_PREAD", false)) {
        // Map the file.
        auto mapStart = _MmapFile(fileName.c_str(), inputFile.get());
        result.reset(new CrateFile(fileName, std::move(mapStart)));
    } else {
        result.reset(new CrateFile(fileName, std::move(inputFile)));
    }

    // If the resulting CrateFile has no filename, reading failed.
    if (result->GetFileName().empty())
        result.reset();

    if (TfGetenvBool("USDC_DEBUG_DUMP", false))
        result->DebugPrint();
    
    return result;
}

/* static */
TfToken const &
CrateFile::GetSoftwareVersionToken()
{
    static TfToken tok(_SoftwareVersion.AsString());
    return tok;
}

TfToken
CrateFile::GetFileVersionToken() const
{
    return TfToken(Version(_boot).AsString());
}

CrateFile::CrateFile(bool useMmap)
    : _useMmap(useMmap)
{
    _DoAllTypeRegistrations();
}

CrateFile::CrateFile(string const &fileName, ArchConstFileMapping mapStart)
    : _mapStart(std::move(mapStart))
    , _fileName(fileName)
    , _useMmap(true)
{
    _DoAllTypeRegistrations();
    _InitMMap();
}

void
CrateFile::_InitMMap() {
    if (_mapStart) {
        int64_t fileSize = ArchGetFileMappingLength(_mapStart);
        
        // Mark the whole file as random access to start to avoid large NFS
        // prefetch.  We explicitly prefetch the structural sections later.
        ArchMemAdvise(_mapStart.get(), fileSize, ArchMemAdviceRandomAccess);

        // If we're debugging access, allocate a debug page map. 
        static string debugPageMapPattern = TfGetenv("USDC_DUMP_PAGE_MAPS");
        // If it's just '1' or '*' do everything, otherwise match.
        if (!debugPageMapPattern.empty() &&
            (debugPageMapPattern == "*" || debugPageMapPattern == "1" ||
             ArchRegex(debugPageMapPattern, ArchRegex::GLOB).Match(_fileName))) {
            int64_t npages = (fileSize + PAGESIZE-1) / PAGESIZE;
            _debugPageMap.reset(new char[npages]);
            memset(_debugPageMap.get(), 0, npages);
        } 

        // Make an mmap stream but disable auto prefetching -- the
        // _ReadStructuralSections() call manages prefetching itself using
        // higher-level knowledge.
        auto reader = _MakeReader(
            _MmapStream(_mapStart, _debugPageMap.get()).DisablePrefetch());
        TfErrorMark m;
        _ReadStructuralSections(reader, fileSize);
        if (!m.IsClean())
            _fileName.clear();

        // Restore default prefetch behavior if we're not doing custom prefetch.
        if (!_GetMMapPrefetchKB())
            ArchMemAdvise(_mapStart.get(), fileSize, ArchMemAdviceNormal);
    } else {
        _fileName.clear();
    }
}

CrateFile::CrateFile(string const &fileName, _UniqueFILE inputFile)
    : _inputFile(std::move(inputFile))
    , _fileName(fileName)
    , _useMmap(false)
{
    _DoAllTypeRegistrations();
}

void
CrateFile::_InitPread()
{
    // Mark the whole file as random access to start to avoid large NFS
    // prefetch.  We explicitly prefetch the structural sections later.
    int64_t fileSize = ArchGetFileLength(_inputFile.get());
    ArchFileAdvise(_inputFile.get(), 0, fileSize, ArchFileAdviceRandomAccess);
    auto reader = _MakeReader(_PreadStream(_inputFile.get()));
    TfErrorMark m;
    _ReadStructuralSections(reader, fileSize);
    if (!m.IsClean())
        _fileName.clear();
    // Restore default prefetch behavior.
    ArchFileAdvise(_inputFile.get(), 0, fileSize, ArchFileAdviceNormal);
}

CrateFile::~CrateFile()
{
    static std::mutex outputMutex;

    // Dump a debug page map if requested.
    if (_useMmap && _mapStart && _debugPageMap) {
        int64_t length = ArchGetFileMappingLength(_mapStart);
        int64_t npages = (length + PAGESIZE-1) / PAGESIZE;
        std::unique_ptr<unsigned char []> mincoreMap(new unsigned char[npages]);
        void const *p = static_cast<void const *>(_mapStart.get());
        if (!ArchQueryMappedMemoryResidency(p, length, mincoreMap.get())) {
            TF_WARN("failed to obtain memory residency information");
            return;
        }
        // Count the pages in core & accessed.
        int64_t pagesInCore = 0;
        int64_t pagesAccessed = 0;
        for (int64_t i = 0; i != npages; ++i) {
            bool inCore = mincoreMap[i] & 1;
            bool accessed = _debugPageMap[i] & 1;
            pagesInCore += (int)inCore;
            pagesAccessed += (int)accessed;
            if (accessed && inCore) {
                mincoreMap.get()[i] = '+';
            } else if (accessed) {
                mincoreMap.get()[i] = '!';
            } else if (inCore) {
                mincoreMap.get()[i] = '-';
            } else {
                mincoreMap.get()[i] = ' ';
            }
        }

        std::lock_guard<std::mutex> lock(outputMutex);

        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
               ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"
               "page map for %s\n"
               "%zd pages, %zd used (%.1f%%), %zd in mem (%.1f%%)\n"
               "used %.1f%% of pages in mem\n"
               "legend: '+': in mem & used,     '-': in mem & unused\n"
               "        '!': not in mem & used, ' ': not in mem & unused\n"
               ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
               ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n",
               _fileName.c_str(),
               npages,
               pagesAccessed, 100.0*pagesAccessed/(double)npages,
               pagesInCore, 100.0*pagesInCore/(double)npages,
               100.0*pagesAccessed / (double)pagesInCore);
               
        constexpr int wrapCol = 80;
        int col = 0;
        for (int64_t i = 0; i != npages; ++i, ++col) {
            putchar(mincoreMap.get()[i]);
            if (col == wrapCol) {
                putchar('\n');
                col = -1;
            }
        }
        printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
               "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    }

    _DeleteValueHandlers();
}

CrateFile::Packer
CrateFile::StartPacking(string const &fileName)
{
    TF_VERIFY(_fileName.empty() || _fileName == fileName);
    // We open the file for read/write (update) here in case we already have the
    // file, since we're not rewriting the whole thing.
    _UniqueFILE out(ArchOpenFile(fileName.c_str(), _fileName.empty() ? "w+b" : "r+b"));
    if (!out) {
        TF_RUNTIME_ERROR("Failed to open '%s' for writing", fileName.c_str());
    } else {
        // Create a packing context so we can start writing.
        _packCtx.reset(new _PackingContext(this, out.release(), fileName));
        // Get rid of our local list of specs, if we have one -- the client is
        // required to repopulate it.
        vector<Spec>().swap(_specs);
    }
    return Packer(this);
}

CrateFile::Packer::operator bool() const {
    return _crate && _crate->_packCtx;
}

bool
CrateFile::Packer::Close()
{
    if (!TF_VERIFY(_crate))
        return false;

    if (FILE *fp = _crate->_packCtx->GetFile()) {

        // Write contents.
        bool writeResult = _crate->_Write();

        // If we wrote successfully, store the fileName and size.
        if (writeResult)
            _crate->_fileName = _crate->_packCtx->fileName;

        // Pull out the file handle and kill the packing context.
        _UniqueFILE file(fp);
        _crate->_packCtx.reset();

        if (!writeResult)
            return false;

        // Reset the mapping or file so we can read values from the newly
        // written file.
        if (_crate->_useMmap) {
            // Must remap the file.
            _crate->_mapStart =
                _MmapFile(_crate->_fileName.c_str(), file.get());
            if (!_crate->_mapStart)
                return false;
            _crate->_InitMMap();
        } else {
            // Must adopt the file handle if we don't already have one.
            _crate->_inputFile = std::move(file);
            _crate->_InitPread();
        }

        return true;
    }
    return false;
}

CrateFile::Packer::Packer(Packer &&other) : _crate(other._crate)
{
    other._crate = nullptr;
}

CrateFile::Packer &
CrateFile::Packer::operator=(Packer &&other)
{
    _crate = other._crate;
    other._crate = nullptr;
    return *this;
}

CrateFile::Packer::~Packer()
{
    if (_crate)
        _crate->_packCtx.reset();
}

vector<tuple<string, int64_t, int64_t>>
CrateFile::GetSectionsNameStartSize() const
{
    vector<tuple<string, int64_t, int64_t> > result;
    for (auto const &sec: _toc.sections) {
        result.emplace_back(sec.name, sec.start, sec.size);
    }
    return result;
}

void
CrateFile::DebugPrint() const
{
    // Count field sets by counting terminators.
    size_t numFieldSets =
        count(_fieldSets.begin(), _fieldSets.end(), FieldIndex());

    printf("%zu specs, %zu paths, %zu tokens, %zu strings, "
           "%zu unique fields, %zu unique field sets.\n",
           _specs.size(), _paths.size(), _tokens.size(), _strings.size(),
           _fields.size(), numFieldSets);

    printf("TOKENS ================================\n");
    auto tmptoks = _tokens;
    std::sort(tmptoks.begin(), tmptoks.end());
    for (auto const &t: tmptoks) {
        printf("%s\n", t.GetText());
    }
    // printf("PATHS ================================\n");
    // for (auto const &p: _paths) {
    //     printf("%s\n", p.GetText());
    // }
    auto stringifyFieldVal = [this](Field const &f) {
        VtValue val;
        this->_UnpackValue(f.valueRep, &val);
        string result = TfStringify(val);
        if (result.size() > 64)
            result.resize(64);
        result = TfStringPrintf(
            "<%s> %s", ArchGetDemangled(val.GetTypeid()).c_str(),
            result.c_str());
        if (result.size() > 72) {
            result.resize(72);
            result.append("...");
        }
        return result;
    };
    // auto tmpfields = _fields;
    // std::sort(
    //     tmpfields.begin(), tmpfields.end(),
    //     [=](Field const &lhs, Field const &rhs) {
    //         auto const &lhstok = this->_tokenValues[lhs.tokenIndex.value];
    //         auto const &rhstok = this->_tokenValues[rhs.tokenIndex.value];
    //         if (lhstok != rhstok)
    //             return lhstok < rhstok;
    //         auto lhsval = stringifyFieldVal(lhs);
    //         auto rhsval = stringifyFieldVal(rhs);
    //         return lhsval < rhsval;
    //     });
    // printf("FIELDS ================================\n");
    // for (auto const &f: tmpfields) {
    //     printf("%s = %s\n", _tokenValues[f.tokenIndex.value].GetText(),
    //            stringifyFieldVal(f).c_str());
    // }
    vector<string> fieldVals;
    fieldVals.reserve(_fields.size());
    for (auto const &f: _fields) {
        fieldVals.emplace_back(stringifyFieldVal(f));
    }
    printf("FIELDSETS ================================\n");
    for (auto const &fi: _fieldSets) {
        if (fi == FieldIndex()) {
            printf("--------------------------------\n");
        } else {
            Field const &f = _fields[fi.value];
            printf("#%d: %s = %s\n", fi.value,
                   _tokens[f.tokenIndex.value].GetText(),
                   fieldVals[fi.value].c_str());
        }
    }
}

template <class Fn>
void
CrateFile::_WriteSection(
    _Writer &w, _SectionName name, _TableOfContents &toc, Fn writeFn) const
{
    toc.sections.emplace_back(name.c_str(), w.Tell(), 0);
    writeFn();
    toc.sections.back().size = w.Tell() - toc.sections.back().start;
}

void
CrateFile::_AddDeferredTimeSampledSpecs()
{
    // A map from sample time to VtValues within TimeSamples instances in
    // _deferredTimeSampledSpecs.
    boost::container::flat_map<double, vector<VtValue *>> allValuesAtAllTimes;

    // Search for the TimeSamples, add to the allValuesAtAllTimes.
    for (auto &spec: _deferredTimeSampledSpecs) {
        for (auto &tsf: spec.timeSampleFields) {
            for (size_t i = 0; i != tsf.second.values.size(); ++i) {
                if (!tsf.second.values[i].IsHolding<ValueRep>()) {
                    allValuesAtAllTimes[tsf.second.times.Get()[i]].push_back(
                        &tsf.second.values[i]);
                }
            }
        }
    }

    // Now walk through allValuesAtAllTimes in order and pack all the values,
    // swapping them out with the resulting reps.  This ensures that when we
    // pack the specs, which will re-pack the values, they'll be noops since
    // they are just holding value reps that point into the file.
    for (auto const &p: allValuesAtAllTimes) {
        for (VtValue *val: p.second)
            *val = _PackValue(*val);
    }

    // Now we've transformed all the VtValues in all the timeSampleFields to
    // ValueReps.  We can call _AddField and add them to ordinaryFields, then
    // add the spec.
    for (auto &spec: _deferredTimeSampledSpecs) {
        for (auto &p: spec.timeSampleFields) {
            spec.ordinaryFields.push_back(
                _AddField(make_pair(p.first, VtValue::Take(p.second))));
        }
        _specs.emplace_back(spec.path, spec.specType,
                            _AddFieldSet(spec.ordinaryFields));
    }

    TfReset(_deferredTimeSampledSpecs);
}

bool
CrateFile::_Write()
{
    // First, add any _deferredTimeSampledSpecs, packing their values
    // time-by-time to ensure that all the data for given times is collocated.
    _AddDeferredTimeSampledSpecs();

    // Now proceed with writing.
    _Writer w(this);

    _TableOfContents toc;

    // Write out the sections we don't know about that the packing context
    // captured.
    using std::get;
    for (auto const &s: _packCtx->unknownSections) {
        _Section sec(get<0>(s).c_str(), w.Tell(), get<2>(s));
        w.WriteContiguous(get<1>(s).get(), sec.size);
        toc.sections.push_back(sec);
    }

    _WriteSection(w, _TokensSectionName, toc, [this, &w]() {_WriteTokens(w);});
    _WriteSection(
        w, _StringsSectionName, toc, [this, &w]() {w.Write(_strings);});
    _WriteSection(w, _FieldsSectionName, toc, [this, &w]() {w.Write(_fields);});
    _WriteSection(
        w, _FieldSetsSectionName, toc, [this, &w]() {w.Write(_fieldSets);});
    _WriteSection(w, _PathsSectionName, toc, [this, &w]() {_WritePaths(w);});

    // VERSIONING: If we're writing version 0.0.1, we need to convert to the old
    // form.
    if (_packCtx->writeVersion == Version(0,0,1)) {
        // Copy and write old-structure specs.
        vector<Spec_0_0_1> old(_specs.begin(), _specs.end());
        _WriteSection(
            w, _SpecsSectionName, toc, [this, &w, &old]() {w.Write(old);});
    } else {
        _WriteSection(
            w, _SpecsSectionName, toc, [this, &w]() {w.Write(_specs);});
    }

    _BootStrap boot(_packCtx->writeVersion);

    // Record TOC location, and write it.
    boot.tocOffset = w.Tell();
    w.Write(toc);

    // Write bootstrap at start of file.
    w.Seek(0);
    w.Write(boot);

    // Flush any buffered writes.
    w.Flush();

    _toc = toc;
    _boot = boot;

    // Clear dedup tables.
    _ClearValueHandlerDedupTables();

    return true;
}

void
CrateFile::_AddSpec(const SdfPath &path, SdfSpecType type,
                   const std::vector<FieldValuePair> &fields) {
    // If any of the fields here are TimeSamples, then defer adding this spec to
    // the call to _Write().  In _Write(), we'll add all the sample values
    // time-by-time to ensure that all the data for a given sample time is
    // as collocated as possible in the file.

    vector<FieldIndex> ordinaryFields; // non time-sample valued fields.
    vector<pair<TfToken, TimeSamples>> timeSampleFields;

    ordinaryFields.reserve(fields.size());
    for (auto const &p: fields) {
        if (p.second.IsHolding<TimeSamples>() &&
            p.second.UncheckedGet<TimeSamples>().IsInMemory()) {
            timeSampleFields.emplace_back(
                p.first, p.second.UncheckedGet<TimeSamples>());
        }
        else {
            ordinaryFields.push_back(_AddField(p));
        }
    }

    // If we have no time sample fields, we can just add the spec now.
    // Otherwise defer so we can write all sample values by time in _Write().
    if (timeSampleFields.empty()) {
        _specs.emplace_back(_AddPath(path), type, _AddFieldSet(ordinaryFields));
    }
    else {
        _deferredTimeSampledSpecs.emplace_back(
            _AddPath(path), type,
            std::move(ordinaryFields), std::move(timeSampleFields));
    }        
}

VtValue
CrateFile::_GetTimeSampleValueImpl(TimeSamples const &ts, size_t i) const
{
    // Need to read the rep from the file for index i.
    auto offset = ts.valuesFileOffset + i * sizeof(ValueRep);
    if (_useMmap) {
        auto reader = _MakeReader(_MmapStream(_mapStart, _debugPageMap.get()));
        reader.Seek(offset);
        return VtValue(reader.Read<ValueRep>());
    } else {
        auto reader = _MakeReader(_PreadStream(_inputFile.get()));
        reader.Seek(offset);
        return VtValue(reader.Read<ValueRep>());
    }
}

void
CrateFile::_MakeTimeSampleValuesMutableImpl(TimeSamples &ts) const
{
    // Read out the reps into the vector.
    ts.values.resize(ts.times.Get().size());
    if (_useMmap) {
        auto reader = _MakeReader(_MmapStream(_mapStart, _debugPageMap.get()));
        reader.Seek(ts.valuesFileOffset);
        for (size_t i = 0, n = ts.times.Get().size(); i != n; ++i)
            ts.values[i] = reader.Read<ValueRep>();
    } else {
        auto reader = _MakeReader(_PreadStream(_inputFile.get()));
        reader.Seek(ts.valuesFileOffset);
        for (size_t i = 0, n = ts.times.Get().size(); i != n; ++i)
            ts.values[i] = reader.Read<ValueRep>();
    }
    // Now in memory, no longer reading everything from file.
    ts.valueRep = ValueRep(0);
}

void
CrateFile::_WritePaths(_Writer &w)
{
    SdfPathTable<PathIndex> pathToIndexTable;

    for (auto const &item: _packCtx->pathToPathIndex)
        pathToIndexTable[item.first] = item.second;

    // Write the total # of paths.
    w.WriteAs<uint64_t>(_paths.size());
    _WritePathTree(w, pathToIndexTable.begin(), pathToIndexTable.end());

    WorkSwapDestroyAsync(pathToIndexTable);
}

template <class Iter>
Iter
CrateFile::_WritePathTree(_Writer &w, Iter cur, Iter end)
{
    // Each element looks like this:
    //
    // (pathIndex, pathElementTokenIndex, hasChild, hasSibling)
    // [offset to sibling, if hasSibling and hasChild]
    //
    // If the element's hasChild bit is set, then the very next element is its
    // first child.  If the element's hasChild bit is not set and its hasSibling
    // bit is set, then the very next element is its next sibling.  If both bits
    // are set then an offset to the sibling appears in the stream and the
    // following element is the first child.
    //

    for (Iter next = cur; cur != end; cur = next) {
        Iter nextSubtree = cur.GetNextSubtree();
        ++next;

        bool hasChild = next != nextSubtree &&
            next->first.GetParentPath() == cur->first;

        bool hasSibling = nextSubtree != end &&
            nextSubtree->first.GetParentPath() == cur->first.GetParentPath();

        bool isPrimPropertyPath = cur->first.IsPrimPropertyPath();

        auto elementToken = isPrimPropertyPath ?
            cur->first.GetNameToken() : cur->first.GetElementToken();

        // VERSIONING: If we're writing version 0.0.1, make sure we use the
        // right header type.
        if (_packCtx->writeVersion == Version(0,0,1)) {
            _PathItemHeader_0_0_1 header(
                cur->second, _GetIndexForToken(elementToken),
                static_cast<uint8_t>(
                    (hasChild ? _PathItemHeader::HasChildBit : 0) |
                    (hasSibling ? _PathItemHeader::HasSiblingBit : 0) |
                    (isPrimPropertyPath ?
                     _PathItemHeader::IsPrimPropertyPathBit : 0)));
            w.Write(header);
        } else {
            _PathItemHeader header(
                cur->second, _GetIndexForToken(elementToken),
                static_cast<uint8_t>(
                    (hasChild ? _PathItemHeader::HasChildBit : 0) |
                    (hasSibling ? _PathItemHeader::HasSiblingBit : 0) |
                    (isPrimPropertyPath ?
                     _PathItemHeader::IsPrimPropertyPathBit : 0)));
            w.Write(header);
        }

        // If there's both a child and a sibling, make space for the sibling
        // offset.
        int64_t siblingPtrOffset = -1;
        if (hasSibling && hasChild) {
            siblingPtrOffset = w.Tell();
            // Temporarily write a bogus value just to make space.
            w.WriteAs<int64_t>(-1);
        }
        // If there is a child, recurse.
        if (hasChild)
            next = _WritePathTree(w, next, end);

        // If we have a sibling, then fill in the offset that it will be
        // written at (it will be written next).
        if (hasSibling && hasChild) {
            int64_t cur = w.Tell();
            w.Seek(siblingPtrOffset);
            w.Write(cur);
            w.Seek(cur);
        }

        if (!hasSibling)
            return next;
    }
    return end;
}

void
CrateFile::_WriteTokens(_Writer &w) {
    // # of strings.
    w.WriteAs<uint64_t>(_tokens.size());
    // Count total bytes.
    uint64_t totalBytes = 0;
    for (auto const &t: _tokens)
        totalBytes += t.GetString().size() + 1;
    w.WriteAs<uint64_t>(totalBytes);
    // Token data.
    for (auto const &t: _tokens) {
        auto const &str = t.GetString();
        w.WriteContiguous(str.c_str(), str.size() + 1);
    }
}

template <class Reader>
void
CrateFile::_ReadStructuralSections(Reader reader, int64_t fileSize)
{
    TfErrorMark m;
    _boot = _ReadBootStrap(reader.src, fileSize);
    if (m.IsClean()) _toc = _ReadTOC(reader, _boot);
    if (m.IsClean()) _PrefetchStructuralSections(reader);
    if (m.IsClean()) _ReadTokens(reader);
    if (m.IsClean()) _ReadStrings(reader);
    if (m.IsClean()) _ReadFields(reader);
    if (m.IsClean()) _ReadFieldSets(reader);
    if (m.IsClean()) _ReadPaths(reader);
    if (m.IsClean()) _ReadSpecs(reader);
}

template <class ByteStream>
/*static*/
CrateFile::_BootStrap
CrateFile::_ReadBootStrap(ByteStream src, int64_t fileSize)
{
    _BootStrap b;
    if (fileSize < sizeof(_BootStrap)) {
        TF_RUNTIME_ERROR("File too small to contain bootstrap structure");
        return b;
    }
    src.Seek(0);
    src.Read(&b, sizeof(b));
    // Sanity check.
    if (memcmp(b.ident, USDC_IDENT, sizeof(b.ident))) {
        TF_RUNTIME_ERROR("Usd crate bootstrap section corrupt");
    }
    // Check version.
    else if (!_SoftwareVersion.CanRead(Version(b))) {
        TF_RUNTIME_ERROR(
            "Usd crate file version mismatch -- file is %s, "
            "software supports %s", Version(b).AsString().c_str(),
            _SoftwareVersion.AsString().c_str());
    }
    return b;
}

template <class Reader>
void
CrateFile::_PrefetchStructuralSections(Reader reader) const
{
    // Go through the _toc and find its maximal range, then ask the reader to
    // prefetch that range.
    int64_t min = -1, max = -1;
    for (_Section const &sec: _toc.sections) {
        if (min == -1 || (sec.start < min))
            min = sec.start;
        int64_t end = sec.start + sec.size;
        if (max == -1 || (end > max))
            max = end;
    }
    if (min != -1 && max != -1)
        reader.Prefetch(min, max-min);
}

template <class Reader>
CrateFile::_TableOfContents
CrateFile::_ReadTOC(Reader reader, _BootStrap const &b) const
{
    reader.Seek(b.tocOffset);
    return reader.template Read<_TableOfContents>();
}

template <class Reader>
void
CrateFile::_ReadFieldSets(Reader reader)
{
    TfAutoMallocTag tag("_ReadFieldSets");
    if (auto fieldSetsSection = _toc.GetSection(_FieldSetsSectionName)) {
        reader.Seek(fieldSetsSection->start);
        _fieldSets = reader.template Read<decltype(_fieldSets)>();
    }
}

template <class Reader>
void
CrateFile::_ReadFields(Reader reader)
{
    TfAutoMallocTag tag("_ReadFields");
    if (auto fieldsSection = _toc.GetSection(_FieldsSectionName)) {
        reader.Seek(fieldsSection->start);
        _fields = reader.template Read<decltype(_fields)>();
    }
}

template <class Reader>
void
CrateFile::_ReadSpecs(Reader reader)
{
    TfAutoMallocTag tag("_ReadSpecs");
    if (auto specsSection = _toc.GetSection(_SpecsSectionName)) {
        reader.Seek(specsSection->start);
        // VERSIONING: Have to read either old or new style specs.
        if (Version(_boot) == Version(0,0,1)) {
            vector<Spec_0_0_1> old = reader.template Read<decltype(old)>();
            _specs.resize(old.size());
            copy(old.begin(), old.end(), _specs.begin());
        } else {
            _specs = reader.template Read<decltype(_specs)>();
        }
    }
}

template <class Reader>
void
CrateFile::_ReadStrings(Reader reader)
{
    TfAutoMallocTag tag("_ReadStrings");
    if (auto stringsSection = _toc.GetSection(_StringsSectionName)) {
        reader.Seek(stringsSection->start);
        _strings = reader.template Read<decltype(_strings)>();
    }
}

template <class Reader>
void
CrateFile::_ReadTokens(Reader reader)
{
    TfAutoMallocTag tag("_ReadTokens");

    auto tokensSection = _toc.GetSection(_TokensSectionName);
    if (!tokensSection)
        return;

    reader.Seek(tokensSection->start);

    // Read number of tokens.
    auto numTokens = reader.template Read<uint64_t>();

    // XXX: To support pread(), we need to read the whole thing into memory to
    // make tokens out of it.  This is a pessimization vs mmap, from which we
    // can just construct from the chars directly.
    auto tokensNumBytes = reader.template Read<uint64_t>();

    RawDataPtr chars(new char[tokensNumBytes]);
    reader.ReadContiguous(chars.get(), tokensNumBytes);

    // Now we read that many null-terminated strings into _tokens.
    char const *p = chars.get();
    _tokens.clear();
    _tokens.resize(numTokens);

    WorkArenaDispatcher wd;
    struct MakeToken {
        void operator()() const { (*tokens)[index] = TfToken(str); }
        vector<TfToken> *tokens;
        size_t index;
        char const *str;
    };
    for (size_t i = 0; i != numTokens; ++i) {
        MakeToken mt { &_tokens, i, p };
        wd.Run(mt);
        p += strlen(p) + 1;
    }
    wd.Wait();

    WorkSwapDestroyAsync(chars);
}

template <class Reader>
void
CrateFile::_ReadPaths(Reader reader)
{
    TfAutoMallocTag tag("_ReadPaths");

    auto pathsSection = _toc.GetSection(_PathsSectionName);
    if (!pathsSection)
        return;

    reader.Seek(pathsSection->start);

    // Read # of paths.
    _paths.resize(reader.template Read<uint64_t>());

    // VERSIONING: PathItemHeader changes size from 0.0.1 to 0.1.0.
    Version fileVer(_boot);
    _PathItemHeader root;
    if (fileVer == Version(0,0,1)) {
        auto old = reader.template Read<_PathItemHeader_0_0_1>();
        root.index = old.index;
        root.elementTokenIndex = old.elementTokenIndex;
        root.bits = old.bits;
    } else {
        root = reader.template Read<_PathItemHeader>();
    }

    _paths[root.index.value] = SdfPath::AbsoluteRootPath();

    bool hasChild = root.bits & _PathItemHeader::HasChildBit;
    bool hasSibling = root.bits & _PathItemHeader::HasSiblingBit;

    // Should never have a sibling on the root.  XXX: probably not true with
    // relative paths.
    auto siblingOffset =
        (hasChild && hasSibling) ? reader.template Read<int64_t>() : 0;

    WorkArenaDispatcher dispatcher;

    if (root.bits & _PathItemHeader::HasChildBit) {
        if (fileVer == Version(0,0,1)) {
            auto firstChild = reader.template Read<_PathItemHeader_0_0_1>();
            dispatcher.Run(
                [this, reader, firstChild, &dispatcher]() {
                    _ReadPathsRecursively(reader, SdfPath::AbsoluteRootPath(),
                                          firstChild, dispatcher);
                });
        } else {
            auto firstChild = reader.template Read<_PathItemHeader>();
            dispatcher.Run(
                [this, reader, firstChild, &dispatcher]() {
                    _ReadPathsRecursively(reader, SdfPath::AbsoluteRootPath(),
                                          firstChild, dispatcher);
                });
        }
    }

    if (root.bits & _PathItemHeader::HasSiblingBit) {
        if (hasChild && hasSibling)
            reader.Seek(siblingOffset);
        if (fileVer == Version(0,0,1)) {
            auto siblingHeader = reader.template Read<_PathItemHeader_0_0_1>();
            dispatcher.Run(
                [this, reader, siblingHeader, &dispatcher]() {
                    _ReadPathsRecursively(
                        reader, SdfPath(), siblingHeader, dispatcher);
                });
        } else {
            auto siblingHeader = reader.template Read<_PathItemHeader>();
            dispatcher.Run(
                [this, reader, siblingHeader, &dispatcher]() {
                    _ReadPathsRecursively(
                        reader, SdfPath(), siblingHeader, dispatcher);
                });
        }
    }

    dispatcher.Wait();
}

template <class Reader, class Header>
void
CrateFile::_ReadPathsRecursively(Reader reader,
                                const SdfPath &parentPath,
                                const Header &h,
                                WorkArenaDispatcher &dispatcher)
{
    // XXX Won't need ANY of these tags when bug #132031 is addressed
    TfAutoMallocTag2 tag("Usd", "Usd_CrateDataImpl::Open");
    TfAutoMallocTag2 tag2("Usd_CrateFile::CrateFile::Open", "_ReadPaths");

    bool hasChild = h.bits & _PathItemHeader::HasChildBit;
    bool hasSibling = h.bits & _PathItemHeader::HasSiblingBit;
    bool isPrimPropertyPath = h.bits & _PathItemHeader::IsPrimPropertyPathBit;

    auto const &elemToken = _tokens[h.elementTokenIndex.value];

    // Create this path.
    _paths[h.index.value] = isPrimPropertyPath ?
        parentPath.AppendProperty(elemToken) :
        parentPath.AppendElementToken(elemToken);

    // If this one has a sibling, read out the pointer.
    auto siblingOffset =
        (hasSibling && hasChild) ? reader.template Read<int64_t>() : 0;

    // If we have either a child or a sibling but not both, then just continue
    // to the neighbor.  If we have both then spawn a task for the sibling and
    // do the child ourself.  We think that our path trees tend to be broader
    // than deep.

    // If this header item has a child, recurse to it.
    auto childHeader = hasChild ? reader.template Read<Header>() : Header();
    auto childReader = reader;
    auto siblingHeader = Header();

    if (hasSibling) {
        if (hasChild)
            reader.Seek(siblingOffset);
        siblingHeader = reader.template Read<Header>();
    }

    if (hasSibling) {
        if (hasChild) {
            dispatcher.Run(
                [this, reader, parentPath, siblingHeader, &dispatcher]() {
                    _ReadPathsRecursively(
                        reader, parentPath, siblingHeader, dispatcher);
                });
        } else {
            _ReadPathsRecursively(
                reader, parentPath, siblingHeader, dispatcher);
        }
    }
    if (hasChild) {
        _ReadPathsRecursively(
            childReader, _paths[h.index.value], childHeader, dispatcher);
    }
}

void
CrateFile::_ReadRawBytes(int64_t start, int64_t size, char *buf) const
{
    if (_useMmap) {
        auto reader = _MakeReader(_MmapStream(_mapStart, _debugPageMap.get()));
        reader.Seek(start);
        reader.template ReadContiguous<char>(buf, size);
    } else {
        auto reader = _MakeReader(_PreadStream(_inputFile.get()));
        reader.Seek(start);
        reader.template ReadContiguous<char>(buf, size);
    }
}

PathIndex
CrateFile::_AddPath(const SdfPath &path)
{
    // Try to insert this path.
    auto iresult = _packCtx->pathToPathIndex.emplace(path, PathIndex());
    if (iresult.second) {
        // If this is a target path, add the target.
        if (path.IsTargetPath())
            _AddPath(path.GetTargetPath());

        // Not present -- ensure parent is added.
        if (path != SdfPath::AbsoluteRootPath())
            _AddPath(path.GetParentPath());

        // Add a token for this path's element string, unless it's a prim
        // property path, in which case we add the name.  We treat prim property
        // paths separately since there are so many, and the name with the dot
        // just basically doubles the number of tokens we store.
        _AddToken(path.IsPrimPropertyPath() ? path.GetNameToken() :
                  path.GetElementToken());

        // Add to the vector and insert the index.
        iresult.first->second = PathIndex(_paths.size());
        _paths.emplace_back(path);
    }
    return iresult.first->second;
}

FieldSetIndex
CrateFile::_AddFieldSet(const std::vector<FieldIndex> &fieldIndexes)
{
    auto iresult =
        _packCtx->fieldsToFieldSetIndex.emplace(fieldIndexes, FieldSetIndex());
    if (iresult.second) {
        // Not yet present.  Copy the fields to _fieldSets, terminate, and store
        // the start index.
        iresult.first->second = FieldSetIndex(_fieldSets.size());
        _fieldSets.insert(_fieldSets.end(),
                          fieldIndexes.begin(), fieldIndexes.end());
        _fieldSets.push_back(FieldIndex());
    }
    return iresult.first->second;
}

FieldIndex
CrateFile::_AddField(const FieldValuePair &fv)
{
    Field field(_AddToken(fv.first), _PackValue(fv.second));
    auto iresult = _packCtx->fieldToFieldIndex.emplace(field, FieldIndex());
    if (iresult.second) {
        // Not yet present.
        iresult.first->second = FieldIndex(_fields.size());
        _fields.push_back(field);
    }
    return iresult.first->second;
}

TokenIndex
CrateFile::_AddToken(const TfToken &token)
{
    auto iresult = _packCtx->tokenToTokenIndex.emplace(token, TokenIndex());
    if (iresult.second) {
        // Not yet present.
        iresult.first->second = TokenIndex(_tokens.size());
        _tokens.emplace_back(token);
    }
    return iresult.first->second;
}

TokenIndex
CrateFile::_GetIndexForToken(const TfToken &token) const
{
    auto iter = _packCtx->tokenToTokenIndex.find(token);
    if (!TF_VERIFY(iter != _packCtx->tokenToTokenIndex.end()))
        return TokenIndex();
    return iter->second;
}

StringIndex
CrateFile::_AddString(const string &str)
{
    auto iresult = _packCtx->stringToStringIndex.emplace(str, StringIndex());
    if (iresult.second) {
        // Not yet present.
        iresult.first->second = StringIndex(_strings.size());
        _strings.push_back(_AddToken(TfToken(str)));
    }
    return iresult.first->second;
}

template <class T>
CrateFile::_ValueHandler<T> &
CrateFile::_GetValueHandler() {
    return *static_cast<_ValueHandler<T> *>(
        _valueHandlers[static_cast<int>(TypeEnumFor<T>())]);
}

template <class T>
CrateFile::_ValueHandler<T> const &
CrateFile::_GetValueHandler() const {
    return *static_cast<_ValueHandler<T> const *>(
        _valueHandlers[static_cast<int>(TypeEnumFor<T>())]);
}

template <class T>
ValueRep
CrateFile::_PackValue(T const &v) {
    return _GetValueHandler<T>().Pack(_Writer(this), v);
}

template <class T>
ValueRep
CrateFile::_PackValue(VtArray<T> const &v) {
    return _GetValueHandler<T>().PackArray(_Writer(this), v);
}

ValueRep
CrateFile::_PackValue(VtValue const &v)
{
    // If the value is holding a ValueRep, then we can just return it, we don't
    // need to add anything.
    if (v.IsHolding<ValueRep>())
        return v.UncheckedGet<ValueRep>();

    // Similarly if the value is holding a TimeSamples that is still reading
    // from the file, we can return its held rep and continue.
    if (v.IsHolding<TimeSamples>()) {
        auto const &ts = v.UncheckedGet<TimeSamples>();
        if (!ts.IsInMemory())
            return ts.valueRep;
    }

    std::type_index ti =
        v.IsArrayValued() ? v.GetElementTypeid() : v.GetTypeid();

    auto it = _packValueFunctions.find(ti);
    if (it != _packValueFunctions.end())
        return it->second(v);

    TF_CODING_ERROR("Attempted to pack unsupported type '%s' "
                    "(%s)\n", ArchGetDemangled(ti).c_str(),
                    TfStringify(v).c_str());

    return ValueRep(0);
}

template <class T>
void
CrateFile::_UnpackValue(ValueRep rep, T *out) const
{
    auto const &h = _GetValueHandler<T>();
    if (_useMmap) {
        h.Unpack(_MakeReader(_MmapStream(_mapStart,
                                         _debugPageMap.get())), rep, out);
    } else {
        h.Unpack(_MakeReader(_PreadStream(_inputFile.get())), rep, out);
    }
}

template <class T>
void
CrateFile::_UnpackValue(ValueRep rep, VtArray<T> *out) const {
    auto const &h = _GetValueHandler<T>();
    if (_useMmap) {
        h.UnpackArray(_MakeReader(_MmapStream(_mapStart,
                                              _debugPageMap.get())), rep, out);
    } else {
        h.UnpackArray(_MakeReader(_PreadStream(_inputFile.get())), rep, out);
    }
}

void
CrateFile::_UnpackValue(ValueRep rep, VtValue *result) const {
    // Look up the function for the type enum, and invoke it.
    auto repType = rep.GetType();
    if (repType == TypeEnum::Invalid || repType >= TypeEnum::NumTypes) {
        TF_CODING_ERROR("Attempted to unpack unsupported type enum value %d",
                        static_cast<int>(repType));
        return;
    }
    auto index = static_cast<int>(repType);
    if (_useMmap) {
        _unpackValueFunctionsMmap[index](rep, result);
    } else {
        _unpackValueFunctionsPread[index](rep, result);
    }
}

// Enum to TfType table.
struct _EnumToTfTypeTablePopulater {
    template <class T, class Table>
    static void _Set(Table &table, TypeEnum typeEnum) {
        auto index = static_cast<int>(typeEnum);
        auto tfType = TfType::Find<T>();
        TF_VERIFY(!tfType.IsUnknown(),
                  "%s not registered with TfType",
                  ArchGetDemangled<T>().c_str());
        table[index] = tfType;
    }

    template <class T, class Table>
    static typename
    std::enable_if<!ValueTypeTraits<T>::supportsArray>::type
    Populate(Table &scalarTable, Table &) {
        _Set<T>(scalarTable, TypeEnumFor<T>());
    }

    template <class T, class Table>
    static typename
    std::enable_if<ValueTypeTraits<T>::supportsArray>::type
    Populate(Table &scalarTable, Table &arrayTable) {
        _Set<T>(scalarTable, TypeEnumFor<T>());
        _Set<VtArray<T>>(arrayTable, TypeEnumFor<T>());
    }
};

template <class T>
void CrateFile::_DoTypeRegistration() {
    auto typeEnumIndex = static_cast<int>(TypeEnumFor<T>());
    auto valueHandler = new _ValueHandler<T>();
    _valueHandlers[typeEnumIndex] = valueHandler;

    // Value Pack/Unpack functions.
    _packValueFunctions[std::type_index(typeid(T))] =
        [this, valueHandler](VtValue const &val) {
            return valueHandler->PackVtValue(_Writer(this), val);
        };

    _unpackValueFunctionsPread[typeEnumIndex] =
        [this, valueHandler](ValueRep rep, VtValue *out) {
            valueHandler->UnpackVtValue(
                _MakeReader(_PreadStream(_inputFile.get())), rep, out);
        };

    _unpackValueFunctionsMmap[typeEnumIndex] =
        [this, valueHandler](ValueRep rep, VtValue *out) {
            valueHandler->UnpackVtValue(
                _MakeReader(_MmapStream(_mapStart,
                                        _debugPageMap.get())), rep, out);
        };

    _EnumToTfTypeTablePopulater::Populate<T>(
        _typeEnumToTfType, _typeEnumToTfTypeForArray);
}

// Functions that populate the value read/write functions.
void
CrateFile::_DoAllTypeRegistrations() {
    TfAutoMallocTag tag("Usd_CrateFile::CrateFile::_DoAllTypeRegistrations");
#define xx(_unused1, _unused2, CPPTYPE, _unused3)       \
    _DoTypeRegistration<CPPTYPE>();

#include "crateDataTypes.h"

#undef xx
}

void
CrateFile::_DeleteValueHandlers() {
#define xx(_unused1, _unused2, T, _unused3)                                    \
    delete static_cast<_ValueHandler<T> *>(                                    \
        _valueHandlers[static_cast<int>(TypeEnumFor<T>())]);

#include "crateDataTypes.h"

#undef xx
}

void
CrateFile::_ClearValueHandlerDedupTables() {
#define xx(_unused1, _unused2, T, _unused3)                                    \
    static_cast<_ValueHandler<T> *>(                                           \
        _valueHandlers[static_cast<int>(TypeEnumFor<T>())])->Clear();

#include "crateDataTypes.h"

#undef xx
}

/* static */
bool
CrateFile::_IsKnownSection(char const *name) {
    for (auto const &secName: _KnownSections) {
        if (secName == name)
            return true;
    }
    return false;
}

void
CrateFile::_Fcloser::operator()(FILE *f) const
{
    if (f) {
        fclose(f);
    }
}

CrateFile::Spec::Spec(Spec_0_0_1 const &s) 
    : Spec(s.pathIndex, s.specType, s.fieldSetIndex) {}

CrateFile::Spec_0_0_1::Spec_0_0_1(Spec const &s) 
    : Spec_0_0_1(s.pathIndex, s.specType, s.fieldSetIndex) {}

CrateFile::_BootStrap::_BootStrap() : _BootStrap(_SoftwareVersion) {}

CrateFile::_BootStrap::_BootStrap(Version const &ver)
{
    memset(this, 0, sizeof(*this));
    tocOffset = 0;
    memcpy(ident, USDC_IDENT, sizeof(ident));
    version[0] = ver.majver;
    version[1] = ver.minver;
    version[2] = ver.patchver;
}

CrateFile::_Section::_Section(char const *inName, int64_t start, int64_t size)
    : start(start), size(size)
{
    memset(name, 0, sizeof(name));
    if (TF_VERIFY(strlen(inName) <= _SectionNameMaxLength))
        strcpy(name, inName);
}

std::ostream &
operator<<(std::ostream &o, ValueRep rep) {
    o << "ValueRep enum=" << int(rep.GetType());
    if (rep.IsArray())
        o << " (array)";
    return o << " payload=" << rep.GetPayload();
}

std::ostream &
operator<<(std::ostream &os, TimeSamples const &samples) {
    return os << "TimeSamples with " <<
        samples.times.Get().size() << " samples";
}

std::ostream &
operator<<(std::ostream &os, Index const &i) {
    return os << i.value;
}

// Size checks for structures written to/read from disk.
static_assert(sizeof(CrateFile::Field) == 16, "");
static_assert(sizeof(CrateFile::Spec) == 12, "");
static_assert(sizeof(CrateFile::Spec_0_0_1) == 16, "");
static_assert(sizeof(_PathItemHeader) == 12, "");
static_assert(sizeof(_PathItemHeader_0_0_1) == 16, "");

} // Usd_CrateFile



PXR_NAMESPACE_CLOSE_SCOPE

