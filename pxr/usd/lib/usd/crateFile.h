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
#ifndef USD_CRATEFILE_H
#define USD_CRATEFILE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/crateData.h"

#include "shared.h"
#include "crateValueInliners.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"

#include <boost/container/flat_map.hpp>
#include <boost/functional/hash.hpp>

#include <tbb/spin_rw_mutex.h>

#include <cstdint>
#include <iosfwd>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Usd_CrateFile {

using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::unordered_map;
using std::tuple;
using std::vector;

// Trait indicating trivially copyable types, a hack since gcc doesn't yet
// implement is_trivially_copyable correctly.
template <class T> struct _IsBitwiseReadWrite;

enum class TypeEnum : int32_t;

// Value in file representation.  Consists of a 2 bytes of type information
// (type enum value, array bit, and inlined-value bit) and 6 bytes of data.
// If possible, we attempt to store certain values directly in the local
// data, such as ints, floats, enums, and special-case values of other types
// (zero vectors, identity matrices, etc).  For values that aren't stored
// inline, the 6 data bytes are the offset from the start of the file to the
// value's location.
struct ValueRep {

    friend class CrateFile;

    ValueRep() = default;

    explicit constexpr ValueRep(uint64_t data) : data(data) {}

    constexpr ValueRep(TypeEnum t,
                       bool isInlined, bool isArray, uint64_t payload)
        : data(_Combine(t, isInlined, isArray, payload)) {}

    static const uint64_t _IsArrayBit = 1ull << 63;
    static const uint64_t _IsInlinedBit = 1ull << 62;

    static const uint64_t _PayloadMask = ((1ull << 48) - 1);

    inline bool IsArray() const { return data & _IsArrayBit; }
    inline void SetIsArray() { data |= _IsArrayBit; }

    inline bool IsInlined() const { return data & _IsInlinedBit; }
    inline void SetIsInlined() { data |= _IsInlinedBit; }

    inline TypeEnum GetType() const {
        return static_cast<TypeEnum>((data >> 48) & 0xFF);
    }
    inline void SetType(TypeEnum t) {
        data &= ~(0xFFull << 48); // clear type byte in data.
        data |= (static_cast<uint64_t>(t) << 48); // set it.
    }

    inline uint64_t GetPayload() const {
        return data & _PayloadMask;
    }

    inline void SetPayload(uint64_t payload) {
        data &= ~_PayloadMask; // clear existing payload.
        data |= payload & _PayloadMask;
    }

    inline uint64_t GetData() const { return data; }

    bool operator==(ValueRep other) const {
        return data == other.data;
    }
    bool operator!=(ValueRep other) const {
        return !(*this == other);
    }

    friend inline size_t hash_value(ValueRep v) {
        return static_cast<size_t>(v.data);
    }

    friend std::ostream &operator<<(std::ostream &o, ValueRep rep);

private:
    static constexpr uint64_t
    _Combine(TypeEnum t, bool isInlined, bool isArray, uint64_t payload) {
        return (isArray ? _IsArrayBit : 0) |
            (isInlined ? _IsInlinedBit : 0) |
            (static_cast<uint64_t>(t) << 48) |
            (payload & _PayloadMask);
    }

    uint64_t data;
};
template <> struct _IsBitwiseReadWrite<ValueRep> : std::true_type {};

struct TimeSamples {
    typedef Usd_Shared<vector<double>> SharedTimes;

    TimeSamples() : valueRep(0), valuesFileOffset(0) {}

    bool IsInMemory() const {
        return valueRep.GetData() == 0;
    }

    // Original rep from file if read from file.  This will have GetData()
    // == 0 if not from file or if the samples have been modified.
    ValueRep valueRep;

    // Sample times.
    SharedTimes times;

    // Sample values, but only if we have any in-memory values.  Otherwise we
    // read from the file via valuesFileOffset.
    vector<VtValue> values;

    // Nonzero if we're looking at values in-file.
    int64_t valuesFileOffset;

