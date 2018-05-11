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
/// \file alembicData.cpp

#include "pxr/pxr.h"
#include "pxr/usd/usdAbc/alembicData.h"
#include "pxr/usd/usdAbc/alembicReader.h"
#include "pxr/usd/usdAbc/alembicUtil.h"
#include "pxr/usd/usdAbc/alembicWriter.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


// Note: The Alembic translator has a few major parts.  Here's a
//       quick description.
//
//   Data type translation
//     Types and functions for describing Alembic data types and for
//     converting Usd <-> Alembic.
//
//   UsdAbc_AlembicDataConversion
//     A class for holding data type conversion tables.  It can convert
//     Alembic properties to Usd values and vice versa.  It does not
//     register any converters, it just tabulates them.  This hopefully
//     supports everything we'll ever need.
//
//   UsdAbc_AlembicConversions
//     The constructor of this class registers all known data conversions.
//     Add to the c'tor when you have a new conversion.
//
//   UsdAbc_AlembicDataReader
//     The backing implementation of UsdAbc_AlembicData.  It acts like a
//     key/value database and is itself backed by Alembic.  When an
//     Alembic file is opened, this scans the object/property hierarchy
//     and caches state for fast lookup later.  It does not do any (well,
//     much) value conversion until the client requests property values.
//
//     Helping this class is the _ReaderSchema, which has a table of object
//     types and for each type a sequence of reader functions to process
//     certain properties of the object and build the database mentioned
//     above.  The _ReaderSchemaBuilder provides a quick way to see what
//     objects/properties are supported and it's where to go first when
//     adding support for new object types.
//
//   UsdAbc_AlembicDataWriter
//     Unlike UsdAbc_AlembicDataReader, UsdAbc_AlembicDataWriter does not
//     support the SdfAbstractData API and we can't use Alembic as an
//     authoring layer.  That's because Alembic is not suitable for
//     interactive editing.  This class only supports creating/truncating
//     an Alembic file, dumping a layer to it and closing the file.
//
//     Helping this class is the _WriterSchema, which is similar to the
//     _ReaderSchema except the writer functions actually create Alembic
//     objects and properties instead of building a database for looking
//     up values later.  The _WriterSchemaBuilder provides a quick way to
//     see what objects/properties are supported and it's where to go first
//     when adding support for new object types.
//
//   UsdAbc_AlembicData
//     Forwards most calls to UsdAbc_AlembicDataReader.  It has a static
//     method for writing an Alembic file.  The UsdAbc_AlembicDataReader
//     exists between a successful Open() and Close().  When there is no
//     reader the data acts as if there's a pseudo-root prim spec at the
//     absolute root path.
//

using namespace UsdAbc_AlembicUtil;

TF_DEFINE_ENV_SETTING(USD_ABC_EXPAND_INSTANCES, false,
                      "Force Alembic instances to be expanded.");
TF_DEFINE_ENV_SETTING(USD_ABC_DISABLE_INSTANCING, false,
                      "Disable instancing on masters created from Alembic.");
TF_DEFINE_ENV_SETTING(USD_ABC_PARENT_INSTANCES, true,
                      "Make parent of instance source into master where possible.");

// The SdfAbstractData time samples type.
// XXX: SdfAbstractData should typedef this.
typedef std::set<double> UsdAbc_TimeSamples;

//
// UsdAbc_AlembicData
//

#define XXX_UNSUPPORTED(M) TF_RUNTIME_ERROR("Alembic file " #M "() not supported")

UsdAbc_AlembicData::UsdAbc_AlembicData()
{
    // Do nothing
}

UsdAbc_AlembicData::~UsdAbc_AlembicData()
{
    // Do nothing
}

UsdAbc_AlembicDataRefPtr
UsdAbc_AlembicData::New()
{
    return TfCreateRefPtr(new UsdAbc_AlembicData);
}

bool
UsdAbc_AlembicData::Open(const std::string& filePath)
{
    TfAutoMallocTag2 tag("UsdAbc_AlembicData", "UsdAbc_AlembicData::Open");
    TRACE_FUNCTION();

    // Prepare the reader.
    _reader.reset(new UsdAbc_AlembicDataReader);
    // Suppress instancing support.
    if (TfGetEnvSetting(USD_ABC_EXPAND_INSTANCES)) {
        _reader->SetFlag(UsdAbc_AlembicContextFlagNames->expandInstances);
    }
    // Create instances but disallow instancing on the master.
    if (TfGetEnvSetting(USD_ABC_DISABLE_INSTANCING)) {
        _reader->SetFlag(UsdAbc_AlembicContextFlagNames->disableInstancing);
    }
    // Use the parent of instance sources as the Usd master prim, where
    // possible.
    if (TfGetEnvSetting(USD_ABC_PARENT_INSTANCES)) {
        _reader->SetFlag(UsdAbc_AlembicContextFlagNames->promoteInstances);
    }
    //_reader->SetFlag(UsdAbc_AlembicContextFlagNames->verbose);

    // Open the archive.
    if (_reader->Open(filePath)) {
        return true;
    }

    TF_RUNTIME_ERROR("Failed to open Alembic archive \"%s\": %s",
                     filePath.c_str(),
                     _reader->GetErrors().c_str());
    Close();
    return false;
}

void
UsdAbc_AlembicData::Close()
{
    _reader.reset();
}

