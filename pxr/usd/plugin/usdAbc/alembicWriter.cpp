//
// Copyright 2016-2017 Pixar
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
/// \file alembicWriter.cpp

#include "pxr/pxr.h"
#include "pxr/usd/usdAbc/alembicWriter.h"
#include "pxr/usd/usdAbc/alembicUtil.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/ostreamMethods.h"
#include <Alembic/Abc/OArchive.h>
#include <Alembic/Abc/OObject.h>
#include <Alembic/AbcGeom/OCamera.h>
#include <Alembic/AbcGeom/OCurves.h>
#include <Alembic/AbcGeom/OPoints.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OSubD.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/Visibility.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <set>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


// The name of this exporter, embedded in written Alembic files.
static const char* writerName = "UsdAbc_AlembicData";

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (transform)
    ((xformOpTransform, "xformOp:transform"))
);

namespace {

using namespace ::Alembic::AbcGeom;
using namespace UsdAbc_AlembicUtil;

// The SdfAbstractData time samples type.
// XXX: SdfAbstractData should typedef this.
typedef std::set<double> UsdAbc_TimeSamples;

struct _Subtract {
    double operator()(double x, double y) const { return x - y; }
};

static
GeometryScope
_GetGeometryScope(const TfToken& interpolation)
{
    static const TfToken constant("constant");
    static const TfToken uniform("uniform");
    static const TfToken varying("varying");
    static const TfToken vertex("vertex");
    static const TfToken faceVarying("faceVarying");
    if (interpolation.IsEmpty() || interpolation == constant) {
        return kConstantScope;
    }
    if (interpolation == uniform) {
        return kUniformScope;
    }
    if (interpolation == varying) {
        return kVaryingScope;
    }
    if (interpolation == vertex) {
        return kVertexScope;
    }
    if (interpolation == faceVarying) {
        return kFacevaryingScope;
    }
    return kUnknownScope;
}

//
// UsdSamples
//

/// \class UsdSamples
/// \brief Wraps time samples from a Usd property.
///
/// Wraps time samples or a default in a Usd property, providing a uniform
/// interface.
class UsdSamples {
public:
    UsdSamples(const SdfPath& primPath, const TfToken& propertyName);

    /// Construct from a property.  If the property has time samples use
    /// those, otherwise use the default as a single time sample at time
    /// zero.  If there's no default then return an empty set.
    ///
    /// This validates the samples to ensure they all have the same type.
    UsdSamples(const SdfPath& primPath,
               const TfToken& propertyName,
               const SdfAbstractData& data);

    /// Returns an id.
    SdfAbstractDataSpecId GetId() const;

    /// Returns \c true iff there are no samples.
    bool IsEmpty() const;

    /// Returns the number of samples.
    size_t GetNumSamples() const;

    /// Returns \c true iff the property is time sampled, \c false if
    /// the value was taken from the default or if there were no opinions.
    bool IsTimeSampled() const;

    /// Returns the type name of the samples.
    const SdfValueTypeName& GetTypeName() const;

    /// Returns a field on the property.
    VtValue GetField(const TfToken& name) const;

    /// Returns the sample closest to time \p time.
    const VtValue& Get(double time) const;

    /// Adds the set of all sample times to \p times.
    void AddTimes(UsdAbc_TimeSamples* times) const;

    /// Returns the sample map.
    const SdfTimeSampleMap& GetSamples() const;

    /// Sets the samples to \p samples.  The contents of \p samples is
    /// undefined after the call.
    void TakeSamples(SdfTimeSampleMap& samples);

private:
    bool _Validate();
    void _Clear();

private:
    SdfPath _path;
    TfToken _name;
    const SdfAbstractData* _data;
    boost::shared_ptr<VtValue> _value;
    boost::shared_ptr<SdfTimeSampleMap> _local;
    const SdfTimeSampleMap* _samples;
    bool _timeSampled;
    SdfValueTypeName _typeName;
};

UsdSamples::UsdSamples(const SdfPath& primPath, const TfToken& propertyName) :
    _path(primPath),
    _name(propertyName),
    _data(NULL)
{
    _Clear();
}

UsdSamples::UsdSamples(
    const SdfPath& primPath,
    const TfToken& propertyName,
    const SdfAbstractData& data) :
    _path(primPath),
    _name(propertyName),
    _data(&data)
{
    VtValue value;
    SdfAbstractDataSpecId id(&_path, &_name);
    if (data.Has(id, SdfFieldKeys->TimeSamples, &value)) {
        if (TF_VERIFY(value.IsHolding<SdfTimeSampleMap>())) {
            _value.reset(new VtValue);
            _value->Swap(value);
            _samples     = &_value->UncheckedGet<SdfTimeSampleMap>();
            _timeSampled = true;
        }
        else {
            _Clear();
            return;
        }
    }
    else if (data.Has(id, SdfFieldKeys->Default, &value)) {
        _local.reset(new SdfTimeSampleMap);
        (*_local)[0.0].Swap(value);
        _samples       = _local.get();
        _timeSampled   = false;
    }
    else {
        _Clear();
        return;
    }
    if (TF_VERIFY(data.Has(id, SdfFieldKeys->TypeName, &value),
                  "No type name on <%s>", id.GetFullSpecPath().GetText())) {
        if (TF_VERIFY(value.IsHolding<TfToken>())) {
            _typeName =
                SdfSchema::GetInstance().
                    FindType(value.UncheckedGet<TfToken>());
            _Validate();
        }
        else {
            _Clear();
        }
    }
    else {
        _Clear();
    }
}

bool
UsdSamples::_Validate()
{
    const TfType type = _typeName.GetType();
    const TfType backupType =
        (type == TfType::Find<float>()) ? TfType::Find<double>() : type;
restart:
    for (const auto& v : *_samples) {
        if (v.second.GetType() != type) {
            if (!TF_VERIFY(v.second.GetType() == backupType,
                              "Expected sample at <%s> time %f of type '%s', "
                              "got '%s'",
                              GetId().GetFullSpecPath().GetText(),
                              v.first, type.GetTypeName().c_str(),
                              v.second.GetType().GetTypeName().c_str())) {
                _Clear();
                return false;
            }
            else {
                // Make sure we have a local copy.
                if (!_local) {
                    _local.reset(new SdfTimeSampleMap(*_samples));
                    _samples = _local.get();
                    goto restart;
                }

                // Convert double to float.
                (*_local)[v.first] =
                    static_cast<float>(v.second.UncheckedGet<double>());
            }
        }
    }
    return true;
}

void
UsdSamples::_Clear()
{
    _value.reset();
    _local.reset(new SdfTimeSampleMap);
    _samples       = _local.get();
    _timeSampled   = false;
    _typeName      = SdfValueTypeName();
}

SdfAbstractDataSpecId
UsdSamples::GetId() const
{
    return SdfAbstractDataSpecId(&_path, &_name);
}

bool
UsdSamples::IsEmpty() const
{
    return _samples->empty();
}

size_t
UsdSamples::GetNumSamples() const
{
    return _samples->size();
}

bool
UsdSamples::IsTimeSampled() const
{
    return _timeSampled;
}

const SdfValueTypeName&
UsdSamples::GetTypeName() const
{
    return _typeName;
}

VtValue
UsdSamples::GetField(const TfToken& name) const
{
    SdfAbstractDataSpecId id(&_path, &_name);
    return _data->Get(id, name);
}

const VtValue&
UsdSamples::Get(double time) const
{
    if (IsEmpty()) {
        static const VtValue empty;
        return empty;
    }
    else {
        SdfTimeSampleMap::const_iterator i = _samples->lower_bound(time);
        return i == _samples->end() ? _samples->rbegin()->second : i->second;
    }
}

void
UsdSamples::AddTimes(UsdAbc_TimeSamples* times) const
{
    for (const auto& v : *_samples) {
        times->insert(v.first);
    }
}

const SdfTimeSampleMap&
UsdSamples::GetSamples() const
{
    return *_samples;
}

void
UsdSamples::TakeSamples(SdfTimeSampleMap& samples)
{
    if (!_local) {
        _value.reset();
        _local.reset(new SdfTimeSampleMap);
    }
    _local.get()->swap(samples);
    _samples = _local.get();
    _Validate();
}

//
// _Parent
//

/// \class _Parent
/// \brief Encapsulates an Alembic parent object
///
/// This mainly exists to extract certain properties from objects that
/// have them.  The Alembic type hierarchy and templating prevents us
/// from dynamic casting to a type that can provide these properties
/// (there isn't a single type to cast to, instead there's a templated
/// class and the template argument depends on the actual type of the
/// object).  This object holds enough type information to get what we
/// want.
class _Parent {
public:
    /// Construct invalid parent.
    _Parent() : _object(new _Prim(shared_ptr<OObject>(new OObject))) { }

    /// Construct from an Alembic shared pointer to an OObject subclass.
    template <class T>
    _Parent(const shared_ptr<T>& prim) :
        _object(new _Prim(static_pointer_cast<OObject>(prim))) { }

    /// Construct from an Alembic shared pointer to a supported
    /// schema based OSchemaObject.
    _Parent(const shared_ptr<OCamera>& prim):
        _object(new _GeomPrim<OCamera>(prim)) { }
    _Parent(const shared_ptr<OCurves>& prim):
        _object(new _GeomPrim<OCurves>(prim)) { }
    _Parent(const shared_ptr<OPoints>& prim):
        _object(new _GeomPrim<OPoints>(prim)) { }
    _Parent(const shared_ptr<OPolyMesh>& prim):
        _object(new _GeomPrim<OPolyMesh>(prim)) { }
    _Parent(const shared_ptr<OSubD>& prim):
        _object(new _GeomPrim<OSubD>(prim)) { }
    _Parent(const shared_ptr<OXform>& prim):
        _object(new _GeomPrim<OXform>(prim)) { }

    /// Returns the OObject.
    operator OObject&() const;

    /// Returns the OCompoundProperty holding the object's properties.
    OCompoundProperty GetProperties() const;

    /// Returns the OCompoundProperty holding the object's schema.
    OCompoundProperty GetSchema() const;

    /// Returns the OCompoundProperty holding the ".arbGeomParams" property.
    /// This returns an invalid property if the object isn't geometric.
    OCompoundProperty GetArbGeomParams() const;

    /// Returns the OCompoundProperty holding the ".userProperties" property.
    /// This returns an invalid property if the object isn't geometric.
    OCompoundProperty GetUserProperties() const;

private:
    class _Prim {
    public:
        explicit _Prim(const shared_ptr<OObject>& object) : _object(object) { }
        virtual ~_Prim();
        const shared_ptr<OObject>& GetObjectPtr() const { return _object; }
        virtual OCompoundProperty GetSchema() const;
        virtual OCompoundProperty GetArbGeomParams() const;
        virtual OCompoundProperty GetUserProperties() const;

    private:
        shared_ptr<OObject> _object;
    };

    template <class T>
    class _GeomPrim : public _Prim {
    public:
        explicit _GeomPrim(const shared_ptr<T>& object) : _Prim(object) { }
        virtual ~_GeomPrim() { }
        virtual OCompoundProperty GetSchema() const;
        virtual OCompoundProperty GetArbGeomParams() const;
        virtual OCompoundProperty GetUserProperties() const;
    };

private:
    shared_ptr<_Prim> _object;
};

_Parent::_Prim::~_Prim()
{
    // Do nothing
}

OCompoundProperty
_Parent::_Prim::GetSchema() const
{
    return OCompoundProperty();
}

OCompoundProperty
_Parent::_Prim::GetArbGeomParams() const
{
    return OCompoundProperty();
}

OCompoundProperty
_Parent::_Prim::GetUserProperties() const
{
    return OCompoundProperty();
}

template <class T>
OCompoundProperty
_Parent::_GeomPrim<T>::GetSchema() const
{
    return static_pointer_cast<T>(GetObjectPtr())->getSchema();
}

template <class T>
OCompoundProperty
_Parent::_GeomPrim<T>::GetArbGeomParams() const
{
    return
        static_pointer_cast<T>(GetObjectPtr())->getSchema().getArbGeomParams();
}

template <class T>
OCompoundProperty
_Parent::_GeomPrim<T>::GetUserProperties() const
{
    return
        static_pointer_cast<T>(GetObjectPtr())->getSchema().getUserProperties();
}

_Parent::operator OObject&() const
{
    return *_object->GetObjectPtr();
}

OCompoundProperty
_Parent::GetProperties() const
{
    return _object->GetObjectPtr()->getProperties();
}

OCompoundProperty
_Parent::GetSchema() const
{
    return _object->GetSchema();
}

OCompoundProperty
_Parent::GetArbGeomParams() const
{
    return _object->GetArbGeomParams();
}

OCompoundProperty
_Parent::GetUserProperties() const
{
    return _object->GetUserProperties();
}

//
// _WriterSchema
//

class _PrimWriterContext;

/// \class _WriterSchema
/// \brief The Alembic to Usd schema.
///
/// This class stores functions to write a Usd prim to Alembic keyed by
/// type.  Each type can have multiple writers, each affected by the
/// previous via a \c _PrimWriterContext.
class _WriterSchema {
public:
    typedef boost::function<void (_PrimWriterContext*)> PrimWriter;
    typedef std::vector<PrimWriter> PrimWriterVector;
    typedef UsdAbc_AlembicDataConversion::FromUsdConverter Converter;

