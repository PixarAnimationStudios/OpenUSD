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

PXR_NAMESPACE_OPEN_SCOPE


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

    // associated glsl program
    void SetGLSLProgram(LoFiGLSLProgram* program) {
      _program = program;
    }
    const LoFiGLSLProgram* GetGLSLProgram() const {return _program;};

    inline void SetBufferArrayHash(size_t hash){ _hash = hash;};

protected:
    
    size_t _GetBufferArraysHash() const;

private:
    // vertex array hash to get it backfrom registry
    size_t              _hash;
    LoFiVertexArray*    _vertexArray;
    LoFiGLSLProgram*    _program;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_DRAW_ITEM_H
