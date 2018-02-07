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
#ifndef HD_BINDING_H
#define HD_BINDING_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hd/bufferResource.h"
#include "pxr/imaging/hd/bufferArrayRange.h"

PXR_NAMESPACE_OPEN_SCOPE


typedef std::vector<class HdBinding> HdBindingVector;
typedef std::vector<class HdBindingRequest> HdBindingRequestVector;

/// \class HdBinding
///
/// Bindings are used for buffers or textures, it simple associates a binding
/// type with a binding location.
///
class HdBinding {
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
                UBO,                 //
                BINDLESS_UNIFORM,    //
                UNIFORM,             //
                UNIFORM_ARRAY,       //
                TBO,                 //

                // shader parameter bindings
                FALLBACK,                     // fallback value
                TEXTURE_2D,          // non-bindless uv texture
                TEXTURE_PTEX_TEXEL,  // non-bindless ptex texels
                TEXTURE_PTEX_LAYOUT, // non-bindless ptex layout
                BINDLESS_TEXTURE_2D,          // bindless uv texture
                BINDLESS_TEXTURE_PTEX_TEXEL,  // bindless ptex texels
                BINDLESS_TEXTURE_PTEX_LAYOUT, // bindless ptex layout
                PRIMVAR_REDIRECT     // primvar redirection
    };
    enum Location {
                // NOT_EXIST is a special value of location for a uniform
                // which is assigned but optimized out after linking program.
                NOT_EXIST = 0xffff
    };
    HdBinding() : _typeAndLocation(-1) { }
    HdBinding(Type type, int location, int textureUnit=0) {
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
    bool operator < (HdBinding const &b) const {
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
class HdBindingRequest {
public:

    HdBindingRequest() = default;

    /// A data binding, not backed by neither BufferArrayRange nor
    /// BufferResource.  This binding request simply
    /// generates named metadata (#define HD_HAS_foo 1, #define HD_foo_Binding)
    HdBindingRequest(HdBinding::Type bindingType, TfToken const& name)
        : _bindingType(bindingType)
        , _dataType(HdTypeInvalid)
        , _name(name)
        , _resource(nullptr)
        , _bar(nullptr)
        , _isInterleaved(false)
    {}

    /// A data binding, not backed by neither BufferArrayRange nor
    /// BufferResource.
    HdBindingRequest(HdBinding::Type bindingType, TfToken const& name,
                     HdType dataType)
        : _bindingType(bindingType)
        , _dataType(dataType)
        , _name(name)
        , _resource(nullptr)
        , _bar(nullptr)
        , _isInterleaved(false)
    {}

    /// A buffer resource binding. Binds a given buffer resource to a specified
    /// name.  The data type is set from the resource.
    HdBindingRequest(HdBinding::Type bindingType, TfToken const& name,
                     HdBufferResourceSharedPtr const& resource)
        : _bindingType(bindingType)
        , _dataType(resource->GetTupleType().type)
        , _name(name)
        , _resource(resource)
        , _bar(nullptr)
        , _isInterleaved(false)
    {}

    /// A named struct binding. From an interleaved BufferArray, an array of
    /// structs will be generated, consuming a single binding point. Note that
    /// all resources in the buffer array must have the same underlying
    /// identifier, hence must be interleaved and bindable as a single resource.
    /// Data types can be derived from each HdBufferResource of bar.
    HdBindingRequest(HdBinding::Type type, TfToken const& name,
                    HdBufferArrayRangeSharedPtr bar,
                    bool interleave)
        : _bindingType(type)
        , _dataType(HdTypeInvalid)
        , _name(name)
        , _resource(nullptr)
        , _bar(bar)
        , _isInterleaved(interleave)
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
    /// Returns the HdBinding type of this request.
    HdBinding::Type GetBindingType() const {
        return _bindingType;
    }
    /// Returns the single resource associated with this binding request or
    /// null when IsResource() returns false.
    HdBufferResourceSharedPtr const& GetResource() const {
        return _resource;
    }
    /// Returns the resource or buffer array range offset, defaults to zero.
    int GetOffset() const {
        if (_resource) return _resource->GetOffset();
        if (_bar) return _bar->GetOffset();
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

    /// Returns the hash corresponding to this buffer request.
    ///
    /// Note that this hash captures the structural state of the request, not
    /// the contents. For example, buffer array versions/reallocations will not
    /// affect hash, but changing the BAR pointer will.
    HD_API
    size_t ComputeHash() const;

private:
    // This class unfortunately represents several concepts packed into a single
    // class.  Ideally, we would break this out as one class per concept,
    // however that would also require virtual dispatch, which is overkill for
    // the current use cases.

    // Named binding request
    HdBinding::Type _bindingType;
    HdType _dataType;
    TfToken _name;

    // Resource binding request
    HdBufferResourceSharedPtr _resource;

    // Struct binding request
    HdBufferArrayRangeSharedPtr _bar;
    bool _isInterleaved;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_BINDING_H