    _WriterSchema();

    /// Returns the prim writers for the given type.  Returns an empty
    /// vector if the type isn't known.
    const PrimWriterVector& GetPrimWriters(const TfToken&) const;

    // Helper for defining types.
    class TypeRef {
    public:
        TypeRef(PrimWriterVector* writers) : _writers(writers) { }

        TypeRef& AppendWriter(const PrimWriter& writer)
        {
            _writers->push_back(writer);
            return *this;
        }

    private:
        PrimWriterVector* _writers;
    };

    /// Adds a type and returns a helper for defining it.
    template <class T>
    TypeRef AddType(T name)
    {
        return TypeRef(&_writers[TfToken(name)]);
    }

    /// Adds the fallback type and returns a helper for defining it.
    TypeRef AddFallbackType()
    {
        return AddType(TfToken());
    }

    /// Returns \c true iff the samples are valid.
    bool IsValid(const UsdSamples&) const;

    /// Returns \c true iff the samples are a shaped type.
    bool IsShaped(const UsdSamples&) const;

    /// Returns the Alembic DataType suitable for the values in \p samples.
    DataType GetDataType(const UsdSamples& samples) const;

    /// Returns the (default) conversion for the Alembic property type with
    /// name \p typeName.
    SdfValueTypeName FindConverter(const UsdAbc_AlembicType& typeName) const;

    /// Returns the (default) conversion for the Usd property type with
    /// name \p typeName.
    UsdAbc_AlembicType FindConverter(const SdfValueTypeName& typeName) const;

    /// Returns the conversion function for the given conversion.
    const Converter& GetConverter(const SdfValueTypeName& typeName) const;

private:
    const UsdAbc_AlembicConversions _conversions;

    typedef std::map<TfToken, PrimWriterVector> _WriterMap;
    _WriterMap _writers;
};

_WriterSchema::_WriterSchema()
{
    // Do nothing
}

const _WriterSchema::PrimWriterVector&
_WriterSchema::GetPrimWriters(const TfToken& name) const
{
    _WriterMap::const_iterator i = _writers.find(name);
    if (i != _writers.end()) {
        return i->second;
    }
    i = _writers.find(TfToken());
    if (i != _writers.end()) {
        return i->second;
    }
    static const PrimWriterVector empty;
    return empty;
}

bool
_WriterSchema::IsValid(const UsdSamples& samples) const
{
    return GetConverter(samples.GetTypeName());
}

bool
_WriterSchema::IsShaped(const UsdSamples& samples) const
{
    return samples.GetTypeName().IsArray();
}

DataType
_WriterSchema::GetDataType(const UsdSamples& samples) const
{
    return FindConverter(samples.GetTypeName()).GetDataType();
}

SdfValueTypeName
_WriterSchema::FindConverter(const UsdAbc_AlembicType& typeName) const
{
    return _conversions.data.FindConverter(typeName);
}

UsdAbc_AlembicType
_WriterSchema::FindConverter(const SdfValueTypeName& typeName) const
{
    return _conversions.data.FindConverter(typeName);
}

const _WriterSchema::Converter&
_WriterSchema::GetConverter(const SdfValueTypeName& typeName) const
{
    return _conversions.data.GetConverter(typeName);
}

/// \class _WriterContext
/// \brief The Alembic to Usd writer context.
///
/// This object holds information used by the writer for a given archive
/// and Usd data.
class _WriterContext {
public:
    _WriterContext();

    /// Returns the archive.
    void SetArchive(const OArchive& archive);

    /// Returns the archive.
    const OArchive& GetArchive() const { return _archive; }

    /// Returns the archive.
    OArchive& GetArchive() { return _archive; }

    /// Sets the writer schema.
    void SetSchema(const _WriterSchema* schema) { _schema = schema; }

    /// Returns the writer schema.
    const _WriterSchema& GetSchema() const { return *_schema; }

    /// Returns the Usd data.
    void SetData(const SdfAbstractDataConstPtr& data) { _data = data; }

    /// Returns the Usd data.
    const SdfAbstractData& GetData() const { return *boost::get_pointer(_data);}

    /// Sets or resets the flag named \p flagName.
    void SetFlag(const TfToken& flagName, bool set);

    /// Returns \c true iff a flag is in the set.
    bool IsFlagSet(const TfToken& flagName) const;

    /// Adds/returns a time sampling.
    uint32_t AddTimeSampling(const UsdAbc_TimeSamples&);

private:
    // Conversion options.
    double _timeScale;              // Scale Alembic time by this factor.
    double _timeOffset;             // Offset Alembic->Usd time (after scale).
    std::set<TfToken, TfTokenFastArbitraryLessThan> _flags;

    // Output state.
    OArchive _archive;
    const _WriterSchema* _schema;
    SdfAbstractDataConstPtr _data;

    // The set of time samplings we've created.  We tend to reuse the same
    // samplings a lot so caching these avoids reanalyzing the samples to
    // determine their kind.  This also avoids having Alembic check for
    // duplicate time samplings.  This maps a set of samples to the index
    // of the time sampling in the archive.  Index 0 is reserved by
    // Alembic to mean uniform starting at 0, cycling at 1.
    std::map<UsdAbc_TimeSamples, uint32_t> _timeSamplings;
};

_WriterContext::_WriterContext() :
    _timeScale(24.0),               // Usd is frames, Alembic is seconds.
    _timeOffset(0.0),               // Time 0.0 to frame 0.
    _schema(NULL)
{
    // Do nothing
}

void
_WriterContext::SetArchive(const OArchive& archive)
{
    _archive = archive;
    _timeSamplings.clear();
}

void
_WriterContext::SetFlag(const TfToken& flagName, bool set)
{
    if (set) {
        _flags.insert(flagName);
    }
    else {
        _flags.erase(flagName);
    }
}

bool
_WriterContext::IsFlagSet(const TfToken& flagName) const
{
    return _flags.count(flagName);
}

uint32_t
_WriterContext::AddTimeSampling(const UsdAbc_TimeSamples& inSamples)
{
    // Handle empty case.
    if (inSamples.empty()) {
        // No samples -> identity time sampling.
        return 0;
    }

    // Get the cached index.  If not zero then we already have this one.
    std::map<UsdAbc_TimeSamples, uint32_t>::const_iterator tsi =
        _timeSamplings.find(inSamples);
    if (tsi != _timeSamplings.end()) {
        return tsi->second;
    }
    uint32_t& index = _timeSamplings[inSamples];

    // Scale and offset samples.
    UsdAbc_TimeSamples samples;
    for (double time : inSamples) {
        samples.insert((time - _timeOffset) / _timeScale);
    }

    // Handy iterators.  i refers to the first element and n to end.
    // Initially j refers to the second element.
    UsdAbc_TimeSamples::const_iterator j = samples.begin();
    const UsdAbc_TimeSamples::const_iterator i = j++;
    const UsdAbc_TimeSamples::const_iterator n = samples.end();

    // Handle other special cases.  Note that we store the index returned
    // by Alembic.  Alembic will compare our TimeSampling() against all of
    // the ones it's seen so far and return the index of the existing one,
    // if any.  So we can map multiple time samples sets to the same
    // time sampling index.
    if (samples.size() == 1) {
        // One sample -> uniform starting at the sample, arbitrary cycle time.
        return index = _archive.addTimeSampling(TimeSampling(1, *i));
    }
    if (samples.size() == 2) {
        // Two samples -> uniform.  Cyclic and acyclic would also work but
        // uniform is probably more likely to match an existing sampling.
        return index =
            _archive.addTimeSampling(TimeSampling(*j - *i, *i));
    }

    // Figure out if the samples are uniform (T0 + N * dT for integer N),
    // cyclic (Ti + N * dT for integer N and samples i=[0,M]), or acyclic
    // (Ti for samples i=[0,M]).  In all cases, the total number of samples
    // is an independent variable and doesn't affect the choice.  That is,
    // 1 1/2 cycles is cyclic even though we don't complete a cycle.  Note,
    // however, that any acyclic sequence is also a cyclic sequence of one
    // less sample (which we'll call the trivial cyclic sequence);  we
    // choose the acyclic type.
    //
    // First find the deltas between samples.
    std::vector<double> dt;
    std::transform(j, n, i, std::back_inserter(dt), _Subtract());

    // Scan forward in dt for element M that matches dt[0] then check that
    // dt[0..M-1] == dt[M..2M-1).  If that checks out then check that the
    // cycle continues until we run out of samples.  Otherwise repeat from
    // M + 1 until we find a cycle or run out of samples.  Don't check
    // dt.back() to avoid trivial cyclic.  We adjust j to point to the
    // time sample at the start of the delta dt[k].
    size_t k = 1;
    const size_t m = dt.size();
    TimeSamplingType timeSamplingType(TimeSamplingType::kAcyclic);
    for (; k != m - 1; ++j, ++k) {
        // Check for a cycle by comparing s[i] == s[i + k] for i in
        // [0..N-k-1] where N = len(s) and k is the cycle length.
        if (std::equal(dt.begin() + k, dt.begin() + m, dt.begin())) {
            // Cyclic or uniform (which is cyclic with samps/cycle == 1).
            timeSamplingType = TimeSamplingType(k, *j - *i);
            break;
        }
    }

    // If we're still acyclic then use every sample.
    if (timeSamplingType.isAcyclic()) {
        j = n;
    }

    // Cyclic or acyclic.
    return index =
        _archive.addTimeSampling(
            TimeSampling(timeSamplingType, std::vector<chrono_t>(i, j)));
}

/// \class _PrimWriterContext
/// \brief The Alembic to Usd prim writer context.
///
/// This object holds information used by the writer for a given prim.
/// Each prim writer can modify the context to change the behavior of
/// later writers for that prim.
class _PrimWriterContext {
public:
    typedef _Parent Parent;

    _PrimWriterContext(_WriterContext&,
                       const Parent& parent,
                       const SdfAbstractDataSpecId& id);

    /// Return the path to this prim.
    const SdfPath& GetPath() const;

    /// Returns the Usd field from the prim.
    VtValue GetField(const TfToken& fieldName) const;

    /// Returns the Usd field from the named property.
    VtValue GetPropertyField(const TfToken& propertyName,
                             const TfToken& fieldName) const;

    /// Returns the archive.
    const OArchive& GetArchive() const;

    /// Returns the archive.
    OArchive& GetArchive();

    /// Returns the writer schema.
    const _WriterSchema& GetSchema() const;

    /// Returns the abstract data.
    const SdfAbstractData& GetData() const;

    /// Returns the spec type for the named property.
    SdfSpecType GetSpecType(const TfToken& propertyName) const;

    /// Tests a flag.
    bool IsFlagSet(const TfToken& flagName) const;

    /// Adds/returns a time sampling.
    uint32_t AddTimeSampling(const UsdAbc_TimeSamples&);

    /// Returns the parent object.
    const Parent& GetParent() const;

    /// Sets the parent object.
    void SetParent(const Parent& parent);

    /// Causes \c GetAlembicPrimName() to have the suffix appended.
    void PushSuffix(const std::string& suffix);

    /// Returns a prim name that is valid Alembic, isn't in use, and has
    /// all suffixes appended.
    std::string GetAlembicPrimName() const;

    /// Returns an Alembic name for \p name that is valid in Alembic and
    /// isn't in use.
    std::string GetAlembicPropertyName(const TfToken& name) const;

    /// Sets the union of extracted sample times to \p timeSamples.
    void SetSampleTimesUnion(const UsdAbc_TimeSamples& timeSamples);

    /// Returns the union of extracted sample times.
    const UsdAbc_TimeSamples& GetSampleTimesUnion() const;

    /// Returns the samples for a Usd property.  If the property doesn't
    /// exist or has already been extracted then this returns an empty
    /// samples object.  The property is extracted from the context so
    /// it cannot be extracted again.  The sample times union is updated
    /// to include the sample times from the returned object.
    UsdSamples ExtractSamples(const TfToken& name)
    {
        UsdSamples result = _ExtractSamples(name);
        result.AddTimes(&_sampleTimes);
        return result;
    }

    /// Returns the samples for a Usd property.  If the property doesn't
    /// exist or has already been extracted then this returns an empty
    /// samples object.  The property is extracted from the context so
    /// it cannot be extracted again.  The sample times union is updated
    /// to include the sample times from the returned object.  This
    /// verifies that the property is holding a value of the given type;
    /// if not it returns an empty samples object.
    UsdSamples ExtractSamples(const TfToken& name, const SdfValueTypeName& type)
    {
        UsdSamples result = _ExtractSamples(name);
        if (!result.IsEmpty() && type != result.GetTypeName()) {
            TF_WARN("Expected property '%s' to have type '%s', got '%s'",
                    GetPath().AppendProperty(name).GetText(),
                    type.GetAsToken().GetText(),
                    result.GetTypeName().GetAsToken().GetText());
            return UsdSamples(GetPath(), name);
        }
        result.AddTimes(&_sampleTimes);
        return result;
    }

    /// Returns the names of properties that have not been extracted yet
    /// in Usd property order.
    TfTokenVector GetUnextractedNames() const;

private:
    UsdSamples _ExtractSamples(const TfToken& name);

private:
    typedef std::vector<SdfValueTypeName> SdfValueTypeNameVector;

