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
#ifndef HD_RENDER_DELEGATE_H
#define HD_RENDER_DELEGATE_H

#include "pxr/base/tf/token.h"

#include "pxr/imaging/hf/pluginDelegateBase.h"

class SdfPath;
class HdRprim;
class HdSprim;
class HdSceneDelegate;

/// \class HdRenderDelegate
///
class HdRenderDelegate : public HfPluginDelegateBase
{
public:
    ///
    /// Allows the delegate an opinion on the default Gal to use.
    /// Return an empty token for no opinion.
    /// Return HdDelegateTokens->None for no Gal.
    ///
    virtual TfToken GetDefaultGalId() const = 0;

    ///
    /// Request to Allocate and Construct a new Rprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param delegate the scene delegate that provides the data for the prim
    /// \param rprimId a unique identifier for the prim
    /// \param instancerId the unique identifier for the instancer that uses
    ///                    the prim (optional: May be empty).
    /// \return A pointer to the new prim or nullptr on error.
    ///                     
    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 HdSceneDelegate* delegate,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    /// 
    virtual void DestroyRprim(HdRprim *rPrim) = 0;

    ///
    /// Request to Allocate and Construct a new Sprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param delegate the scene delegate that provides the data for the prim
    /// \param sprimId a unique identifier for the prim
    /// \return A pointer to the new prim or nullptr on error.
    ///
    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 HdSceneDelegate* delegate,
                                 SdfPath const& sprimId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    ///
    virtual void DestroySprim(HdSprim *sPrim) = 0;

protected:
    /// This class must be derived from
    HdRenderDelegate()          = default;
    virtual ~HdRenderDelegate();

    ///
    /// This class is not intended to be copied.
    ///
    HdRenderDelegate(const HdRenderDelegate &) = delete;
    HdRenderDelegate &operator=(const HdRenderDelegate &) = delete;
};

#endif //HD_RENDER_DELEGATE_H
