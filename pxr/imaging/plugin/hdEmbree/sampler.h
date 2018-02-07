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
#ifndef HDEMBREE_SAMPLER_H
#define HDEMBREE_SAMPLER_H

#include "pxr/pxr.h"
#include <cstddef>

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeTypeHelper
///
/// A utility class that helps map between C++ types and Hd type tags.
class HdEmbreeTypeHelper {
public:
    /// Return the HdTupleType corresponding to the given C++ type.
    template<typename T>
    static HdTupleType GetTupleType();

    /// Define a type that can hold one sample of any primvar.
    typedef char PrimvarTypeContainer[sizeof(GfMatrix4d)];
};

// Define template specializations of HdEmbreeTypeHelper methods for
// all our supported types...
#define TYPE_HELPER(T,type)\
template<> inline HdTupleType \
HdEmbreeTypeHelper::GetTupleType<T>() { return HdTupleType{type, 1}; }

    TYPE_HELPER(bool, HdTypeBool)
    TYPE_HELPER(char, HdTypeInt8)
    TYPE_HELPER(short, HdTypeInt16)
    TYPE_HELPER(unsigned short, HdTypeUInt16)
    TYPE_HELPER(int, HdTypeInt32)
    TYPE_HELPER(GfVec2i, HdTypeInt32Vec2)
    TYPE_HELPER(GfVec3i, HdTypeInt32Vec3)
    TYPE_HELPER(GfVec4i, HdTypeInt32Vec4)
    TYPE_HELPER(unsigned int, HdTypeUInt32)
    TYPE_HELPER(float, HdTypeFloat)
    TYPE_HELPER(GfVec2f, HdTypeFloatVec2)
    TYPE_HELPER(GfVec3f, HdTypeFloatVec3)
    TYPE_HELPER(GfVec4f, HdTypeFloatVec4)
    TYPE_HELPER(double, HdTypeDouble)
    TYPE_HELPER(GfVec2d, HdTypeDoubleVec2)
    TYPE_HELPER(GfVec3d, HdTypeDoubleVec3)
    TYPE_HELPER(GfVec4d, HdTypeDoubleVec4)
    TYPE_HELPER(GfMatrix4f, HdTypeFloatMat4)
    TYPE_HELPER(GfMatrix4d, HdTypeDoubleMat4)
#undef TYPE_HELPER

/// \class HdEmbreeBufferSampler
///
/// A utility class that knows how to sample an element from a type-tagged
/// buffer (like HdVtBufferSource).
///
/// This class provides templated accessors to let the caller directly get the
/// final sample type; it also does bounds checks and type checks.
///
class HdEmbreeBufferSampler {
public:
    /// The constructor takes a reference to a buffer source. The data is
    /// owned externally; the caller is responsible for ensuring the buffer
    /// is alive while Sample() is being called.
    /// \param buffer The buffer being sampled.
    HdEmbreeBufferSampler(HdVtBufferSource const& buffer)
        : _buffer(buffer) {}

    /// Sample the buffer at element index \p index, and write the sample to
    /// \p value. Interpret \p value as having arity \p numComponents, each of
    /// type \p componentType. These parameters may not match the datatype
    /// declaration of the underlying buffer, in which case Sample returns
    /// false. Sample also returns false if \p index is out of bounds.
    ///
    /// For example, to sample data as GfVec3, \p dataType would be
    /// HdTupleType { HdTypeFloatVec3, 1 }.
    ///
    /// \param index The element index to sample.
    /// \param value The memory to write the value to (only written on success).
    /// \param dataType The HdTupleType describing element values.
    /// \return True if the value was successfully sampled.
    bool Sample(int index, void* value, HdTupleType dataType) const;

    // Convenient, templated frontend for Sample().
    template<typename T> bool Sample(int index, T* value) const {
        return Sample(index, static_cast<void*>(value),
            HdEmbreeTypeHelper::GetTupleType<T>());
    }

private:
    HdVtBufferSource const& _buffer;
};

/// \class HdEmbreePrimvarSampler
///
/// An abstract base class that knows how to sample a primvar signal given
/// a ray hit coordinate: an <element, u, v> tuple. It provides templated
/// accessors, but derived classes are responsible for implementing appropriate
/// sampling or interpolation modes.
class HdEmbreePrimvarSampler {
public:
    /// Default constructor.
    HdEmbreePrimvarSampler() = default;
    /// Default destructor.
    virtual ~HdEmbreePrimvarSampler() = default;

    /// Sample the primvar at element index \p index and local basis coordinates
    /// \p u and \p v, writing the sample to \p value.  Interpret \p value as
    /// having arity \p numComponents, each of type \p componentType. These
    /// parameters may not match the datatype declaration of the underlying
    /// buffer.
    ///
    /// Derived classes are responsible for implementing sampling logic for
    /// their particular interpolation modes. Sample returns true if a value
    /// was successfully retrieved.
    ///
    /// \param element The element index to sample.
    /// \param u The u coordinate to sample.
    /// \param v The v coordinate to sample.
    /// \param value The memory to write the value to (only written on success).
    /// \param dataType The HdTupleType describing element values.
    /// \return True if the value was successfully sampled.
    virtual bool Sample(unsigned int element, float u, float v, void* value,
                        HdTupleType dataType) const = 0;

    // Convenient, templated frontend for Sample().
    template<typename T> bool Sample(unsigned int element, float u, float v,
                                     T* value) const {
        return Sample(element, u, v, static_cast<void*>(value),
            HdEmbreeTypeHelper::GetTupleType<T>());
    }

protected:
    /// Utility function for derived classes: combine multiple samples with
    /// blend weights: \p out = sum_i { \p samples[i] * \p weights[i] }.
    /// \param out The memory to write the output to (only written on success).
    /// \param samples The array of sample pointers (length \p sampleCount).
    /// \param weights The array of sample weights (length \p sampleCount).
    /// \param sampleCount The number of samples to combine.
    /// \param dataType The HdTupleType describing element values.
    /// \return True if the samples were successfully combined.
    static bool _Interpolate(void* out, void** samples, float* weights,
        size_t sampleCount, HdTupleType dataType);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_SAMPLER_H