    _WriterContext& _context;
    Parent _parent;
    const SdfAbstractDataSpecId& _id;
    std::string _suffix;
    UsdAbc_TimeSamples _sampleTimes;
    TfTokenVector _unextracted;
};

_PrimWriterContext::_PrimWriterContext(
    _WriterContext& context,
    const Parent& parent,
    const SdfAbstractDataSpecId& id) :
    _context(context),
    _parent(parent),
    _id(id)
{
    // Fill _unextracted with all of the property names.
    VtValue tmp;
    if (_context.GetData().Has(id, SdfChildrenKeys->PropertyChildren, &tmp)) {
        if (tmp.IsHolding<TfTokenVector>()) {
            _unextracted = tmp.UncheckedGet<TfTokenVector>();
        }
    }
}

const SdfPath&
_PrimWriterContext::GetPath() const
{
    return _id.GetPropertyOwningSpecPath();
}

VtValue
_PrimWriterContext::GetField(const TfToken& fieldName) const
{
    return _context.GetData().Get(_id, fieldName);
}

VtValue
_PrimWriterContext::GetPropertyField(
    const TfToken& propertyName,
    const TfToken& fieldName) const
{
    const SdfAbstractDataSpecId propId(&_id.GetPropertyOwningSpecPath(),
                                       &propertyName);
    return _context.GetData().Get(propId, fieldName);
}

const OArchive&
_PrimWriterContext::GetArchive() const
{
    return _context.GetArchive();
}

OArchive&
_PrimWriterContext::GetArchive()
{
    return _context.GetArchive();
}

const _WriterSchema&
_PrimWriterContext::GetSchema() const
{
    return _context.GetSchema();
}

const SdfAbstractData&
_PrimWriterContext::GetData() const
{
    return _context.GetData();
}

SdfSpecType
_PrimWriterContext::GetSpecType(const TfToken& propertyName) const
{
    const SdfAbstractDataSpecId propId(&_id.GetPropertyOwningSpecPath(),
                                       &propertyName);
    return _context.GetData().GetSpecType(propId);
}

bool
_PrimWriterContext::IsFlagSet(const TfToken& flagName) const
{
    return _context.IsFlagSet(flagName);
}

uint32_t
_PrimWriterContext::AddTimeSampling(const UsdAbc_TimeSamples& samples)
{
    return _context.AddTimeSampling(samples);
}

const _PrimWriterContext::Parent&
_PrimWriterContext::GetParent() const
{
    return _parent;
}

void
_PrimWriterContext::SetParent(const Parent& parent)
{
    _parent = parent;
}

void
_PrimWriterContext::PushSuffix(const std::string& suffix)
{
    _suffix += suffix;
}

std::string
_PrimWriterContext::GetAlembicPrimName() const
{
    // Valid Alembic prim name set is a superset of valid Usd prim names.
    // XXX: Should verify this name is not in use, however we know
    //      we're not given how we use it (we only add a suffix to
    //      an only child).
    return _id.GetPropertyOwningSpecPath().GetName() + _suffix;
}

std::string
_PrimWriterContext::GetAlembicPropertyName(const TfToken& name) const
{
    // Valid Alembic property name set is a superset of valid Usd property
    // names.  Alembic accepts the Usd namespace delimiter as-is.
    return name.GetString();
}

void
_PrimWriterContext::SetSampleTimesUnion(const UsdAbc_TimeSamples& samples)
{   
    _sampleTimes = samples;
}

const UsdAbc_TimeSamples&
_PrimWriterContext::GetSampleTimesUnion() const
{
    return _sampleTimes;
}

UsdSamples
_PrimWriterContext::_ExtractSamples(const TfToken& name)
{
    TfTokenVector::iterator i =
        std::find(_unextracted.begin(), _unextracted.end(), name);
    if (i != _unextracted.end()) {
        _unextracted.erase(i);
        return UsdSamples(GetPath(), name, GetData());
    }
    return UsdSamples(GetPath(), name);
}

TfTokenVector
_PrimWriterContext::GetUnextractedNames() const
{
    return _unextracted;
}

// ----------------------------------------------------------------------------

//
// Utilities
//

/// Returns the Alembic metadata name for a Usd metadata field name.
static
std::string
_AmdName(const std::string& name)
{
    return "Usd:" + name;
}

static
bool
_IsOver(const _PrimWriterContext& context)
{
    if (context.GetField(SdfFieldKeys->TypeName).IsEmpty()) {
        return true;
    }
    const VtValue value = context.GetField(SdfFieldKeys->Specifier);
    return ! value.IsHolding<SdfSpecifier>() ||
               value.UncheckedGet<SdfSpecifier>() == SdfSpecifierOver;
}

// Reverse the order of the subsequences in \p valuesMap where the subsequence
// lengths are given by \p counts.
template <class T>
static
void
_ReverseWindingOrder(UsdSamples* valuesMap, const UsdSamples& countsMap)
{
    typedef VtArray<T> ValueArray;
    typedef VtArray<int> CountArray;

    SdfTimeSampleMap result;
    for (const auto& v : valuesMap->GetSamples()) {
        const VtValue& valuesValue = v.second;
        const VtValue& countsValue = countsMap.Get(v.first);
        if (! TF_VERIFY(valuesValue.IsHolding<ValueArray>())) {
            continue;
        }
        if (! TF_VERIFY(countsValue.IsHolding<CountArray>())) {
            continue;
        }
        ValueArray values        = valuesValue.UncheckedGet<ValueArray>();
        const CountArray& counts = countsValue.UncheckedGet<CountArray>();
        if (! UsdAbc_ReverseOrderImpl(values, counts)) {
            continue;
        }
        result[v.first].Swap(values);
    }
    valuesMap->TakeSamples(result);
}

// Adjust faceVertexIndices for winding order if orientation is right-handed.
static
void
_ReverseWindingOrder(
    const _PrimWriterContext* context,
    UsdSamples* faceVertexIndices,
    const UsdSamples& faceVertexCounts)
{
    // Get property value.  We must reverse the winding order if it's
    // right-handed in Usd.  (Alembic is always left-handed.)
    // XXX: Should probably check time samples, too, but we expect
    //      this to be uniform.
    const VtValue value =
        context->GetPropertyField(UsdGeomTokens->orientation,
                                  SdfFieldKeys->Default);
    if (! value.IsHolding<TfToken>() ||
            value.UncheckedGet<TfToken>() != UsdGeomTokens->leftHanded) {
        _ReverseWindingOrder<int>(faceVertexIndices, faceVertexCounts);
    }
}

static
std::string
_GetInterpretation(const SdfValueTypeName& typeName)
{
    const TfToken& roleName = typeName.GetRole();
    if (roleName == SdfValueRoleNames->Point) {
        return "point";
    }
    if (roleName == SdfValueRoleNames->Normal) {
        return "normal";
    }
    if (roleName == SdfValueRoleNames->Vector) {
        return "vector";
    }
    if (roleName == SdfValueRoleNames->Color) {
        if (typeName == SdfValueTypeNames->Float4 ||
            typeName == SdfValueTypeNames->Double4) {
            return "rgba";
        }
        return "rgb";
    }
    if (roleName == SdfValueRoleNames->Transform) {
        return "matrix";
    }
    if (typeName == SdfValueTypeNames->Quatd ||
        typeName == SdfValueTypeNames->Quatf) {
        return "quat";
    }
    return std::string();
}

static
std::string
_Stringify(const DataType& type)
{
    if (type.getExtent() > 1) {
        return TfStringPrintf("%s[%d]",PODName(type.getPod()),type.getExtent());
    }
    else {
        return PODName(type.getPod());
    }
}

// Make a sample, converting the Usd value to the given Alembic data type.
// If the given conversion doesn't perform that conversion then fail.
//
// Note:  This is the one and only place we invoke a converter to create a
// _SampleForAlembic.
static _SampleForAlembic
_MakeSample(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    const SdfValueTypeName& usdType,
    const VtValue& usdValue,
    const DataType& expectedAlembicType,
    bool skipAlembicTypeCheck = false)
{
    TRACE_SCOPE("UsdAbc_AlembicDataWriter:_MakeSample");

    // Done if there is no value.
    if (usdValue.IsEmpty()) {
        return _SampleForAlembic();
    }

    // This catches when the programmer fails to supply a conversion function
    // for the conversion.
    if (! converter) {
        return _ErrorSampleForAlembic(TfStringPrintf(
                    "No conversion for '%s'",
                    usdType.GetAsToken().GetText()));
    }

    // This catches when the programmer supplies a conversion that doesn't
    // yield the correct Alembic type.  This should never happen.
    if (! skipAlembicTypeCheck) {
        const UsdAbc_AlembicType actualAlembicType = schema.FindConverter(usdType);
        if (actualAlembicType.GetDataType() != expectedAlembicType) {
            return _ErrorSampleForAlembic(TfStringPrintf(
                        "Internal error: trying to convert '%s' to '%s'",
                        usdType.GetAsToken().GetText(),
                        _Stringify(expectedAlembicType).c_str()));
        }
    }

    // This catches when the Usd property doesn't have the expected type.
    // This should never happen because we check properties for the
    // expected type when we extract samples from the _PrimWriterContext,
    // or we choose our conversion based on the type of the samples.
    const SdfValueTypeName actualUsdType =
        SdfSchema::GetInstance().FindType(usdValue);
    if (actualUsdType != usdType) {
        // Handle role types.  These have a different name but the same
        // value type.
        if (usdType.GetType() != actualUsdType.GetType()) {
            return _ErrorSampleForAlembic(TfStringPrintf(
                        "Internal error: Trying to use conversion for '%s' to "
                        "convert from '%s'",
                        usdType.GetAsToken().GetText(),
                        actualUsdType.GetAsToken().GetText()));
        }
    }

    // Convert.
    _SampleForAlembic result(converter(usdValue));

    // Check extent.
    if (expectedAlembicType.getExtent() != 1) {
        if (result.GetCount() % expectedAlembicType.getExtent() != 0) {
            return _ErrorSampleForAlembic(TfStringPrintf(
                        "Internal error: didn't get a multiple of the extent "
                        "(%zd %% %d = %zd)",
                        result.GetCount(), expectedAlembicType.getExtent(),
                        result.GetCount() % expectedAlembicType.getExtent()));
        }
    }

    return result;
}

// Helper to handle enum types in the following _MakeSample() function.
template <typename T, typename Enable = void>
struct PodEnum {
    static const PlainOldDataType pod_enum = PODTraitsFromType<T>::pod_enum;
};
template <typename T>
struct PodEnum<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    static const PlainOldDataType pod_enum = kUint8POD;
};

// Make a sample, converting the Usd value to the given data type, T.
// If the given conversion doesn't perform that conversion then fail.
template <class T>
static _SampleForAlembic
_MakeSample(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    const SdfValueTypeName& usdType,
    const VtValue& usdValue,
    bool skipAlembicTypeCheck = false)
{
    return _MakeSample(schema, converter, usdType, usdValue,
                       DataType(PodEnum<T>::pod_enum),
                       skipAlembicTypeCheck);
}

template <class T, int N>
static _SampleForAlembic
_MakeSample(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    const SdfValueTypeName& usdType,
    const VtValue& usdValue,
    bool skipAlembicTypeCheck = false)
{
    return _MakeSample(schema, converter, usdType, usdValue,
                       DataType(PODTraitsFromType<T>::pod_enum, N),
                       skipAlembicTypeCheck);
}

static bool
_CheckSample(
    const _SampleForAlembic& sample,
    const UsdSamples& samples,
    const SdfValueTypeName& usdType)
{
    std::string message;
    if (sample.IsError(&message)) {
        TF_WARN("Can't convert from '%s' on <%s>: %s",
                usdType.GetAsToken().GetText(),
                samples.GetId().GetFullSpecPath().GetText(),
                message.c_str());
        return false;
    }
    return sample;
}

// An object we can use for mapping in _MakeIndexed.  It holds a pointer to
// its data.  It's not local to _MakeIndexed because we use it as a template
// argument.
template<class POD, size_t extent>
struct _MakeIndexedValue {
    const POD* value;
    _MakeIndexedValue(const POD* value_) : value(value_) { }

    // Copy value to \p dst.
    void CopyTo(POD* dst) const
    {
        std::copy(value, value + extent, dst);
    }

    // Compare for equality.
    bool operator==(const _MakeIndexedValue<POD, extent>& rhs) const
    {
        return std::equal(value, value + extent, rhs.value);
    }

    // Compare for less-than.
    bool operator<(const _MakeIndexedValue<POD, extent>& rhs) const
    {
        for (size_t i = 0; i != extent; ++i) {
            if (value[i] < rhs.value[i]) {
                return true;
            }
            if (rhs.value[i] < value[i]) {
                return false;
            }
        }
        return false;
    }
};