    // Note that equality does a very shallow equality check since otherwise
    // we'd have to pull all the values from the file.
    bool operator==(TimeSamples const &other) const {
        return valueRep == other.valueRep && 
            times == other.times &&
            values == other.values &&
            valuesFileOffset == other.valuesFileOffset;
    }

    inline void swap(TimeSamples &other) {
        std::swap(valueRep, other.valueRep);
        times.swap(other.times);
        values.swap(other.values);
        std::swap(valuesFileOffset, other.valuesFileOffset);
    }

    friend size_t hash_value(TimeSamples const &ts) {
        size_t h = 0;
        boost::hash_combine(h, ts.valueRep);
        boost::hash_combine(h, ts.times);
        boost::hash_combine(h, ts.values);
        boost::hash_combine(h, ts.valuesFileOffset);
        return h;
    }

    friend std::ostream &
    operator<<(std::ostream &os, TimeSamples const &samples);

    friend inline void swap(TimeSamples &l, TimeSamples &r) { l.swap(r); }
};

// Value type enum.
enum class TypeEnum {
    Invalid = 0,
#define xx(ENUMNAME, ENUMVALUE, _unused1, _unused2)     \
    ENUMNAME = ENUMVALUE,

#include "crateDataTypes.h"

#undef xx
    NumTypes
};

// Index base class.  Used to index various tables.  Deriving adds some
// type-safety so we don't accidentally use one kind of index with the wrong
// kind of table.
struct Index {
    Index() : value(~0) {}
    explicit Index(uint32_t value) : value(value) {}
    bool operator==(const Index &other) const { return value == other.value; }
    bool operator!=(const Index &other) const { return !(*this == other); }
    uint32_t value;
};

inline size_t hash_value(const Index &i) { return i.value; }

std::ostream &operator<<(std::ostream &os, const Index &i);

// Various specific indexes.
struct FieldIndex : Index { using Index::Index; };
struct FieldSetIndex : Index { using Index::Index; };
struct PathIndex : Index { using Index::Index; };
struct StringIndex : Index { using Index::Index; };
struct TokenIndex : Index { using Index::Index; };

constexpr size_t _SectionNameMaxLength = 15;

// Compile time constant section names, enforces max length.
struct _SectionName {
    template <size_t N>
    constexpr _SectionName(char const (&a)[N]) : _p(a), _size(N-1) {
        static_assert(N <= _SectionNameMaxLength,
                      "Section name cannot exceed _SectionNameMaxLength");
    }
    constexpr size_t size() const { return _size; }
    constexpr char const *c_str() const { return _p; }
    constexpr bool operator==(_SectionName other) const {
        return _p == other._p; }
    constexpr bool operator!=(_SectionName other) const {
        return _p != other._p; }
    bool operator==(char const *otherStr) const {
        return !strcmp(_p, otherStr); }
private:
    const char * const _p;
    const size_t _size;
};

struct _Hasher {
    template <class T>
    inline size_t operator()(const T &val) const {
        return boost::hash<T>()(val);
    }
};

class CrateFile
{
public:
    struct Version;

private:
    struct _Fcloser {
        void operator()(FILE *f) const;
    };
    typedef std::unique_ptr<FILE, _Fcloser> _UniqueFILE;

    ////////////////////////////////////////////////////////////////////////

    // _BootStrap structure.  Appears at end of file, houses version, file
    // identifier string and offset to _TableOfContents.
    struct _BootStrap {
        _BootStrap();
        explicit _BootStrap(Version const &);
        uint8_t ident[8]; // "PXR-USDC"
        uint8_t version[8]; // 0: major, 1: minor, 2: patch, rest unused.
        int64_t tocOffset;
        int64_t _reserved[8];
    };

    struct _Section {
        _Section() { memset(this, 0, sizeof(*this)); }
        _Section(char const *name, int64_t start, int64_t size);
        char name[_SectionNameMaxLength+1];
        int64_t start, size;
    };

    struct _TableOfContents {
        _Section const *GetSection(_SectionName) const;
        int64_t GetMinimumSectionStart() const;
        vector<_Section> sections;
    };

public:
    friend struct ValueRep;
    friend struct TimeSamples;

