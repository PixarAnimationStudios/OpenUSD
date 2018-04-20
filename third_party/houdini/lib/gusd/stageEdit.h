//
// Copyright 2017 Pixar
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
#ifndef _GUSD_STAGEEDIT_H_
#define _GUSD_STAGEEDIT_H_

#include "gusd/api.h"

#include <pxr/pxr.h>
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <UT/UT_Array.h>
#include <UT/UT_Error.h>
#include <UT/UT_IntrusivePtr.h>


PXR_NAMESPACE_OPEN_SCOPE


using GusdStageEditPtr = UT_IntrusivePtr<class GusdStageEdit>;


/// Object defining an edit operation on a cached stages.
/// This is used to allow the GusdStageCache to apply stage-mutating operations
/// to a cached stage, such as layer muting and variant selections.
/// The stage cache will produce unique stages corresponding to the
/// types of edits that are requested.
/// @warning: When abused, layer edits can cause an explosion in
/// the number of stages created. Use with caution.
class GUSD_API GusdStageEdit : public UT_IntrusiveRefCounter<GusdStageEdit>
{
public:
    GusdStageEdit() {}
    virtual ~GusdStageEdit() {}

    /// Apply an edit on the session layer, prior to stage loading.
    virtual bool    Apply(const SdfLayerHandle& layer,
                          UT_ErrorSeverity sev=UT_ERROR_ABORT) const
                    { return true; }
    
    /// Apply an edit on the loaded stage.
    virtual bool    Apply(const UsdStagePtr& stage,
                          UT_ErrorSeverity sev=UT_ERROR_ABORT) const
                    { return true; }
    
    virtual size_t  GetHash() const = 0;

    virtual bool    operator==(const GusdStageEdit& o) const = 0;

    bool            operator!=(const GusdStageEdit& o) const
                    { return !(*this == o); }
};


using GusdStageBasicEditPtr = UT_IntrusivePtr<class GusdStageBasicEdit>;


/// Basic stage edit covering common types of edits.
/// While the GusdStageCache supports caching with arbitrary stage edits,
/// there's no cache sharing if those edits are of different types,
/// even if they're functionally the same.
/// This class provides a single point for describing all of the common
/// types of edits so that, at least in the typical cases, code pulling
/// data from the stage cache are using a common type of edit.
///
/// Note that when applying variant edits, variant selection paths should be
/// stripped of any trailing path components following the variant selection.
/// For example, rather than creating an edit applying variant selection
/// `/foo{a=b}bar`, it is better to use path `/foo{a=b}` as the variant
/// selection path. The GetPrimPathAndEditFromVariantsPath helper automatically
/// strips all such trailing path components.
class GUSD_API GusdStageBasicEdit : public GusdStageEdit
{
public:

    /// Extract a prim path and an edit from a path string,
    /// which may include variant selections.
    /// This covers the common case where a single parameter
    /// provides a prim path, which may include variant selections
    /// (Eg., as /foo{variant=sel}bar).
    static void
    GetPrimPathAndEditFromVariantsPath(const SdfPath& pathWithVariants,
                                       SdfPath& primPath,
                                       GusdStageBasicEditPtr& edit);

    virtual bool    Apply(const SdfLayerHandle& layer,
                          UT_ErrorSeverity sev=UT_ERROR_ABORT) const override;

    virtual bool    Apply(const UsdStagePtr& stage,
                          UT_ErrorSeverity sev=UT_ERROR_ABORT) const override;

    virtual size_t  GetHash() const override;

    virtual bool    operator==(const GusdStageEdit& o) const override;

    const UT_Array<SdfPath>&        GetVariants() const
                                    { return _variants; }

    UT_Array<SdfPath>&              GetVariants()
                                    { return _variants; }

    const std::vector<std::string>& GetLayersToMute() const
                                    { return _layersToMute; }

    std::vector<std::string>&       GetLayersToMute()
                                    { return _layersToMute; }

private:
    UT_Array<SdfPath>           _variants;
    std::vector<std::string>    _layersToMute;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_STAGEEDIT_H_*/
