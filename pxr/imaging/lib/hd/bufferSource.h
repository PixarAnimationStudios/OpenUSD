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
#ifndef HD_BUFFER_SOURCE_H
#define HD_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <atomic>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferSource;
typedef boost::shared_ptr<HdBufferSource> HdBufferSourceSharedPtr;
typedef boost::shared_ptr<HdBufferSource const> HdBufferSourceConstSharedPtr;
typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceVector;
typedef boost::weak_ptr<HdBufferSource> HdBufferSourceWeakPtr;

/// \class HdBufferSource
///
/// A transient buffer of data that has not yet been committed to the GPU.
///
/// HdBufferSource is an abstract interface class, to be registered to the
/// resource registry with the buffer array range that specifies the
/// destination on the GPU memory.
///
/// The public interface provided is intended to be convenient for OpenGL API
/// calls.
///
class HdBufferSource : public boost::noncopyable {
public:
    HdBufferSource() : _state(UNRESOLVED) { }

    HD_API
    virtual ~HdBufferSource();

    /// Return the name of this buffer source.
    virtual TfToken const &GetName() const = 0;

    /// Add the buffer spec for this buffer source into given bufferspec vector.
    /// note: buffer specs has to be determined before the source resolution.
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const = 0;

    /// Prepare the access of GetData(). This process may include some
    /// computations (e.g. cpu smooth normals).
    /// Important notes: Resolve itself doesn't have to be thread safe, but
    /// it will be called in parallel from multiple threads
    /// across buffer sources, so be careful if it uses static/shared
    /// states (including GL calls) among objects.
    /// Returns true if it resolved. If the buffer source has to wait
    /// some results of other buffer sources, or the buffer source is
    /// being resolved by other threads, it returns false.
    virtual bool Resolve() = 0;

    /// Following interfaces will be called after Resolve.

    /// Returns the raw pointer to the underlying data.
    virtual void const* GetData() const = 0;

    /// OpenGL data type; GL_UNSIGNED_INT, etc
    virtual int GetGLComponentDataType() const = 0;

    /// OpenGL data type; GL_FLOAT_VEC3, etc
    virtual int GetGLElementDataType() const = 0;

    /// Returns the number of elements (e.g. VtVec3dArray().GetLength()) from
    /// the source array.
    virtual int GetNumElements() const = 0;

    /// Returns the number of components in a single element.
    ///
    /// For example, for a VtBufferSource created from a VtIntArray, this method
    /// would return 1, but for a VtVec3dArray this method would return 3.
    ///
    /// This value is always in the range [1,4] or 16 (GfMatrix4d).
    virtual short GetNumComponents() const = 0;


    // Following interfaces have default implementations, can be
    // overriden for efficiency.

    /// The size of a single element in the array, e.g. sizeof(GLuint)
    HD_API
    virtual size_t GetElementSize() const;

    /// Returns the size of a single component.
    /// For example: sizeof(GLuint)
    HD_API
    virtual size_t GetComponentSize() const;

    /// Returns the flat array size in bytes.
    HD_API
    virtual size_t GetSize() const;

    /// Returns true it this computation has already been resolved.
    bool IsResolved() const {
        return _state == RESOLVED;
    }

    /// \name Chained Buffers
    /// Buffer sources may be daisy-chained together.
    ///
    /// Pre-chained buffer sources typically represent sources that
    /// are inputs to computed buffer sources (e.g. coarse vertex
    /// privmar data needing to be quadrangulated or refined) and
    /// will be scheduled to be resolved along with their owning
    /// buffer sources.
    ///
    /// Post-chained buffer sources typically represent additional
    /// results produced by a computation (e.g. primitive param data
    /// computed along with index buffer data) and will be scheduled
    /// to be committed along with their owning buffer sources.
    /// @{

    /// Returns true if this buffer has a pre-chained buffer.
    HD_API
    virtual bool HasPreChainedBuffer() const;

    /// Returns the pre-chained buffer.
    HD_API
    virtual HdBufferSourceSharedPtr GetPreChainedBuffer() const;

    /// Returns true if this buffer has a chained buffer.
    HD_API
    virtual bool HasChainedBuffer() const;

    /// Returns the chained buffer.
    HD_API
    virtual HdBufferSourceSharedPtr GetChainedBuffer() const;

    /// @}

    /// Checks the validity of the source buffer.
    /// The function should be called to determine if
    /// AddBufferSpec() and Resolve() would return valid
    /// results.
    HD_API
    bool IsValid() const;

protected:
    /// Marks this buffer source as resolved. It has to be called
    /// at the end of Resolve on concrete implementations.
    void _SetResolved() {
        TF_VERIFY(_state == BEING_RESOLVED);
        _state = RESOLVED;
    }

    /// Non-blocking lock acquisition.
    /// If no one else is resolving this buffer source, returns true.
    /// In that case the caller needs to call _SetResolved at the end
    /// of computation.
    /// It returns false if anyone else has already acquired lock.
    bool _TryLock() {
        State oldState = UNRESOLVED;
        return _state.compare_exchange_strong(oldState, BEING_RESOLVED);
    }

    /// Checks the validity of the source buffer.
    /// This function is called by IsValid() to do the real checking.
    ///
    /// Should only be implemented in classes at leafs of the class hierarchy
    /// (Please place common validation code in a new non-virtual method)
    ///
    /// This code should return false:
    ///   - If the buffer would produce an invalid BufferSpec
    ///   - If a required dependent buffer is invalid
    /// For example, return false when:
    ///   The GL Type or Number of components is invalid causing an invalid
    ///   BufferSpec.
    ///
    ///   The resolve step requires a 'source' buffer and that buffer is invalid.
    ///
    /// If returning false, the buffer will not be registered with the resource
    /// registry.  AddBufferSpec and Resolve will not be called
    virtual bool _CheckValid() const = 0;

private:
    enum State { UNRESOLVED=0, BEING_RESOLVED, RESOLVED };
    std::atomic<State> _state;
};

/// A abstract base class for cpu computation followed by buffer
/// transfer to the GPU.
///
/// concrete class needs to implement
///   virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
///   virtual void Resolve();
/// and set the result via _SetResult().
///
class HdComputedBufferSource : public HdBufferSource {
public:
    HD_API
    virtual TfToken const &GetName() const;
    HD_API
    virtual void const* GetData() const;
    HD_API
    virtual int GetGLComponentDataType() const;
    HD_API
    virtual int GetGLElementDataType() const;
    HD_API
    virtual int GetNumElements() const;
    HD_API
    virtual short GetNumComponents() const;

protected:
    void _SetResult(HdBufferSourceSharedPtr const &result) {
        _result = result;
    }

private:
    HdBufferSourceSharedPtr _result;
};

/// A abstract base class for pure cpu computation.
/// the result won't be scheduled for GPU transfer.
///
class HdNullBufferSource : public HdBufferSource {
public:
    HD_API
    virtual TfToken const &GetName() const;
    HD_API
    virtual void const* GetData() const;
    HD_API
    virtual int GetGLComponentDataType() const;
    HD_API
    virtual int GetGLElementDataType() const;
    HD_API
    virtual int GetNumElements() const;
    HD_API
    virtual short GetNumComponents() const;
    HD_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_BUFFER_SOURCE_H