/// Make the values indexed.  This stores only unique values and makes
/// an index vector with an element for each original value indexing the
/// unique value.
template<class POD, size_t extent>
static
void
_MakeIndexed(_SampleForAlembic* values)
{
    // A map of values to an index.
    typedef _MakeIndexedValue<POD, extent> Value;
    typedef std::map<Value, uint32_t> IndexMap;
    typedef std::pair<typename IndexMap::iterator, bool> IndexMapResult;
    typedef _SampleForAlembic::IndexArray IndexArray;
    typedef IndexArray::value_type Index;

    // Allocate a vector of indices with the right size.
    const size_t n = values->GetCount() / extent;
    _SampleForAlembic::IndexArrayPtr indicesPtr(new IndexArray(n, 0));
    IndexArray& indices = *indicesPtr;

    // Find unique values.
    Index index = 0;
    IndexMap indexMap;
    std::vector<Value> unique;
    const POD* ptr = values->GetDataAs<POD>();
    for (size_t i = 0; i != n; ptr += extent, ++i) {
        const IndexMapResult result =
            indexMap.insert(std::make_pair(Value(ptr), index));
        if (result.second) {
            // Found a unique value.
            unique.push_back(result.first->first);
            ++index;
        }
        indices[i] = result.first->second;
    }

    // If there are enough duplicates use indexing otherwise don't.
    if (n * sizeof(POD[extent]) <=
            unique.size() * sizeof(POD[extent]) + n * sizeof(uint32_t)) {
        return;
    }

    // Build the result.
    const size_t numPODs = extent * unique.size();
    boost::shared_array<POD> uniqueBuffer(new POD[numPODs]);
    for (size_t i = 0, n = unique.size(); i != n; ++i) {
        unique[i].CopyTo(uniqueBuffer.get() + i * extent);
    }

    // Create a new sample object with the indexes.
    _SampleForAlembic result(uniqueBuffer, numPODs);
    result.SetIndices(indicesPtr);

    // Cut over.
    *values = result;
}

//
// Data copy/conversion functions.
//

// Copy to a T.
template <class DST, class R, class T>
void
_Copy(
    const _WriterSchema& schema,
    double time,
    const UsdSamples& samples,
    DST* dst,
    R (DST::*method)(T))
{
    typedef typename boost::remove_const<
                typename boost::remove_reference<T>::type
            >::type SampleValueType;

    const SdfValueTypeName& usdType = samples.GetTypeName();
    const _WriterSchema::Converter& converter = schema.GetConverter(usdType);

    // Make a sample holding a value of type SampleValueType.
    _SampleForAlembic sample =
        _MakeSample<SampleValueType>(schema, converter,
                                     usdType, samples.Get(time));
    if (! _CheckSample(sample, samples, usdType)) {
        return;
    }

    // Write to dst.
    (dst->*method)(*sample.GetDataAs<SampleValueType>());
}

// Copy to a T with explicit converter.
template <class DST, class R, class T>
void
_Copy(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    double time,
    const UsdSamples& samples,
    DST* dst,
    R (DST::*method)(T))
{
    typedef typename boost::remove_const<
                typename boost::remove_reference<T>::type
            >::type SampleValueType;

    const SdfValueTypeName& usdType = samples.GetTypeName();

    // Make a sample holding a value of type SampleValueType.
    static const bool skipAlembicTypeCheck = true;
    _SampleForAlembic sample =
        _MakeSample<SampleValueType>(schema, converter,
                                     usdType, samples.Get(time),
                                     skipAlembicTypeCheck);
    if (! _CheckSample(sample, samples, usdType)) {
        return;
    }

    // Write to dst.
    (dst->*method)(*sample.GetDataAs<SampleValueType>());
}

// Copy to a TypedArraySample<T>.  The client *must* hold the returned
// _SampleForAlembic until the sample is finally consumed.
template <class DST, class R, class T>
_SampleForAlembic
_Copy(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    double time,
    const UsdSamples& samples,
    DST* dst,
    R (DST::*method)(const TypedArraySample<T>&),
    bool skipAlembicTypeCheck = true)
{
    typedef T SampleValueTraits;
    typedef typename SampleValueTraits::value_type Type;
    typedef TypedArraySample<SampleValueTraits> AlembicSample;

    // The property is ultimately an array of this type where each element
    // has length extent.
    typedef typename PODTraitsFromEnum<
                        SampleValueTraits::pod_enum>::value_type PodType;
    static const int extent = SampleValueTraits::extent;

    const SdfValueTypeName& usdType = samples.GetTypeName();

    // Make a sample holding an array of type PodType.
    _SampleForAlembic sample =
        _MakeSample<PodType, extent>(schema, converter,
                                     usdType, samples.Get(time),
                                     skipAlembicTypeCheck);
    if (! _CheckSample(sample, samples, usdType)) {
        return sample;
    }

    // Write to dst.
    (dst->*method)(AlembicSample(sample.GetDataAs<Type>(),
                                 sample.GetCount() / extent));

    return sample;
}

// Copy to a TypedArraySample<T>.  The client *must* hold the returned
// _SampleForAlembic until the sample is finally consumed.
template <class DST, class R, class T>
_SampleForAlembic
_Copy(
    const _WriterSchema& schema,
    double time,
    const UsdSamples& samples,
    DST* dst,
    R (DST::*method)(const TypedArraySample<T>&))
{
    static const bool skipAlembicTypeCheck = true;
    return _Copy(schema, schema.GetConverter(samples.GetTypeName()),
                 time, samples, dst, method, !skipAlembicTypeCheck);
}

// Copy to a OTypedGeomParam<T>::Sample.  The client *must* hold the returned
// _SampleForAlembic until the sample is finally consumed.
// XXX: For some reason the compiler can't deduce the type T so clients
//      are forced to provide it.
template <class T, class DST, class R>
_SampleForAlembic
_Copy(
    const _WriterSchema& schema,
    double time,
    const UsdSamples& valueSamples,
    DST* sample,
    R (DST::*method)(const typename OTypedGeomParam<T>::Sample&))
{
    typedef T SampleValueTraits;
    typedef typename SampleValueTraits::value_type Type;
    typedef typename OTypedGeomParam<SampleValueTraits>::Sample AlembicSample;
    typedef TypedArraySample<SampleValueTraits> AlembicValueSample;

    // The property is ultimately an array of this type where each element
    // has length extent.
    typedef typename PODTraitsFromEnum<
                        SampleValueTraits::pod_enum>::value_type PodType;
    static const int extent = SampleValueTraits::extent;

    const SdfValueTypeName& usdType = valueSamples.GetTypeName();
    const _WriterSchema::Converter& converter = schema.GetConverter(usdType);

    // Make a sample holding an array of type PodType.
    _SampleForAlembic vals =
        _MakeSample<PodType, extent>(schema, converter,
                                     usdType, valueSamples.Get(time));
    if (! _CheckSample(vals, valueSamples, usdType)) {
        return vals;
    }

    // Get the interpolation.
    VtValue value = valueSamples.GetField(UsdGeomTokens->interpolation);
    const GeometryScope geoScope =
        value.IsHolding<TfToken>()
            ? _GetGeometryScope(value.UncheckedGet<TfToken>())
            : kUnknownScope;

    // Make the values indexed if desired.
    _MakeIndexed<PodType, extent>(&vals);

    // Get the indices.
    if (_SampleForAlembic::IndexArrayPtr indicesPtr = vals.GetIndices()) {
        // Note that the indices pointer is valid so long as vals is valid.
        const _SampleForAlembic::IndexArray& indices = *indicesPtr;
        UInt32ArraySample indicesSample(&indices[0], indices.size());
        (sample->*method)(
            AlembicSample(
                AlembicValueSample(vals.GetDataAs<Type>(),
                                   vals.GetCount() / extent),
                indicesSample,
                geoScope));
    }
    else {
        (sample->*method)(
            AlembicSample(
                AlembicValueSample(vals.GetDataAs<Type>(),
                                   vals.GetCount() / extent),
                geoScope));
    }

    return vals;
}

// Copy to a scalar property.
static
void
_Copy(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    double time,
    const UsdSamples& samples,
    OScalarProperty* property)
{
    const SdfValueTypeName& usdType = samples.GetTypeName();
    const DataType& dataType = property->getDataType();

    // Make a sample holding a value of the type expected by the property.
    static const bool skipAlembicTypeCheck = true;
    _SampleForAlembic sample =
        _MakeSample(schema, converter, usdType,
                    samples.Get(time), dataType,
                    skipAlembicTypeCheck);
    if (! _CheckSample(sample, samples, usdType)) {
        return;
    }

    // Write to dst.
    property->set(sample.GetData());
}

// Copy to an array property.
static
void
_Copy(
    const _WriterSchema& schema,
    const _WriterSchema::Converter& converter,
    double time,
    const UsdSamples& samples,
    OArrayProperty* property)
{
    const SdfValueTypeName& usdType = samples.GetTypeName();
    const DataType& dataType = property->getDataType();

    // Make a sample holding an array of the type expected by the property.
    static const bool skipAlembicTypeCheck = true;
    _SampleForAlembic sample =
        _MakeSample(schema, converter, usdType,
                    samples.Get(time), dataType,
                    skipAlembicTypeCheck);
    if (! _CheckSample(sample, samples, usdType)) {
        return;
    }

    // Write to dst.
    Dimensions count(sample.GetCount() / dataType.getExtent());
    property->set(ArraySample(sample.GetData(), dataType, count));
}

static
void
_CopyXform(
    double time,
    const UsdSamples& samples,
    XformSample* sample)
{
    const VtValue value = samples.Get(time);
    if (value.IsHolding<GfMatrix4d>()) {
        const GfMatrix4d& transform = value.UncheckedGet<GfMatrix4d>();
        sample->addOp(
            XformOp(kMatrixOperation, kMatrixHint),
            M44d(reinterpret_cast<const double(*)[4]>(transform.GetArray())));
    }
    else {
        TF_WARN("Expected type 'GfMatrix4d', got '%s'",
                ArchGetDemangled(value.GetTypeName()).c_str());
    }
}

template <class DST> 
static
void
_CopySelfBounds(
    double time,
    const UsdSamples& samples,
    DST* sample)
{
    const VtValue value = samples.Get(time);
    if (value.IsHolding<VtArray<GfVec3f> >()) {
        const VtArray<GfVec3f>& a = value.UncheckedGet<VtArray<GfVec3f> >();
        Box3d box(V3d(a[0][0], a[0][1], a[0][2]),
                  V3d(a[1][0], a[1][1], a[1][2]));
        sample->setSelfBounds(box);
    }
    else {
        TF_WARN("Expected type 'VtArray<GfVec3f>', got '%s'",
                ArchGetDemangled(value.GetTypeName()).c_str());
    }
}