    typedef std::pair<TfToken, VtValue> FieldValuePair;

    struct Field {
        // This padding field accounts for a bug in an earlier implementation,
        // where both this class and its first member derived an empty base
        // class.  The standard requires that those not have the same address so
        // GCC & clang inserted 4 bytes for this class's empty base, causing the
        // first member to land at offset 4.  Porting to MSVC revealed this,
        // since MSVC didn't implement this correctly and the first member
        // landed at offset 0.  To fix this, we've removed the empty base and
        // inserted our own 4 byte padding to ensure the layout comes out the
        // same everywhere.  This doesn't actually change the overall structure
        // size since the first member is 4 bytes and the second is 8.  It's
        // still 16 bytes however you slice it.
        uint32_t _unused_padding_;

        Field() {}
        Field(TokenIndex ti, ValueRep v) : tokenIndex(ti), valueRep(v) {}
        bool operator==(const Field &other) const {
            return tokenIndex == other.tokenIndex &&
                valueRep == other.valueRep;
        }
        friend size_t hash_value(const Field &f) {
            _Hasher h;
            size_t result = h(f.tokenIndex);
            boost::hash_combine(result, f.valueRep);
            return result;
        }
        TokenIndex tokenIndex;
        ValueRep valueRep;
    };

    struct Spec_0_0_1;
    
    struct Spec {
        Spec() {}
        Spec(PathIndex pi, SdfSpecType type, FieldSetIndex fsi)
            : pathIndex(pi), fieldSetIndex(fsi), specType(type) {}
        Spec(Spec_0_0_1 const &);
        bool operator==(const Spec &other) const {
            return pathIndex == other.pathIndex &&
                fieldSetIndex == other.fieldSetIndex &&
                specType == other.specType;
        }
        bool operator!=(const Spec &other) const {
            return !(*this == other);
        }
        friend size_t hash_value(Spec const &s) {
            _Hasher h;
            size_t result = h(s.pathIndex);
            boost::hash_combine(result, s.fieldSetIndex);
            boost::hash_combine(result, s.specType);
            return result;
        }
        PathIndex pathIndex;
        FieldSetIndex fieldSetIndex;
        SdfSpecType specType;
    };

    struct Spec_0_0_1 {
        // This padding field accounts for a bug in this earlier implementation,
        // where both this class and its first member derived an empty base
        // class.  The standard requires that those not have the same address so
        // GCC & clang inserted 4 bytes for this class's empty base, causing the
        // first member to land at offset 4.  Porting to MSVC revealed this,
        // since MSVC didn't implement this correctly and the first member
        // landed at offset 0.  To fix this, we've removed the empty base and
        // inserted our own 4 byte padding to ensure the layout comes out the
        // same everywhere.  File version 0.1.0 revises this structure to the
        // smaller size with no padding.
        uint32_t _unused_padding_;

        Spec_0_0_1() {}
        Spec_0_0_1(PathIndex pi, SdfSpecType type, FieldSetIndex fsi)
            : pathIndex(pi), fieldSetIndex(fsi), specType(type) {}
        Spec_0_0_1(Spec const &);
        bool operator==(const Spec_0_0_1 &other) const {
            return pathIndex == other.pathIndex &&
                fieldSetIndex == other.fieldSetIndex &&
                specType == other.specType;
        }
        bool operator!=(const Spec_0_0_1 &other) const {
            return !(*this == other);
        }
        friend size_t hash_value(Spec_0_0_1 const &s) {
            _Hasher h;
            size_t result = h(s.pathIndex);
            boost::hash_combine(result, s.fieldSetIndex);
            boost::hash_combine(result, s.specType);
            return result;
        }
        PathIndex pathIndex;
        FieldSetIndex fieldSetIndex;
        SdfSpecType specType;
    };

    ~CrateFile();

    static bool CanRead(string const &fileName);
    static TfToken const &GetSoftwareVersionToken();
    TfToken GetFileVersionToken() const;

    static std::unique_ptr<CrateFile> CreateNew();

