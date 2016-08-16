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
#include "crateFile.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/errno.h"
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
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/dispatcher.h"
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

#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>

#include <sys/mman.h>

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<Usd_CrateFile::TimeSamples>();
}

// Write nbytes bytes to fd at pos.
static inline ssize_t
WriteToFd(int fd, void const *bytes, ssize_t nbytes, int64_t pos) {
    // It's claimed that correct, modern POSIX will never return 0 for (p)write
    // unless nbytes is zero.  It will either be the case that some bytes were
    // written, or we get an error return.

    // Write and check if all got written (most common case).
    ssize_t nwritten = pwrite(fd, bytes, nbytes, pos);
    if (ARCH_LIKELY(nwritten == nbytes))
        return nwritten;

    // Track a total and retry until we write everything or hit an error.
    ssize_t total = std::max<ssize_t>(nwritten, 0);
    while (nwritten != -1) {
        // Update bookkeeping and retry.
        total += nwritten;
        nbytes -= nwritten;
        pos += nwritten;
        bytes = static_cast<char const *>(bytes) + nwritten;
        nwritten = pwrite(fd, bytes, nbytes, pos);
        if (ARCH_LIKELY(nwritten == nbytes))
            return total + nwritten;
    }

    // Error case.
    TF_RUNTIME_ERROR("Failed writing usdc data: %s", ArchStrerror().c_str());
    return total;
}    

