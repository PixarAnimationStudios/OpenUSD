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
#include "pxr/imaging/hdEmbree/sampler.h"
#include "pxr/imaging/hd/conversions.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
HdEmbreeBufferSampler::Sample(int index, void* value, int componentType,
                              short numComponents) const
{
    // Sanity checks: index is within the bounds of buffer,
    // and the sample type and buffer type (defined by the componentType
    // and numComponents) are the same.
    if (_buffer.GetNumElements() <= index ||
        _buffer.GetGLComponentDataType() != componentType ||
        _buffer.GetNumComponents() != numComponents) {
        return false;
    }

    // Reconstruct the size of the element type (e.g. GfVec3f) by
    // multiplying the component size (e.g. float) by the component
    // arity (e.g. 3).
    size_t elemSize = HdConversions::GetComponentSize(componentType) *
        numComponents;
    // Calculate the element's byte offset in the array.
    size_t offset = elemSize * index;

    // Equivalent to:
    // *static_cast<ElementType*>(value) =
    //     static_cast<ElementType*>(_buffer.GetData())[index];
    memcpy(value,
        static_cast<const uint8_t*>(_buffer.GetData()) + offset, elemSize);

    return true;
}

template<typename T>
static void
_InterpolateImpl(void* out, void** samples, float* weights,
             size_t sampleCount, short numComponents)
{
    // This is an implementation of a general blend of samples:
    // out = sum_j { sample[j] * weights[j] }.
    // Since the vector length comes in as a parameter, and not part
    // of the type, the blend is implemented per component.
    for (short i = 0; i < numComponents; ++i) {
        static_cast<T*>(out)[i] = 0;
        for (size_t j = 0; j < sampleCount; ++j) {
            static_cast<T*>(out)[i] +=
                static_cast<T*>(samples[j])[i] * weights[j];
        }
    }
}

/* static */ bool
HdEmbreePrimvarSampler::_Interpolate(void* out, void** samples, float* weights,
    size_t sampleCount, int componentType, short numComponents)
{
    // Combine maps from component type tag to C++ type, and delegates to
    // the templated _InterpolateImpl.

    switch(componentType) {
        case GL_BOOL:
            /* This function isn't meaningful on boolean types. */
            return false;
        case GL_BYTE:
            _InterpolateImpl<char>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_SHORT:
            _InterpolateImpl<short>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_UNSIGNED_SHORT:
            _InterpolateImpl<unsigned short>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_INT:
            _InterpolateImpl<int>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_UNSIGNED_INT:
            _InterpolateImpl<unsigned int>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_FLOAT:
            _InterpolateImpl<float>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        case GL_DOUBLE:
            _InterpolateImpl<double>(out, samples, weights, sampleCount,
                numComponents);
            return true;
        default:
            TF_CODING_ERROR("Unsupported type passed to _Interpolate");
            return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
