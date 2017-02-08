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
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/base/tracelite/trace.h"

#include <iostream>
#include <vector>
#include <utility>

using std::vector;
using std::pair;
using std::make_pair;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdfDataTokens, SDF_DATA_TOKENS);

////////////////////////////////////////////////////////////

std::string 
SdfAbstractDataSpecId::GetString() const
{
    return GetFullSpecPath().GetString();
}

bool 
SdfAbstractDataSpecId::IsProperty() const
{
    return (_propertyName || _path->IsPropertyPath());
}

const SdfPath&
SdfAbstractDataSpecId::_ComputeFullSpecPath() const
{
    if (!_fullSpecPathBuffer) {
        _fullSpecPathBuffer.reset(
            (_path->IsTargetPath() ? 
                _path->AppendRelationalAttribute(*_propertyName) :
                _path->AppendProperty(*_propertyName)));
    }

    return *_fullSpecPathBuffer;
}

const SdfPath&
SdfAbstractDataSpecId::_ComputePropertyOwningSpecPath() const
{
    if (!_propertySpecPathBuffer) {
        _propertySpecPathBuffer.reset(_path->GetParentPath());
    }

    return *_propertySpecPathBuffer;
}

const TfToken&
SdfAbstractDataSpecId::GetPropertyName() const
{
    if (_propertyName) {
        return *_propertyName;
    }

    if (_path->IsPropertyPath()) {
        return _path->GetNameToken();
    }

    static const TfToken empty;
    return empty;
}

////////////////////////////////////////////////////////////

SdfAbstractData::~SdfAbstractData()
{

}

struct SdfAbstractData_IsEmptyChecker : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_IsEmptyChecker() : isEmpty(true) { }
    virtual bool VisitSpec(const SdfAbstractData&, const SdfAbstractDataSpecId&)
    {
        isEmpty = false;
        return false;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool isEmpty;
};

bool
SdfAbstractData::IsEmpty() const
{
    SdfAbstractData_IsEmptyChecker checker;
    VisitSpecs(&checker);
    return checker.isEmpty;
}

struct SdfAbstractData_CopySpecs : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CopySpecs(SdfAbstractData* dest_) : dest(dest_) { }

    virtual bool VisitSpec(
        const SdfAbstractData& src, const SdfAbstractDataSpecId& id)
    {
        const std::vector<TfToken> keys = src.List(id);

        dest->CreateSpec(id, src.GetSpecType(id));
        TF_FOR_ALL(keyIt, keys) {
            dest->Set(id, *keyIt, src.Get(id, *keyIt));
        }
        return true;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    SdfAbstractData* dest;
};

void
SdfAbstractData::CopyFrom(const SdfAbstractDataConstPtr& source)
{
    SdfAbstractData_CopySpecs copySpecsToThis(this);
    source->VisitSpecs(&copySpecsToThis);
}

// Visitor that checks whether all specs in the visited SdfAbstractData object
// exist in another SdfAbstractData object.
struct SdfAbstractData_CheckAllSpecsExist : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CheckAllSpecsExist(const SdfAbstractData& data) 
        : passed(true), _data(data) { }

    virtual bool VisitSpec(
        const SdfAbstractData&, const SdfAbstractDataSpecId& id)
    {
        if (!_data.HasSpec(id)) {
            passed = false;
        }
        return passed;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool passed;

private:
    const SdfAbstractData& _data;
};

// Visitor that checks whether all specs in the visited SdfAbstractData object
// have the same fields and contents as another SdfAbstractData object.
struct SdfAbstractData_CheckAllSpecsMatch : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CheckAllSpecsMatch(const SdfAbstractData& rhs) 
        : passed(true), _rhs(rhs) { }

    virtual bool VisitSpec(
        const SdfAbstractData& lhs, const SdfAbstractDataSpecId& id)
    {
        return (passed = _AreSpecsAtPathEqual(lhs, _rhs, id));
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool passed;

private:
    static bool _AreSpecsAtPathEqual(
        const SdfAbstractData& lhs, const SdfAbstractData& rhs, 
        const SdfAbstractDataSpecId& id)
    {
        const TfTokenVector lhsFields = lhs.List(id);
        const TfTokenVector rhsFields = rhs.List(id);
        std::set<TfToken> lhsFieldSet( lhsFields.begin(), lhsFields.end() );
        std::set<TfToken> rhsFieldSet( rhsFields.begin(), rhsFields.end() );

        if (lhs.GetSpecType(id) != rhs.GetSpecType(id))
            return false;
        if (lhsFieldSet != rhsFieldSet)
            return false;

        TF_FOR_ALL(field, lhsFields) {
            // Note: this comparison forces manufacturing of VtValues.
            if (lhs.Get(id, *field) != rhs.Get(id, *field))
                return false;
        }

        return true;
    }

private:
    const SdfAbstractData& _rhs;
};

