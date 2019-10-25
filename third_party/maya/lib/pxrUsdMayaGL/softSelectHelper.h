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
#ifndef PXRUSDMAYAGL_SOFT_SELECT_HELPER_H
#define PXRUSDMAYAGL_SOFT_SELECT_HELPER_H

/// \file pxrUsdMayaGL/softSelectHelper.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"

#include "pxr/base/tf/hash.h"

#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MRampAttribute.h>
#include <maya/MString.h>

#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMayaGLSoftSelectHelper
/// \brief Helper class to store soft ("rich") selection state while
/// computing render params for a frame.
///
/// When rendering, we want to be able to draw things that will be influenced by
/// soft selection with a different wireframe.  Querying this maya state is too
/// expensive do in the middle of the render loop so this class lets us compute
/// it once at the beginning of a frame render, and then query it later.
///
/// While this class doesn't have anything particular to rendering, it is only
/// used by the render and is therefore here.  We can move this to usdMaya if
/// we'd like to use it outside of the rendering.
class UsdMayaGLSoftSelectHelper 
{
public:
    PXRUSDMAYAGL_API
    UsdMayaGLSoftSelectHelper();

    /// \brief Clears the saved soft selection state.
    PXRUSDMAYAGL_API
    void Reset();

    /// \brief Repopulates soft selection state
    PXRUSDMAYAGL_API
    void Populate();

    /// \brief Returns true if \p dagPath is in the softSelection.  Also returns
    /// the \p weight.  
    ///
    /// NOTE: until MAYA-73448 (and MAYA-73513) is fixed, the \p weight value is
    /// arbitrary.
    PXRUSDMAYAGL_API
    bool GetWeight(const MDagPath& dagPath, float* weight) const;

    /// \brief Returns true if \p dagPath is in the softSelection.  Also returns
    /// the appropriate color based on the distance/weight and the current soft
    /// select color curve.  It will currently always return (0, 0, 1) at the
    /// moment.
    PXRUSDMAYAGL_API
    bool GetFalloffColor(const MDagPath& dagPath, MColor* falloffColor) const;

private:

    void _PopulateWeights();
    void _PopulateSoftSelectColorRamp();

    struct _MDagPathHash {
        inline size_t operator()(const MDagPath& dagPath) const {
            return TfHash()(std::string(dagPath.fullPathName().asChar()));
        }
    };
    typedef std::unordered_map<MDagPath, float, _MDagPathHash> _MDagPathsToWeights;

    _MDagPathsToWeights _dagPathsToWeight;
    MColor _wireColor;
    bool _populated;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYAGL_SOFT_SELECT_HELPER_H