    // Return nullptr on failure.
    static std::unique_ptr<CrateFile> Open(string const &fileName);

    // Helper for saving to a file.
    struct Packer {
        // Move ctor/assign.
        Packer(Packer &&);
        Packer &operator=(Packer &&);

        // Save the contents to disk.
        ~Packer();

        // Pack an additional spec in the crate.
        inline void PackSpec(const SdfPath &path, SdfSpecType type,
                             const std::vector<FieldValuePair> &fields) {
            _crate->_AddSpec(path, type, fields);
        }

        // Write remaining data and structural sections to disk to produce a
        // complete file.  Return true if the writing completed successfully,
        // false otherwise.
        bool Close();

        // Return true if this Packer is valid to use, false otherwise.
        // Typically false when we failed to open the output file for writing.
        explicit operator bool() const;

    private:
        Packer(Packer const &) = delete;
        Packer &operator=(Packer const &) = delete;

        friend class CrateFile;
        explicit Packer(CrateFile *crate) : _crate(crate) {}
        CrateFile *_crate;
    };

    Packer StartPacking(string const &fileName);

    string const &GetFileName() const { return _fileName; }

    inline Field const &
    GetField(FieldIndex i) const { return _fields[i.value]; }

    inline vector<Field> const & GetFields() const { return _fields; }

    inline vector<FieldIndex> const &
    GetFieldSets() const { return _fieldSets; }

    inline size_t GetNumUniqueFieldSets() const {
        // Count field sets by counting terminators.
        return count(_fieldSets.begin(), _fieldSets.end(), FieldIndex());
    }

    inline SdfPath const &
    GetPath(PathIndex i) const { return _paths[i.value]; }

    inline vector<SdfPath> const &GetPaths() const { return _paths; }

    inline vector<Spec> const &
    GetSpecs() const { return _specs; }

    // Get all structural data out in \p outSpecs, \p outFields,
    // \p outFieldSets, leave those data in this CrateFile empty.
    inline void RemoveStructuralData(
        vector<Spec> &outSpecs,
        vector<Field> &outFields,
        vector<FieldIndex> &outFieldSets) {
        // We swap through temporaries to ensure we leave our structs empty.
        { vector<Spec> tmp; tmp.swap(_specs); outSpecs.swap(tmp); }
        { vector<Field> tmp; tmp.swap(_fields); outFields.swap(tmp); }
        { vector<FieldIndex> tmp; tmp.swap(_fieldSets); outFieldSets.swap(tmp); }
    }

    inline TfToken const &
    GetToken(TokenIndex i) const { return _tokens[i.value]; }

    inline vector<TfToken> const &GetTokens() const { return _tokens; }
    
    inline std::string const &
    GetString(StringIndex i) const {
        return GetToken(_strings[i.value]).GetString();
    }
    inline vector<TokenIndex> const &GetStrings() const { return _strings; }

    vector<tuple<string, int64_t, int64_t>> GetSectionsNameStartSize() const;

    inline VtValue GetTimeSampleValue(TimeSamples const &ts, size_t i) const {
        return ts.IsInMemory() ? ts.values[i] : _GetTimeSampleValueImpl(ts, i);
    }

    // Make \p ts mutable in a way that can accommodate changing the size of ts.
    inline void MakeTimeSampleTimesAndValuesMutable(TimeSamples &ts) const {
        ts.times.MakeUnique();
        MakeTimeSampleValuesMutable(ts);
    }

    // Make \p ts mutable so that individual sample values may be modified, but
    // not the number of samples.
    inline void MakeTimeSampleValuesMutable(TimeSamples &ts) const {
        if (!ts.IsInMemory()) {
            _MakeTimeSampleValuesMutableImpl(ts);
        }
    }

    void DebugPrint() const;

    inline VtValue UnpackValue(ValueRep rep) const {
        VtValue ret;
        _UnpackValue(rep, &ret);
        return ret;
    }

private:
    explicit CrateFile(bool useMmap);
    CrateFile(string const &fileName,
              ArchConstFileMapping mapStart, int64_t fileSize);
    CrateFile(string const &fileName, _UniqueFILE inputFile, int64_t fileSize);

