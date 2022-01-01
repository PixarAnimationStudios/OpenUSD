//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_BINDING_H
#define PXR_IMAGING_PLUGIN_LOFI_BINDING_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<class LoFiBinding> LoFiBindingVector;
typedef std::vector<class LoFiBindingRequest> LoFiBindingRequestVector;

/// \class LoFiBinding
///
/// Bindings are used for buffers or textures, it simple associates a binding
/// type with a binding location.
///
class LoFiBinding {
public:
    enum Type { 
                UNKNOWN,
                VERTEX_ATTR,
                INDEX_ATTR,
                SSBO,
                UBO,
                UNIFORM,
                UNIFORM_ARRAY,
                TBO,
                TEXTURE_2D,
                TEXTURE_3D
    };
    enum Location {
                NOT_EXIST = 0xffff
    };
    LoFiBinding() : _typeAndLocation(-1) { }
    LoFiBinding(Type type, int location, int textureUnit=0) 
    {
        Set(type, location, textureUnit);
    }
    void Set(Type type, int location, int textureUnit) 
    {
        _typeAndLocation = (textureUnit << 24)|(location << 8)|(int)(type);
    }
    bool IsValid() const { return _typeAndLocation >= 0; }
    Type GetType() const { return (Type)(_typeAndLocation & 0xff); }
    int GetLocation() const { return (_typeAndLocation >> 8) & 0xffff; }
    int GetTextureUnit() const { return (_typeAndLocation >> 24) & 0xff; }
    int GetValue() const { return _typeAndLocation; }
    bool operator < (LoFiBinding const &b) const {
        return (_typeAndLocation < b._typeAndLocation);
    }
private:
    int _typeAndLocation;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_BINDING_H