static
_SampleForAlembic
_CopyVisibility(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->inherited) {
        return _SampleForAlembic(int8_t(kVisibilityDeferred));
    }
    if (value == UsdGeomTokens->invisible) {
        return _SampleForAlembic(int8_t(kVisibilityHidden));
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported invisibility '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopySubdivisionScheme(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->catmullClark) {
        return _SampleForAlembic(std::string("catmull-clark"));
    }
    if (value == UsdGeomTokens->loop) {
        return _SampleForAlembic(std::string("loop"));
    }
    if (value == UsdGeomTokens->bilinear) {
        return _SampleForAlembic(std::string("bilinear"));
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported subdivisionScheme '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyInterpolateBoundary(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->none) {
        return _SampleForAlembic(int32_t(0));
    }
    if (value == UsdGeomTokens->edgeAndCorner) {
        return _SampleForAlembic(int32_t(1));
    }
    if (value == UsdGeomTokens->edgeOnly) {
        return _SampleForAlembic(int32_t(2));
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported interpolateBoundary '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyFaceVaryingInterpolateBoundary(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->all) {
        return _SampleForAlembic(int32_t(0));
    }
    if (value == UsdGeomTokens->cornersPlus1) {
        return _SampleForAlembic(int32_t(1));
    }
    if (value == UsdGeomTokens->none) {
        return _SampleForAlembic(int32_t(2));
    }
    if (value == UsdGeomTokens->boundaries) {
        return _SampleForAlembic(int32_t(3));
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported faceVaryingLinearInterpolation '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyAdskColor(const VtValue& src)
{
    const VtArray<GfVec3f>& color = src.UncheckedGet<VtArray<GfVec3f> >();
    std::vector<float> result(color[0].GetArray(), color[0].GetArray() + 3);
    result.push_back(1.0);
    return _SampleForAlembic(result);
}

static
_SampleForAlembic
_CopyCurveBasis(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->none) {
        return _SampleForAlembic(kNoBasis);
    }
    if (value == UsdGeomTokens->bezier) {
        return _SampleForAlembic(kBezierBasis);
    }
    if (value == UsdGeomTokens->bspline) {
        return _SampleForAlembic(kBsplineBasis);
    }
    if (value == UsdGeomTokens->catmullRom) {
        return _SampleForAlembic(kCatmullromBasis);
    }
    if (value == UsdGeomTokens->hermite) {
        return _SampleForAlembic(kHermiteBasis);
    }
    if (value == UsdGeomTokens->power) {
        return _SampleForAlembic(kPowerBasis);
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported curve basis '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyCurveType(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->none) {
        return _SampleForAlembic(kCubic);
    }
    if (value == UsdGeomTokens->linear) {
        return _SampleForAlembic(kLinear);
    }
    if (value == UsdGeomTokens->cubic) {
        return _SampleForAlembic(kCubic);
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported curve type '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyCurveWrap(const VtValue& src)
{
    const TfToken& value = src.UncheckedGet<TfToken>();
    if (value.IsEmpty() || value == UsdGeomTokens->none) {
        return _SampleForAlembic(kNonPeriodic);
    }
    if (value == UsdGeomTokens->nonperiodic) {
        return _SampleForAlembic(kNonPeriodic);
    }
    if (value == UsdGeomTokens->periodic) {
        return _SampleForAlembic(kPeriodic);
    }
    return _ErrorSampleForAlembic(TfStringPrintf(
                            "Unsupported curve wrap '%s'",
                            value.GetText()));
}

static
_SampleForAlembic
_CopyKnots(const VtValue& src)
{
    const VtDoubleArray& value = src.UncheckedGet<VtDoubleArray>();
    return _SampleForAlembic(std::vector<float>(value.begin(), value.end()));
}

static
_SampleForAlembic
_CopyOrder(const VtValue& src)
{
    const VtIntArray& value = src.UncheckedGet<VtIntArray>();
    return _SampleForAlembic(std::vector<uint8_t>(value.begin(), value.end()));
}

static
_SampleForAlembic
_CopyPointIds(const VtValue& src)
{
    const VtInt64Array& value = src.UncheckedGet<VtInt64Array>();
    return _SampleForAlembic(std::vector<uint64_t>(value.begin(), value.end()));
}


// ----------------------------------------------------------------------------

//
// Property writers
//

static
VtValue
_GetField(
    const _PrimWriterContext& context,
    const TfToken& field,
    const TfToken& usdName)
{
    return usdName.IsEmpty() ? context.GetField(field)
                             : context.GetPropertyField(usdName, field);
}

static
void
_SetBoolMetadata(
    MetaData* metadata,
    const _PrimWriterContext& context,
    const TfToken& field,
    const TfToken& usdName = TfToken())
{
    VtValue value = _GetField(context, field, usdName);
    if (value.IsHolding<bool>()) {
        metadata->set(_AmdName(field),
                      value.UncheckedGet<bool>() ? "true" : "false");
    }
}

static
void
_SetStringMetadata(
    MetaData* metadata,
    const _PrimWriterContext& context,
    const TfToken& field,
    const TfToken& usdName = TfToken())
{
    VtValue value = _GetField(context, field, usdName);
    if (value.IsHolding<std::string>()) {
        const std::string& tmp = value.UncheckedGet<std::string>();
        if (! tmp.empty()) {
            metadata->set(_AmdName(field), tmp);
        }
    }
}

static
void
_SetTokenMetadata(
    MetaData* metadata,
    const _PrimWriterContext& context,
    const TfToken& field,
    const TfToken& usdName = TfToken())
{
    VtValue value = _GetField(context, field, usdName);
    if (value.IsHolding<TfToken>()) {
        const TfToken& tmp = value.UncheckedGet<TfToken>();
        if (! tmp.IsEmpty()) {
            metadata->set(_AmdName(field), tmp);
        }
    }
}

static
void
_SetDoubleMetadata(
    MetaData* metadata,
    const _PrimWriterContext& context,
    const TfToken& field,
    const TfToken& usdName = TfToken())
{
    VtValue value = _GetField(context, field, usdName);
    if (value.IsHolding<double>()) {
        metadata->set(_AmdName(field), TfStringify(value));
    }
}

static
MetaData
_GetPropertyMetadata(
    const _PrimWriterContext& context,
    const TfToken& usdName,
    const UsdSamples& samples)
{
    MetaData metadata;

    VtValue value;

    // Custom.
    _SetBoolMetadata(&metadata, context, SdfFieldKeys->Custom, usdName);

    // Write the usd type for exact reverse conversion if we can't deduce it
    // when reading the Alembic.
    value = context.GetPropertyField(usdName, SdfFieldKeys->TypeName);
    const TfToken typeNameToken =
        value.IsHolding<TfToken>() ? value.UncheckedGet<TfToken>() : TfToken();
    const SdfValueTypeName typeName =
        SdfSchema::GetInstance().FindType(typeNameToken);
    const SdfValueTypeName roundTripTypeName =
        context.GetSchema().FindConverter(
            context.GetSchema().FindConverter(typeName));
    if (typeName != roundTripTypeName) {
        metadata.set(_AmdName(SdfFieldKeys->TypeName), typeNameToken);
    }

    // Note a single time sample (as opposed to a default value).
    if (samples.IsTimeSampled() && samples.GetNumSamples() == 1) {
        metadata.set(_AmdName(SdfFieldKeys->TimeSamples), "true");
    }

    // Set the interpretation if there is one.
    const std::string interpretation = _GetInterpretation(typeName);
    if (! interpretation.empty()) {
        metadata.set("interpretation", interpretation);
    }

    // Other Sdf metadata.
    _SetStringMetadata(&metadata, context, SdfFieldKeys->DisplayGroup, usdName);
    _SetStringMetadata(&metadata, context, SdfFieldKeys->Documentation,usdName);
    _SetBoolMetadata(&metadata, context, SdfFieldKeys->Hidden, usdName);
    value = context.GetPropertyField(usdName, SdfFieldKeys->Variability);
    if (value.IsHolding<SdfVariability>() && 
            value.UncheckedGet<SdfVariability>() ==
                SdfVariabilityUniform) {
        metadata.set(_AmdName(SdfFieldKeys->Variability), "uniform");
    }
    value = context.GetPropertyField(usdName, UsdGeomTokens->interpolation);
    if (value.IsHolding<TfToken>()) {
        SetGeometryScope(metadata,
                         _GetGeometryScope(value.UncheckedGet<TfToken>()));
    }

    // Custom metadata.
    _SetStringMetadata(&metadata, context, 
                       UsdAbcCustomMetadata->riName, usdName);
    _SetStringMetadata(&metadata, context, 
                       UsdAbcCustomMetadata->riType, usdName);
    _SetBoolMetadata(&metadata, context,
                     UsdAbcCustomMetadata->gprimDataRender, usdName);

    return metadata;
}

static
bool
_WriteOutOfSchemaProperty(
    _PrimWriterContext* context,
    OCompoundProperty parent,
    const TfToken& usdName,
    const std::string& alembicName)
{
    // Ignore non-attributes.
    if (context->GetSpecType(usdName) != SdfSpecTypeAttribute) {
        if (context->IsFlagSet(UsdAbc_AlembicContextFlagNames->verbose)) {
            TF_WARN("No conversion for <%s> with spec type '%s'",
                    context->GetPath().AppendProperty(usdName).GetText(),
                    TfEnum::GetDisplayName(
                        context->GetSpecType(usdName)).c_str());
        }
        return false;
    }

    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples samples = context->ExtractSamples(usdName);
    if (context->GetSchema().IsValid(samples)) {
        const SdfValueTypeName& usdType = samples.GetTypeName();
        const _WriterSchema::Converter& converter =
            context->GetSchema().GetConverter(usdType);
        if (context->GetSchema().IsShaped(samples)) {
            OArrayProperty property(parent, alembicName,
                                    context->GetSchema().GetDataType(samples),
                                    _GetPropertyMetadata(*context, usdName,
                                                         samples));
            for (double time : context->GetSampleTimesUnion()) {
                _Copy(context->GetSchema(), converter, time, samples,&property);
            }
            property.setTimeSampling(
                context->AddTimeSampling(context->GetSampleTimesUnion()));
        }
        else {
            OScalarProperty property(parent, alembicName,
                                     context->GetSchema().GetDataType(samples),
                                     _GetPropertyMetadata(*context, usdName,
                                                          samples));
            for (double time : context->GetSampleTimesUnion()) {
                _Copy(context->GetSchema(), converter, time, samples,&property);
            }
            property.setTimeSampling(
                context->AddTimeSampling(context->GetSampleTimesUnion()));
        }
        return true;
    }
    else {
        return false;
    }
}

template <class T>
static
void
_WriteGenericProperty(
    _PrimWriterContext* context,
    OCompoundProperty parent,
    const _WriterSchema::Converter& converter,
    const DataType& alembicDataType,
    const TfToken& usdName,
    const std::string& alembicName)
{
    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples samples = context->ExtractSamples(usdName);
    if (context->GetSchema().IsValid(samples)) {
        T property(parent, alembicName, alembicDataType,
                   _GetPropertyMetadata(*context, usdName, samples));
        for (double time : context->GetSampleTimesUnion()) {
            _Copy(context->GetSchema(), converter, time, samples, &property);
        }
        property.setTimeSampling(
            context->AddTimeSampling(context->GetSampleTimesUnion()));
    }
}

static
void
_WriteGenericScalar(
    _PrimWriterContext* context,
    const _WriterSchema::Converter& converter,
    const DataType& alembicDataType,
    const TfToken& usdName,
    const std::string& alembicName)
{
    _WriteGenericProperty<OScalarProperty>(context,
                                           context->GetParent().GetProperties(),
                                           converter, alembicDataType, usdName,
                                           alembicName);
}

/* Not currently used.
static
void
_WriteGenericArray(
    _PrimWriterContext* context,
    const TfToken& usdType,
    const TfToken& usdName,
    const std::string& alembicName)
{
    // XXX: This doesn't have the correct arguments.
    _WriteGenericProperty<OArrayProperty>(context,
                                          context->GetParent().GetProperties(),
                                          conv, usdName, alembicName);
}
*/

//
// Abstract object writers
//

// Helper for converting property namespaces into a hierarchy of
// OCompoundProperty.
class _CompoundPropertyTable {
public:
    _CompoundPropertyTable(OCompoundProperty root)
{
        _table[TfTokenVector()] = root;
    }

    OCompoundProperty FindOrCreate(const TfTokenVector& names);

private:
    OCompoundProperty _FindOrCreate(TfTokenVector& names);

private:
    std::map<TfTokenVector, OCompoundProperty> _table;
};

OCompoundProperty
_CompoundPropertyTable::FindOrCreate(const TfTokenVector& names)
{
    OCompoundProperty result = _table[names];
    if (! result.valid()) {
        TfTokenVector tmpNames = names;
        return _FindOrCreate(tmpNames);
    }
    else {
        return result;
    }
}

OCompoundProperty
_CompoundPropertyTable::_FindOrCreate(TfTokenVector& names)
{
    OCompoundProperty& result = _table[names];
    if (! result.valid()) {
        // We don't have an entry for this path.  Recursively get parent
        // and add the child.
        TfToken name = names.back();
        names.pop_back();
        OCompoundProperty parent = _FindOrCreate(names);
        result = OCompoundProperty(parent, name);
    }
    return result;
}

static
void
_WriteNamespacedPropertyGroup(
    _PrimWriterContext* context,
    const TfToken& namespaceName,
    const boost::function<OCompoundProperty()>& getParentProperty)
{
    // First check if there are any properties to convert.  We only ask
    // for that property if so, because asking for it will create it on
    // demand and we don't want to create it if unnecessary.  Note,
    // however, that we don't confirm that conversion will succeed so
    // we may still create the property with nothing in it.
    bool anyProperties = false;
    for (const auto& name : context->GetUnextractedNames()) {
        TfTokenVector names = SdfPath::TokenizeIdentifierAsTokens(name);
        if (names.size() >= 2 && names[0] == namespaceName) {
            anyProperties = true;
            break;
        }
    }

    // Convert everything in the namespace into the parent compound property.
    // Strip the namespace name from each name before copying.
    if (anyProperties) {
        OCompoundProperty parent = getParentProperty();
        if (! parent.valid()) {
            // We can't get the parent property.  Just put the properties
            // at the top level.
            parent = context->GetParent().GetProperties();
        }

        // Support sub-namespaces as compound properties.
        _CompoundPropertyTable subgroups(parent);

        // Convert each property.
        for (const auto& name : context->GetUnextractedNames()) {
            TfTokenVector names = SdfPath::TokenizeIdentifierAsTokens(name);
            if (names.size() >= 2 && names[0] == namespaceName) {
                // Remove the namespace prefix.
                names.erase(names.begin());

                // The Alembic name is just the last name (i.e. no namespaces).
                const std::string alembicName = names.back();
                names.pop_back();

                // Get/create the subgroup compound property.
                OCompoundProperty group = subgroups.FindOrCreate(names);

                // Write it.
                _WriteOutOfSchemaProperty(context, group, name, alembicName);
            }
            }
        }
    }

static
void
_WriteArbGeomParams(_PrimWriterContext* context)
{
    // Convert everything in the primvars namespace to the getArbGeomParams()
    // compound property.
    const _Parent& parent = context->GetParent();
    _WriteNamespacedPropertyGroup(context,
                                  UsdAbcPropertyNames->primvars,
                                  boost::bind(&_Parent::GetArbGeomParams,
                                              boost::cref(parent)));
}

static
void
_WriteUserProperties(_PrimWriterContext* context)
{
    // Convert everything in the userProperties namespace to the
    // getUserProperties() compound property.
    const _Parent& parent = context->GetParent();
    _WriteNamespacedPropertyGroup(context,
                                  UsdAbcPropertyNames->userProperties,
                                  boost::bind(&_Parent::GetUserProperties,
                                              boost::cref(parent)));
    }

static
void
_WriteGprim(_PrimWriterContext* context)
{
    // extent is handled by GeomBase subclasses automatically.

    // Write the orientation.
    _WriteOutOfSchemaProperty(context, context->GetParent().GetProperties(),
                              UsdGeomTokens->orientation,
                              _AmdName(UsdGeomTokens->orientation));
}

static
void
_WriteMayaColor(_PrimWriterContext* context)
{
    static const TfToken displayColor("primvars:displayColor");
    static const TfToken name("adskDiffuseColor");

    UsdSamples color(context->GetPath(), displayColor);
    if (context->GetData().HasSpec(
            SdfAbstractDataSpecId(&context->GetPath(), &displayColor))) {
        color =
            UsdSamples(context->GetPath(), displayColor, context->GetData());
    }
    if (color.IsEmpty()) {
        // Copy existing Maya color.
        if (! _WriteOutOfSchemaProperty(context,
                                          context->GetParent().GetSchema(),
                                          name, name)) {
            return;
        }
    }
    else {
        // Use displayColor.
        UsdAbc_TimeSamples sampleTimes;
        color.AddTimes(&sampleTimes);

        MetaData metadata;
        metadata.set("interpretation", "rgba");

        OScalarProperty property(context->GetParent().GetSchema(),
                                 name,
                                 DataType(kFloat32POD, 4),
                                 metadata);
        for (double time : sampleTimes) {
            _Copy(context->GetSchema(), _CopyAdskColor, time, color,&property);
        }
        property.setTimeSampling(context->AddTimeSampling(sampleTimes));

        // Don't try writing the Maya color.
        context->ExtractSamples(name);
    }
}

static
void
_WriteUnknownMayaColor(_PrimWriterContext* context)
{
    // XXX -- Write the Maya color to a .geom OCompoundProperty.
}

static
void
_WriteImageable(_PrimWriterContext* context)
{
    _WriteGenericScalar(context, _CopyVisibility, DataType(kInt8POD),
                        UsdGeomTokens->visibility, kVisibilityPropertyName);
}

static
void
_WriteOther(_PrimWriterContext* context)
{
    // Write every unextracted property to Alembic using default converters.
    // This handles any property we don't have specific rules for.  Any
    // Usd name with namespaces is written to Alembic with the namespaces
    // embedded in the name.
    //
    for (const auto& name : context->GetUnextractedNames()) {
        _WriteOutOfSchemaProperty(context,
                                  context->GetParent().GetProperties(),
                                  name, context->GetAlembicPropertyName(name));
    }
}

//
// Object writers -- these create an OObject.
//

void
_AddOrderingMetadata(
    const _PrimWriterContext& context,
    const TfToken& fieldName,
    const std::string& metadataName,
    MetaData* metadata)
{
    VtValue value = context.GetField(fieldName);
    if (value.IsHolding<TfTokenVector>()) {
        const TfTokenVector& order = value.UncheckedGet<TfTokenVector>();
        if (! order.empty()) {
            // Write as space separated names all surrounded by square
            // brackets.
            metadata->set(metadataName, TfStringify(order));
        }
    }
}

static
MetaData
_GetPrimMetadata(const _PrimWriterContext& context)
{
    MetaData metadata;

    // Add "over".
    if (_IsOver(context)) {
        metadata.set(_AmdName(SdfFieldKeys->Specifier), "over");
    }

    _SetBoolMetadata(&metadata, context, SdfFieldKeys->Active);
    _SetBoolMetadata(&metadata, context, SdfFieldKeys->Hidden);
    _SetStringMetadata(&metadata, context, SdfFieldKeys->DisplayGroup);
    _SetStringMetadata(&metadata, context, SdfFieldKeys->Documentation);
    _SetTokenMetadata(&metadata, context, SdfFieldKeys->Kind);

    // Add name children ordering.
    _AddOrderingMetadata(context, SdfFieldKeys->PrimOrder,
                         _AmdName(SdfFieldKeys->PrimOrder), &metadata);

    // Add property ordering.
    _AddOrderingMetadata(context, SdfFieldKeys->PropertyOrder,
                         _AmdName(SdfFieldKeys->PropertyOrder), &metadata);

    return metadata;
}

static
void
_WriteRoot(_PrimWriterContext* context)
{
    // Create the Alembic root.
    shared_ptr<OObject> root(new OObject(context->GetArchive(), kTop));
    context->SetParent(root);

    // Make the root metadata.
    MetaData metadata;
    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->StartTimeCode);
    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->EndTimeCode);

    // Always author a value for timeCodesPerSecond and frameCodesPerSecond 
    // to preserve proper round-tripping from USD->alembic->USD.
    // 
    // First, set them to the corresponding fallback values, then overwrite them 
    // with the values from the input layer.
    // 
    const SdfSchema &sdfSchema = SdfSchema::GetInstance();
    double fallbackTimeCodesPerSecond = sdfSchema.GetFallback(
        SdfFieldKeys->TimeCodesPerSecond).Get<double>();
    double fallbackFramesPerSecond = sdfSchema.GetFallback(
        SdfFieldKeys->FramesPerSecond).Get<double>();

    metadata.set(_AmdName(SdfFieldKeys->TimeCodesPerSecond), 
                  TfStringify(fallbackTimeCodesPerSecond));
    metadata.set(_AmdName(SdfFieldKeys->FramesPerSecond), 
                  TfStringify(fallbackFramesPerSecond));

    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->TimeCodesPerSecond);
    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->FramesPerSecond);

    // XXX(Frame->Time): backwards compatibility
    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->StartFrame);
    _SetDoubleMetadata(&metadata, *context, SdfFieldKeys->EndFrame);

    _SetTokenMetadata(&metadata, *context, SdfFieldKeys->DefaultPrim);

    _SetTokenMetadata(&metadata, *context, UsdGeomTokens->upAxis);

    // Create a compound property to hang metadata off of.  We'd kinda like
    // to put this on the top object but that was created when we opened
    // the file, prior to knowing which SdfAbstractData we were writing.
    OCompoundProperty prop(root->getProperties(), "Usd", metadata);
}