    CrateFile(CrateFile const &) = delete;
    CrateFile &operator=(CrateFile const &) = delete;

    static ArchConstFileMapping _MmapFile(char const *fileName, FILE *file);

    class _Writer;
    class _BufferedOutput;
    class _ReaderBase;
    template <class ByteStream> class _Reader;

    template <class ByteStream>
    _Reader<ByteStream> _MakeReader(ByteStream src) const;

    template <class Fn>
    void _WriteSection(
        _Writer &w, _SectionName name, _TableOfContents &toc, Fn writeFn) const;

    void _AddDeferredTimeSampledSpecs();

    bool _Write();

    void _AddSpec(const SdfPath &path, SdfSpecType type,
                  const std::vector<FieldValuePair> &fields);

    VtValue _GetTimeSampleValueImpl(TimeSamples const &ts, size_t i) const;
    void _MakeTimeSampleValuesMutableImpl(TimeSamples &ts) const;

    void _WritePaths(_Writer &w);

    template <class Iter>
    Iter _WritePathTree(_Writer &w, Iter cur, Iter end);
    
    inline void _WriteTokens(_Writer &w);

    template <class Reader>
    void _ReadStructuralSections(Reader src, int64_t fileSize);

    template <class ByteStream>
    static _BootStrap _ReadBootStrap(ByteStream src, int64_t fileSize);

    template <class Reader>
    _TableOfContents _ReadTOC(Reader src, _BootStrap const &b) const;

    template <class Reader> void _PrefetchStructuralSections(Reader src) const; 
    template <class Reader> void _ReadFieldSets(Reader src);
    template <class Reader> void _ReadFields(Reader src);
    template <class Reader> void _ReadSpecs(Reader src);
    template <class Reader> void _ReadStrings(Reader src);
    template <class Reader> void _ReadTokens(Reader src);
    template <class Reader> void _ReadPaths(Reader src);
    template <class Reader, class Header>
    void _ReadPathsRecursively(
        Reader src, const SdfPath &parentPath,
        const Header &h,
        WorkArenaDispatcher &dispatcher);

    void _ReadRawBytes(int64_t start, int64_t size, char *buf) const;

    PathIndex _AddPath(const SdfPath &path);
    FieldSetIndex _AddFieldSet(const std::vector<FieldIndex> &fieldIndexes);
    FieldIndex _AddField(const FieldValuePair &fv);
    TokenIndex _AddToken(const TfToken &token);
    TokenIndex _GetIndexForToken(const TfToken &token) const;
    StringIndex _AddString(const string &str);


    ////////////////////////////////////////////////////////////////////////

    // Base class, to have a pointer type.
    struct _ValueHandlerBase;
    template <class, class=void> struct _ScalarValueHandlerBase;
    template <class, class=void> struct _ArrayValueHandlerBase;
    template <class> struct _ValueHandler;

    friend struct _ValueHandlerBase;
    template <class, class> friend struct _ScalarValueHandlerBase;
    template <class, class> friend struct _ArrayValueHandlerBase;
    template <class> friend struct _ValueHandler;

    template <class T> inline _ValueHandler<T> &_GetValueHandler();
    template <class T> inline _ValueHandler<T> const &_GetValueHandler() const;

    template <class T> inline ValueRep _PackValue(T const &v);
    template <class T> inline ValueRep _PackValue(VtArray<T> const &v);
    ValueRep _PackValue(VtValue const &v);

    template <class T> void _UnpackValue(ValueRep rep, T *out) const;
    template <class T> void _UnpackValue(ValueRep rep, VtArray<T> *out) const;
    void _UnpackValue(ValueRep rep, VtValue *result) const;

    // Functions that populate the value read/write functions.
    template <class T> void _DoTypeRegistration();
    void _DoAllTypeRegistrations();
    void _DeleteValueHandlers();
    void _ClearValueHandlerDedupTables();

    static bool _IsKnownSection(char const *name);

    struct _PackingContext;