namespace {

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

// Metafunction that determines if a T instance can be read/written by simple
// bitwise copy.
template <class T>
struct _IsBitwiseReadWrite {
    static const bool value =
        std::is_enum<T>::value or
        std::is_arithmetic<T>::value or
        std::is_same<T, half>::value or
        std::is_trivial<T>::value or
        GfIsGfVec<T>::value or
        GfIsGfMatrix<T>::value or
        GfIsGfQuat<T>::value or
        std::is_base_of<_BitwiseReadWrite, T>::value;
};

template <class T>
constexpr bool _IsInlinedType() {
    using std::is_same;
    return is_same<T, string>::value or
        is_same<T, TfToken>::value or
        is_same<T, SdfPath>::value or
        is_same<T, SdfAssetPath>::value or
        (sizeof(T) <= sizeof(uint32_t) and _IsBitwiseReadWrite<T>::value);
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

int64_t
_GetFileSize(FILE *f)
{
    const int fd = fileno(f);
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0) {
        TF_RUNTIME_ERROR("Error retrieving file size");
        return -1;
    }
    return fileInfo.st_size;
}

string
_GetVersionString(uint8_t major, uint8_t minor, uint8_t patch) {
    return TfStringPrintf("%d.%d.%d", major, minor, patch);
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

constexpr uint8_t USDC_MAJOR = 0;
constexpr uint8_t USDC_MINOR = 0;
constexpr uint8_t USDC_PATCH = 1;

constexpr char const *USDC_IDENT = "PXR-USDC"; // 8 chars.

struct _Token : _BitwiseReadWrite {
    _Token() {}
    explicit _Token(StringIndex si) : stringIndex(si) {}
    StringIndex stringIndex;
};

struct _PathItemHeader : _BitwiseReadWrite {
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

struct _ListOpHeader : _BitwiseReadWrite {
    enum _Bits { IsExplicitBit = 1 << 0,
                 HasExplicitItemsBit = 1 << 1,
                 HasAddedItemsBit = 1 << 2,
                 HasDeletedItemsBit = 1 << 3,
                 HasOrderedItemsBit = 1 << 4 };

    _ListOpHeader() : bits(0) {}

    template <class T>
    explicit _ListOpHeader(SdfListOp<T> const &op) : bits(0) {
        bits |= op.IsExplicit() ? IsExplicitBit : 0;
        bits |= op.GetExplicitItems().size() ? HasExplicitItemsBit : 0;
        bits |= op.GetAddedItems().size() ? HasAddedItemsBit : 0;
        bits |= op.GetDeletedItems().size() ? HasDeletedItemsBit : 0;
        bits |= op.GetOrderedItems().size() ? HasOrderedItemsBit : 0;
    }

    bool IsExplicit() const { return bits & IsExplicitBit; }

    bool HasExplicitItems() const { return bits & HasExplicitItemsBit; }
    bool HasAddedItems() const { return bits & HasAddedItemsBit; }
    bool HasDeletedItems() const { return bits & HasDeletedItemsBit; }
    bool HasOrderedItems() const { return bits & HasOrderedItemsBit; }

    uint8_t bits;
};

struct _MmapStream {
    explicit _MmapStream(char const *mapStart)
        : _cur(mapStart), _mapStart(mapStart) {}

    inline void Read(void *dest, size_t nBytes) {
        memcpy(dest, _cur, nBytes);
        _cur += nBytes;
    }
    inline int64_t Tell() const { return _cur - _mapStart; }
    inline void Seek(int64_t offset) { _cur = _mapStart + offset; }
private:
    char const *_cur;
    char const *_mapStart;
};

struct _PreadStream {
    explicit _PreadStream(FILE *file) : _cur(0), _fd(fileno(file)) {}
    inline void Read(void *dest, size_t nBytes) {
        _cur += pread(_fd, dest, nBytes, _cur);
    }
    inline off_t Tell() const { return _cur; }
    inline void Seek(off_t offset) { _cur = offset; }
private:
    off_t _cur;
    int _fd;
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
// PackingContext
struct CrateFile::_PackingContext
{
    _PackingContext(CrateFile *crate, FILE *file) 
        : file(file)
        , filefd(fileno(file)) {
        // Populate this context with everything we need from \p crate in order
        // to do deduplication, etc.
        WorkArenaDispatcher wd;

        // Read in any unknown sections so we can rewrite them later.
        wd.Run([this, crate]() {
                for (auto const &sec: crate->_toc.sections) {
                    if (not _IsKnownSection(sec.name)) {
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
        outFilePos = crate->_toc.GetMinimumSectionStart();

        wd.Wait();
    }

    // Read the bytes of some unknown section into memory so we can rewrite them
    // out later (to preserve it).
    unique_ptr<char[]>
    _ReadSectionBytes(_Section const &sec, CrateFile *crate) const {
        unique_ptr<char[]> result(new char[sec.size]);
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
    vector<tuple<string, unique_ptr<char[]>, size_t>> unknownSections;

    // File we're writing to, and corresponding file descriptor.
    FILE *file;
    int filefd;
    // Current position in output file.
    int64_t outFilePos;
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
        if (h.HasDeletedItems()) { listOp.SetDeletedItems(Read<vector<T>>()); }
        if (h.HasOrderedItems()) { listOp.SetOrderedItems(Read<vector<T>>()); }
        return listOp;
    }
    VtValue Read(VtValue *) {
        _RecursiveRead();
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
    typename std::enable_if<not _IsBitwiseReadWrite<T>::value>::type
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
        : crate(crate), sinkfd(crate->_packCtx->filefd) {}

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

    int64_t Tell() const { return crate->_packCtx->outFilePos; }

    void Seek(int64_t offset) { crate->_packCtx->outFilePos = offset; }

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
    Write(T const &bits) {
        crate->_packCtx->outFilePos += WriteToFd(
            sinkfd, &bits, sizeof(bits), crate->_packCtx->outFilePos);
    }

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
        Write(h);
        if (h.HasExplicitItems()) { Write(listOp.GetExplicitItems()); }
        if (h.HasAddedItems()) { Write(listOp.GetAddedItems()); }
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
        crate->_packCtx->outFilePos += WriteToFd(
            sinkfd, values, sizeof(*values) * sz, crate->_packCtx->outFilePos);
    }

    template <class T>
    typename std::enable_if<not _IsBitwiseReadWrite<T>::value>::type
    WriteContiguous(T const *values, size_t sz) {
        std::for_each(values, values + sz, [this](T const &v) { Write(v); });
    }

    CrateFile *crate;
    int sinkfd;
};


////////////////////////////////////////////////////////////////////////
// ValueHandler class hierarchy.  See comment for _ValueHandler itself for more
// information.

struct CrateFile::_ValueHandlerBase {};

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
        if (not _valueDedup) {
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

        if (not _arrayDedup) {
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
    _UniqueFILE in(fopen(fileName.c_str(), "rb"));

    if (not in)
        return false;

    TfErrorMark m;
    _ReadBootStrap(_PreadStream(in.get()), _GetFileSize(in.get()));

    // Clear any issued errors again to avoid propagation, and return true if
    // there were no errors issued.
    return not m.Clear();
}

/* static */
std::unique_ptr<CrateFile>
CrateFile::CreateNew()
{
    bool useMmap = not TfGetenvBool("USDC_USE_PREAD", false);
    return std::unique_ptr<CrateFile>(new CrateFile(useMmap));
}

/* static */
CrateFile::_UniqueMap
CrateFile::_MmapFile(char const *fileName, FILE *file)
{
    _UniqueMap map;
    auto fileSize = _GetFileSize(file);
    if (fileSize > 0) {
        map = _UniqueMap(
            static_cast<char *>(
                mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fileno(file), 0)),
            _Munmapper(fileSize));
        if (not map)
            TF_RUNTIME_ERROR("Couldn't mmap file '%s'", fileName);
    }
    return map;
}

/* static */
std::unique_ptr<CrateFile>
CrateFile::Open(string const &fileName)
{
    TfAutoMallocTag tag2("Usd_CrateFile::CrateFile::Open");

    std::unique_ptr<CrateFile> result;

    // Create a unique_ptr with a functor that fclose()s for a deleter.
    _UniqueFILE inputFile(fopen(fileName.c_str(), "rb"));

    if (not inputFile) {
        TF_RUNTIME_ERROR("Failed to open file '%s'", fileName.c_str());
        return result;
    }

    auto fileSize = _GetFileSize(inputFile.get());
    if (not TfGetenvBool("USDC_USE_PREAD", false)) {
        // Map the file.
        _UniqueMap mapStart = _MmapFile(fileName.c_str(), inputFile.get());
        result.reset(new CrateFile(fileName, std::move(mapStart), fileSize));
    } else {
        result.reset(new CrateFile(fileName, std::move(inputFile), fileSize));
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
    static TfToken tok(_GetVersionString(USDC_MAJOR, USDC_MINOR, USDC_PATCH));
    return tok;
}

TfToken
CrateFile::GetFileVersionToken() const
{
    return TfToken(
        _GetVersionString(
            _boot.version[0], _boot.version[1], _boot.version[2]));
}

CrateFile::CrateFile(bool useMmap)
    : _useMmap(useMmap)
{
    _DoAllTypeRegistrations();
}

CrateFile::CrateFile(
    string const &fileName, _UniqueMap mapStart, int64_t fileSize)
    : _mapStart(std::move(mapStart))
    , _fileName(fileName)
    , _useMmap(true)
{
    _DoAllTypeRegistrations();

    if (_mapStart) {
        auto reader = _MakeReader(_MmapStream(_mapStart.get()));
        TfErrorMark m;
        _ReadStructuralSections(reader, fileSize);
        if (not m.IsClean())
            _fileName.clear();
    } else {
        _fileName.clear();
    }
}

CrateFile::CrateFile(
    string const &fileName, _UniqueFILE inputFile, int64_t fileSize)
    : _inputFile(std::move(inputFile))
    , _fileName(fileName)
    , _useMmap(false)
{
    _DoAllTypeRegistrations();

    auto reader = _MakeReader(_PreadStream(_inputFile.get()));
    TfErrorMark m;
    _ReadStructuralSections(reader, fileSize);
    if (not m.IsClean())
        _fileName.clear();
}

CrateFile::~CrateFile()
{
    _DeleteValueHandlers();
}

CrateFile::Packer
CrateFile::StartPacking(string const &fileName)
{
    TF_VERIFY(_fileName.empty() or _fileName == fileName);
    // We open the file for read/write (update) here in case we already have the
    // file, since we're not rewriting the whole thing.
    _UniqueFILE out(fopen(fileName.c_str(), _fileName.empty() ? "w+b" : "r+b"));
    if (not out) {
        TF_RUNTIME_ERROR("Failed to open '%s' for writing", fileName.c_str());
    } else {
        // Create a packing context so we can start writing.
        _packCtx.reset(new _PackingContext(this, out.release()));
        // Get rid of our local list of specs, if we have one -- the client is
        // required to repopulate it.
        vector<Spec>().swap(_specs);
        _fileName = fileName;
    }
    return Packer(this);
}

CrateFile::Packer::operator bool() const {
    return _crate and _crate->_packCtx;
}

bool
CrateFile::Packer::Close()
{
    if (not TF_VERIFY(_crate))
        return false;

    if (FILE *fp = _crate->_packCtx->file) {

        // Write contents.
        bool writeResult = _crate->_Write();

        // Pull out the file handle and kill the packing context.
        _UniqueFILE file(fp);
        _crate->_packCtx.reset();

        if (not writeResult)
            return false;

        // Reset the mapping or file so we can read values from the newly
        // written file.
        if (_crate->_useMmap) {
            // Must remap the file.
            _crate->_mapStart =
                _MmapFile(_crate->_fileName.c_str(), file.get());
            if (not _crate->_mapStart)
                return false;
        } else {
            // Must adopt the file handle if we don't already have one.
            _crate->_inputFile = std::move(file);
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

bool
CrateFile::_Write()
{
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
    _WriteSection(w, _SpecsSectionName, toc, [this, &w]() {w.Write(_specs);});

    _BootStrap boot;

    // Record TOC location, and write it.
    boot.tocOffset = w.Tell();
    w.Write(toc);

    // Write bootstrap at start of file.
    w.Seek(0);
    w.Write(boot);

    _toc = toc;
    _boot = boot;

    return true;
}

void
CrateFile::_AddSpec(const SdfPath &path, SdfSpecType type,
                   const std::vector<FieldValuePair> &fields) {
    _specs.push_back(Spec(_AddPath(path), type, _AddFieldSet(fields)));
}

VtValue
CrateFile::_GetTimeSampleValueImpl(TimeSamples const &ts, size_t i) const
{
    // Need to read the rep from the file for index i.
    auto offset = ts.valuesFileOffset + i * sizeof(ValueRep);
    if (_useMmap) {
        auto reader = _MakeReader(_MmapStream(_mapStart.get()));
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
        auto reader = _MakeReader(_MmapStream(_mapStart.get()));
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

        bool hasChild = next != nextSubtree and
            next->first.GetParentPath() == cur->first;

        bool hasSibling = nextSubtree != end and
            nextSubtree->first.GetParentPath() == cur->first.GetParentPath();

        bool isPrimPropertyPath = cur->first.IsPrimPropertyPath();

        auto elementToken = isPrimPropertyPath ?
            cur->first.GetNameToken() : cur->first.GetElementToken();

        _PathItemHeader header(
            cur->second, _GetIndexForToken(elementToken),
            static_cast<uint8_t>(
                (hasChild ? _PathItemHeader::HasChildBit : 0) |
                (hasSibling ? _PathItemHeader::HasSiblingBit : 0) |
                (isPrimPropertyPath ?
                 _PathItemHeader::IsPrimPropertyPathBit : 0)));

        w.Write(header);

        // If there's both a child and a sibling, make space for the sibling
        // offset.
        int64_t siblingPtrOffset = -1;
        if (hasSibling and hasChild) {
            siblingPtrOffset = w.Tell();
            // Temporarily write a bogus value just to make space.
            w.WriteAs<int64_t>(-1);
        }
        // If there is a child, recurse.
        if (hasChild)
            next = _WritePathTree(w, next, end);

        // If we have a sibling, then fill in the offset that it will be
        // written at (it will be written next).
        if (hasSibling and hasChild) {
            int64_t cur = w.Tell();
            w.Seek(siblingPtrOffset);
            w.Write(cur);
            w.Seek(cur);
        }

        if (not hasSibling)
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
    else if (b.version[0] != USDC_MAJOR or b.version[1] > USDC_MINOR) {
        char const *type = b.version[0] != USDC_MAJOR ? "major" : "minor";
        TF_RUNTIME_ERROR(
            "Usd crate file %s version mismatch -- file is %s, "
            "software supports %s", type,
            _GetVersionString(b.version[0], b.version[1], b.version[2]).c_str(),
            GetSoftwareVersionToken().GetText());
    }
    return b;
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
        _specs = reader.template Read<decltype(_specs)>();
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
    if (not tokensSection)
        return;

    reader.Seek(tokensSection->start);

    // Read number of tokens.
    auto numTokens = reader.template Read<uint64_t>();

    // XXX: To support pread(), we need to read the whole thing into memory to
    // make tokens out of it.  This is a pessimization vs mmap, from which we
    // can just construct from the chars directly.
    auto tokensNumBytes = reader.template Read<uint64_t>();

    std::unique_ptr<char[]> chars(new char[tokensNumBytes]);
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
}

template <class Reader>
void
CrateFile::_ReadPaths(Reader reader)
{
    TfAutoMallocTag tag("_ReadPaths");

    auto pathsSection = _toc.GetSection(_PathsSectionName);
    if (not pathsSection)
        return;

    reader.Seek(pathsSection->start);

    // Read # of paths.
    _paths.resize(reader.template Read<uint64_t>());

    auto root = reader.template Read<_PathItemHeader>();
    _paths[root.index.value] = SdfPath::AbsoluteRootPath();

    bool hasChild = root.bits & _PathItemHeader::HasChildBit;
    bool hasSibling = root.bits & _PathItemHeader::HasSiblingBit;

    // Should never have a sibling on the root.  XXX: probably not true with
    // relative paths.
    auto siblingOffset =
        (hasChild and hasSibling) ? reader.template Read<int64_t>() : 0;

    WorkArenaDispatcher dispatcher;

    if (root.bits & _PathItemHeader::HasChildBit) {
        auto firstChild = reader.template Read<_PathItemHeader>();
        dispatcher.Run(
            [this, reader, firstChild, &dispatcher]() {
                _ReadPathsRecursively(reader, SdfPath::AbsoluteRootPath(),
                                      firstChild, dispatcher);
            });
    }

    if (root.bits & _PathItemHeader::HasSiblingBit) {
        if (hasChild and hasSibling)
            reader.Seek(siblingOffset);
        auto siblingHeader = reader.template Read<_PathItemHeader>();
        dispatcher.Run(
            [this, reader, siblingHeader, &dispatcher]() {
                _ReadPathsRecursively(
                    reader, SdfPath(), siblingHeader, dispatcher);
            });
    }

    dispatcher.Wait();
}

template <class Reader>
void
CrateFile::_ReadPathsRecursively(Reader reader,
                                const SdfPath &parentPath,
                                const _PathItemHeader &h,
                                WorkArenaDispatcher &dispatcher)
{
    // XXX Won't need ANY of these tags when bug #132031 is addressed
    TfAutoMallocTag2 tag("Usd", "Usd_CrateDataImpl::Open");
    TfAutoMallocTag2 tag2("Usd_CrateFile::CrateFile::Open", "_ReadPaths");

    bool hasChild = h.bits & _PathItemHeader::HasChildBit;
    bool hasSibling = h.bits & _PathItemHeader::HasSiblingBit;
    bool isPrimPropertyPath = h.bits & _PathItemHeader::IsPrimPropertyPathBit;

    auto const &elemToken = _tokens[h.elementTokenIndex.value];

    auto thisPath = isPrimPropertyPath ?
        parentPath.AppendProperty(elemToken) :
        parentPath.AppendElementToken(elemToken);

    // Create this path.
    _paths[h.index.value] = thisPath;

    // If this one has a sibling, read out the pointer.
    auto siblingOffset =
        (hasSibling and hasChild) ? reader.template Read<int64_t>() : 0;

    // If we have either a child or a sibling but not both, then just continue
    // to the neighbor.  If we have both then spawn a task for the sibling and
    // do the child ourself.  We think that our path trees tend to be broader
    // than deep.

    // If this header item has a child, recurse to it.
    auto childHeader =
        hasChild ? reader.template Read<_PathItemHeader>() : _PathItemHeader();
    auto childReader = reader;
    auto siblingHeader = _PathItemHeader();

    if (hasSibling) {
        if (hasChild)
            reader.Seek(siblingOffset);
        siblingHeader = reader.template Read<_PathItemHeader>();
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
        auto reader = _MakeReader(_MmapStream(_mapStart.get()));
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
CrateFile::_AddFieldSet(const std::vector<FieldValuePair> &fields)
{
    auto fieldIndexes = std::vector<FieldIndex>(fields.size());
    transform(fields.begin(), fields.end(), fieldIndexes.begin(),
              [this](FieldValuePair const &f) { return _AddField(f); });

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
    if (not TF_VERIFY(iter != _packCtx->tokenToTokenIndex.end()))
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
        if (not ts.IsInMemory())
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
        h.Unpack(_MakeReader(_MmapStream(_mapStart.get())), rep, out);
    } else {
        h.Unpack(_MakeReader(_PreadStream(_inputFile.get())), rep, out);
    }
}

template <class T>
void
CrateFile::_UnpackValue(ValueRep rep, VtArray<T> *out) const {
    auto const &h = _GetValueHandler<T>();
    if (_useMmap) {
        h.UnpackArray(_MakeReader(_MmapStream(_mapStart.get())), rep, out);
    } else {
        h.UnpackArray(_MakeReader(_PreadStream(_inputFile.get())), rep, out);
    }
}

void
CrateFile::_UnpackValue(ValueRep rep, VtValue *result) const {
    // Look up the function for the type enum, and invoke it.
    auto repType = rep.GetType();
    if (repType == TypeEnum::Invalid or repType >= TypeEnum::NumTypes) {
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
        TF_VERIFY(not tfType.IsUnknown(),
                  "%s not registered with TfType",
                  ArchGetDemangled<T>().c_str());
        table[index] = tfType;
    }

    template <class T, class Table>
    static typename
    std::enable_if<not ValueTypeTraits<T>::supportsArray>::type
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
                _MakeReader(_MmapStream(_mapStart.get())), rep, out);
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
CrateFile::_Munmapper::operator()(char *mapStart) const
{
    if (mapStart) {
        munmap(mapStart, fileSize);
    }
}

void
CrateFile::_Fcloser::operator()(FILE *f) const
{
    if (f) {
        fclose(f);
    }
}

CrateFile::_BootStrap::_BootStrap()
{
    memset(this, 0, sizeof(*this));
    tocOffset = 0;
    memcpy(ident, USDC_IDENT, sizeof(ident));
    version[0] = USDC_MAJOR;
    version[1] = USDC_MINOR;
    version[2] = USDC_PATCH;
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

} // Usd_CrateFile


