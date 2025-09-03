//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LAYER_H
#define PXR_USD_SDF_LAYER_H

/// \file sdf/layer.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/identity.h"
#include "pxr/usd/sdf/layerHints.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/ar/ar.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/work/dispatcher.h"

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(SdfFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerStateDelegateBase);

class SdfChangeList;
struct Sdf_AssetInfo;

/// \class SdfLayer 
///
/// A scene description container that can combine with other such containers
/// to form simple component assets, and successively larger aggregates.  The
/// contents of an SdfLayer adhere to the SdfData data model.  A layer can be
/// ephemeral, or be an asset accessed and serialized through the ArAsset and
/// ArResolver interfaces.
///
/// The SdfLayer class provides a consistent API for accesing and serializing
/// scene description, using any data store provided by Ar plugins.  Sdf
/// itself provides a UTF-8 text format for layers identified by the ".usda"
/// identifier extension, but via the SdfFileFormat abstraction, allows
/// downstream modules and plugins to adapt arbitrary data formats to the
/// SdfData/SdfLayer model.
///
/// The FindOrOpen() method returns a new SdfLayer object with scene
/// description from any supported asset format. Once read, a layer
/// remembers which asset it was read from. The Save() method saves the layer
/// back out to the original asset.  You can use the Export() method to write
/// the layer to a different location. You can use the GetIdentifier() method
/// to get the layer's Id or GetRealPath() to get the resolved, full URI.
///
/// Layer identifiers are UTF-8 encoded strings. A layer's file format is
/// determined via the identifier's extension (as resolved by Ar) with [A-Z]
/// (and no other characters) explicitly case folded.
///
/// Layers can have a timeCode range (startTimeCode and endTimeCode). This range
/// represents the suggested playback range, but has no impact on the extent of 
/// the animation data that may be stored in the layer. The metadatum 
/// "timeCodesPerSecond" is used to annotate how the time ordinate for samples
/// contained in the file scales to seconds. For example, if timeCodesPerSecond
/// is 24, then a sample at time ordinate 24 should be viewed exactly one second
/// after the sample at time ordinate 0.
/// 
class SdfLayer 
    : public TfRefBase
    , public TfWeakBase
{
public:
    /// Destructor
    SDF_API
    virtual ~SdfLayer(); 

    /// Noncopyable
    SdfLayer(const SdfLayer&) = delete;
    SdfLayer& operator=(const SdfLayer&) = delete;

    ///
    /// \name Primary API
    /// @{

    /// Returns the schema this layer adheres to. This schema provides details
    /// about the scene description that may be authored in this layer.
    SDF_API const SdfSchemaBase& GetSchema() const;

    /// Returns the file format used by this layer.
    SDF_API const SdfFileFormatConstPtr& GetFileFormat() const;

    /// Type for specifying additional file format-specific arguments to
    /// layer API.
    typedef std::map<std::string, std::string> FileFormatArguments;

    /// Returns the file format-specific arguments used during the construction
    /// of this layer.
    SDF_API const FileFormatArguments& GetFileFormatArguments() const;

    /// Creates a new empty layer with the given identifier.
    ///
    /// Additional arguments may be supplied via the \p args parameter.
    /// These arguments may control behavior specific to the layer's
    /// file format.
    SDF_API
    static SdfLayerRefPtr CreateNew(const std::string &identifier,
                                    const FileFormatArguments &args =
                                    FileFormatArguments());

    /// Creates a new empty layer with the given identifier for a given file
    /// format class.
    ///
    /// This function has the same behavior as the other CreateNew function,
    /// but uses the explicitly-specified \p fileFormat instead of attempting
    /// to discern the format from \p identifier.
    SDF_API
    static SdfLayerRefPtr CreateNew(const SdfFileFormatConstPtr& fileFormat,
                                    const std::string &identifier,
                                    const FileFormatArguments &args =
                                    FileFormatArguments());

    /// Creates a new empty layer with the given identifier for a given file
    /// format class.
    ///
    /// The new layer will not be dirty and will not be saved.
    ///
    /// Additional arguments may be supplied via the \p args parameter. 
    /// These arguments may control behavior specific to the layer's
    /// file format.
    SDF_API
    static SdfLayerRefPtr New(const SdfFileFormatConstPtr& fileFormat,
                              const std::string &identifier,
                              const FileFormatArguments &args = 
                              FileFormatArguments());

    /// Return an existing layer with the given \p identifier and \p args.  If
    /// the layer can't be found, an error is posted and a null layer is
    /// returned.
    ///
    /// Arguments in \p args will override any arguments specified in
    /// \p identifier.
    SDF_API
    static SdfLayerHandle Find(
        const std::string &identifier,
        const FileFormatArguments &args = FileFormatArguments());

    /// Return an existing layer with the given \p identifier and \p args.
    /// The given \p identifier will be resolved relative to the \p anchor
    /// layer. If the layer can't be found, an error is posted and a null
    /// layer is returned.
    ///
    /// If the \p anchor layer is invalid, a coding error is raised, and a null
    /// handle is returned.
    ///
    /// Arguments in \p args will override any arguments specified in
    /// \p identifier.
    SDF_API
    static SdfLayerHandle FindRelativeToLayer(
        const SdfLayerHandle &anchor,
        const std::string &identifier,
        const FileFormatArguments &args = FileFormatArguments());

    /// Return an existing layer with the given \p identifier and \p args, or
    /// else load it. If the layer can't be found or loaded, an error is posted
    /// and a null layer is returned.
    ///
    /// Arguments in \p args will override any arguments specified in
    /// \p identifier.
    SDF_API
    static SdfLayerRefPtr FindOrOpen(
        const std::string &identifier,
        const FileFormatArguments &args = FileFormatArguments());

    /// Return an existing layer with the given \p identifier and \p args, or
    /// else load it. The given \p identifier will be resolved relative to the
    /// \p anchor layer. If the layer can't be found or loaded, an error is
    /// posted and a null layer is returned.
    ///
    /// If the \p anchor layer is invalid, issues a coding error and returns
    /// a null handle.
    ///
    /// Arguments in \p args will override any arguments specified in
    /// \p identifier.
    SDF_API
    static SdfLayerRefPtr FindOrOpenRelativeToLayer(
        const SdfLayerHandle &anchor,
        const std::string &identifier,
        const FileFormatArguments &args = FileFormatArguments());
        
    /// Load the given layer from disk as a new anonymous layer. If the
    /// layer can't be found or loaded, an error is posted and a null
    /// layer is returned.
    ///
    /// The anonymous layer does not retain any knowledge of the backing
    /// file on the filesystem.
    ///
    /// \p metadataOnly is a flag that asks for only the layer metadata
    /// to be read in, which can be much faster if that is all that is
    /// required.  Note that this is just a hint: some FileFormat readers
    /// may disregard this flag and still fully populate the layer contents.
    ///
    /// An optional \p tag may be specified.  See CreateAnonymous for details.
    SDF_API
    static SdfLayerRefPtr OpenAsAnonymous(
        const std::string &layerPath,
        bool metadataOnly = false,
        const std::string& tag = std::string());

    /// Returns the data from the absolute root path of this layer.
    SDF_API
    SdfDataRefPtr GetMetadata() const;

    /// Return hints about the layer's current contents.  Any operation that
    /// dirties the layer will invalidate all hints.
    /// \sa SdfLayerHints
    SDF_API
    SdfLayerHints GetHints() const;

    /// Returns handles for all layers currently held by the layer registry.
    SDF_API
    static SdfLayerHandleSet GetLoadedLayers();

    /// Returns whether this layer has no significant data.
    SDF_API
    bool IsEmpty() const;

    /// Returns true if this layer streams data from its serialized data
    /// store on demand, false otherwise.
    ///
    /// Layers with streaming data are treated differently to avoid pulling
    /// in data unnecessarily. For example, reloading a streaming layer 
    /// will not perform fine-grained change notification, since doing 
    /// so would require the full contents of the layer to be loaded.
    SDF_API
    bool StreamsData() const;

    /// Returns true if this layer is detached from its serialized data
    /// store, false otherwise.
    ///
    /// Detached layers are isolated from external changes to their serialized
    /// data.
    SDF_API
    bool IsDetached() const;

    /// Copies the content of the given layer into this layer.
    /// Source layer is unmodified.
    SDF_API
    void TransferContent(const SdfLayerHandle& layer);

    /// Creates a new \e anonymous layer with an optional \p tag. An anonymous
    /// layer is a layer with a system assigned identifier, that cannot be
    /// saved to disk via Save(). Anonymous layers have an identifier, but no
    /// real path or other asset information fields.
    ///
    /// Anonymous layers may be tagged, which can be done to aid debugging
    /// subsystems that make use of anonymous layers.  The tag becomes the
    /// display name of an anonymous layer, and is also included in the
    /// generated identifier. Untagged anonymous layers have an empty display
    /// name.
    ///
    /// Additional arguments may be supplied via the \p args parameter.
    /// These arguments may control behavior specific to the layer's
    /// file format.
    SDF_API
    static SdfLayerRefPtr CreateAnonymous(
        const std::string& tag = std::string(),
        const FileFormatArguments& args = FileFormatArguments());

    /// Create an anonymous layer with a specific \p format.
    SDF_API
    static SdfLayerRefPtr CreateAnonymous(
        const std::string &tag, const SdfFileFormatConstPtr &format,
        const FileFormatArguments& args = FileFormatArguments());

    /// Returns true if this layer is an anonymous layer.
    SDF_API
    bool IsAnonymous() const;

    /// Returns true if the \p identifier is an anonymous layer unique
    /// identifier.
    SDF_API
    static bool IsAnonymousLayerIdentifier(const std::string& identifier);

    /// Returns the display name for the given \p identifier, using the same 
    /// rules as GetDisplayName.
    SDF_API
    static std::string GetDisplayNameFromIdentifier(
        const std::string& identifier);

    /// @}
    /// \name File I/O
    /// @{

    /// Returns \c true if successful, \c false if an error occurred.
    /// Returns \c false if the layer has no remembered file name or the 
    /// layer type cannot be saved. The layer will not be overwritten if the 
    /// file exists and the layer is not dirty unless \p force is true.
    SDF_API
    bool Save(bool force = false) const;

    /// Exports this layer to a file.
    /// Returns \c true if successful, \c false if an error occurred.
    ///
    /// If \p comment is not empty, the layer gets exported with the given
    /// comment. Additional arguments may be supplied via the \p args parameter.
    /// These arguments may control behavior specific to the exported layer's
    /// file format.
    ///
    /// Note that the file name or comment of the original layer is not
    /// updated. This only saves a copy of the layer to the given filename.
    /// Subsequent calls to Save() will still save the layer to it's
    /// previously remembered file name.
    SDF_API
    bool Export(const std::string& filename, 
                const std::string& comment = std::string(),
                const FileFormatArguments& args = FileFormatArguments()) const;

    /// Writes this layer to the given string.
    ///
    /// Returns \c true if successful and sets \p result, otherwise
    /// returns \c false.
    SDF_API
    bool ExportToString(std::string* result) const;

    /// Reads this layer from the given string.
    ///
    /// Returns \c true if successful, otherwise returns \c false.
    SDF_API
    bool ImportFromString(const std::string &string);

    /// Clears the layer of all content.
    ///
    /// This restores the layer to a state as if it had just been created
    /// with CreateNew().  This operation is Undo-able.
    ///
    /// The fileName and whether journaling is enabled are not affected
    /// by this method.
    SDF_API
    void Clear();

    /// Reloads the layer from its persistent representation.
    ///
    /// This restores the layer to a state as if it had just been created
    /// with FindOrOpen().  This operation is Undo-able.
    ///
    /// The fileName and whether journaling is enabled are not affected
    /// by this method.
    ///
    /// When called with force = false (the default), Reload attempts to
    /// avoid reloading layers that have not changed on disk. It does so
    /// by comparing the file's modification time (mtime) to when the
    /// file was loaded. If the layer has unsaved modifications, this
    /// mechanism is not used, and the layer is reloaded from disk. If the 
    /// layer has any 
    /// \ref GetExternalAssetDependencies "external asset dependencies"
    /// their modification state will also be consulted when determining if 
    /// the layer needs to be reloaded.
    ///
    /// Passing true to the \p force parameter overrides this behavior,
    /// forcing the layer to be reloaded from disk regardless of whether
    /// it has changed.
    SDF_API
    bool Reload(bool force = false);

    /// Reloads the specified layers.
    ///
    /// Returns \c false if one or more layers failed to reload.
    ///
    /// See \c Reload() for a description of the \p force flag.
    ///
    SDF_API
    static bool ReloadLayers(const std::set<SdfLayerHandle>& layers,
                             bool force = false);

    /// Imports the content of the given layer path, replacing the content
    /// of the current layer.
    /// Note: If the layer path is the same as the current layer's real path,
    /// no action is taken (and a warning occurs). For this case use
    /// Reload().
    SDF_API
    bool Import(const std::string &layerPath);

    /// @}
    /// \name External references
    /// @{

    /// \deprecated 
    /// Use GetCompositionAssetDependencies instead.
    SDF_API
    std::set<std::string> GetExternalReferences() const;

    /// \deprecated 
    /// Use UpdateCompositionAssetDependency instead.
    SDF_API
    bool UpdateExternalReference(
        const std::string &oldAssetPath,
        const std::string &newAssetPath=std::string());

    /// Return paths of all assets this layer depends on due to composition 
    /// fields.
    ///
    /// This includes the paths of all layers referred to by reference, 
    /// payload, and sublayer fields in this layer. This function only returns 
    /// direct composition dependencies of this layer, i.e. it does not recurse 
    /// to find composition dependencies from its dependent layer assets.
    SDF_API
    std::set<std::string> GetCompositionAssetDependencies() const;

    /// Updates the asset path of a composation dependency in this layer.
    /// 
    /// If \p newAssetPath is supplied, the update works as "rename", updating
    /// any occurrence of \p oldAssetPath to \p newAssetPath in all reference,
    /// payload, and sublayer fields.
    ///
    /// If \p newAssetPath is not given, this update behaves as a "delete", 
    /// removing all occurrences of \p oldAssetPath from all reference, payload,
    /// and sublayer fields.
    SDF_API
    bool UpdateCompositionAssetDependency(
        const std::string &oldAssetPath,
        const std::string &newAssetPath=std::string());

    /// Returns a set of resolved paths to all external asset dependencies
    /// the layer needs to generate its contents. These are additional asset 
    /// dependencies that are determined by the layer's 
    /// \ref SdfFileFormat::GetExternalAssetDependencies "file format" and
    /// will be consulted during Reload() when determining if the layer needs 
    /// to be reloaded. This specifically does not include dependencies related 
    /// to composition, i.e. this will not include assets from references, 
    /// payloads, and sublayers.
    SDF_API
    std::set<std::string> GetExternalAssetDependencies() const;

    /// @}
    /// \name Identification
    ///
    /// A layer's identifier is a string that uniquely identifies a layer.
    /// At minimum, it is the string by which the layer was created, either
    /// via FindOrOpen or CreateNew. If additional arguments were passed
    /// to those functions, those arguments will be encoded in the identifier.
    /// 
    /// For example: 
    ///     FindOrOpen('foo.usda', args={'a':'b', 'c':'d'}).identifier
    ///         => "foo.usda:SDF_FORMAT_ARGS:a=b&c=d"
    ///
    /// Note that this means the identifier may in general not be a path.
    ///
    /// The identifier format is subject to change; consumers should NOT
    /// parse layer identifiers themselves, but should use the supplied
    /// SplitIdentifier and CreateIdentifier helper functions.
    ///
    /// @{

    /// Splits the given layer identifier into its constituent layer path
    /// and arguments.
    SDF_API
    static bool SplitIdentifier(
        const std::string& identifier,
        std::string* layerPath,
        FileFormatArguments* arguments);

    /// Joins the given layer path and arguments into an identifier.
    SDF_API
    static std::string CreateIdentifier(
        const std::string& layerPath,
        const FileFormatArguments& arguments);

    /// Returns the layer identifier.
    SDF_API
    const std::string& GetIdentifier() const;

    /// Sets the layer identifier. 
    /// Note that the new identifier must have the same arguments (if any)
    /// as the old identifier.
    SDF_API
    void SetIdentifier(const std::string& identifier);

    /// Update layer asset information. Calling this method re-resolves the
    /// layer identifier, which updates asset information such as the layer's
    /// resolved path and other asset info. This may be used to update the
    /// layer after external changes to the underlying asset system.
    SDF_API
    void UpdateAssetInfo();

    /// Returns the layer's display name.
    ///
    /// The display name is the base filename of the identifier.
    SDF_API
    std::string GetDisplayName() const;

    /// Returns the resolved path for this layer. This is the path where
    /// this layer exists or may exist after a call to Save().
    SDF_API
    const ArResolvedPath& GetResolvedPath() const;

    /// Returns the resolved path for this layer. This is equivalent to
    /// GetResolvedPath().GetPathString().
    SDF_API
    const std::string& GetRealPath() const;

    /// Returns the file extension to use for this layer.
    /// If this layer was loaded from disk, it should match the extension
    /// of the file format it was loaded as; if this is an anonymous
    /// in-memory layer it will be the default extension.
    SDF_API
    std::string GetFileExtension() const;

    /// Returns the asset system version of this layer. If a layer is loaded
    /// from a location that is not version managed, or a configured asset
    /// system is not present when the layer is loaded or created, the version
    /// is empty. By default, asset version tracking is disabled; this method
    /// returns empty unless asset version tracking is enabled.
    SDF_API
    const std::string& GetVersion() const;

    /// Returns the layer identifier in asset path form. In the presence of a
    /// properly configured path resolver, the asset path is a double-slash
    /// prefixed depot path. If the path resolver is not configured, the asset
    /// path of a layer is empty.
    SDF_API
    const std::string& GetRepositoryPath() const;

    /// Returns the asset name associated with this layer.
    SDF_API
    const std::string& GetAssetName() const;

    /// Returns resolve information from the last time the layer identifier
    /// was resolved.
    SDF_API
    const VtValue& GetAssetInfo() const;

    /// Returns the path to the asset specified by \p assetPath using this layer
    /// to anchor the path if necessary. Returns \p assetPath if it's empty or
    /// an anonymous layer identifier.
    ///
    /// This method can be used on asset paths that are authored in this layer
    /// to create new asset paths that can be copied to other layers.  These new
    /// asset paths should refer to the same assets as the original asset
    /// paths. For example, if the underlying ArResolver is filesystem-based and
    /// \p assetPath is a relative filesystem path, this method might return the
    /// absolute filesystem path using this layer's location as the anchor.
    ///
    /// The returned path should in general not be assumed to be an absolute
    /// filesystem path or any other specific form. It is "absolute" in that it
    /// should resolve to the same asset regardless of what layer it's authored
    /// in.
    SDF_API
    std::string ComputeAbsolutePath(const std::string& assetPath) const;

    /// @}

    /// \name Fields
    ///
    /// All scene description for a given object is stored as a set of
    /// key/value pairs called fields. These methods provide direct access to
    /// those fields, though most clients should use the Spec API to ensure
    /// data consistency.
    ///
    /// These methods all take SdfPath to identify the queried spec.
    ///
    /// @{

    /// Return the spec type for \a path. This returns SdfSpecTypeUnknown if no
    /// spec exists at \a path.
    SDF_API
    SdfSpecType GetSpecType(const SdfPath& path) const;

    /// Return whether a spec exists at \a path.
    SDF_API
    bool HasSpec(const SdfPath& path) const;

    /// Return the names of all the fields that are set at \p path.
    SDF_API
    std::vector<TfToken> ListFields(const SdfPath& path) const;

    /// Return whether a value exists for the given \a path and \a fieldName.
    /// Optionally returns the value if it exists.
    SDF_API
    bool HasField(const SdfPath& path, const TfToken& fieldName,
                  VtValue *value=NULL) const;
    SDF_API
    bool HasField(const SdfPath& path, const TfToken& fieldName,
                  SdfAbstractDataValue *value) const;

    /// Returns \c true if the object has a non-empty value with name
    /// \p name and type \p T.  If value ptr is provided, returns the
    /// value found.
    template <class T>
    bool HasField(const SdfPath& path, const TfToken &name, 
        T* value) const
    {
        if (!value) {
            return HasField(path, name, static_cast<VtValue *>(NULL));
        }

        SdfAbstractDataTypedValue<T> outValue(value);
        const bool hasValue = HasField(
            path, name, static_cast<SdfAbstractDataValue *>(&outValue));

        if (std::is_same<T, SdfValueBlock>::value) {
            return hasValue && outValue.isValueBlock;
        }

        return hasValue && (!outValue.isValueBlock);
    }

    /// Return the type of the value for \p name on spec \p path.  If no such
    /// field exists, return typeid(void).
    std::type_info const &GetFieldTypeid(
        const SdfPath &path, const TfToken &name) const {
        return _data->GetTypeid(path, name);
    }

    /// Return whether a value exists for the given \a path and \a fieldName and
    /// \a keyPath.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.  Optionally returns the value if it exists.
    SDF_API
    bool HasFieldDictKey(const SdfPath& path,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         VtValue *value=NULL) const;
    SDF_API
    bool HasFieldDictKey(const SdfPath& path,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         SdfAbstractDataValue *value) const;

    /// Returns \c true if the object has a non-empty value with name \p name
    /// and \p keyPath and type \p T.  If value ptr is provided, returns the
    /// value found.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.
    template <class T>
    bool HasFieldDictKey(const SdfPath& path, const TfToken &name,
                         const TfToken &keyPath, T* value) const
    {
        if (!value) {
            return HasFieldDictKey(path, name, keyPath,
                                   static_cast<VtValue *>(NULL));
        }

        SdfAbstractDataTypedValue<T> outValue(value);
        return HasFieldDictKey(path, name, keyPath,
                               static_cast<SdfAbstractDataValue *>(&outValue));
    }


    /// Return the value for the given \a path and \a fieldName. Returns an
    /// empty value if none is set.
    SDF_API
    VtValue GetField(const SdfPath& path,
                     const TfToken& fieldName) const;

    /// Return the value for the given \a path and \a fieldName. Returns the
    /// provided \a defaultValue value if none is set.
    template <class T>
    inline T GetFieldAs(const SdfPath& path, 
        const TfToken& fieldName, const T& defaultValue = T()) const
    {
        return _data->GetAs<T>(path, fieldName, defaultValue);
    }

    /// Return the value for the given \a path and \a fieldName at \p
    /// keyPath. Returns an empty value if none is set.  The \p keyPath is a
    /// ':'-separated path addressing an element in sub-dictionaries.
    SDF_API
    VtValue GetFieldDictValueByKey(const SdfPath& path,
                                   const TfToken& fieldName,
                                   const TfToken& keyPath) const;

    /// Set the value of the given \a path and \a fieldName.
    SDF_API
    void SetField(const SdfPath& path, const TfToken& fieldName,
        const VtValue& value);
    SDF_API
    void SetField(const SdfPath& path, const TfToken& fieldName,
        const SdfAbstractDataConstValue& value);

    /// Set the value of the given \a path and \a fieldName.
    template <class T>
    void SetField(const SdfPath& path, const TfToken& fieldName, 
        const T& val) 
    {
        // Ideally, this would make use of the SdfAbstractDataConstValue
        // API to avoid unnecessarily copying the value into a VtValue.
        // However, Sdf needs to create a VtValue for change processing.
        // If the underlying SdAbstractData implementation also needs a 
        // VtValue, using the SdfAbstractDataConstValue API would cause
        // another copy to be made. So, it's more efficient to just create 
        // the VtValue once here and push that along.
        SetField(path, fieldName, VtValue(val));
    }

    /// Set the value of the given \a path and \a fieldName.  The \p keyPath is a
    /// ':'-separated path addressing an element in sub-dictionaries.
    SDF_API
    void SetFieldDictValueByKey(const SdfPath& path,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                const VtValue& value);
    SDF_API
    void SetFieldDictValueByKey(const SdfPath& path,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                const SdfAbstractDataConstValue& value);

    /// Set the value of the given \a path and \a fieldName.  The \p keyPath is
    /// a ':'-separated path addressing an element in sub-dictionaries.
    template <class T>
    void SetFieldDictValueByKey(const SdfPath& path,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                const T& val)
    {
        // Ideally, this would make use of the SdfAbstractDataConstValue
        // API to avoid unnecessarily copying the value into a VtValue.
        // However, Sdf needs to create a VtValue for change processing.
        // If the underlying SdAbstractData implementation also needs
        // VtValue, using the SdfAbstractDataConstValue API would cause
        // another copy to be made. So, it's more efficient to just create
        // the VtValue once here and push that along.
        SetFieldDictValueByKey(path, fieldName, keyPath, VtValue(val));
    }

    /// Remove the field at \p path and \p fieldName, if one exists.
    SDF_API
    void EraseField(const SdfPath& path, const TfToken& fieldName);

    /// Remove the field at \p path and \p fieldName and \p keyPath, if one
    /// exists.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.
    SDF_API
    void EraseFieldDictValueByKey(const SdfPath& path,
                                  const TfToken& fieldName,
                                  const TfToken& keyPath);

    /// \name Traversal
    /// @{

    /// Callback function for Traverse. This callback will be invoked with
    /// the path of each spec that is visited.
    /// \sa Traverse
    typedef std::function<void(const SdfPath&)> TraversalFunction;

    // Traverse will perform a traversal of the scene description hierarchy
    // rooted at \a path, calling \a func on each spec that it finds.
    SDF_API
    void Traverse(const SdfPath& path, const TraversalFunction& func);

    /// @}

    /// \name Metadata
    /// @{

    /// Returns the color configuration asset-path for this layer.
    ///
    /// The default value is an empty asset-path.
    SDF_API
    SdfAssetPath GetColorConfiguration() const;

    /// Sets the color configuration asset-path for this layer.
    SDF_API
    void SetColorConfiguration(const SdfAssetPath &colorConfiguration);

    /// Returns true if color configuration metadata is set in this layer.
    /// \sa GetColorConfiguration(), SetColorConfiguration()
    SDF_API 
    bool HasColorConfiguration() const;
    
    /// Clears the color configuration metadata authored in this layer. 
    /// \sa HasColorConfiguration(), SetColorConfiguration()
    SDF_API
    void ClearColorConfiguration();

    /// Returns the color management system used to interpret the color 
    /// configuration asset-path authored in this layer.
    ///
    /// The default value is an empty token, which implies that the clients 
    /// will have to determine the color management system from the color 
    /// configuration asset path (i.e. from its file extension), if it's 
    /// specified. 
    SDF_API
    TfToken GetColorManagementSystem() const;

    /// Sets the color management system used to interpret the color 
    /// configuration asset-path authored this layer.
    SDF_API
    void SetColorManagementSystem(const TfToken &cms);

    /// Returns true if colorManagementSystem metadata is set in this layer.
    /// \sa GetColorManagementSystem(), SetColorManagementSystem()
    SDF_API 
    bool HasColorManagementSystem() const;
    
    /// Clears the 'colorManagementSystem' metadata authored in this layer. 
    /// \sa HascolorManagementSystem(), SetColorManagementSystem()
    SDF_API
    void ClearColorManagementSystem();

    /// Returns the comment string for this layer.
    ///
    /// The default value for comment is "".
    SDF_API
    std::string GetComment() const;

    /// Sets the comment string for this layer.
    SDF_API
    void SetComment(const std::string &comment);
    
    /// Return the defaultPrim metadata for this layer.  This field
    /// indicates the name or path of which prim should be targeted by a
    /// reference or payload to this layer that doesn't specify a prim path.
    ///
    /// The default value is the empty token.
    SDF_API
    TfToken GetDefaultPrim() const;

    /// Return this layer's default prim metadata interpreted as an absolute 
    /// prim path regardless of whether it was authored as a root prim name or a
    /// prim path. For example, if the authored default prim value is 
    /// "rootPrim", return </rootPrim>. If the authored default prim value is 
    /// "/path/to/non/root/prim", return </path/to/non/root/prim>. If the 
    /// authored default prim value cannot be interpreted as a prim path, 
    /// return the empty SdfPath.
    ///
    /// The default value is an empty path.
    SDF_API
    SdfPath GetDefaultPrimAsPath() const;

    /// Set the default prim metadata for this layer.  The prim at this path
    /// will be targeted by a reference or a payload to this layer that doesn't 
    /// specify a prim path.
    /// Note that this can be a name if it refers to a root prim, or a path to
    /// any prim in this layer. E.g. "rootPrim", "/path/to/non/root/prim" or
    /// "/rootPrim".  See GetDefaultPrim().
    SDF_API
    void SetDefaultPrim(const TfToken &name);

    /// Clear the default prim metadata for this layer.  See GetDefaultPrim()
    /// and SetDefaultPrim().
    SDF_API
    void ClearDefaultPrim();

    /// Return true if the default prim metadata is set in this layer.  See
    /// GetDefaultPrim() and SetDefaultPrim().
    SDF_API
    bool HasDefaultPrim();

    /// Converts the given \p defaultPrim token into a prim path.
    ///
    /// If the input token is the string representation of an absolute prim,
    /// that path is returned. If the token represents a relative prim path, the
    /// returned path is coverted into a absolute path anchored to the absolute
    /// root path. If the token does not represent a valid relative or absolute
    /// prim path, an empty path is returned.
    SDF_API
    static SdfPath ConvertDefaultPrimTokenToPath(const TfToken &defaultPrim);

    /// Converts the path \p primPath into a token value that can be used to 
    /// set the default prim metadata for the layer to refer to the prim at that
    /// path.
    ///
    /// If the given path is a root prim path, the returned token will just be
    /// the name of the prim. For all other prim paths, this will return the 
    /// absolute path as a string token. If the path is not a prim path, this
    /// will return an empty token.
    SDF_API
    static TfToken ConvertDefaultPrimPathToToken(const SdfPath &primPath);

    /// Returns the documentation string for this layer.
    ///
    /// The default value for documentation is "".
    SDF_API
    std::string GetDocumentation() const;

    /// Sets the documentation string for this layer.
    SDF_API
    void SetDocumentation(const std::string &documentation);

    /// Returns the layer's start timeCode.
    ///
    /// The start and end timeCodes of a layer represent the suggested playback 
    /// range.  However, time-varying content is not limited to the timeCode range 
    /// of the layer.
    ///
    /// The default value for startTimeCode is 0.
    SDF_API
    double GetStartTimeCode() const;

    /// Sets the layer's start timeCode.
    SDF_API
    void SetStartTimeCode(double startTimecode);

    /// Returns true if the layer has a startTimeCode opinion.
    SDF_API
    bool HasStartTimeCode() const;

    /// Clear the startTimeCode opinion.
    SDF_API
    void ClearStartTimeCode();
    
    /// Returns the layer's end timeCode.
    /// The start and end timeCode of a layer represent a suggested playback range.  
    /// However, time-varying content is not limited to the timeCode range of the 
    /// layer.
    ///
    /// The default value for endTimeCode is 0.
    SDF_API
    double GetEndTimeCode() const;

    /// Sets the layer's end timeCode.
    SDF_API
    void SetEndTimeCode(double endTimeCode);

    /// Returns true if the layer has an endTimeCode opinion.
    SDF_API
    bool HasEndTimeCode() const;

    /// Clear the endTimeCode opinion.
    SDF_API
    void ClearEndTimeCode();
    
    /// Returns the layer's timeCodes per second.
    /// 
    /// Scales the time ordinate for samples contained in the file to seconds.  
    /// If timeCodesPerSecond is 24, then a sample at time ordinate 24 should 
    /// be viewed exactly one second after the sample at time ordinate 0.
    ///
    /// If this layer doesn't have an authored value for timeCodesPerSecond, but
    /// it does have an authored value for framesPerSecond, this method will
    /// return the value of framesPerSecond.  This "dynamic fallback" allows
    /// layers to lock framesPerSecond and timeCodesPerSecond to the same value
    /// by specifying only framesPerSecond.
    /// 
    /// The default value of timeCodesPerSecond, used only if there is no 
    /// authored value for either timeCodesPerSecond or framesPerSecond, is 24.
    SDF_API
    double GetTimeCodesPerSecond() const;

    /// Sets the layer's timeCodes per second
    SDF_API
    void SetTimeCodesPerSecond(double timeCodesPerSecond);

    /// Returns true if the layer has a timeCodesPerSecond opinion.
    SDF_API
    bool HasTimeCodesPerSecond() const;

    /// Clear the timeCodesPerSecond opinion.
    SDF_API
    void ClearTimeCodesPerSecond();

    /// Returns the layer's frames per second.
    /// 
    /// This makes an advisory statement about how the contained data can be 
    /// most usefully consumed and presented.  It's primarily an indication of 
    /// the expected playback rate for the data, but a timeline editing tool 
    /// might also want to use this to decide how to scale and label its 
    /// timeline.  
    /// 
    /// The default  value for framesPerSecond is 24.
    SDF_API
    double GetFramesPerSecond() const;

    /// Sets the layer's frames per second
    SDF_API
    void SetFramesPerSecond(double framesPerSecond);

    /// Returns true if the layer has a frames per second opinion.
    SDF_API
    bool HasFramesPerSecond() const;

    /// Clear the framesPerSecond opinion.
    SDF_API
    void ClearFramesPerSecond();
    
    /// Returns the layer's frame precision.
    SDF_API
    int GetFramePrecision() const;

    /// Sets the layer's frame precision.
    SDF_API
    void SetFramePrecision(int framePrecision);

    /// Returns true if the layer has a frames precision opinion.
    SDF_API
    bool HasFramePrecision() const;

    /// Clear the framePrecision opinion.
    SDF_API
    void ClearFramePrecision();

    /// Returns the layer's owner.
    SDF_API
    std::string GetOwner() const;

    /// Sets the layer's owner.
    SDF_API
    void SetOwner(const std::string& owner);

    /// Returns true if the layer has an owner opinion.
    SDF_API
    bool HasOwner() const;

    /// Clear the owner opinion.
    SDF_API
    void ClearOwner();

    /// Returns the layer's session owner.
    /// Note: This should only be used by session layers.
    SDF_API
    std::string GetSessionOwner() const;

    /// Sets the layer's session owner.
    /// Note: This should only be used by session layers.
    SDF_API
    void SetSessionOwner(const std::string& owner);

    /// Returns true if the layer has a session owner opinion.
    SDF_API
    bool HasSessionOwner() const;

    // Clear the session owner opinion.
    SDF_API
    void ClearSessionOwner();

    /// Returns true if the layer's sublayers are expected to have owners.
    SDF_API
    bool GetHasOwnedSubLayers() const;

    /// Sets whether the layer's sublayers are expected to have owners.
    SDF_API
    void SetHasOwnedSubLayers(bool);

    /// Returns the CustomLayerData dictionary associated with this layer.
    /// 
    /// This is a dictionary is custom metadata that is associated with
    /// this layer. It allows users to encode any set of information for
    /// human or program consumption.
    SDF_API
    VtDictionary GetCustomLayerData() const;

    /// Sets the CustomLayerData dictionary associated with this layer.
    SDF_API
    void SetCustomLayerData(const VtDictionary& value);

    /// Returns true if CustomLayerData is authored on the layer.
    SDF_API
    bool HasCustomLayerData() const;

    /// Clears out the CustomLayerData dictionary associated with this layer.
    SDF_API
    void ClearCustomLayerData();

    /// Returns the expression variables dictionary authored on this layer.
    /// See \ref Sdf_Page_VariableExpressions for more details.
    SDF_API
    VtDictionary GetExpressionVariables() const;
    
    /// Sets the expression variables dictionary for this layer.
    SDF_API
    void SetExpressionVariables(const VtDictionary& expressionVars);

    /// Returns true if expression variables are authored on this layer.
    SDF_API
    bool HasExpressionVariables() const;

    /// Clears the expression variables dictionary authored on this layer.
    SDF_API
    void ClearExpressionVariables();

    /// @}
    /// \name Prims
    /// @{

    // Type for root prims view.
    typedef SdfPrimSpecView RootPrimsView;

    /// Returns a vector of the layer's root prims
    SDF_API
    RootPrimsView GetRootPrims() const;

    /// Sets a new vector of root prims.
    /// You can re-order, insert and remove prims but cannot 
    /// rename them this way.  If any of the listed prims have 
    /// an existing owner, they will be reparented.
    SDF_API
    void SetRootPrims(const SdfPrimSpecHandleVector &rootPrims);

    /// Adds a new root prim at the given index.
    /// If the index is -1, the prim is inserted at the end.
    /// The layer will take ownership of the prim, via a TfRefPtr.
    /// Returns true if successful, false if failed (for example,
    /// due to a duplicate name).
    SDF_API
    bool InsertRootPrim(const SdfPrimSpecHandle &prim, int index = -1);

    /// Remove a root prim.
    SDF_API
    void RemoveRootPrim(const SdfPrimSpecHandle &prim);

    /// Cause \p spec to be removed if it no longer affects the scene when the 
    /// last change block is closed, or now if there are no change blocks.
    SDF_API
    void ScheduleRemoveIfInert(const SdfSpec& spec);

    /// Removes scene description that does not affect the scene in the 
    /// layer namespace beginning with \p prim.
    ///
    /// Calling this method on a prim will only clean up prims with specifier
    /// 'over' that are not contributing any opinions.  The \p prim will only
    /// be removed if all of its nameChildren are also inert. The hierarchy 
    /// \p prim is defined in will be pruned up to the layer root for each 
    /// successive inert parent that has specifier 'over'.
    /// 
    /// note: PrimSpecs that contain any PropertySpecs, even PropertySpecs with 
    ///       required fields only (see PropertySpec::HasRequiredFieldsOnly) 
    ///       are not considered inert, and thus the prim won't be removed.
    SDF_API
    void RemovePrimIfInert(SdfPrimSpecHandle prim);

    /// Removes prop if it has only required fields (i.e. is not 
    /// contributing any opinions to the scene other than property 
    /// instantiation).
    /// 
    /// The hierarchy \p prop is defined in will then be pruned up to the 
    /// layer root for each successive inert parent.
    SDF_API
    void RemovePropertyIfHasOnlyRequiredFields(SdfPropertySpecHandle prop);

    /// Removes all scene description in this layer that does not affect the
    /// scene.
    ///
    /// This method walks the layer namespace hierarchy and removes any prims
    /// and that are not contributing any opinions.
    SDF_API
    void RemoveInertSceneDescription();

    /// Returns the list of prim names for this layer's reorder rootPrims
    /// statement.
    ///
    /// See SetRootPrimOrder() for more info.
    SDF_API
    SdfNameOrderProxy GetRootPrimOrder() const;
   
    /// Given a list of (possible sparse) prim names, authors a reorder
    /// rootPrims statement for this prim. 
    ///
    /// This reorder statement can modify the order of root prims that have 
    /// already been explicitly ordered with InsertRootPrim() or SetRootPrims();
    /// but only during composition.  Therefore, GetRootPrims(), 
    /// InsertRootPrim(), SetRootPrims(), etc. do not read, author, or pay any 
    /// attention to this statement.
    SDF_API
    void SetRootPrimOrder(const std::vector<TfToken>& names);

    /// Adds a new root prim name in the root prim order.
    /// If the index is -1, the name is inserted at the end.
    SDF_API
    void InsertInRootPrimOrder(const TfToken &name, int index = -1);

    /// Removes a root prim name from the root prim order.
    SDF_API
    void RemoveFromRootPrimOrder(const TfToken & name);

    /// Removes a root prim name from the root prim order by index.
    SDF_API
    void RemoveFromRootPrimOrderByIndex(int index);

    /// Reorders the given list of prim names according to the reorder rootPrims
    /// statement for this layer.
    ///
    /// This routine employs the standard list editing operations for ordered
    /// items in a ListEditor.
    SDF_API
    void ApplyRootPrimOrder(std::vector<TfToken> *vec) const;

    /// @}
    /// \name Sublayers
    /// @{

    /// Returns a proxy for this layer's sublayers.
    ///
    /// Sub-layers are the weaker layers directly included by this layer.
    /// They're in order from strongest to weakest and they're all weaker
    /// than this layer.
    ///
    /// Edits through the proxy changes the sublayers.  If this layer does
    /// not have any sublayers the proxy is empty.
    ///
    /// Sub-layer paths are asset paths, and thus must contain valid asset path
    /// characters (UTF-8 without C0 and C1 controls).  See SdfAssetPath for
    /// more details.
    SDF_API
    SdfSubLayerProxy GetSubLayerPaths() const;

    /// Sets the paths of the layer's sublayers.
    SDF_API
    void SetSubLayerPaths(const std::vector<std::string>& newPaths);

    /// Returns the number of sublayer paths (and offsets).
    SDF_API
    size_t GetNumSubLayerPaths() const;

    /// Inserts new sublayer path at the given index.
    ///
    /// The default index of -1 means to insert at the end.
    SDF_API
    void InsertSubLayerPath(const std::string& path, int index = -1);

    /// Removes sublayer path at the given index.
    SDF_API
    void RemoveSubLayerPath(int index);

    /// Returns the layer offsets for all the subLayer paths.
    SDF_API
    SdfLayerOffsetVector GetSubLayerOffsets() const;

    /// Returns the layer offset for the subLayer path at the given index.
    SDF_API
    SdfLayerOffset GetSubLayerOffset(int index) const;

    /// Sets the layer offset for the subLayer path at the given index.
    SDF_API
    void SetSubLayerOffset(const SdfLayerOffset& offset, int index);

    /// @}
    /// \name Relocates
    /// @{

    /// Get the list of relocates specified in this layer's metadata.
    ///
    /// Each individual relocate in the list is specified as a pair of 
    /// \c \SdfPath where the first is the source path of the relocate and the
    /// second is target path.
    ///
    /// Note that is NOT a proxy object and cannot be used to edit the field in
    /// place. 
    SDF_API
    SdfRelocates GetRelocates() const;
    
    /// Set the entire list of namespace relocations specified on this layer to
    /// \p relocates.
    SDF_API
    void SetRelocates(const SdfRelocates& relocates);

    /// Returns true if this layer's metadata has any relocates opinion, 
    /// including that there should be no relocates (i.e. an empty list).  An 
    /// empty list (no relocates) does not mean the same thing as a missing list
    /// (no opinion).
    SDF_API
    bool HasRelocates() const;
    
    /// Clears the layer relocates opinion in the layer's metadata.
    SDF_API
    void ClearRelocates();

    /// @}

    /// \name Detached Layers
    ///
    /// Detached layers are layers that are detached from the serialized
    /// data store and isolated from any external changes to that serialized
    /// data.
    ///
    /// File format plugins may produce layers that maintain a persistent
    /// connection to their serialized representation to read data on-demand.
    /// For example, a file format might set up layers to hold an open file
    /// handle and read attribute time samples from it only when requested, to
    /// avoid pulling in unnecessary data. However, there may be times when
    /// keeping this connection is undesirable. In the previous example, a
    /// crash might occur if some other process were to change the file on
    /// disk, or users might be prevented from overwriting the file at all
    /// which could interfere with workflow.
    ///
    /// To avoid these problems, the functions below may be used to specify
    /// layers that are to be detached from the original serialized data.
    ///
    /// @{

    /// \class DetachedLayerRules
    ///
    /// Object used to specify detached layers. Layers may be included or
    /// excluded from the detached layer set by specifying simple substring
    /// patterns for layer identifiers. For example, the following will
    /// include all layers in the detached layer set, except for those whose
    /// identifiers contain the substring "sim" or "geom":
    ///
    /// \code
    /// SdfLayer::SetDetachedLayerRules(
    ///     SdfLayer::DetachedLayerRules()
    ///         .IncludeAll();
    ///         .Exclude({"sim", "geom"})
    /// );
    /// \endcode
    class DetachedLayerRules
    {
    public:
        /// A default constructed rules object Excludes all layers from
        /// the detached layer set.
        DetachedLayerRules() = default;

        /// Include all layers in the detached layer set.
        DetachedLayerRules& IncludeAll() 
        { 
            _includeAll = true; 
            _include.clear();
            return *this; 
        }

        /// Include layers whose identifiers contain any of the strings in
        /// \p patterns in the detached layer set.
        SDF_API
        DetachedLayerRules& Include(const std::vector<std::string>& patterns);

        /// Exclude layers whose identifiers contain any of the strings in
        /// \p patterns from the detached layer set.
        SDF_API
        DetachedLayerRules& Exclude(const std::vector<std::string>& patterns);

        bool IncludedAll() const { return _includeAll; }
        const std::vector<std::string>& GetIncluded() const { return _include; }
        const std::vector<std::string>& GetExcluded() const { return _exclude; }

        /// Returns true if \p identifier is included in the detached layer set,
        /// false otherwise.
        ///
        /// \p identifier is included if it matches an include pattern (or the
        /// mask includes all identifiers) and it does not match any of the
        /// exclude patterns. Anonymous layer identifiers are always excluded
        /// from the mask.
        SDF_API
        bool IsIncluded(const std::string& identifier) const;

    private:
        friend class SdfLayer;

        std::vector<std::string> _include;
        std::vector<std::string> _exclude;
        bool _includeAll = false;
    };

    /// Sets the rules specifying detached layers.
    ///
    /// Newly-created or opened layers whose identifiers are included in
    /// \p rules will be opened as detached layers. Existing layers that are now
    /// included or no longer included will be reloaded. Any unsaved
    /// modifications to those layers will be lost.
    ///
    /// This function is not thread-safe. It may not be run concurrently with
    /// any other functions that open, close, or read from any layers.
    ///
    /// The detached layer rules are initially set to exclude all layers.
    /// This may be overridden by setting the environment variables
    /// SDF_LAYER_INCLUDE_DETACHED and SDF_LAYER_EXCLUDE_DETACHED to specify
    /// the initial set of include and exclude patterns in the rules. These
    /// variables can be set to a comma-delimited list of patterns.
    /// SDF_LAYER_INCLUDE_DETACHED may also be set to "*" to include
    /// all layers. Note that these environment variables only set the initial
    /// state of the detached layer rules; these values may be overwritten by
    /// subsequent calls to this function.
    ///
    /// See SdfLayer::DetachedLayerRules::IsIncluded for details on how the
    /// rules are applied to layer identifiers.
    SDF_API
    static void SetDetachedLayerRules(const DetachedLayerRules& mask);

    /// Returns the current rules for the detached layer set.
    SDF_API
    static const DetachedLayerRules& GetDetachedLayerRules();

    /// Returns whether the given layer identifier is included in the
    /// current rules for the detached layer set. This is equivalent to
    /// GetDetachedLayerRules().IsIncluded(identifier).
    SDF_API
    static bool IsIncludedByDetachedLayerRules(const std::string& identifier);

    /// @}

    /// \name Muting
    /// @{

    /// Returns the set of muted layer paths.
    SDF_API
    static std::set<std::string> GetMutedLayers();

    /// Returns \c true if the current layer is muted.
    SDF_API
    bool IsMuted() const;

    /// Returns \c true if the specified layer path is muted.
    SDF_API
    static bool IsMuted(const std::string &path);

    /// Mutes the current layer if \p muted is \c true, and unmutes it
    /// otherwise.
    SDF_API
    void SetMuted(bool muted);

    /// Add the specified path to the muted layers set.
    SDF_API
    static void AddToMutedLayers(const std::string &mutedPath);

    /// Remove the specified path from the muted layers set.
    SDF_API
    static void RemoveFromMutedLayers(const std::string &mutedPath);

    /// @}
    /// \name Lookup
    /// @{

    /// Returns the layer's pseudo-root prim.
    ///
    /// The layer's root prims are namespace children of the pseudo-root.
    /// The pseudo-root exists to make the namespace hierarchy a tree
    /// instead of a forest.  This simplifies the implementation of
    /// some algorithms.
    ///
    /// A layer always has a pseudo-root prim.
    SDF_API
    SdfPrimSpecHandle GetPseudoRoot() const;

    /// Returns the object at the given \p path.
    ///
    /// There is no distinction between an absolute and relative path
    /// at the SdLayer level.
    ///
    /// Returns \c NULL if there is no object at \p path.
    SDF_API
    SdfSpecHandle GetObjectAtPath(const SdfPath &path);

    /// Returns the prim at the given \p path.
    ///
    /// Returns \c NULL if there is no prim at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SDF_API
    SdfPrimSpecHandle GetPrimAtPath(const SdfPath &path);

    /// Returns a property at the given \p path.
    ///
    /// Returns \c NULL if there is no property at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SDF_API
    SdfPropertySpecHandle GetPropertyAtPath(const SdfPath &path);

    /// Returns an attribute at the given \p path.
    ///
    /// Returns \c NULL if there is no attribute at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SDF_API
    SdfAttributeSpecHandle GetAttributeAtPath(const SdfPath &path);

    /// Returns a relationship at the given \p path.
    ///
    /// Returns \c NULL if there is no relationship at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SDF_API
    SdfRelationshipSpecHandle GetRelationshipAtPath(const SdfPath &path);

    /// @}
    /// \name Permissions
    /// @{

    /// Returns true if the caller is allowed to modify the layer and 
    /// false otherwise.  A layer may have to perform some action to acquire 
    /// permission to be edited.
    SDF_API
    bool PermissionToEdit() const;

    /// Returns true if the caller is allowed to save the layer to its 
    /// existing fileName and false otherwise.
    SDF_API
    bool PermissionToSave() const;

    /// Sets permission to edit.
    SDF_API
    void SetPermissionToEdit(bool allow);

    /// Sets permission to save.
    SDF_API
    void SetPermissionToSave(bool allow);

    /// @}
    /// \name Batch namespace editing
    /// @{

    /// Check if a batch of namespace edits will succeed.  This returns
    /// \c SdfNamespaceEditDetail::Okay if they will succeed as a batch,
    /// \c SdfNamespaceEditDetail::Unbatched if the edits will succeed but
    /// will be applied unbatched, and \c SdfNamespaceEditDetail::Error
    /// if they will not succeed.  No edits will be performed in any case.
    ///
    /// If \p details is not \c NULL and the method does not return \c Okay
    /// then details about the problems will be appended to \p details.  A
    /// problem may cause the method to return early, so \p details may not
    /// list every problem.
    ///
    /// Note that Sdf does not track backpointers so it's unable to fix up
    /// targets/connections to namespace edited objects.  Clients must fix
    /// those to prevent them from falling off.  In addition, this method
    /// will report failure if any relational attribute with a target to
    /// a namespace edited object is subsequently edited (in the same
    /// batch).  Clients should perform edits on relational attributes
    /// first.
    ///
    /// Clients may wish to report unbatch details to the user to confirm
    /// that the edits should be applied unbatched.  This will give the
    /// user a chance to correct any problems that cause batching to fail
    /// and try again.
    SDF_API
    SdfNamespaceEditDetail::Result
    CanApply(const SdfBatchNamespaceEdit&,
             SdfNamespaceEditDetailVector* details = NULL) const;

    /// Performs a batch of namespace edits.  Returns \c true on success
    /// and \c false on failure.  On failure, no namespace edits will have
    /// occurred.
    SDF_API
    bool Apply(const SdfBatchNamespaceEdit&);

    /// @}
    /// \name Layer state
    /// @{

    /// Returns the state delegate used to manage this layer's authoring
    /// state.
    SDF_API
    SdfLayerStateDelegateBasePtr GetStateDelegate() const;

    /// Sets the state delegate used to manage this layer's authoring
    /// state. The 'dirty' state of this layer will be transferred to
    /// the new delegate.
    SDF_API
    void SetStateDelegate(const SdfLayerStateDelegateBaseRefPtr& delegate);

    /// Returns \c true if the layer is dirty, i.e. has changed from
    /// its persistent representation.
    SDF_API
    bool IsDirty() const;

    /// @}

    /// \name Time-sample API
    /// @{
    SDF_API
    std::set<double> ListAllTimeSamples() const;
    
    SDF_API
    std::set<double> 
    ListTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    bool GetBracketingTimeSamples(double time, double* tLower, double* tUpper);

    SDF_API
    size_t GetNumTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    bool GetBracketingTimeSamplesForPath(const SdfPath& path, 
                                         double time,
                                         double* tLower, double* tUpper) const;

    /// Returns the previous time sample authored just before the querying \p 
    /// time.
    ///
    /// If there is no time sample authored just before \p time, this function
    /// returns false. Otherwise, it returns true and sets \p tPrevious to the
    /// time of the previous sample.
    SDF_API
    bool GetPreviousTimeSampleForPath(const SdfPath& path, double time,
                                      double* tPrevious) const;

    SDF_API
    bool QueryTimeSample(const SdfPath& path, double time, 
                         VtValue *value=NULL) const;

    SDF_API
    bool QueryTimeSample(const SdfPath& path, double time, 
                         SdfAbstractDataValue *value) const;

    template <class T>
    bool QueryTimeSample(const SdfPath& path, double time, 
                         T* data) const
    {
        if (!data) {
            return QueryTimeSample(path, time);
        }

        SdfAbstractDataTypedValue<T> outValue(data);
        const bool hasValue = QueryTimeSample(
            path, time, static_cast<SdfAbstractDataValue *>(&outValue));

        if (std::is_same<T, SdfValueBlock>::value) {
            return hasValue && outValue.isValueBlock;
        }

        return hasValue && (!outValue.isValueBlock);
    }

    SDF_API
    void SetTimeSample(const SdfPath& path, double time, 
                       const VtValue & value);

    SDF_API
    void SetTimeSample(const SdfPath& path, double time, 
                       const SdfAbstractDataConstValue& value);

    template <class T>
    void SetTimeSample(const SdfPath& path, double time, 
                       const T& value)
    {
        const SdfAbstractDataConstTypedValue<T> inValue(&value);
        const SdfAbstractDataConstValue& untypedInValue = inValue;
        return SetTimeSample(path, time, untypedInValue);
    }

    SDF_API
    void EraseTimeSample(const SdfPath& path, double time);

    /// @}

    // Debugging
    // @{

    SDF_API
    static void DumpLayerInfo();

    // Write this layer's SdfData to a file in a simple generic format.
    SDF_API
    bool WriteDataFile(const std::string &filename);

    // @}

    /// Returns a \ref SdfChangeList containing the minimal edits that would be
    /// needed to transform this layer to match the contents of the given
    /// \p layer parameter. If \p processPropertyFields is false, property 
    /// fields will be ignored during the diff computation. Any differences in 
    /// fields between properties with the same path in this layer and \p layer
    /// will not be captured in the returned SdfChangeList.  This can, however,
    /// avoid potentially expensive data retrieval operations.
    SDF_API
    SdfChangeList CreateDiff(
        const SdfLayerHandle& layer,
        bool processPropertyFields = true) const;

protected:
    // Private constructor -- use New(), FindOrCreate(), etc.
    // Precondition: _layerRegistryMutex must be locked.
    SdfLayer(const SdfFileFormatConstPtr& fileFormat,
             const std::string &identifier,
             const std::string &realPath = std::string(),
             const ArAssetInfo& assetInfo = ArAssetInfo(),
             const FileFormatArguments &args = FileFormatArguments(),
             bool validateAuthoring = false);

private:
    // Create a new layer.
    // Precondition: _layerRegistryMutex must be locked.
    static SdfLayerRefPtr _CreateNew(
        SdfFileFormatConstPtr fileFormat,
        const std::string& identifier,
        const FileFormatArguments& args,
        bool saveLayer = true);

    static SdfLayerRefPtr _CreateNewWithFormat(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo = ArAssetInfo(),
        const FileFormatArguments& args = FileFormatArguments());

    static SdfLayerRefPtr _CreateAnonymousWithFormat(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string& tag,
        const FileFormatArguments& args);

    // Finish initializing this layer (which may have succeeded or not)
    // and publish the results to other threads by unlocking the mutex.
    // Sets _initializationWasSuccessful.
    void _FinishInitialization(bool success);

    // Layers retrieved from the layer registry may still be in the
    // process of having their contents initialized.  Other threads
    // retrieving layers from the registry must wait until initialization
    // is complete, using this method.
    // Returns _initializationWasSuccessful.
    //
    // Callers *must* be holding an SdfLayerRefPtr to this layer to
    // ensure that it is not deleted out from under them, in
    // case initialization fails.  (This method cannot acquire the
    // reference itself internally without being susceptible to a race.)
    bool _WaitForInitializationAndCheckIfSuccessful();

    // Returns whether or not this layer should post change 
    // notification.  This simply returns (!_GetIsLoading())
    bool _ShouldNotify() const;

    // This function keeps track of the last state of IsDirty() before
    // updating it. It returns false if the last saved dirty state is the
    // same than the current state. It returns true if the state differs and
    // will update the 'last dirty state' to the current state. So, after
    // returning true, it would return false for subsequent calls until the
    // IsDirty() state would change again...
    bool _UpdateLastDirtinessState() const;

    // Returns a handle to the spec at the given path if it exists and matches
    // type T.
    template <class T>
    SdfHandle<T> _GetSpecAtPath(const SdfPath& path);

    // Returns true if a spec can be retrieved at the given path, false
    // otherwise. This function will return the canonicalized path to the
    // spec as well as the spec type.
    bool _CanGetSpecAtPath(const SdfPath& path, 
                           SdfPath* canonicalPath, SdfSpecType* specType) const;

    /// Initialize layer internals that are based on it's path.
    /// This includes the asset path and show path the layer to be loaded
    /// reflects at the point of initialization.
    void _InitializeFromIdentifier(
        const std::string &identifier,
        const std::string &realPath = std::string(),
        const std::string &fileVersion = std::string(),
        const ArAssetInfo& assetInfo = ArAssetInfo());

    // Helper for computing the necessary information to lookup a layer
    // in the registry or open the layer.
    struct _FindOrOpenLayerInfo;
    static bool _ComputeInfoToFindOrOpenLayer(
        const std::string& identifier,
        const SdfLayer::FileFormatArguments& args,
        _FindOrOpenLayerInfo* info,
        bool computeAssetInfo = false);

    // Open a layer, adding an entry to the registry and releasing
    // the registry lock.
    // Precondition: _layerRegistryMutex must be locked.
    template <class Lock>
    static SdfLayerRefPtr _OpenLayerAndUnlockRegistry(
        Lock &lock,
        const _FindOrOpenLayerInfo& info,
        bool metadataOnly);

    // Helper function for finding a layer with \p identifier and \p args.
    // \p lock must be unlocked initially and will be locked by this
    // function when needed. See docs for \p retryAsWriter argument on
    // _TryToFindLayer for details on the final state of the lock when
    // this function returns.
    template <class ScopedLock>
    static SdfLayerRefPtr
    _Find(const std::string &identifier,
          const FileFormatArguments &args,
          ScopedLock &lock, bool retryAsWriter);

    // Helper function to try to find the layer with \p identifier and
    // pre-resolved path \p resolvedPath in the registry.  Caller must hold
    // registry \p lock for reading.  If \p retryAsWriter is false, lock is
    // released upon return.  Otherwise the lock is released upon return if a
    // layer is found successfully.  If no layer is found then the lock is
    // upgraded to a writer lock upon return.  Note that this upgrade may not be
    // atomic, but this function ensures that if upon return there does not
    // exist a matching layer in the registry.
    template <class ScopedLock>
    static SdfLayerRefPtr
    _TryToFindLayer(const std::string &identifier,
                    const ArResolvedPath &resolvedPath,
                    ScopedLock &lock, bool retryAsWriter);

    /// Returns true if the spec at the specified path has no effect on the 
    /// scene.
    /// 
    /// If ignoreChildren is true, this will ignore prim and property
    /// children of prim specs. Property specs are always considered to be 
    /// non-inert unless they have only required fields and 
    /// requiredFieldOnlyPropertiesareInert is set to false.
    bool _IsInert(const SdfPath &path, bool ignoreChildren, 
                  bool requiredFieldOnlyPropertiesAreInert = false) const;

    /// Return true if the entire subtree rooted at \a path does not affect the 
    /// scene. For this purpose, property specs that have only required fields 
    /// are considered inert.
    bool _IsInertSubtree(const SdfPath &path) const;

    /// Cause \p spec to be removed if it does not affect the scene. This 
    /// removes any empty descendants before checking if \p spec itself is 
    /// inert. Property specs are always considered non-inert, so this will 
    /// remove them if they have only required fields (see 
    /// PropertySpec::HasOnlyRequiredFields). This also removes inert ancestors.
    void _RemoveIfInert(const SdfSpec& spec);

    /// Performs a depth first search of the namespace hierarchy, beginning at
    /// \p prim, removing prims that do not affect the scene. The return value 
    /// indicates whether the prim passed in is now inert as a result of this 
    /// call, and can itself be removed.
    bool _RemoveInertDFS(SdfPrimSpecHandle prim);

    /// If \p prim is inert (has no affect on the scene), removes prim, then 
    /// prunes inert parent prims back to the root.
    void _RemoveInertToRootmost(SdfPrimSpecHandle prim);

    /// Returns whether this layer is validating authoring operations.
    bool _ValidateAuthoring() const { return _validateAuthoring; }

    /// Returns the path used in the muted layers set.
    std::string _GetMutedPath() const;

    // If old and new asset path is given, rename all external prim
    // composition dependency referring to the old path.
    void _UpdatePrimCompositionDependencyPaths(
        const SdfPrimSpecHandle &parent,
        const std::string &oldLayerPath,
        const std::string &newLayerPath);

    // Set the clean state to the current state.
    void _MarkCurrentStateAsClean() const;

    // Return the field definition for \p fieldName if \p fieldName is a
    // required field for the spec type identified by \p path.
    inline SdfSchema::FieldDefinition const *
    _GetRequiredFieldDef(const SdfPath &path,
                         const TfToken &fieldName,
                         SdfSpecType specType = SdfSpecTypeUnknown) const;

    // Return the field definition for \p fieldName if \p fieldName is a
    // required field for \p specType subject to \p schema.
    static inline SdfSchema::FieldDefinition const *
    _GetRequiredFieldDef(const SdfSchemaBase &schema,
                         const TfToken &fieldName,
                         SdfSpecType specType);

    // Helper to list all fields on \p data at \p path subject to \p schema.
    static std::vector<TfToken>
    _ListFields(SdfSchemaBase const &schema,
                SdfAbstractData const &data, const SdfPath& path);

    // Helper for HasField for \p path in \p data subject to \p schema.
    static inline bool
    _HasField(const SdfSchemaBase &schema,
              const SdfAbstractData &data,
              const SdfPath& path,
              const TfToken& fieldName,
              VtValue *value);

    // Helper to get a field value for \p path in \p data subject to \p schema.
    static inline VtValue
    _GetField(const SdfSchemaBase &schema,
              const SdfAbstractData &data,
              const SdfPath& path,
              const TfToken& fieldName);
    
    // Set a value.
    template <class T>
    void _SetValue(const TfToken& key, T value);

    // Get a value.
    template <class T>
    T _GetValue(const TfToken& key) const;

    enum _ReloadResult { _ReloadFailed, _ReloadSucceeded, _ReloadSkipped };
    _ReloadResult _Reload(bool force);

    // Reads contents of asset specified by \p identifier with resolved
    // path \p resolvedPath into this layer.
    bool _Read(const std::string& identifier, 
               const ArResolvedPath& resolvedPath, 
               bool metadataOnly);
    
    // Saves this layer if it is dirty or the layer doesn't already exist
    // on disk. If \p force is true, the layer will be written out
    // regardless of those conditions.
    bool _Save(bool force) const;

    // A helper method used by Save and Export.
    // This method allows Save to specify the existing file format and Export
    // to use the format provided by the file extension in newFileName. If no
    // file format can be discovered from the file name, the existing file
    // format associated with the layer will be used in both cases. This allows
    // users to export and save to any file name, regardless of extension.
    bool _WriteToFile(const std::string& newFileName, 
                      const std::string& comment, 
                      SdfFileFormatConstPtr fileFormat,
                      const FileFormatArguments& args) const;

    // Swap contents of _data and data. This operation does not register
    // inverses or emit change notification.
    void _SwapData(SdfAbstractDataRefPtr &data);

    // Set _data to \p newData and send coarse DidReplaceLayerContent
    // invalidation notice.
    void _AdoptData(const SdfAbstractDataRefPtr &newData);

    // Helper function which will process incoming data to this layer in a
    // generic way. 
    // If \p processPropertyFields is false, this method will not
    // consider property spec fields. In some cases, this can avoid expensive
    // operations which would pull large amounts of data.
    template<typename DeleteSpecFunc, typename CreateSpecFunc, 
            typename GetFieldValuesFunc, typename SetFieldFunc, typename ErrorFunc>
    void _ProcessIncomingData(const SdfAbstractDataPtr &newData,
                              const SdfSchemaBase *newDataSchema,
                              bool processPropertyFields,
                              const DeleteSpecFunc &deleteSpecFunc,
                              const CreateSpecFunc &createSpecFunc,
                              const GetFieldValuesFunc &getFieldValuesFunc,
                              const SetFieldFunc &setFieldFunc,
                              const ErrorFunc &errorFunc) const;

    // Set _data to match data, calling other primitive setter methods to
    // provide fine-grained inverses and notification.  If \p data might adhere
    // to a different schema than this layer's, pass a pointer to it as \p
    // newDataSchema.  In this case, check to see if fields from \p data are
    // known to this layer's schema, and if not, omit them and issue a TfError
    // with SdfAuthoringErrorUnrecognizedFields, but continue to set all other
    // known fields.
    void _SetData(const SdfAbstractDataPtr &newData,
                  const SdfSchemaBase *newDataSchema=nullptr);

    // Returns const handle to _data.
    SdfAbstractDataConstPtr _GetData() const;

    // Returns a new SdfAbstractData object for this layer.
    SdfAbstractDataRefPtr _CreateData() const;

    // Inverse primitive for setting a single field. The previous value for the
    // field may be given via \p oldValue. If \p oldValue is non-nullptr, the
    // VtValue it points to will be moved-from after the function completes. If
    // \p oldValue is nullptr, the old field value will be retrieved
    // automatically.
    template <class T>
    void _PrimSetField(const SdfPath& path, 
                       const TfToken& fieldName,
                       const T& value,
                       VtValue *oldValue = nullptr,
                       bool useDelegate = true);

    // Inverse primitive for setting a single key in a dict-valued field.  The
    // previous dictionary value for the field (*not* the individual entry) may
    // be supplied via \p oldValue. If \p oldValue is non-nullptr, the VtValue
    // it points to will be moved-from after the function completes. If \p
    // oldValue is nullptr, the old field value will be retrieved automatically.
    template <class T>
    void _PrimSetFieldDictValueByKey(const SdfPath& path,
                                     const TfToken& fieldName,
                                     const TfToken& keyPath,
                                     const T& value,
                                     VtValue *oldValue = nullptr,
                                     bool useDelegate = true);

    // Primitive for appending a child to the list of children.
    template <class T>
    void _PrimPushChild(const SdfPath& parentPath,
                        const TfToken& fieldName,
                        const T& value,
                        bool useDelegate = true);
    template <class T>
    void _PrimPopChild(const SdfPath& parentPath,
                       const TfToken& fieldName,
                       bool useDelegate = true);

    // Move all the fields at all paths at or below \a oldPath to be
    // at a corresponding location at or below \a newPath. This does
    // not update the children fields of the parents of these paths.
    bool _MoveSpec(const SdfPath &oldPath, const SdfPath &newPath);

    // Inverse primitive for moving a spec.
    void _PrimMoveSpec(const SdfPath &oldPath, const SdfPath &newPath,
                       bool useDelegate = true);

    // Create a new spec of type \p specType at \p path.
    // Returns true if spec was successfully created, false otherwise.
    bool _CreateSpec(const SdfPath& path, SdfSpecType specType, bool inert);

    // Delete all the fields at or below the specified path. This does
    // not update the children field of the parent of \a path.
    bool _DeleteSpec(const SdfPath &path);

    // Inverse primitive for deleting a spec.
    void _PrimCreateSpec(const SdfPath &path, SdfSpecType specType, bool inert,
                         bool useDelegate = true);

    // Inverse primitive for deleting a spec.
    void _PrimDeleteSpec(const SdfPath &path, bool inert, 
                         bool useDelegate = true);

    // Inverse primitive for setting time samples.
    template <class T>
    void _PrimSetTimeSample(const SdfPath& path, double time,
                            const T& value,
                            bool useDelegate = true);

    // Helper method for Traverse. Visits the children of \a path using the
    // specified \a ChildPolicy.
    template <typename ChildPolicy>
    void _TraverseChildren(const SdfPath &path, const TraversalFunction &func);

private:
    SdfLayerHandle _self;

    // File format and arguments for this layer.
    SdfFileFormatConstPtr _fileFormat;
    FileFormatArguments _fileFormatArgs;

    // Cached reference to the _fileFormat's schema -- we need access to this to
    // be as fast as possible since we look at it on every SetField(), for
    // example.
    const SdfSchemaBase &_schema;

    // Registry of Sdf Identities
    mutable Sdf_IdentityRegistry _idRegistry;

    // The underlying SdfData which stores all the data in the layer.
    SdfAbstractDataRefPtr _data;

    // The state delegate for this layer.
    SdfLayerStateDelegateBaseRefPtr _stateDelegate;

    // Dispatcher used in layer initialization, letting waiters participate in
    // loading instead of just busy-waiting.
    WorkDispatcher _initDispatcher;
    
    // Atomic variable protecting layer initialization -- the interval between
    // adding a layer to the layer registry and finishing the process of
    // initializing its contents, at which point we can truly publish the layer
    // for consumption by concurrent threads. We add the layer to the registry
    // before initialization completes so that other threads can discover and
    // wait for it to finish initializing.
    std::atomic<bool> _initializationComplete;

    // This is an optional<bool> that is only set once initialization
    // is complete, before _initializationComplete is set.
    std::optional<bool> _initializationWasSuccessful;

    // remembers the last 'IsDirty' state.
    mutable bool _lastDirtyState;

    // Asset information for this layer.
    std::unique_ptr<Sdf_AssetInfo> _assetInfo;

    // Modification timestamp of the backing file asset when last read.
    mutable VtValue _assetModificationTime;

    // All external asset dependencies, with their modification timestamps, of
    // the layer when last read.
    mutable VtDictionary _externalAssetModificationTimes;

    // Mutable revision number for cache invalidation.
    mutable size_t _mutedLayersRevisionCache;

    // Cache of whether or not this layer is muted.  Only valid if
    // _mutedLayersRevisionCache is up-to-date with the global revision number.
    mutable bool _isMutedCache;

    // Layer permission bits.
    bool _permissionToEdit;
    bool _permissionToSave;

    // Whether layer edits are validated.
    bool _validateAuthoring;

    // Layer hints as of the most recent save operation.
    mutable SdfLayerHints _hints;

    // Allow access to _ValidateAuthoring() and _IsInert().
    friend class SdfSpec;
    friend class SdfPropertySpec;
    friend class SdfAttributeSpec;

    friend class Sdf_ChangeManager;

    // Allow access to _CreateSpec and _DeleteSpec and _MoveSpec
    template <class ChildPolicy> friend class Sdf_ChildrenUtils;

    // Give the file format access to our data.  Limit breaking encapsulation
    // to the base SdFileFormat class so we don't have to friend every
    // implementation here.
    friend class SdfFileFormat;

    // Give layer state delegates access to our data as well as to
    // the various _Prim functions.
    friend class SdfLayerStateDelegateBase;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LAYER_H