bool
UsdAbc_AlembicData::Write(
    const SdfAbstractDataConstPtr& data,
    const std::string& filePath,
    const std::string& comment)
{
    TfAutoMallocTag2 tag("UsdAbc_AlembicData", "UsdAbc_AlembicData::Write");
    TRACE_FUNCTION();

    std::string finalComment = comment;
    if (data && finalComment.empty()) {
        SdfAbstractDataSpecId id(&SdfPath::AbsoluteRootPath());
        VtValue value = data->Get(id, SdfFieldKeys->Comment);
        if (value.IsHolding<std::string>()) {
            finalComment = value.UncheckedGet<std::string>();
        }
    }

    // Prepare the writer.
    UsdAbc_AlembicDataWriter writer;
    //writer.SetFlag(UsdAbc_AlembicContextFlagNames->verbose);

    // Write the archive.
    if (writer.Open(filePath, finalComment)) {
        if (writer.Write(data) && writer.Close()) {
            return true;
        }
        TfDeleteFile(filePath);
    }
    TF_RUNTIME_ERROR("Alembic error: %s", writer.GetErrors().c_str());
    return false;
}

void
UsdAbc_AlembicData::CreateSpec(
    const SdfAbstractDataSpecId& id, 
    SdfSpecType specType)
{
    XXX_UNSUPPORTED(CreateSpec);
}

bool
UsdAbc_AlembicData::HasSpec(const SdfAbstractDataSpecId& id) const
{
    return _reader ? _reader->HasSpec(id)
                   : (id.GetFullSpecPath() == SdfPath::AbsoluteRootPath());
}

void
UsdAbc_AlembicData::EraseSpec(const SdfAbstractDataSpecId& id)
{
    XXX_UNSUPPORTED(EraseSpec);
}

void
UsdAbc_AlembicData::MoveSpec(
    const SdfAbstractDataSpecId& oldId,
    const SdfAbstractDataSpecId& newId)
{
    XXX_UNSUPPORTED(MoveSpec);
}

SdfSpecType
UsdAbc_AlembicData::GetSpecType(const SdfAbstractDataSpecId& id) const
{
    if (_reader) {
        return _reader->GetSpecType(id);
    }
    if (id.GetFullSpecPath() == SdfPath::AbsoluteRootPath()) {
        return SdfSpecTypePseudoRoot;
    }
    return SdfSpecTypeUnknown;
}

void
UsdAbc_AlembicData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    if (_reader) {
        _reader->VisitSpecs(*this, visitor);
    }
}

bool
UsdAbc_AlembicData::Has(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    SdfAbstractDataValue* value) const
{
    return _reader ? _reader->HasField(id, fieldName, value) : false;
}

bool
UsdAbc_AlembicData::Has(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    VtValue* value) const
{
    return _reader ? _reader->HasField(id, fieldName, value) : false;
}

VtValue
UsdAbc_AlembicData::Get(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName) const
{
    VtValue result;
    if (_reader) {
        _reader->HasField(id, fieldName, &result);
    }
    return result;
}

void
UsdAbc_AlembicData::Set(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const VtValue& value)
{
    XXX_UNSUPPORTED(Set);
}

void
UsdAbc_AlembicData::Set(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const SdfAbstractDataConstValue& value)
{
    XXX_UNSUPPORTED(Set);
}

void
UsdAbc_AlembicData::Erase(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName)
{
    XXX_UNSUPPORTED(Erase);
}

std::vector<TfToken>
UsdAbc_AlembicData::List(const SdfAbstractDataSpecId& id) const
{
    return _reader ? _reader->List(id) : std::vector<TfToken>();
}

std::set<double>
UsdAbc_AlembicData::ListAllTimeSamples() const
{
    return _reader ? _reader->ListAllTimeSamples() : std::set<double>();
}

std::set<double>
UsdAbc_AlembicData::ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _reader ? _reader->ListTimeSamplesForPath(id).GetTimes()
                   : std::set<double>();
}

bool
UsdAbc_AlembicData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    const std::set<double>& samples = _reader->ListAllTimeSamples();
    return UsdAbc_AlembicDataReader::TimeSamples::Bracket(samples, time,
                                                          tLower, tUpper);
}

size_t
UsdAbc_AlembicData::GetNumTimeSamplesForPath(
    const SdfAbstractDataSpecId& id) const
{
    return _reader ? _reader->ListTimeSamplesForPath(id).GetSize() : 0u;
}

bool
UsdAbc_AlembicData::GetBracketingTimeSamplesForPath(
    const SdfAbstractDataSpecId& id,
    double time, double* tLower, double* tUpper) const
{
    return _reader &&
           _reader->ListTimeSamplesForPath(id).Bracket(time, tLower, tUpper);
}

bool
UsdAbc_AlembicData::QueryTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    SdfAbstractDataValue* value) const
{
    UsdAbc_AlembicDataReader::Index index;
    return _reader &&
           _reader->ListTimeSamplesForPath(id).FindIndex(time, &index) && 
           _reader->HasValue(id, index, value);
}

bool
UsdAbc_AlembicData::QueryTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    VtValue* value) const
{
    UsdAbc_AlembicDataReader::Index index;
    return _reader->ListTimeSamplesForPath(id).FindIndex(time, &index) && 
           _reader->HasValue(id, index, value);
}

void
UsdAbc_AlembicData::SetTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    const VtValue& value)
{
    XXX_UNSUPPORTED(SetTimeSample);
}

void
UsdAbc_AlembicData::EraseTimeSample(const SdfAbstractDataSpecId& id, double time)
{
    XXX_UNSUPPORTED(EraseTimeSample);
}

PXR_NAMESPACE_CLOSE_SCOPE