    ////////////////////////////////////////////////////////////////////////
    // Member data.

    ////////////////////////////////////////////////////////////////////////
    // These tables need not persist, they must be fully regenerated from
    // in-memory data upon Save, for example.

    // An index into the path list, plus a range of fields.
    vector<Spec> _specs;

    // Deferred specs with timeSamples that we write separately at the end,
    // time-by-time so that time-sampled data is collocated by time.
    struct _DeferredTimeSampledSpec {
        _DeferredTimeSampledSpec() = default;
        _DeferredTimeSampledSpec(PathIndex p, SdfSpecType t,
                                 vector<FieldIndex> &&of,
                                 vector<pair<TfToken, TimeSamples>> &&ts)
            : path(p)
            , specType(t)
            , ordinaryFields(std::move(of))
            , timeSampleFields(std::move(ts)) {}
        PathIndex path;
        SdfSpecType specType;
        vector<FieldIndex> ordinaryFields;
        vector<pair<TfToken, TimeSamples>> timeSampleFields;
    };
    vector<_DeferredTimeSampledSpec> _deferredTimeSampledSpecs;

    // All unique fields.
    vector<Field> _fields;

    // A vector of groups of fields, invalid-index terminated.
    vector<FieldIndex> _fieldSets;

    ////////////////////////////////////////////////////////////////////////
    // These tables must persist, since deferred values in the file may refer to
    // their contents by index.

    // All unique paths.
    vector<SdfPath> _paths;

    // TfToken to token index.
    vector<TfToken> _tokens;

    // std::string to string index.
    vector<TokenIndex> _strings;

    // ValueRep to vector<double> for deduplicating timesamples times in memory.
    mutable unordered_map<
        ValueRep, TimeSamples::SharedTimes, _Hasher> _sharedTimes;
    mutable tbb::spin_rw_mutex _sharedTimesMutex;

    // functions to write VtValues to file by type.
    boost::container::flat_map<
        std::type_index, std::function<ValueRep (VtValue const &)>>
        _packValueFunctions;

    // functions to read VtValues from file by type.
    std::function<void (ValueRep, VtValue *)>
    _unpackValueFunctionsPread[static_cast<int>(TypeEnum::NumTypes)];

    std::function<void (ValueRep, VtValue *)>
    _unpackValueFunctionsMmap[static_cast<int>(TypeEnum::NumTypes)];

    _ValueHandlerBase *_valueHandlers[static_cast<int>(TypeEnum::NumTypes)];

    TfType _typeEnumToTfType[static_cast<int>(TypeEnum::NumTypes)];
    TfType _typeEnumToTfTypeForArray[static_cast<int>(TypeEnum::NumTypes)];

    ////////////////////////////////////////////////////////////////////////

    // Temporary -- only valid during Save().
    std::unique_ptr<_PackingContext> _packCtx;

    _TableOfContents _toc; // only valid if we read a file.
    _BootStrap _boot; // only valid if we read a file.

    // We'll only have one of these, depending on whether we're doing mmap() or
    // pread().
    ArchConstFileMapping _mapStart; // NULL if this wasn't populated from file.
    _UniqueFILE _inputFile; // NULL if this wasn't populated from file.

    std::string _fileName; // Empty if this file data is in-memory only.

    std::unique_ptr<char []> _debugPageMap; // Debug page access map, see
                                            // USDC_DUMP_PAGE_MAPS.

    const bool _useMmap; // If true, use mmap for reads, otherwise use pread.
};

template <>
struct _IsBitwiseReadWrite<CrateFile::_BootStrap> : std::true_type {};

template <>
struct _IsBitwiseReadWrite<CrateFile::_Section> : std::true_type {};

template <>
struct _IsBitwiseReadWrite<CrateFile::Field> : std::true_type {};

template <>
struct _IsBitwiseReadWrite<CrateFile::Spec> : std::true_type {};

template <>
struct _IsBitwiseReadWrite<CrateFile::Spec_0_0_1> : std::true_type {};


} // Usd_CrateFile


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_CRATEFILE_H
