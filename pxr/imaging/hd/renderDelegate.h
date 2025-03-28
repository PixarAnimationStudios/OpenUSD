//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_DELEGATE_H
#define PXR_IMAGING_HD_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/command.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdRprim;
class HdSprim;
class HdBprim;
class HdSceneDelegate;
class HdRenderIndex;
class HdRenderPass;
class HdInstancer;
class HdDriver;

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

using HdRenderPassSharedPtr = std::shared_ptr<class HdRenderPass>;
using HdRenderPassStateSharedPtr = std::shared_ptr<class HdRenderPassState>;
using HdResourceRegistrySharedPtr = std::shared_ptr<class HdResourceRegistry>;
using HdDriverVector = std::vector<HdDriver*>;

///
/// The HdRenderParam is an opaque (to core Hydra) handle, to an object
/// that is obtained from the render delegate and passed to each prim
/// during Sync processing.
///
class HdRenderParam 
{
public:
    HdRenderParam() {}
    HD_API
    virtual ~HdRenderParam();

    /// Set a custom value in the render param's implementation. 
    /// The \p key identifies the value, while \p value holds the data to set. 
    /// Return true if the value was successfully set, false if the operation is
    /// not supported for the specified key or value.
    /// Since this method can be called by client code, it must be thread-safe.
    HD_API
    virtual bool SetArbitraryValue(const TfToken& key, const VtValue& value);

    /// Retrieve a custom value identified by \p key from the render param's
    /// implementation. The value could have been set via SetArbitraryValue() or
    /// provided internally by the render param. 
    /// Return an empty VtValue if no value is associated with the key. 
    /// This can either be used to retrieve arbitrary values by the 
    /// render delegate or by Hydra client code.
    /// Since this method can be called by client code, it must be thread-safe.
    HD_API
    virtual VtValue GetArbitraryValue(const TfToken& key) const;

    /// Check whether a valid custom value exists for the specified \p key 
    /// in the render param's implementation. Returns true if the value 
    /// exists, false otherwise.
    /// Since this method can be called by client code, it must be thread-safe.
    HD_API
    virtual bool HasArbitraryValue(const TfToken& key) const;

private:
    // Hydra will not attempt to copy the class.
    HdRenderParam(const HdRenderParam &) = delete;
    HdRenderParam &operator =(const HdRenderParam &) = delete;
};

typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> HdRenderSettingsMap;

///
/// HdRenderSettingDescriptor represents a render setting that a render delegate
/// wants to export (e.g. to UI).
///
struct HdRenderSettingDescriptor 
{
    // A human readable name.
    std::string name;
    // The key for HdRenderDelegate::SetRenderSetting/GetRenderSetting.
    TfToken key;
    // The default value.
    VtValue defaultValue;
};

typedef std::vector<HdRenderSettingDescriptor> HdRenderSettingDescriptorList;

/// \class HdRenderDelegate
///
class HdRenderDelegate
{
public:
    HD_API
    virtual ~HdRenderDelegate();

    ///
    /// Set list of driver objects, such as a rendering context / devices.
    /// This is automatically called from HdRenderIndex when a HdDriver is
    /// provided during its construction. Default implementation does nothing.
    ///
    HD_API
    virtual void SetDrivers(HdDriverVector const& drivers);

    ///
    /// Returns a list of typeId's of all supported Rprims by this render
    /// delegate.
    ///
    virtual const TfTokenVector &GetSupportedRprimTypes() const = 0;

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
    virtual HdRenderParam *GetRenderParam() const;

    ///
    /// Returns a shared ptr to the resource registry of the current render
    /// delegate.
    ///
    virtual HdResourceRegistrySharedPtr GetResourceRegistry() const = 0;

    ///
    /// Set a custom render setting on this render delegate.
    ///
    HD_API
    virtual void SetRenderSetting(TfToken const& key, VtValue const& value);

    ///
    /// Get the current value for a render setting.
    ///
    HD_API
    virtual VtValue GetRenderSetting(TfToken const& key) const;

    ///
    /// Get the current value for a render setting, taking a desired type
    /// and a fallback value in case of type mismatch.
    ///
    template<typename T>
    T GetRenderSetting(TfToken const& key, T const& defValue) const {
        return GetRenderSetting(key).Cast<T>().GetWithDefault(defValue);
    }

    ///
    /// Get the backend-exported render setting descriptors.
    ///
    HD_API
    virtual HdRenderSettingDescriptorList GetRenderSettingDescriptors() const;

    ///
    /// Get the current version of the render settings dictionary.
    ///
    HD_API
    virtual unsigned int GetRenderSettingsVersion() const;

    ///
    /// Returns an open-format dictionary of render statistics
    ///
    HD_API
    virtual VtDictionary GetRenderStats() const;

    ///
    /// Gives capabilities of render delegate as data source
    /// (conforming to HdRenderCapabilitiesSchema).
    ///
    HD_API
    virtual HdContainerDataSourceHandle GetCapabilities() const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Control of background rendering threads.
    ///
    ////////////////////////////////////////////////////////////////////////////

    ///
    /// Advertise whether this delegate supports pausing and resuming of
    /// background render threads. Default implementation returns false.
    ///
    HD_API
    virtual bool IsPauseSupported() const;

    ///
    /// Query the delegate's pause state. Returns true if the background
    /// rendering threads are currently paused.
    ///
    HD_API
    virtual bool IsPaused() const;

    ///
    /// Pause all of this delegate's background rendering threads. Default
    /// implementation does nothing.
    ///
    /// Returns \c true if successful.
    ///
    HD_API
    virtual bool Pause();