bool 
SdfAbstractData::Equals(const SdfAbstractDataRefPtr &rhs) const
{
    TRACE_FUNCTION();

    // Check that the set of specs matches.
    SdfAbstractData_CheckAllSpecsExist 
        rhsHasAllSpecsInThis(*boost::get_pointer(rhs));
    VisitSpecs(&rhsHasAllSpecsInThis);
    if (!rhsHasAllSpecsInThis.passed)
        return false;

    SdfAbstractData_CheckAllSpecsExist thisHasAllSpecsInRhs(*this);
    rhs->VisitSpecs(&thisHasAllSpecsInRhs);
    if (!thisHasAllSpecsInRhs.passed)
        return false;

    // Check that every spec matches.
    SdfAbstractData_CheckAllSpecsMatch 
        thisSpecsMatchRhsSpecs(*boost::get_pointer(rhs));
    VisitSpecs(&thisSpecsMatchRhsSpecs);
    return thisSpecsMatchRhsSpecs.passed;
}

// Visitor for collecting a sorted set of all paths in an SdfAbstractData.
struct SdfAbstractData_SortedPathCollector : public SdfAbstractDataSpecVisitor
{
    virtual bool VisitSpec(
        const SdfAbstractData& data, const SdfAbstractDataSpecId& id) 
    { 
        paths.insert(id.GetFullSpecPath()); 
        return true; 
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    SdfPathSet paths;
};

void 
SdfAbstractData::WriteToStream(std::ostream& os) const
{
    TRACE_FUNCTION();

    // We sort keys and fields below to ensure a stable output ordering.
    SdfAbstractData_SortedPathCollector collector;
    VisitSpecs(&collector);

    TF_FOR_ALL(i, collector.paths) {
        const SdfAbstractDataSpecId id(&*i);
        const SdfSpecType specType = GetSpecType(id);

        os << *i << " " << TfEnum::GetDisplayName(specType)
            << std::endl;

        const TfTokenVector fields = List(id);
        const std::set<TfToken> fieldSet( fields.begin(), fields.end() );

        TF_FOR_ALL(it, fieldSet) {
            const VtValue value = Get(id, *it);
            os << "    " 
                 << *it << " "
                 << value.GetTypeName() << " " 
                 << value << std::endl;
        }
    }
}

void
SdfAbstractData::VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    if (TF_VERIFY(visitor)) {
        _VisitSpecs(visitor);
        visitor->Done(*this);
    }
}

bool
SdfAbstractData::HasDictKey(const SdfAbstractDataSpecId& id,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            SdfAbstractDataValue* value) const
{
    VtValue tmp;
    bool result = HasDictKey(id, fieldName, keyPath, value ? &tmp : NULL);
    if (result && value) {
        value->StoreValue(tmp);
    }
    return result;
}

bool
SdfAbstractData::HasDictKey(const SdfAbstractDataSpecId& id,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            VtValue *value) const
{
    // Attempt to look up field.
    VtValue dictVal;
    if (Has(id, fieldName, &dictVal) && dictVal.IsHolding<VtDictionary>()) {
        // It's a dictionary -- attempt to find element at keyPath.
        if (VtValue const *v =
            dictVal.UncheckedGet<VtDictionary>().GetValueAtPath(keyPath)) {
            if (value)
                *value = *v;
            return true;
        }
    }
    return false;
}

VtValue
SdfAbstractData::GetDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath) const
{
    VtValue result;
    HasDictKey(id, fieldName, keyPath, &result);
    return result;
}

void
SdfAbstractData::SetDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const VtValue &value)
{
    if (value.IsEmpty()) {
        EraseDictValueByKey(id, fieldName, keyPath);
        return;
    }

    VtValue dictVal = Get(id, fieldName);

    // Swap out existing dictionary (if present).
    VtDictionary dict;
    dictVal.Swap(dict);

    // Now modify dict.
    dict.SetValueAtPath(keyPath, value);

    // Swap it back into the VtValue, and set it.
    dictVal.Swap(dict);
    Set(id, fieldName, dictVal);
}

void
SdfAbstractData::SetDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const SdfAbstractDataConstValue& value)
{
    VtValue vtval;
    value.GetValue(&vtval);
    SetDictValueByKey(id, fieldName, keyPath, vtval);
}

void
SdfAbstractData::EraseDictValueByKey(const SdfAbstractDataSpecId& id,
                                     const TfToken &fieldName,
                                     const TfToken &keyPath)
{
    VtValue dictVal = Get(id, fieldName);

    if (dictVal.IsHolding<VtDictionary>()) {
        // Swap out existing dictionary (if present).
        VtDictionary dict;
        dictVal.Swap(dict);

        // Now modify dict.
        dict.EraseValueAtPath(keyPath);

        // Swap it back into the VtValue, and set it.
        if (dict.empty()) {
            Erase(id, fieldName);
        } else {
            dictVal.Swap(dict);
            Set(id, fieldName, dictVal);
        }
    }
}

std::vector<TfToken>
SdfAbstractData::ListDictKeys(const SdfAbstractDataSpecId& id,
                              const TfToken &fieldName,
                              const TfToken &keyPath) const
{
    vector<TfToken> result;
    VtValue dictVal = GetDictValueByKey(id, fieldName, keyPath);
    if (dictVal.IsHolding<VtDictionary>()) {
        VtDictionary const &dict = dictVal.UncheckedGet<VtDictionary>();
        result.reserve(dict.size());
        TF_FOR_ALL(i, dict)
            result.push_back(TfToken(i->first));
    }
    return result;
}


////////////////////////////////////////////////////////////

SdfAbstractDataSpecVisitor::~SdfAbstractDataSpecVisitor()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
