//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hd/extCompPrimvarBufferSource.h"

#include "pxr/imaging/hd/extCompCpuComputation.h"

#include <limits>
PXR_NAMESPACE_OPEN_SCOPE

HdExtCompPrimvarBufferSource::HdExtCompPrimvarBufferSource(
                                 const TfToken &primvarName,
                                 const HdExtCompCpuComputationSharedPtr &source,
                                 const TfToken &sourceOutputName,
                                 const VtValue &defaultValue)
 : HdBufferSource()
 , _primvarName(primvarName)
 , _source(source)
 , _sourceOutputIdx(HdExtCompCpuComputation::INVALID_OUTPUT_INDEX)
 , _tupleType(HdGetValueTupleType(defaultValue))
 , _rawDataPtr(nullptr)
{
    _sourceOutputIdx = source->GetOutputIndex(sourceOutputName);

    // For the common case of a default value that is an empty
    // VtArray<T>, treat it as representing one T per element,
    // rather than a zero-sized tuple.
    if (_tupleType.count == 0) {
        _tupleType.count = 1;
    }
}

TfToken const &
HdExtCompPrimvarBufferSource::GetName() const
{
    return _primvarName;
}

void
HdExtCompPrimvarBufferSource::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(_primvarName, _tupleType);
}

bool
HdExtCompPrimvarBufferSource::Resolve()
{
    bool sourceValid = _source->IsValid();
    if (sourceValid) {
        if (!_source->IsResolved()) {
            return false;
        }
    }

    if (!_TryLock()) return false;

    if (!sourceValid || _source->HasResolveError()) {
        _SetResolveError();
        return true;
    }

    VtValue output = _source->GetOutputByIndex(_sourceOutputIdx);

    // Validate output type and count matches what is expected.
    {
        HdTupleType outputTupleType = HdGetValueTupleType(output);
        if (_source->GetNumElements() > 0) {
            // Adjust tuple size to account for distribution over elements.
            outputTupleType.count /= _source->GetNumElements();
        }
        if (outputTupleType != _tupleType) {
            TF_WARN("Output type mismatch on %s. ", _primvarName.GetText());
            _SetResolveError();
            return true;
        }
    }

    _rawDataPtr = HdGetValueData(output);

    _SetResolved();
    return true;
}


void const *
HdExtCompPrimvarBufferSource::GetData() const
{
    return _rawDataPtr;
}

HdTupleType
HdExtCompPrimvarBufferSource::GetTupleType() const
{
    return _tupleType;
}

int
HdExtCompPrimvarBufferSource::GetNumElements() const
{
    return _source->GetNumElements();
}

bool
HdExtCompPrimvarBufferSource::_CheckValid() const
{
    return (_source &&
            (_sourceOutputIdx !=
                               HdExtCompCpuComputation::INVALID_OUTPUT_INDEX) &&
            (_tupleType.type != HdTypeInvalid));
}

PXR_NAMESPACE_CLOSE_SCOPE