    ///
    /// Resume all of this delegate's background rendering threads previously
    /// paused by a call to Pause. Default implementation does nothing.
    ///
    /// Returns \c true if successful.
    ///
    HD_API
    virtual bool Resume();

    ///
    /// Advertise whether this delegate supports stopping and restarting of
    /// background render threads. Default implementation returns false.
    ///
    HD_API
    virtual bool IsStopSupported() const;

    ///
    /// Query the delegate's stop state. Returns true if the background
    /// rendering threads are not currently active.
    ///
    HD_API
    virtual bool IsStopped() const;

    ///
    /// Stop all of this delegate's background rendering threads; if blocking
    /// is true, the function waits until they exit.
    /// Default implementation does nothing.
    ///
    /// Returns \c true if successfully stopped.
    ///
    HD_API
    virtual bool Stop(bool blocking = true);

    ///
    /// Restart all of this delegate's background rendering threads previously
    /// stopped by a call to Stop. Default implementation does nothing.
    ///
    /// Returns \c true if successful.
    ///
    HD_API
    virtual bool Restart();

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
    /// \return A pointer to the new instancer or nullptr on error.
    ///
    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) = 0;

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
    /// \return A pointer to the new prim or nullptr on error.
    ///                     
    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) = 0;

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


    /// \deprecated use GetMaterialRenderContexts()
    HD_API
    virtual TfToken GetMaterialNetworkSelector() const;

    ///
    /// Returns a list, in descending order of preference, that can be used to
    /// select among multiple material network implementations. The default 
    /// list contains an empty token.
    ///
    HD_API
    virtual TfTokenVector GetMaterialRenderContexts() const;

    /// Returns a list of namespace prefixes for render settings attributes 
    /// relevant to a render delegate. This list is used to gather just the 
    /// relevant attributes from render settings scene description. The default
    /// is an empty list, which will gather all custom (non-schema) attributes.
    ///
    HD_API
    virtual TfTokenVector GetRenderSettingsNamespaces() const;

    ///
    /// Return true to indicate that the render delegate wants rprim primvars
    /// to be filtered by the scene delegate to reduce the amount of primvars
    /// that are send to the render delegate. For example the scene delegate
    /// may check the bound material primvar requirements and send only those
    /// to the render delegate. Return false to not apply primvar filtering in
    /// the scene delegate. Defaults to false.
    ///
    HD_API
    virtual bool IsPrimvarFilteringNeeded() const;

    ///
    /// Returns the ordered list of shader source types that the render delegate 
    /// supports.
    /// 
    HD_API
    virtual TfTokenVector GetShaderSourceTypes() const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// AOVs
    ///
    ////////////////////////////////////////////////////////////////////////////

    ///
    /// Returns a default AOV descriptor for the given named AOV, specifying
    /// things like preferred format.
    ///
    HD_API
    virtual HdAovDescriptor GetDefaultAovDescriptor(TfToken const& name) const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Commands API
    ///
    ////////////////////////////////////////////////////////////////////////////
    
    ///
    /// Get the descriptors for the commands supported by this render delegate.
    ///
    HD_API
    virtual HdCommandDescriptors GetCommandDescriptors() const;

    ///
    /// Invokes the command described by the token \p command with optional
    /// \p args.
    ///
    /// If the command succeeds, returns \c true, otherwise returns \c false.
    /// A command will generally fail if it is not among those returned by
    /// GetCommandDescriptors().
    ///
    HD_API
    virtual bool InvokeCommand(
        const TfToken &command,
        const HdCommandArgs &args = HdCommandArgs());

    ///
    /// Populated when instantiated via the HdRendererPluginRegistry
    HD_API
    const std::string &GetRendererDisplayName() {
        return _displayName;
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Hydra 2.0 API
    ///
    /// \note The following methods aid in migrating existing 1.0 based
    ///       render delegates to the Hydra 2.0 API.
    ///
    ////////////////////////////////////////////////////////////////////////////
    
    /// Called after the scene index graph is created during render index
    /// construction, providing a hook point for the render delegate to
    /// register an observer of the terminal scene index.
    ///
    /// \note Render delegates should not assume that the scene index is fully
    ///       populated at this point.
    ///
    HD_API
    virtual void SetTerminalSceneIndex(
        const HdSceneIndexBaseRefPtr &terminalSceneIndex);

    /// Called at the beginning of HdRenderIndex::SyncAll, before render index
    /// prim sync, to provide the render delegate an opportunity to directly
    /// process change notices from observing the terminal scene index,
    /// rather than using the Hydra 1.0 Sync algorithm.
    ///
    HD_API
    virtual void Update();

    /// Whether or not multithreaded sync is enabled for the
    /// specified prim type.
    HD_API
    virtual bool IsParallelSyncEnabled(const TfToken &primType) const;

protected:
    /// This class must be derived from.
    HD_API
    HdRenderDelegate();
    /// Allow derived classes to pass construction-time render settings.
    HD_API
    HdRenderDelegate(HdRenderSettingsMap const& settingsMap);

    ///
    /// This class is not intended to be copied.
    ///
    HdRenderDelegate(const HdRenderDelegate &) = delete;
    HdRenderDelegate &operator=(const HdRenderDelegate &) = delete;

    HD_API
    void _PopulateDefaultSettings(
        HdRenderSettingDescriptorList const& defaultSettings);

    /// Render settings state.
    HdRenderSettingsMap _settingsMap;
    unsigned int _settingsVersion;

private:

    friend class HdRendererPlugin;
    ///
    /// Populated when instantiated via the HdRendererPluginRegistry and
    /// currently used to associate a renderer delegate instance with related
    /// code and resources. 
    void _SetRendererDisplayName(const std::string &displayName) {
        _displayName = displayName;
    }
    std::string _displayName;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_RENDER_DELEGATE_H
