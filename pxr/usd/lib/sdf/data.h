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
#ifndef SDF_DATA_H
#define SDF_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfData);

/// \class SdfData
///
/// SdfData provides concrete scene description data storage.
///
/// An SdfData is an implementation of SdfAbstractData that simply
/// stores specs and fields in a map keyed by path.
///
class SdfData : public SdfAbstractData
{
public:
    SdfData() {}
    SDF_API
    virtual ~SdfData();

    /// SdfAbstractData overrides

    SDF_API
    virtual void CreateSpec(const SdfAbstractDataSpecId& id, 
                            SdfSpecType specType);
    SDF_API
    virtual bool HasSpec(const SdfAbstractDataSpecId& id) const;
    SDF_API
    virtual void EraseSpec(const SdfAbstractDataSpecId& id);
    SDF_API
    virtual void MoveSpec(const SdfAbstractDataSpecId& oldId, 
                          const SdfAbstractDataSpecId& newId);
    SDF_API
    virtual SdfSpecType GetSpecType(const SdfAbstractDataSpecId& id) const;

    SDF_API
    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken &fieldName,
                     SdfAbstractDataValue* value) const;
    SDF_API
    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     VtValue *value = NULL) const;
    SDF_API
    virtual VtValue Get(const SdfAbstractDataSpecId& id, 
                        const TfToken& fieldName) const;
    SDF_API
    virtual void Set(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     const VtValue & value);
    SDF_API
    virtual void Set(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    SDF_API
    virtual void Erase(const SdfAbstractDataSpecId& id, 
                       const TfToken& fieldName);
    SDF_API
    virtual std::vector<TfToken> List(const SdfAbstractDataSpecId& id) const;

    SDF_API
    virtual std::set<double>
    ListAllTimeSamples() const;
    
    SDF_API
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    SDF_API
    virtual size_t
    GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId& id, 
                                    double time,
                                    double* tLower, double* tUpper) const;

    SDF_API
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                    SdfAbstractDataValue *optionalValue) const;
    SDF_API
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                    VtValue *value) const;

    SDF_API
    virtual void
    SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                  const VtValue & value);

    SDF_API
    virtual void
    EraseTimeSample(const SdfAbstractDataSpecId& id, double time);

protected:
    // SdfAbstractData overrides
    SDF_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    const VtValue* _GetFieldValue(const SdfAbstractDataSpecId& id,
                                  const TfToken& field) const;
    VtValue* _GetOrCreateFieldValue(const SdfAbstractDataSpecId& id,
                                    const TfToken& field);

private:
    // Backing storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    struct _SpecData {
        _SpecData() : specType(SdfSpecTypeUnknown) {}
        
        SdfSpecType specType;
        std::vector<_FieldValuePair> fields;
    };

    // Hashtable storing _SpecData.
    typedef SdfPath _Key;
    typedef SdfPath::Hash _KeyHash;
    typedef TfHashMap<_Key, _SpecData, _KeyHash> _HashTable;

    _HashTable _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_DATA_H
