//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDST_BINDING_H
#define PXR_IMAGING_HDST_BINDING_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferResource.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE


using HdStBindingVector = std::vector<class HdStBinding>;
using HdStBindingRequestVector = std::vector<class HdStBindingRequest>;

/// \class HdStBinding
///
/// Bindings are used for buffers or textures, it simply associates a binding
/// type with a binding location.
///
class HdStBinding {
public:
    enum Type { // primvar, drawing coordinate and dispatch buffer bindings
                // also shader fallback values
                UNKNOWN,
                DISPATCH,            // GL_DRAW_INDIRECT_BUFFER
                DRAW_INDEX,          // per-drawcall. not instanced
                DRAW_INDEX_INSTANCE, // per-drawcall. attribdivisor=on
                DRAW_INDEX_INSTANCE_ARRAY, // per-drawcall. attribdivisor=on, array
                VERTEX_ATTR,         // vertex-attribute
                INDEX_ATTR,          // GL_ELEMENT_ARRAY_BUFFER
                SSBO,                //
                BINDLESS_SSBO_RANGE, //
                UBO,                 //
                BINDLESS_UNIFORM,    //
                UNIFORM,             //
                UNIFORM_ARRAY,       //

                // shader parameter bindings
                FALLBACK,             // fallback value
                TEXTURE_2D,           // non-bindless uv texture
                ARRAY_OF_TEXTURE_2D,  // non-bindless array of uv textures. Not 
                                      // to be confused with a texture array 
                                      // (what udim and ptex textures use).
                TEXTURE_FIELD,        // non-bindless field texture
                                      // creates accessor that samples uvw
                                      // texture after transforming coordinates
                                      // by a sampling transform
                TEXTURE_UDIM_ARRAY,   // non-bindless udim texture array
                TEXTURE_UDIM_LAYOUT,  // non-bindless udim layout
                TEXTURE_PTEX_TEXEL,   // non-bindless ptex texels
                TEXTURE_PTEX_LAYOUT,  // non-bindless ptex layout
                BINDLESS_TEXTURE_2D,          // bindless uv texture
                BINDLESS_ARRAY_OF_TEXTURE_2D, // bindless array of uv textures
                BINDLESS_TEXTURE_FIELD,       // bindless field texture
                                              // (see above)
                BINDLESS_TEXTURE_UDIM_ARRAY,  // bindless uv texture array
                BINDLESS_TEXTURE_UDIM_LAYOUT, // bindless udim layout
                BINDLESS_TEXTURE_PTEX_TEXEL,  // bindless ptex texels
                BINDLESS_TEXTURE_PTEX_LAYOUT, // bindless ptex layout
                PRIMVAR_REDIRECT,    // primvar redirection
                FIELD_REDIRECT, // accesses a field texture by name and
                                // uses fallbackValue if no accessor for
                                // the texture exists.
                TRANSFORM_2D    // transform2d
    };
    enum Location {
                // NOT_EXIST is a special value of location for a uniform
                // which is assigned but optimized out after linking program.
                NOT_EXIST = 0xffff
    };
    HdStBinding() : _typeAndLocation(-1) { }
    HdStBinding(Type type, int location, int textureUnit=0) {
        Set(type, location, textureUnit);
    }
    void Set(Type type, int location, int textureUnit) {
        _typeAndLocation = (textureUnit << 24)|(location << 8)|(int)(type);
    }
    bool IsValid() const { return _typeAndLocation >= 0; }
    Type GetType() const { return (Type)(_typeAndLocation & 0xff); }
    int GetLocation() const { return (_typeAndLocation >> 8) & 0xffff; }
    int GetTextureUnit() const { return (_typeAndLocation >> 24) & 0xff; }
    int GetValue() const { return _typeAndLocation; }
    bool operator < (HdStBinding const &b) const {
        return (_typeAndLocation < b._typeAndLocation);
    }
private:
    int _typeAndLocation;
};

/// BindingRequest allows externally allocated buffers to be bound at render
/// time. The different modes of binding discussed below allow the caller a
/// range of opt-in binding behaviors, from simply reserving a binding location
/// so it can be managed from client code, to fully generating buffer accessor 
/// code at compile time (i.e. when using a BufferArrayRange or BufferResource).
///
/// This is a "request" because the caller makes a request before bindings are
/// resolved. All requests are consulted and fulfilled during binding
/// resolution.
class HdStBindingRequest {
public:

    HdStBindingRequest() = default;

    /// A data binding, not backed by neither BufferArrayRange nor
    /// BufferResource.  This binding request simply
    /// generates named metadata (#define HD_HAS_foo 1, #define HD_foo_Binding)
    HdStBindingRequest(HdStBinding::Type bindingType, TfToken const& name)
        : _bindingType(bindingType)
        , _dataType(HdTypeInvalid)
        , _name(name)
        , _resource(nullptr)
        , _bar(nullptr)
        , _isInterleaved(false)
        , _isWritable(false)
        , _arraySize(0)
        , _concatenateNames(false)
    {}

    /// A data binding, not backed by neither BufferArrayRange nor
    /// BufferResource.
    HdStBindingRequest(HdStBinding::Type bindingType, TfToken const& name,
                       HdType dataType)
        : _bindingType(bindingType)
        , _dataType(dataType)
        , _name(name)
        , _resource(nullptr)
        , _bar(nullptr)
        , _isInterleaved(false)
        , _isWritable(false)
        , _arraySize(0)
        , _concatenateNames(false)
    {}

