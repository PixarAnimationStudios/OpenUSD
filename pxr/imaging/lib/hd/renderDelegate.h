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

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include "pxr/imaging/hf/pluginDelegateBase.h"

PXR_NAMESPACE_OPEN_SCOPE


class SdfPath;
class HdRprim;
class HdSprim;
class HdBprim;

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
    /// Returns a list of typeId's of all supported Sprims by this render
    /// delegate.
    ///
    virtual const TfTokenVector &GetSupportedSprimTypes() const = 0;


    ///
    /// Returns a list of typeId's of all supported Bprims by this render
    /// delegate.
    ///
    virtual const TfTokenVector &GetSupportedBprimTypes() const = 0;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Prim Factories
    ///
    ////////////////////////////////////////////////////////////////////////////


    ///
    /// Request to Allocate and Construct a new Rprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param rprimId a unique identifier for the prim
    /// \param instancerId the unique identifier for the instancer that uses
    ///                    the prim (optional: May be empty).
    /// \return A pointer to the new prim or nullptr on error.
    ///                     
    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    /// 
    virtual void DestroyRprim(HdRprim *rPrim) = 0;

    ///
    /// Request to Allocate and Construct a new Sprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param sprimId a unique identifier for the prim
    /// \return A pointer to the new prim or nullptr on error.
    ///
    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) = 0;

    ///
    /// Request to Allocate and Construct an Sprim to use as a standin, if there
    /// if an error with another another Sprim of the same type.  For example,
    /// if another prim references a non-exisiting Sprim, the fallback could
    /// be used.
    ///
    /// \param typeId the type identifier of the prim to allocate
    /// \return A pointer to the new prim or nullptr on error.
    ///
    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    ///
    virtual void DestroySprim(HdSprim *sprim) = 0;

    ///
    /// Request to Allocate and Construct a new Bprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param sprimId a unique identifier for the prim
    /// \return A pointer to the new prim or nullptr on error.
    ///
    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) = 0;


    ///
    /// Request to Allocate and Construct a Bprim to use as a standin, if there
    /// if an error with another another Bprim of the same type.  For example,
    /// if another prim references a non-exisiting Bprim, the fallback could
    /// be used.
    ///
    /// \param typeId the type identifier of the prim to allocate
    /// \return A pointer to the new prim or nullptr on error.
    ///
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    ///
    virtual void DestroyBprim(HdBprim *bprim) = 0;


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


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RENDER_DELEGATE_H