static
void
_WriteCameraParameters(_PrimWriterContext* context)
{
    typedef OCamera Type;

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need to compute the frustum.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples focalLength =
        context->ExtractSamples(UsdGeomTokens->focalLength,
                                SdfValueTypeNames->Float);
    UsdSamples horizontalAperture =
        context->ExtractSamples(UsdGeomTokens->horizontalAperture,
                                SdfValueTypeNames->Float);
    UsdSamples verticalAperture =
        context->ExtractSamples(UsdGeomTokens->verticalAperture,
                                SdfValueTypeNames->Float);
    UsdSamples horizontalApertureOffset =
        context->ExtractSamples(UsdGeomTokens->horizontalApertureOffset,
                                SdfValueTypeNames->Float);
    UsdSamples verticalApertureOffset =
        context->ExtractSamples(UsdGeomTokens->verticalApertureOffset,
                                SdfValueTypeNames->Float);

    UsdSamples clippingRange =
        context->ExtractSamples(UsdGeomTokens->clippingRange,
                                SdfValueTypeNames->Float2);

    // Copy all the samples to set up alembic camera frustum.
    typedef CameraSample SampleT;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        SampleT sample;

        {
            // Horizontal aperture is in cm in ABC, but mm in USD
            const VtValue value = horizontalAperture.Get(time);
            if (value.IsHolding<float>()) {
                sample.setHorizontalAperture(
                    value.UncheckedGet<float>() / 10.0);
            } else {
                TF_WARN("Expected type 'float', '%s' for horizontal aperture",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }

        {
            // Vertical aperture is in cm in ABC, but mm in USD
            const VtValue value = verticalAperture.Get(time);
            if (value.IsHolding<float>()) {
                sample.setVerticalAperture(
                    value.UncheckedGet<float>() / 10.0);
            } else {
                TF_WARN("Expected type 'float', '%s' for vertical aperture",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }

        {
            // Horizontal aperture is in cm in ABC, but mm in USD
            const VtValue value = horizontalApertureOffset.Get(time);
            if (value.IsHolding<float>()) {
                sample.setHorizontalFilmOffset(
                    value.UncheckedGet<float>() / 10.0);
            } else {
                TF_WARN("Expected type 'float', '%s' for horizontal aperture "
                        "offset",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }

        {
            // Vertical aperture is in cm in ABC, but mm in USD
            const VtValue value = verticalApertureOffset.Get(time);
            if (value.IsHolding<float>()) {
                sample.setVerticalFilmOffset(
                    value.UncheckedGet<float>() / 10.0);
            } else {
                TF_WARN("Expected type 'float', '%s' for vertical aperture "
                        "offset",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }

        {
            // Focal length in USD and ABC is both in mm
            const VtValue value = focalLength.Get(time);
            if (value.IsHolding<float>()) {
                sample.setFocalLength(
                    value.UncheckedGet<float>());
            } else {
                TF_WARN("Expected type 'float', '%s' for focal length",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }
    
        {
            const VtValue value = clippingRange.Get(time);
            if (value.IsHolding<GfVec2f>()) {
                sample.setNearClippingPlane(
                    value.UncheckedGet<GfVec2f>()[0]);
                sample.setFarClippingPlane(
                    value.UncheckedGet<GfVec2f>()[1]);
            } else {
                TF_WARN("Expected type 'Vec2f', '%s' for clipping range",
                        ArchGetDemangled(value.GetTypeName()).c_str());
            }
        }

        // Write the sample.
        object->getSchema().set(sample);
    }
    
    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WriteUnknown(_PrimWriterContext* context)
{
    typedef OObject Type;

    // Get the standard metadata and add the Usd prim type, if any.
    MetaData metadata = _GetPrimMetadata(*context);
    const VtValue value = context->GetField(SdfFieldKeys->TypeName);
    if (value.IsHolding<TfToken>()) {
        const TfToken& typeName = value.UncheckedGet<TfToken>();
        if (! typeName.IsEmpty()) {
            metadata.set(_AmdName(SdfFieldKeys->TypeName), typeName);
        }
    }

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     metadata));
    context->SetParent(object);
}

static
void
_WriteXform(_PrimWriterContext* context)
{
    // Collect the properties we need.  We'll need these to compute the
    // metadata.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());

    UsdSamples xformOpOrder = context->ExtractSamples(
        UsdGeomTokens->xformOpOrder, SdfValueTypeNames->TokenArray);

    bool hasXformOpOrder = (context->GetSampleTimesUnion().size()>0);

    // Clear samples from xformOpOrder.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());

    // XXX: NOTE
    // We can't use the GetLocalTranformation API available in UsdGeomXformable
    // here, as there is no UsdPrim (or UsdStage) from which we can construct a 
    // UsdGeomXformable schema object. Hence, for now, if xformOpOrder has a 
    // value, then assuming that the custom "xformOp:transform" attribute will 
    // have the composed local transformation in it.
    // 
    // If xformOpOrder has no authored value, then fallback to reading the
    // old-style transform attribute.
    // 
    const TfToken &transformAttrName = hasXformOpOrder ? 
        _tokens->xformOpTransform : _tokens->transform;
    const SdfValueTypeName &transformValueType = hasXformOpOrder ? 
        SdfValueTypeNames->Matrix4d : SdfValueTypeNames->Matrix4d;

    if (hasXformOpOrder) {
        // Extract and clear samples from the old-style transform attribute, if 
        // it exists, so it doesn't get written out as blind data.
        context->ExtractSamples(_tokens->transform, 
                                SdfValueTypeNames->Matrix4d);                                                   
        context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    }

    UsdSamples transform = context->ExtractSamples(transformAttrName, 
                                                   transformValueType);

    // At this point, all transform related attributes (including all xformOps)
    // should have been extracted. Validate here to make sure there aren't 
    // any unextracted xformOp attributes. 
    for (const auto& name : context->GetUnextractedNames()) {
        if (UsdGeomXformOp::IsXformOp(name)) {
            TF_RUNTIME_ERROR("Found unextracted property '%s' in xformOp "
                "namespace.", name.GetText());
        }
    }

    // Collect the metadata.  Here we have to combine metadata from the
    // prim and from the transform attribute since Alembic will not give
    // us a chance to set metadata on the Alembic properties.
    MetaData metadata = _GetPrimMetadata(*context);
    {
        // Get the transform property metadata.
        MetaData transformMetadata =
            _GetPropertyMetadata(*context, transformAttrName, transform);

        // Merge the property metadata into the prim metadata in a way we
        // can extract later for round-tripping.
        for (MetaData::const_iterator i  = transformMetadata.begin();
                                      i != transformMetadata.end(); ++i) {
            if (! i->second.empty()) {
                metadata.set("Usd.transform:" + i->first, i->second);
            }
        }
    }

    // Create the object and make it the parent.
    OXformPtr object(new OXform(context->GetParent(),
                                context->GetAlembicPrimName(),
                                metadata));
    context->SetParent(object);

    // Copy all the samples.
    typedef XformSample SampleT;
    SampleT sample;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        sample.reset();
        _CopyXform(time, transform, &sample);
        sample.setInheritsXforms(true);

        // Write it.
        object->getSchema().set(sample);
    }

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WriteXformParent(_PrimWriterContext* context)
{
    // Used to split transform into a parent object.
    _WriteXform(context);

    // Put a "Shape" suffix on the geometry.
    context->PushSuffix("Shape");
}

static
void
_WritePolyMesh(_PrimWriterContext* context)
{
    typedef OPolyMesh Type;

    const _WriterSchema& schema = context->GetSchema();

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples extent =
        context->ExtractSamples(UsdGeomTokens->extent,
                                SdfValueTypeNames->Float3Array);
    UsdSamples points =
        context->ExtractSamples(UsdGeomTokens->points,
                                SdfValueTypeNames->Point3fArray);
    UsdSamples velocities =
        context->ExtractSamples(UsdGeomTokens->velocities,
                                SdfValueTypeNames->Vector3fArray);
    UsdSamples faceVertexIndices =
        context->ExtractSamples(UsdGeomTokens->faceVertexIndices,
                                SdfValueTypeNames->IntArray);
    UsdSamples faceVertexCounts =
        context->ExtractSamples(UsdGeomTokens->faceVertexCounts,
                                SdfValueTypeNames->IntArray);
    UsdSamples normals =
        context->ExtractSamples(UsdGeomTokens->normals,
                                SdfValueTypeNames->Normal3fArray);
    UsdSamples uv =
        context->ExtractSamples(UsdAbcPropertyNames->uv,
                                SdfValueTypeNames->Float2Array);

    // Adjust faceVertexIndices for winding order.
    _ReverseWindingOrder(context, &faceVertexIndices, faceVertexCounts);

    // Copy all the samples.
    typedef Type::schema_type::Sample SampleT;
    SampleT sample;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        sample.reset();
        _CopySelfBounds(time, extent, &sample);
        _SampleForAlembic alembicPoints =
        _Copy(schema,
              time, points,
              &sample, &SampleT::setPositions);
        _SampleForAlembic alembicVelocities =
        _Copy(schema,
              time, velocities,
              &sample, &SampleT::setVelocities);
        _SampleForAlembic alembicFaceIndices =
        _Copy(schema,
              time, faceVertexIndices,
              &sample, &SampleT::setFaceIndices);
        _SampleForAlembic alembicFaceCounts =
        _Copy(schema,
              time, faceVertexCounts,
              &sample, &SampleT::setFaceCounts);
        _SampleForAlembic alembicNormals =
        _Copy<ON3fGeomParam::prop_type::traits_type>(schema,
              time, normals,
              &sample, &SampleT::setNormals);
        _SampleForAlembic alembicUVs =
        _Copy<OV2fGeomParam::prop_type::traits_type>(schema,
              time, uv,
              &sample, &SampleT::setUVs);

        // Write the sample.
        object->getSchema().set(sample);
    }

    // Alembic doesn't need this since it knows it's a PolyMesh.
    context->ExtractSamples(UsdGeomTokens->subdivisionScheme);

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WriteFaceSets(_PrimWriterContext* context)
{
    typedef OFaceSet Type;

    // Because we don't have access to the UsdGeomFaceSetAPI (since we don't
    // have a UsdPrim to use), we'll have to replicate some functionality
    // here for dealing with Usd facesets.
    std::set<std::string> faceSetNames;
    for (const auto& token : context->GetUnextractedNames()) {
        const auto nameTokens =
            SdfPath::TokenizeIdentifierAsTokens(token.GetString());
        if (nameTokens.size() < 3 || nameTokens[0] != UsdGeomTokens->faceSet)
            continue;

        faceSetNames.insert(nameTokens[1].GetString());
    }

    for (const auto& faceSetName : faceSetNames) {
        const std::string baseFaceSetName =
            SdfPath::JoinIdentifier(
                UsdGeomTokens->faceSet.GetString(),
                faceSetName);
        const TfToken faceIndicesName(
            SdfPath::JoinIdentifier(baseFaceSetName, "faceIndices"));
        const TfToken faceCountsName(
            SdfPath::JoinIdentifier(baseFaceSetName, "faceCounts"));

        // Because isPartition is a per-FaceSet thing in Usd and exclusivity
        // is a global thing among all FaceSets in Alembic, we'll just
        // simplify and leave the Alembic default (which doesn't guarantee
        // exclusivity among FaceSets).
        const TfToken isPartitionName(
            SdfPath::JoinIdentifier(baseFaceSetName, "isPartition"));
        context->ExtractSamples(isPartitionName);

        UsdSamples faceCounts =
            context->ExtractSamples(faceCountsName,
                                    SdfValueTypeNames->IntArray);
        UsdSamples faceIndices =
            context->ExtractSamples(faceIndicesName,
                                    SdfValueTypeNames->IntArray);

        // If there are missing faceCounts or faceIndices properties, or there
        // is a variable number of faceCounts, simply skip this faceset, as
        // it is invalid.
        if (faceCounts.IsEmpty() || faceIndices.IsEmpty())
            continue;

        bool isInvalid = false;
        const size_t numGroups =
            faceCounts.GetSamples().begin()->second.GetArraySize();
        for (const auto& sample : faceCounts.GetSamples()) {
            if (sample.second.GetArraySize() != numGroups) {
                isInvalid = true;
                break;
            }
        }

        if (isInvalid)
            continue;

        // Copy all the samples, splitting apart the faceIndices based on the
        // number of groups and faceCounts.
        typedef Type::schema_type::Sample SampleT;

        std::vector<shared_ptr<Type> > objects(numGroups);
        for (double time : context->GetSampleTimesUnion()) {

            const VtValue faceIndexValue = faceIndices.Get(time);
            const VtValue faceCountValue = faceCounts.Get(time);
            if (!faceIndexValue.IsHolding<VtIntArray>())
                continue;
            if (!faceCountValue.IsHolding<VtIntArray>())
                continue;

            const VtIntArray& faceIndexArray = 
                faceIndexValue.UncheckedGet<VtIntArray>();
            const VtIntArray& faceCountArray =
                faceCountValue.UncheckedGet<VtIntArray>();

            // For each sample, iterate over the groups (creating the
            // necessary Alembic objects) and break faceIndices into the
            // appropriate bits per group.
            VtIntArray::const_iterator fii = faceIndexArray.cbegin();
            for (size_t i = 0; i < numGroups; ++i) {
                const int fc = faceCountArray[i];

                std::vector<int32_t> faces;
                std::copy_n(fii, fc, std::back_inserter(faces));
                fii += fc;
                Int32ArraySample alembicFaces(faces);
                SampleT sample(alembicFaces);

                // Create an Alembic object for the face group, if it does
                // not yet exist.
                if (!objects[i]) {
                    std::string faceGroupName = faceSetName;
                    if (numGroups > 1)
                        faceGroupName = TfStringPrintf("%s_%zu", faceGroupName.c_str(), i);
                    objects[i].reset(new Type(
                        context->GetParent(),
                        faceGroupName,
                        _GetPrimMetadata(*context)));
                }

                // Write the sample.
                objects[i]->getSchema().set(sample);
            }
        }

        // Set the time sampling.
        for (const auto& object : objects)
        {
            object->getSchema().setTimeSampling(
                context->AddTimeSampling(context->GetSampleTimesUnion()));
        }
    }
}

// As of Alembic-1.5.1, OSubD::schema_type::Sample has a bug:
// setHoles() actually sets cornerIndices.  The member, m_holes, is
// protected so we subclass and fix setHoles().
// XXX: Remove this when Alembic is fixed.
class MyOSubDSample : public OSubD::schema_type::Sample {
public:
    void setHoles( const Abc::Int32ArraySample &iHoles )
    { m_holes = iHoles; }
};

static
void
_WriteSubD(_PrimWriterContext* context)
{
    typedef OSubD Type;

    const _WriterSchema& schema = context->GetSchema();

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples extent =
        context->ExtractSamples(UsdGeomTokens->extent,
                                SdfValueTypeNames->Float3Array);
    UsdSamples points =
        context->ExtractSamples(UsdGeomTokens->points,
                                SdfValueTypeNames->Point3fArray);
    UsdSamples velocities =
        context->ExtractSamples(UsdGeomTokens->velocities,
                                SdfValueTypeNames->Vector3fArray);
    UsdSamples faceVertexIndices =
        context->ExtractSamples(UsdGeomTokens->faceVertexIndices,
                                SdfValueTypeNames->IntArray);
    UsdSamples faceVertexCounts =
        context->ExtractSamples(UsdGeomTokens->faceVertexCounts,
                                SdfValueTypeNames->IntArray);
    UsdSamples subdivisionScheme =
        context->ExtractSamples(UsdGeomTokens->subdivisionScheme,
                                SdfValueTypeNames->Token);
    UsdSamples interpolateBoundary =
        context->ExtractSamples(UsdGeomTokens->interpolateBoundary,
                                SdfValueTypeNames->Token);
    UsdSamples faceVaryingLinearInterpolation =
        context->ExtractSamples(UsdGeomTokens->faceVaryingLinearInterpolation,
                                SdfValueTypeNames->Token);
    UsdSamples holeIndices =
        context->ExtractSamples(UsdGeomTokens->holeIndices,
                                SdfValueTypeNames->IntArray);
    UsdSamples cornerIndices =
        context->ExtractSamples(UsdGeomTokens->cornerIndices,
                                SdfValueTypeNames->IntArray);
    UsdSamples cornerSharpnesses =
        context->ExtractSamples(UsdGeomTokens->cornerSharpnesses,
                                SdfValueTypeNames->FloatArray);
    UsdSamples creaseIndices =
        context->ExtractSamples(UsdGeomTokens->creaseIndices,
                                SdfValueTypeNames->IntArray);
    UsdSamples creaseLengths =
        context->ExtractSamples(UsdGeomTokens->creaseLengths,
                                SdfValueTypeNames->IntArray);
    UsdSamples creaseSharpnesses =
        context->ExtractSamples(UsdGeomTokens->creaseSharpnesses,
                                SdfValueTypeNames->FloatArray);
    UsdSamples uv =
        context->ExtractSamples(UsdAbcPropertyNames->uv,
                                SdfValueTypeNames->Float2Array);

    // Adjust faceVertexIndices for winding order.
    _ReverseWindingOrder(context, &faceVertexIndices, faceVertexCounts);

    // Copy all the samples.
    typedef MyOSubDSample MySampleT;
    typedef Type::schema_type::Sample SampleT;
    MySampleT mySample;
    SampleT& sample = mySample;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.  Usd defaults faceVaryingLinearInterpolation to
        // edgeAndCorner but Alembic defaults to bilinear so set that first
        // in case we have no opinion.
        sample.reset();
        sample.setFaceVaryingInterpolateBoundary(1);
        _CopySelfBounds(time, extent, &sample);
        _SampleForAlembic alembicPositions =
        _Copy(schema,
              time, points,
              &sample, &SampleT::setPositions);
        _SampleForAlembic alembicVelocities =
        _Copy(schema,
              time, velocities,
              &sample, &SampleT::setVelocities);
        _SampleForAlembic alembicFaceIndices =
        _Copy(schema,
              time, faceVertexIndices,
              &sample, &SampleT::setFaceIndices);
        _SampleForAlembic alembicFaceCounts =
        _Copy(schema,
              time, faceVertexCounts,
              &sample, &SampleT::setFaceCounts);
        _Copy(schema,
              _CopySubdivisionScheme,
              time, subdivisionScheme,
              &sample, &SampleT::setSubdivisionScheme);
        _Copy(schema,
              _CopyInterpolateBoundary,
              time, interpolateBoundary,
              &sample, &SampleT::setInterpolateBoundary);
        _Copy(schema,
              _CopyFaceVaryingInterpolateBoundary,
              time, faceVaryingLinearInterpolation,
              &sample, &SampleT::setFaceVaryingInterpolateBoundary);
        _SampleForAlembic alembicHoles =
        _Copy(schema,
              time, holeIndices,
              &mySample, &MySampleT::setHoles);
        _SampleForAlembic alembicCornerIndices =
        _Copy(schema,
              time, cornerIndices,
              &sample, &SampleT::setCornerIndices);
        _SampleForAlembic alembicCornerSharpnesses =
        _Copy(schema,
              time, cornerSharpnesses,
              &sample, &SampleT::setCornerSharpnesses);
        _SampleForAlembic alembicCreaseIndices =
        _Copy(schema,
              time, creaseIndices,
              &sample, &SampleT::setCreaseIndices);
        _SampleForAlembic alembicCreaseLengths =
        _Copy(schema,
              time, creaseLengths,
              &sample, &SampleT::setCreaseLengths);
        _SampleForAlembic alembicCreaseSharpnesses =
        _Copy(schema,
              time, creaseSharpnesses,
              &sample, &SampleT::setCreaseSharpnesses);
        _SampleForAlembic alembicUVs =
        _Copy<OV2fGeomParam::prop_type::traits_type>(schema,
              time, uv,
              &sample, &SampleT::setUVs);

        // Write the sample.
        object->getSchema().set(sample);
    }

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WriteNurbsCurves(_PrimWriterContext* context)
{
    typedef OCurves Type;

    const _WriterSchema& schema = context->GetSchema();

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples extent =
        context->ExtractSamples(UsdGeomTokens->extent,
                                SdfValueTypeNames->Float3Array);
    UsdSamples points =
        context->ExtractSamples(UsdGeomTokens->points,
                                SdfValueTypeNames->Point3fArray);
    UsdSamples velocities =
        context->ExtractSamples(UsdGeomTokens->velocities,
                                SdfValueTypeNames->Vector3fArray);
    UsdSamples normals =
        context->ExtractSamples(UsdGeomTokens->normals,
                                SdfValueTypeNames->Normal3fArray);
    UsdSamples curveVertexCounts =
        context->ExtractSamples(UsdGeomTokens->curveVertexCounts,
                                SdfValueTypeNames->IntArray);
    UsdSamples widths =
        context->ExtractSamples(UsdGeomTokens->widths,
                                SdfValueTypeNames->FloatArray);
    UsdSamples knots =
        context->ExtractSamples(UsdGeomTokens->knots,
                                SdfValueTypeNames->DoubleArray);
    UsdSamples order =
        context->ExtractSamples(UsdGeomTokens->order,
                                SdfValueTypeNames->IntArray);

    // Copy all the samples.
    typedef Type::schema_type::Sample SampleT;
    SampleT sample;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        sample.reset();
        _CopySelfBounds(time, extent, &sample);
        _SampleForAlembic alembicPositions =
        _Copy(schema,
              time, points,
              &sample, &SampleT::setPositions);
        _SampleForAlembic alembicVelocities =
        _Copy(schema,
              time, velocities,
              &sample, &SampleT::setVelocities);
        _SampleForAlembic alembicNormals =
        _Copy<ON3fGeomParam::prop_type::traits_type>(schema,
              time, normals,
              &sample, &SampleT::setNormals);
        _SampleForAlembic alembicCurveVertexCounts =
        _Copy(schema,
              time, curveVertexCounts,
              &sample, &SampleT::setCurvesNumVertices);
        _SampleForAlembic alembicWidths =
        _Copy<OFloatGeomParam::prop_type::traits_type>(schema,
              time, widths,
              &sample, &SampleT::setWidths);
        _SampleForAlembic alembicKnots =
        _Copy(schema,
              _CopyKnots,
              time, knots,
              &sample, &SampleT::setKnots);
        _SampleForAlembic alembicOrders =
        _Copy(schema,
              _CopyOrder,
              time, order,
              &sample, &SampleT::setOrders);

        // This is how Alembic knows it's a NURBS curve.
        sample.setType(kVariableOrder);

        // Write the sample.
        object->getSchema().set(sample);
    }

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WriteBasisCurves(_PrimWriterContext* context)
{
    typedef OCurves Type;

    const _WriterSchema& schema = context->GetSchema();

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples extent =
        context->ExtractSamples(UsdGeomTokens->extent,
                                SdfValueTypeNames->Float3Array);
    UsdSamples points =
        context->ExtractSamples(UsdGeomTokens->points,
                                SdfValueTypeNames->Point3fArray);
    UsdSamples velocities =
        context->ExtractSamples(UsdGeomTokens->velocities,
                                SdfValueTypeNames->Vector3fArray);
    UsdSamples normals =
        context->ExtractSamples(UsdGeomTokens->normals,
                                SdfValueTypeNames->Normal3fArray);
    UsdSamples curveVertexCounts =
        context->ExtractSamples(UsdGeomTokens->curveVertexCounts,
                                SdfValueTypeNames->IntArray);
    UsdSamples widths =
        context->ExtractSamples(UsdGeomTokens->widths,
                                SdfValueTypeNames->FloatArray);
    UsdSamples basis =
        context->ExtractSamples(UsdGeomTokens->basis,
                                SdfValueTypeNames->Token);
    UsdSamples type =
        context->ExtractSamples(UsdGeomTokens->type,
                                SdfValueTypeNames->Token);
    UsdSamples wrap =
        context->ExtractSamples(UsdGeomTokens->wrap,
                                SdfValueTypeNames->Token);

    // Copy all the samples.
    typedef Type::schema_type::Sample SampleT;
    SampleT sample;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        sample.reset();
        _CopySelfBounds(time, extent, &sample);
        _SampleForAlembic alembicPositions =
        _Copy(schema,
              time, points,
              &sample, &SampleT::setPositions);
        _SampleForAlembic alembicVelocities =
        _Copy(schema,
              time, velocities,
              &sample, &SampleT::setVelocities);
        _SampleForAlembic alembicNormals =
        _Copy<ON3fGeomParam::prop_type::traits_type>(schema,
              time, normals,
              &sample, &SampleT::setNormals);
        _SampleForAlembic alembicCurveVertexCounts =
        _Copy(schema,
              time, curveVertexCounts,
              &sample, &SampleT::setCurvesNumVertices);
        _SampleForAlembic alembicWidths =
        _Copy<OFloatGeomParam::prop_type::traits_type>(schema,
              time, widths,
              &sample, &SampleT::setWidths);
        _Copy(schema,
              _CopyCurveBasis,
              time, basis,
              &sample, &SampleT::setBasis);
        _Copy(schema,
              _CopyCurveType,
              time, type,
              &sample, &SampleT::setType);
        _Copy(schema,
              _CopyCurveWrap,
              time, wrap,
              &sample, &SampleT::setWrap);

        // Write the sample.
        object->getSchema().set(sample);
    }

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
void
_WritePoints(_PrimWriterContext* context)
{
    typedef OPoints Type;

    const _WriterSchema& schema = context->GetSchema();

    // Create the object and make it the parent.
    shared_ptr<Type> object(new Type(context->GetParent(),
                                     context->GetAlembicPrimName(),
                                     _GetPrimMetadata(*context)));
    context->SetParent(object);

    // Collect the properties we need.
    context->SetSampleTimesUnion(UsdAbc_TimeSamples());
    UsdSamples extent =
        context->ExtractSamples(UsdGeomTokens->extent,
                                SdfValueTypeNames->Float3Array);
    UsdSamples points =
        context->ExtractSamples(UsdGeomTokens->points,
                                SdfValueTypeNames->Point3fArray);
    UsdSamples velocities =
        context->ExtractSamples(UsdGeomTokens->velocities,
                                SdfValueTypeNames->Vector3fArray);
    UsdSamples widths =
        context->ExtractSamples(UsdGeomTokens->widths,
                                SdfValueTypeNames->FloatArray);
    UsdSamples ids =
        context->ExtractSamples(UsdGeomTokens->ids,
                                SdfValueTypeNames->Int64Array);

    // Copy all the samples.
    typedef Type::schema_type::Sample SampleT;
    SampleT sample;
    bool first = true;
    for (double time : context->GetSampleTimesUnion()) {
        // Build the sample.
        sample.reset();
        _CopySelfBounds(time, extent, &sample);
        _SampleForAlembic alembicPoints =
        _Copy(schema,
              time, points,
              &sample, &SampleT::setPositions);
        _SampleForAlembic alembicVelocities =
        _Copy(schema,
              time, velocities,
              &sample, &SampleT::setVelocities);
        _SampleForAlembic alembicWidths =
        _Copy<OFloatGeomParam::prop_type::traits_type>(schema,
              time, widths,
              &sample, &SampleT::setWidths);
        _SampleForAlembic alembicIds =
        _Copy(schema,
              _CopyPointIds,
              time, ids,
              &sample, &SampleT::setIds);

        // Alembic requires ids.  We only need to write one and it'll
        // be reused for all the other samples.  We also use a valid
        // but empty array because we don't actually have any data.
        if (first && !sample.getIds()) {
            static const uint64_t data = 0;
            first = false;
            sample.setIds(UInt64ArraySample(&data, 0));
        }

        // Write the sample.
        object->getSchema().set(sample);
    }

    // Set the time sampling.
    object->getSchema().setTimeSampling(
        context->AddTimeSampling(context->GetSampleTimesUnion()));
}

static
TfToken
_ComputeTypeName(
    const _WriterContext& context,
    const SdfAbstractDataSpecId& id)
{
    // Special case.
    if (id.GetPropertyOwningSpecPath() == SdfPath::AbsoluteRootPath()) {
        return UsdAbcPrimTypeNames->PseudoRoot;
    }

    // General case.
    VtValue value = context.GetData().Get(id, SdfFieldKeys->TypeName);
    if (! value.IsHolding<TfToken>()) {
        return TfToken();
    }
    TfToken typeName = value.UncheckedGet<TfToken>();

    // Special cases.
    if (typeName == UsdAbcPrimTypeNames->Mesh) {
        SdfAbstractDataSpecId propId(&id.GetPropertyOwningSpecPath(),
                                     &UsdGeomTokens->subdivisionScheme);
        value = context.GetData().Get(propId, SdfFieldKeys->Default);
        if (value.IsHolding<TfToken>() && 
                value.UncheckedGet<TfToken>() == "none") {
            typeName = UsdAbcPrimTypeNames->PolyMesh;
        }
    }

    return typeName;
}

static
void
_WritePrim(
    _WriterContext& context,
    const _Parent& parent,
    const SdfPath& path)
{
    SdfAbstractDataSpecId id(&path);

    _Parent prim;
    {
        // Compute the type name.
        const TfToken typeName = _ComputeTypeName(context, id);

        // Write the properties.
        _PrimWriterContext primContext(context, parent, id);
        for (const auto& writer : context.GetSchema().GetPrimWriters(typeName)) {
            TRACE_SCOPE("UsdAbc_AlembicDataWriter:_WritePrim");
            writer(&primContext);
        }
        prim = primContext.GetParent();
    }

    // Write the name children.
    const VtValue childrenNames =
        context.GetData().Get(id, SdfChildrenKeys->PrimChildren);
    if (childrenNames.IsHolding<TfTokenVector>()) {
        for (const auto& childName : childrenNames.UncheckedGet<TfTokenVector>()) {
            _WritePrim(context, prim, path.AppendChild(childName));
        }
    }
}

// ----------------------------------------------------------------------------

//
// Schema builder
//

struct _WriterSchemaBuilder {
    _WriterSchema schema;

    _WriterSchemaBuilder();
};

_WriterSchemaBuilder::_WriterSchemaBuilder()
{
    schema.AddType(UsdAbcPrimTypeNames->Scope)
        .AppendWriter(_WriteUnknown)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->Xform)
        .AppendWriter(_WriteXform)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->Mesh)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WriteSubD)
        .AppendWriter(_WriteMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteFaceSets)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->PolyMesh)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WritePolyMesh)
        .AppendWriter(_WriteMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteFaceSets)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->NurbsCurves)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WriteNurbsCurves)
        .AppendWriter(_WriteMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->BasisCurves)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WriteBasisCurves)
        .AppendWriter(_WriteMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->Points)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WritePoints)
        .AppendWriter(_WriteMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
    schema.AddType(UsdAbcPrimTypeNames->Camera)
        .AppendWriter(_WriteXformParent)
        .AppendWriter(_WriteCameraParameters)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;

    // This handles the root.
    schema.AddType(UsdAbcPrimTypeNames->PseudoRoot)
        .AppendWriter(_WriteRoot)
        ;

    // This handles overs with no type and any unknown prim type.
    schema.AddFallbackType()
        .AppendWriter(_WriteUnknown)
        .AppendWriter(_WriteUnknownMayaColor)
        .AppendWriter(_WriteGprim)
        .AppendWriter(_WriteImageable)
        .AppendWriter(_WriteArbGeomParams)
        .AppendWriter(_WriteUserProperties)
        .AppendWriter(_WriteOther)
        ;
}

} // anonymous namespace

