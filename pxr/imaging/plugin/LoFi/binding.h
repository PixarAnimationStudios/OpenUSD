//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_BINDING_H
#define PXR_IMAGING_PLUGIN_LOFI_BINDING_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<class LoFiBinding> LoFiBindingVector;

class LoFiDrawItem;

/// \class LoFiBinding
///
class LoFiBinding {
public:
    enum Type { 
      UNKNOWN,
      VERTEX_ATTR,
      INDEX_ATTR,
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

class LoFiUniformBinding : public LoFiBinding {
public:
  LoFiUniformBinding(const TfToken& name, const TfToken& type, size_t arraySize=0)
    : _name(name), _type(type), _arraySize(arraySize){};
  ~LoFiUniformBinding(){};
  TfToken& GetName(){return _name;};
  TfToken& GetType(){return _type;};
  size_t GetArraySize(){return _arraySize;};
private:
  TfToken _name;
  TfToken _type;
  size_t _arraySize;

};

typedef std::vector<LoFiUniformBinding> LoFiUniformBindingList;

class LoFiVertexBufferBinding : public LoFiBinding {
public:
  LoFiVertexBufferBinding(const TfToken& name, const TfToken& type)
    : _name(name), _type(type){};
  ~LoFiVertexBufferBinding(){};
  TfToken& GetName(){return _name;};
  TfToken& GetType(){return _type;};
private:
  TfToken _name;
  TfToken _type;
};

typedef std::vector<LoFiVertexBufferBinding> LoFiVertexBufferBindingList;




PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_BINDING_H