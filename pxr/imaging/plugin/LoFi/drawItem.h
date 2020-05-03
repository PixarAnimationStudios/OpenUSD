//
// Copyright 2020 benmalartre
//
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_ITEM_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_ITEM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/plugin/LoFi/binding.h"
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiBinder;
/// \class LoFiDrawItem
///
class LoFiDrawItem : public HdDrawItem  {
public:
  HF_MALLOC_TAG_NEW("new LoFiDrawItem");

  LoFiDrawItem(HdRprimSharedData const *sharedData);
  
  virtual ~LoFiDrawItem();

  // Associated vertex array
  void SetVertexArray(LoFiVertexArray* vertexArray) {
    _vertexArray = vertexArray;
  }
  const LoFiVertexArray* GetVertexArray() const {return _vertexArray;};

  LoFiBinder* Binder(){return &_binder;};
  const LoFiBinder* GetBinder() const {return &_binder;};

  // associated glsl program
  void SetGLSLProgram(LoFiGLSLProgramSharedPtr program) {
    _program = program;
  }
  LoFiGLSLProgramSharedPtr GetGLSLProgram() const {return _program;};

  inline void SetBufferArrayHash(size_t hash){ _hash = hash;};

  void PopulateInstancesXforms(const VtArray<GfMatrix4d>& xforms);
  const VtArray<GfMatrix4f>& GetInstancesXforms() const {return _instancesXform;};

protected:
    
  size_t _GetBufferArraysHash() const;

private:
  // vertex array hash to get it backfrom registry
  size_t                      _hash;
  LoFiVertexArray*            _vertexArray;
  LoFiGLSLProgramSharedPtr    _program;
  LoFiBinder                  _binder;
  VtArray<GfMatrix4f>         _instancesXform;

};

typedef std::set<const LoFiDrawItem*> LoFiDrawItemPtrSet;

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_DRAW_ITEM_H
