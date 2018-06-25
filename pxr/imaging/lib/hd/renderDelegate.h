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
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class SdfPath;
class HdRprim;
class HdSprim;
class HdBprim;
class HdSceneDelegate;
class HdRenderPass;
class HdInstancer;

typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdResourceRegistry> HdResourceRegistrySharedPtr;

///
/// The HdRenderParam is an opaque (to core Hydra) handle, to an object
/// that is obtained from the render delegate and passed to each prim
/// during Sync processing.
///
class HdRenderParam {
public:
    HdRenderParam() {}
    HD_API
    virtual ~HdRenderParam();

private:
    // Hydra will not attempt to copy the class.
    HdRenderParam(const HdRenderParam &) = delete;
    HdRenderParam &operator =(const HdRenderParam &) = delete;
};

/// \class HdRenderDelegate
///
class HdRenderDelegate
{
public:
    HD_API
    virtual ~HdRenderDelegate();

    ///
    /// Returns a list of typeId's of all supported Rprims by this render
    /// delegate.
    ///
    HD_API
    virtual const TfTokenVector &GetSupportedRprimTypes() const = 0;

    ///
    /// Returns a list of typeId's of all supported Sprims by this render
    /// delegate.
    ///
    HD_API
    virtual const TfTokenVector &GetSupportedSprimTypes() const = 0;


    ///
    /// Returns a list of typeId's of all supported Bprims by this render
    /// delegate.
    ///
    HD_API
    virtual const TfTokenVector &GetSupportedBprimTypes() const = 0;

    ///
    /// Returns an opaque handle to a render param, that in turn is
    /// passed to each prim created by the render delegate during sync
    /// processing.  This avoids the need to store a global state pointer
    /// in each prim.
    ///
    /// The typical lifetime of the renderParam would match that of the
    /// RenderDelegate, however the minimal lifetime is that of the Sync
    /// processing.  The param maybe queried multiple times during sync.
    ///
    /// A render delegate may return null for the param.
    ///
    HD_API
    virtual HdRenderParam *GetRenderParam() const = 0;

    ///
    /// Returns a shared ptr to the resource registry of the current render
    /// delegate.
    ///
    HD_API
    virtual HdResourceRegistrySharedPtr GetResourceRegistry() const = 0;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Renderpass Factory
    ///
    ////////////////////////////////////////////////////////////////////////////

    ///
    /// Request to create a new renderpass.
    /// \param index the render index to bind to the new renderpass.
    /// \param collection the rprim collection to bind to the new renderpass.
    /// \return A shared pointer to the new renderpass or empty on error.
    ///
    HD_API
    virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                                      HdRprimCollection const& collection) = 0;

    ///
    /// Request to create a new renderpass state.
    /// The default implementation creates an HdRenderPassState instance,
    /// but derived render delegates may instantiate their own state type.
    /// \param shader The render pass shader to use.  If null, a new
    ///               HdRenderPassShared will be created.
    /// \return A shared pointer to the new renderpass state.
    ///
    HD_API
    virtual HdRenderPassStateSharedPtr CreateRenderPassState() const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Instancer Factory
    ///
    ////////////////////////////////////////////////////////////////////////////

    ///
    /// Request to create a new instancer.
    /// \param id The unique identifier of this instancer.
    /// \param instancerId The unique identifier for the parent instancer that
    ///                    uses this instancer as a prototype (may be empty).
    /// \return A pointer to the new instancer or nullptr on error.
    ///
    HD_API
    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id,
                                         SdfPath const& instancerId) = 0;

    HD_API
    virtual void DestroyInstancer(HdInstancer *instancer) = 0;

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
    HD_API
    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    /// 
    HD_API
    virtual void DestroyRprim(HdRprim *rPrim) = 0;

    ///
    /// Request to Allocate and Construct a new Sprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param sprimId a unique identifier for the prim
    /// \return A pointer to the new prim or nullptr on error.
    ///
    HD_API
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
    HD_API
    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    ///
    HD_API
    virtual void DestroySprim(HdSprim *sprim) = 0;

    ///
    /// Request to Allocate and Construct a new Bprim.
    /// \param typeId the type identifier of the prim to allocate
    /// \param sprimId a unique identifier for the prim
    /// \return A pointer to the new prim or nullptr on error.
    ///
    HD_API
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
    HD_API
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) = 0;

    ///
    /// Request to Destruct and deallocate the prim.
    ///
    HD_API
    virtual void DestroyBprim(HdBprim *bprim) = 0;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Sync, Execute & Dispatch Hooks
    ///
    ////////////////////////////////////////////////////////////////////////////

    ///
    /// Notification point from the Engine to the delegate.
    /// This notification occurs after all Sync's have completed and
    /// before task execution.
    ///
    /// This notification gives the Render Delegate a chance to
    /// update and move memory that the render may need.
    ///
    /// For example, the render delegate might fill primvar buffers or texture
    /// memory.
    ///
    HD_API
    virtual void CommitResources(HdChangeTracker *tracker) = 0;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Materials
    ///
    ////////////////////////////////////////////////////////////////////////////
    
    ///
    /// Returns a token that indicates material bindings should be used,
    /// based on the indicated purpose.  The default purpose is
    /// HdTokens->preview.
    ///
    HD_API
    virtual TfToken GetMaterialBindingPurpose() const;

    ///
    /// Returns a token that can be used to select among multiple
    /// material network implementations.  The default is empty.
    ///
    HD_API
    virtual TfToken GetMaterialNetworkSelector() const { return TfToken(); }

    ///
    /// Returns the ordered list of shader source types that the render delegate 
    /// supports.
    /// 
    HD_API
    virtual TfTokenVector GetShaderSourceTypes() const;

protected:
    /// This class must be derived from
    HdRenderDelegate()          = default;

    ///
    /// This class is not intended to be copied.
    ///
    HdRenderDelegate(const HdRenderDelegate &) = delete;
    HdRenderDelegate &operator=(const HdRenderDelegate &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RENDER_DELEGATE_H