    /// A buffer resource binding. Binds a given buffer resource to a specified
    /// name.  The data type is set from the resource.
    HdStBindingRequest(HdStBinding::Type bindingType, TfToken const& name,
                       HdStBufferResourceSharedPtr const& resource)
        : _bindingType(bindingType)
        , _dataType(resource->GetTupleType().type)
        , _name(name)
        , _resource(resource)
        , _bar(nullptr)
        , _isInterleaved(false)
        , _isWritable(false)
        , _arraySize(0)
        , _concatenateNames(false)
    {}

    /// A named struct binding. From an interleaved BufferArray, an array of
    /// structs will be generated, consuming a single binding point. Note that
    /// all resources in the buffer array must have the same underlying
    /// identifier, hence must be interleaved and bindable as a single resource.
    /// Data types can be derived from each HdStBufferResource of bar.
    HdStBindingRequest(HdStBinding::Type type, TfToken const& name,
                       HdBufferArrayRangeSharedPtr bar,
                       bool interleave, bool writable = false,
                       size_t arraySize = 0, bool concatenateNames = false)
        : _bindingType(type)
        , _dataType(HdTypeInvalid)
        , _name(name)
        , _resource(nullptr)
        , _bar(bar)
        , _isInterleaved(interleave)
        , _isWritable(writable)
        , _arraySize(arraySize)
        , _concatenateNames(concatenateNames)
    {}

    // ---------------------------------------------------------------------- //
    /// \name Discriminators 
    // ---------------------------------------------------------------------- //

    /// Resource bingings have a single associated Hydra resource, but no buffer
    /// array.
    bool IsResource() const {
        return bool(_resource);
    }

    /// A buffer array binding has several buffers bundled together and each
    /// buffer will be bound individually and exposed as independent arrays in
    /// the shader.
    bool IsBufferArray() const {
        return _bar && !_isInterleaved;
    }

    /// Like BufferArray binding requests, struct bindings have several buffers,
    /// however they must be allocated into a single resource and interleaved.
    /// This type of binding request is exposed in the shader an array of
    ///  structs.
    bool IsInterleavedBufferArray() const {
        return _bar && _isInterleaved;
    }

    /// True when the resource is being bound so that it can be written to.
    /// This affects whether it will be declared 'const' or not.
    bool isWritable() const {
        return _bar && _isWritable;
    }

    /// This binding is typelss. CodeGen only allocate location and
    /// skip emitting declarations and accessors.
    bool IsTypeless() const {
        return (!_bar) && (!_resource) && (_dataType == HdTypeInvalid);
    }

    // ---------------------------------------------------------------------- //
    /// \name Accessors
    // ---------------------------------------------------------------------- //
    
    /// Returns the name of the binding point, if any; buffer arrays and structs
    /// need not be named.
    TfToken const& GetName() const {
        return _name;
    }
    /// Returns the HdStBinding type of this request.
    HdStBinding::Type GetBindingType() const {
        return _bindingType;
    }
    /// Returns the single resource associated with this binding request or
    /// null when IsResource() returns false.
    HdStBufferResourceSharedPtr const& GetResource() const {
        return _resource;
    }
    /// Returns the resource or buffer array range offset, defaults to zero.
    int GetByteOffset() const {
        // buffer resource binding
        if (_resource) return _resource->GetOffset();

        // named struct binding (interleaved) - the resource name doesn't matter
        // since a single binding point is used.
        if (_bar) return _bar->GetByteOffset(TfToken());
        return 0;
    }
    /// Returns the buffer array range associated with this binding request or
    /// null when IsBufferArrqay() returns false.
    HdBufferArrayRangeSharedPtr const& GetBar() const {
        return _bar;
    }

    /// Return the data type of this request
    HdType GetDataType() const {
        return _dataType;
    }

    /// Array size if request is for an array of structs. 
    size_t GetArraySize() const {
        return _arraySize;
    }

    /// Returns whether the struct binding point and struct member names 
    /// should be concatenated when codegen'ing the accessor.
    bool ConcatenateNames() const {
        return _concatenateNames;
    }

    // ---------------------------------------------------------------------- //
    /// \name Comparison
    // ---------------------------------------------------------------------- //
    HDST_API
    bool operator==(HdStBindingRequest const &other) const;

    HDST_API
    bool operator!=(HdStBindingRequest const &other) const;

    // ---------------------------------------------------------------------- //
    /// \name Hash
    // ---------------------------------------------------------------------- //

    /// Returns the hash corresponding to this buffer request.
    ///
    /// Note that this hash captures the structural state of the request, not
    /// the contents. For example, buffer array versions/reallocations will not
    /// affect hash, but changing the BAR pointer will.
    HDST_API
    size_t ComputeHash() const;

    // TfHash support.
    template <class HashState>
    friend void TfHashAppend(HashState &h, HdStBindingRequest const &br) {
        h.Append(br._name,
                 br._bindingType,
                 br._dataType,
                 br._isInterleaved);
    }

private:
    // This class unfortunately represents several concepts packed into a single
    // class.  Ideally, we would break this out as one class per concept,
    // however that would also require virtual dispatch, which is overkill for
    // the current use cases.

    // Named binding request
    HdStBinding::Type _bindingType;
    HdType _dataType;
    TfToken _name;

    // Resource binding request
    HdStBufferResourceSharedPtr _resource;

    // Struct binding request
    HdBufferArrayRangeSharedPtr _bar;
    bool _isInterleaved;

    bool _isWritable;

    size_t _arraySize;

    bool _concatenateNames;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HDST_BINDING_H
