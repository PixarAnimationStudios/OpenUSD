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

#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/vtExtractor.h"

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
 , _glComponentDataType(0)
 , _glElementDataType(0)
 , _rawDataPtr(nullptr)
{
    Hd_VtExtractor extractor;

    extractor.Extract(defaultValue);

    _glComponentDataType = extractor.GetGLCompontentType();
    _glElementDataType   = extractor.GetGLElementType();
    _numComponents       = extractor.GetNumComponents();

    _sourceOutputIdx = source->GetOutputIndex(sourceOutputName);
}

TfToken const &
HdExtCompPrimvarBufferSource::GetName() const
{
    return _primvarName;
}

void
HdExtCompPrimvarBufferSource::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(_primvarName,
                        _glComponentDataType,
                        _numComponents,
                        _source->GetNumElements());
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



    Hd_VtExtractor extractor;
    extractor.Extract(output);

    // Validate output type matches what is expected.
    if ((_glComponentDataType != extractor.GetGLCompontentType()) ||
        (_glElementDataType   != extractor.GetGLElementType())    ||
        (_numComponents       != extractor.GetNumComponents())) {
        TF_WARN("Output type mismatch on %s. ", _primvarName.GetText());
        _SetResolveError();
        return true;
    }

    // Validate output size matches what is expected.
    size_t expectedSize = GetNumElements()  * _numComponents *
                          HdConversions::GetComponentSize(_glComponentDataType);
    if ( extractor.GetSize() !=  expectedSize) {
        TF_WARN("Not enough data for primvar %s. ", _primvarName.GetText());
        _SetResolveError();
        return true;
    }

    _rawDataPtr = extractor.GetData();

    _SetResolved();
    return true;
}


void const *
HdExtCompPrimvarBufferSource::GetData() const
{
    return _rawDataPtr;
}


int
HdExtCompPrimvarBufferSource::GetGLComponentDataType() const
{
    return _glComponentDataType;
}


int
HdExtCompPrimvarBufferSource::GetGLElementDataType() const
{
    return _glElementDataType;
}


int
HdExtCompPrimvarBufferSource::GetNumElements() const
{
    return _source->GetNumElements();
}


short
HdExtCompPrimvarBufferSource::GetNumComponents() const
{
    return _numComponents;
}


bool
HdExtCompPrimvarBufferSource::_CheckValid() const
{
    return (_source &&
            (_sourceOutputIdx !=
                               HdExtCompCpuComputation::INVALID_OUTPUT_INDEX) &&
            (_numComponents > 0) &&
            (_glComponentDataType > 0) &&
            (_glElementDataType > 0));
}

PXR_NAMESPACE_CLOSE_SCOPE