static
const _WriterSchema&
_GetSchema()
{
    static _WriterSchemaBuilder builder;
    return builder.schema;
}

//
// UsdAbc_AlembicDataWriter
//

class UsdAbc_AlembicDataWriterImpl : public _WriterContext { };

UsdAbc_AlembicDataWriter::UsdAbc_AlembicDataWriter() :
    _impl(new UsdAbc_AlembicDataWriterImpl)
{
    // Do nothing
}

UsdAbc_AlembicDataWriter::~UsdAbc_AlembicDataWriter()
{
    Close();
}

bool
UsdAbc_AlembicDataWriter::Open(
    const std::string& filePath,
    const std::string& comment)
{
    TRACE_FUNCTION();

    _errorLog.clear();
    try {
        _impl->SetArchive(
            CreateArchiveWithInfo(Alembic::AbcCoreOgawa::WriteArchive(),
                                  filePath, writerName, comment));
        return true;
    }
    catch (std::exception& e) {
        _errorLog.append(e.what());
        _errorLog.append("\n");
        return false;
    }
}

bool
UsdAbc_AlembicDataWriter::Write(const SdfAbstractDataConstPtr& data)
{
    TRACE_FUNCTION();

    try {
        if (_impl->GetArchive().valid() && data) {
            const _WriterSchema& schema = _GetSchema();
            _impl->SetSchema(&schema);
            _impl->SetData(data);
            _WritePrim(*_impl, _Parent(), SdfPath::AbsoluteRootPath());
        }
        return true;
    }
    catch (std::exception& e) {
        _errorLog.append(e.what());
        _errorLog.append("\n");
        return false;
    }
}

bool
UsdAbc_AlembicDataWriter::Close()
{
    TRACE_FUNCTION();

    // Alembic does not appear to be robust when closing an archive.
    // ~AwImpl does real writes and the held std::ostream is configured
    // to throw exceptions, so any exceptions when writing should leak
    // from the d'tor.  This is a fatal error.
    //
    // For now we just destroy the archive and don't bother looking for
    // errors.
    _impl->SetArchive(OArchive());
    return true;
}

std::string
UsdAbc_AlembicDataWriter::GetErrors() const
{
    return _errorLog;
}

void
UsdAbc_AlembicDataWriter::SetFlag(const TfToken& flagName, bool set)
{
    _impl->SetFlag(flagName, set);
}

PXR_NAMESPACE_CLOSE_SCOPE

