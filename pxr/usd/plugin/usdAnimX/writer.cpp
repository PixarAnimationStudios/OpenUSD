//
// Copyright 2020 benmalarre
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
/// \file usdAnimX/writer.cpp

#include "pxr/pxr.h"
#include "pxr/usd/plugin/usdAnimX/writer.h"
//#include "pxr/usd/plugin/usdAnimX/util.h"
#include "pxr/usd/usdGeom/hermiteCurves.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/ostreamMethods.h"
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


// The name of this exporter, embedded in written animx files.
static const char* writerName = "UsdAnimXData";

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (transform)
    ((xformOpTransform, "xformOp:transform"))
);


namespace {

//using namespace UsdAnimXUtil;

// The SdfAbstractData time samples type.
// XXX: SdfAbstractData should typedef this.
typedef std::set<double> UsdAnimXTimeSamples;

struct _Subtract {
    double operator()(double x, double y) const { return x - y; }
};


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

    /// Returns a path.
    SdfPath GetPath() const;

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
    void AddTimes(UsdAnimXTimeSamples* times) const;

    /// Returns the sample map.
    const SdfTimeSampleMap& GetSamples() const;

    /// Sets the samples to \p samples.  The contents of \p samples is
    /// undefined after the call.
    void TakeSamples(SdfTimeSampleMap& samples);

private:
    bool _Validate();
    void _Clear();

private:
    SdfPath _propPath;
    const SdfAbstractData* _data;
    boost::shared_ptr<VtValue> _value;
    boost::shared_ptr<SdfTimeSampleMap> _local;
    const SdfTimeSampleMap* _samples;
    bool _timeSampled;
    SdfValueTypeName _typeName;
};

UsdSamples::UsdSamples(const SdfPath& primPath, const TfToken& propertyName) :
    _propPath(primPath.AppendProperty(propertyName)),
    _data(NULL)
{
    _Clear();
}

UsdSamples::UsdSamples(
    const SdfPath& primPath,
    const TfToken& propertyName,
    const SdfAbstractData& data) :
    _propPath(primPath.AppendProperty(propertyName)),
    _data(&data)
{
    VtValue value;
    if (data.Has(_propPath, SdfFieldKeys->TimeSamples, &value)) {
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
    else if (data.Has(_propPath, SdfFieldKeys->Default, &value)) {
        _local.reset(new SdfTimeSampleMap);
        (*_local)[0.0].Swap(value);
        _samples       = _local.get();
        _timeSampled   = false;
    }
    else {
        _Clear();
        return;
    }
    if (TF_VERIFY(data.Has(_propPath, SdfFieldKeys->TypeName, &value),
                  "No type name on <%s>", _propPath.GetText())) {
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
                              GetPath().GetText(),
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

SdfPath
UsdSamples::GetPath() const
{
    return _propPath;
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
    return _data->Get(_propPath, name);
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
UsdSamples::AddTimes(UsdAnimXTimeSamples* times) const
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
// UsdAnimXDataWriter
//

class UsdAnimXDataWriterImpl : public _WriterContext { };

UsdAnimXDataWriter::UsdAnimXDataWriter() :
    _impl(new UsdAnimXDataWriterImpl)
{
    // Do nothing
}

UsdAnimXDataWriter::~UsdAnimXDataWriter()
{
    Close();
}

bool
UsdAnimXDataWriter::Open(
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
UsdAnimXDataWriter::Write(const SdfAbstractDataConstPtr& data)
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
UsdAnimXDataWriter::Close()
{
    TRACE_FUNCTION();

    // Alembic does not appear to be robust when closing an archive.
    // ~AwImpl does real writes and the held std::ostream is configured
    // to throw exceptions, so any exceptions when writing should leak
    // from the d'tor.  This is a fatal error.
    //
    // For now we just destroy the archive and don't bother looking for
    // errors.
    //_impl->SetArchive(OArchive());
    return true;
}

std::string
UsdAnimXDataWriter::GetErrors() const
{
    return _errorLog;
}

void
UsdAnimXDataWriter::SetFlag(const TfToken& flagName, bool set)
{
    //_impl->SetFlag(flagName, set);
}

PXR_NAMESPACE_CLOSE_SCOPE

