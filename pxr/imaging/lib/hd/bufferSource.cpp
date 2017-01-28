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
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/conversions.h"

PXR_NAMESPACE_OPEN_SCOPE


HdBufferSource::~HdBufferSource()
{
}

size_t
HdBufferSource::GetElementSize() const
{
    return GetNumComponents() * GetComponentSize();
}

size_t
HdBufferSource::GetComponentSize() const
{
    return HdConversions::GetComponentSize(GetGLComponentDataType());
}

size_t
HdBufferSource::GetSize() const
{
    return GetNumElements() * GetElementSize();
}

bool
HdBufferSource::HasChainedBuffer() const
{
    return false;
}

HdBufferSourceSharedPtr
HdBufferSource::GetChainedBuffer() const
{
    return HdBufferSourceSharedPtr();
}

bool
HdBufferSource::IsValid() const
{
    return _CheckValid();
}

// ---------------------------------------------------------------------------

TfToken const &
HdComputedBufferSource::GetName() const
{
    if (!_result) {
        static TfToken empty;
        return empty;
    }
    return _result->GetName();
}

void const*
HdComputedBufferSource::GetData() const
{
    if (!_result) {
        TF_CODING_ERROR("HdComputedBufferSource::GetData() called without "
                        "setting the result.");
        return NULL;
    }
    return _result->GetData();
}

int
HdComputedBufferSource::GetGLComponentDataType() const
{
    if (!_result) {
        TF_CODING_ERROR("HdComputedBufferSource::GetGLComponentDataType() "
                        "called without setting the result.");
        return 0;
    }
    return _result->GetGLComponentDataType();
}

int
HdComputedBufferSource::GetGLElementDataType() const
{
    if (!_result) {
        TF_CODING_ERROR("HdComputedBufferSource::GetGLElementDataType() "
                        "called without setting the result.");
        return 0;
    }
    return _result->GetGLElementDataType();
}

int
HdComputedBufferSource::GetNumElements() const
{
    // GetNumElements returns 0 for the empty result.
    return _result ? _result->GetNumElements() : 0;
}

short
HdComputedBufferSource::GetNumComponents() const
{
    // GetNumComponents returns 0 for the empty result.
    return _result ? _result->GetNumComponents() : 0;
}

// ---------------------------------------------------------------------------

TfToken const &
HdNullBufferSource::GetName() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    static TfToken empty;
    return empty;
}

void const*
HdNullBufferSource::GetData() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    return NULL;
}

int
HdNullBufferSource::GetGLComponentDataType() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    return 0;
}

int
HdNullBufferSource::GetGLElementDataType() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    return 0;
}

int
HdNullBufferSource::GetNumElements() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    return 0;
}

short
HdNullBufferSource::GetNumComponents() const
{
    TF_CODING_ERROR("HdNullBufferSource can't be scheduled with a buffer range");
    return 0;
}

void
HdNullBufferSource::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
}

PXR_NAMESPACE_CLOSE_SCOPE

