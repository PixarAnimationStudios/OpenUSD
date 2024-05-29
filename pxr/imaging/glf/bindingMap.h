//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_BINDING_MAP_H
#define PXR_IMAGING_GLF_BINDING_MAP_H

/// \file glf/bindingMap.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE


class GlfBindingMap : public TfRefBase, public TfWeakBase {
public:
    typedef TfHashMap<TfToken, int, TfToken::HashFunctor> BindingMap;

    GlfBindingMap()
      : _samplerBindingBaseIndex(0)
      , _uniformBindingBaseIndex(0)
      { }

    GLF_API
    int GetSamplerUnit(std::string const &name);
    GLF_API
    int GetSamplerUnit(TfToken const & name);

    // If GetAttributeIndex is called with an unknown
    // attribute token they return -1
    GLF_API
    int GetAttributeIndex(std::string const & name);
    GLF_API
    int GetAttributeIndex(TfToken const & name);

    GLF_API
    int GetUniformBinding(std::string const & name);
    GLF_API
    int GetUniformBinding(TfToken const & name);

    GLF_API
    bool HasUniformBinding(std::string const & name) const;
    GLF_API
    bool HasUniformBinding(TfToken const & name) const;

    int GetNumSamplerBindings() const {
        return (int)_samplerBindings.size();
    }

    void ClearAttribBindings() {
        _attribBindings.clear();
    }

    /// \name Sampler and UBO Bindings
    ///
    /// Sampler units and uniform block bindings are reset and will be
    /// assigned sequentially starting from the specified baseIndex.
    /// This allows other subsystems to claim sampler units and uniform
    /// block bindings before additional indices are assigned by this
    /// binding map.
    ///
    /// @{

    void ResetSamplerBindings(int baseIndex) {
        _samplerBindings.clear();
        _samplerBindingBaseIndex = baseIndex;
    }

    void ResetUniformBindings(int baseIndex) {
        _uniformBindings.clear();
        _uniformBindingBaseIndex = baseIndex;
    }

    /// @}

    void AddAttribBinding(TfToken const &name, int location) {
        _attribBindings[name] = location;
    }

    BindingMap const &GetAttributeBindings() const {
        return _attribBindings;
    }

    GLF_API
    void AssignSamplerUnitsToProgram(GLuint program);

    GLF_API
    void AssignUniformBindingsToProgram(GLuint program);

    GLF_API
    void AddCustomBindings(GLuint program);

    GLF_API
    void Debug() const;

private:
    void _AddActiveAttributeBindings(GLuint program);
    void _AddActiveUniformBindings(GLuint program);
    void _AddActiveUniformBlockBindings(GLuint program);

    BindingMap _attribBindings;
    BindingMap _samplerBindings;
    BindingMap _uniformBindings;

    int _samplerBindingBaseIndex;
    int _uniformBindingBaseIndex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GLF_BINDING_MAP_H
