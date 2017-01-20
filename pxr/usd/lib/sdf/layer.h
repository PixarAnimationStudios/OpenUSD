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
#ifndef SDF_LAYER_H
#define SDF_LAYER_H

/// \file sdf/layer.h

#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/identity.h"
#include "pxr/usd/sdf/layerBase.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/vt/value.h"

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include <mutex>
#include <set>
#include <string>
#include <vector>

TF_DECLARE_WEAK_PTRS(SdfFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerStateDelegateBase);

struct Sdf_AssetInfo;

/// \class SdfLayer 
///
/// A unit of scene description that you combine with other units of scene
/// description to form a shot, model, set, shader, and so on.
///
/// SdfLayer objects provide a persistent way to store layers on the
/// filesystem in .menva files.  Currently the supported file format is 
/// <c>.menva</c>, the ASCII file format. 
///
/// The FindOrOpen() method returns a new SdfLayer object with scene 
/// description from a <c>.menva</c> file. Once read, a layer remembers which 
/// asset it was read from. The Save() method saves the layer back out to the 
/// original file.  You can use the Export() method to write the layer to a 
/// different location. You can use the GetIdentifier() method to get the layer's
/// Id or GetRealPath() to get the resolved, full file path.
///
/// Layers can have a timeCode range (startTimeCode and endTimeCode). This range
/// represents the suggested playback range, but has no impact on the extent of 
/// the animation data that may be stored in the layer. The metadatum 
/// "timeCodesPerSecond" is used to annotate how the time ordinate for samples
/// contained in the file scales to seconds. For example, if timeCodesPerSecond
/// is 24, then a sample at time ordinate 24 should be viewed exactly one second
/// after the sample at time ordinate 0.
/// 
/// Compared to Menv2x, layers are most closely analogous to 
/// hooksets, <c>.hook</c> files, and <c>.cue</c> files.
///
/// \todo 
/// \li Insert a discussion of subLayers semantics here.
///
/// \todo
/// \li Should have validate... methods for rootPrims
///
class SdfLayer : public SdfLayerBase
{
    typedef SdfLayerBase Parent;
    typedef SdfLayer This;
public:
    /// Destructor.
    virtual ~SdfLayer(); 

    ///
    /// \name Primary API
    /// @{

    using SdfLayerBase::FileFormatArguments;

    /// Creates a new empty layer with the given identifier.
    ///
    /// The \p identifier must be either a real filesystem path or an asset
    /// path without version modifier. Attempting to create a layer using an
    /// identifier with a version specifier (e.g.  layer.menva@300100,
    /// layer.menva#5) raises a coding error, and returns a null layer
    /// pointer.
    ///
    /// Additional arguments may be supplied via the \p args parameter.
    /// These arguments may control behavior specific to the layer's
    /// file format.
    static SdfLayerRefPtr CreateNew(const std::string &identifier,
                                    const std::string &realPath = std::string(),
                                    const FileFormatArguments &args =
                                    FileFormatArguments());

    /// Creates a new empty layer with the given identifier for a given file
    /// format class.
    ///
    /// This function has the same behavior as the other CreateNew function,
    /// but uses the explicitly-specified \p fileFormat instead of attempting
    /// to discern the format from \p identifier.
    static SdfLayerRefPtr CreateNew(const SdfFileFormatConstPtr& fileFormat,
                                    const std::string &identifier,
                                    const std::string &realPath = std::string(),
                                    const FileFormatArguments &args =
                                    FileFormatArguments());

    /// Creates a new empty layer with the given identifier for a given file
    /// format class.
    ///
    /// This is so that Python File Format classes can create layers
    /// (CreateNew() doesn't work, because it already saves during
    /// construction of the layer. That is something specific (python
    /// generated) layer types may choose to not do.)
    ///
    /// The new layer will not be dirty.
    ///
    /// Additional arguments may be supplied via the \p args parameter. 
    /// These arguments may control behavior specific to the layer's
    /// file format.
    static SdfLayerRefPtr New(const SdfFileFormatConstPtr& fileFormat,
                              const std::string &identifier,
                              const std::string &realPath = std::string(),
                              const FileFormatArguments &args = 
                              FileFormatArguments());

    /// Returns the layer for the given path if found in the layer registry.
    /// If the layer cannot be found, a null handle is returned.
    static SdfLayerHandle Find(
        const std::string &identifier,
        const FileFormatArguments &args = FileFormatArguments());

    /// Returns the layer for \p layerPath, assumed to be relative to the path
    /// of the \p anchor layer. If the \p anchor layer is invalid, a coding
    /// error is raised, and a null handle is returned. If \p layerPath is not
    /// relative, this method is equivalent to \c Find(layerPath).
    static SdfLayerHandle FindRelativeToLayer(
        const SdfLayerHandle &anchor,
        const std::string &layerPath,
        const FileFormatArguments &args = FileFormatArguments());

    /// Return an existing layer with the given \p identifier and \p args, or 
    /// else load it from disk. If the layer can't be found or loaded, 
    /// an error is posted and a null layer is returned.
    ///
    /// Arguments in \p args will override any arguments specified in
    /// \p identifier.
    static SdfLayerRefPtr FindOrOpen(
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
    static SdfLayerRefPtr OpenAsAnonymous(
        const std::string &layerPath,
        bool metadataOnly = false);

    /// Returns the scene description schema for this layer.
    virtual const SdfSchemaBase& GetSchema() const;

    /// Returns the data from the absolute root path of this layer.
    SdfDataRefPtr GetMetadata() const;

    /// Returns handles for all layers currently held by the layer registry.
    static SdfLayerHandleSet GetLoadedLayers();

    /// Returns whether this layer has no significant data.
    bool IsEmpty() const;

    /// Copies the content of the given layer into this layer.
    /// Source layer is unmodified.
    void TransferContent(const SdfLayerHandle& layer);

    /// Creates a new \e anonymous layer with an optional \p tag. An anonymous
    /// layer is a layer with a system assigned identifier, that cannot be
    /// saved to disk via Save(). Anonymous layers have an identifier, but no
    /// repository, overlay, real path, or other asset information fields.
    /// Anonymous layers may be tagged, which can be done to aid debugging
    /// subsystems that make use of anonymous layers.  The tag becomes the
    /// display name of an anonymous layer. Untagged anonymous layers have an
    /// empty display name.
    static SdfLayerRefPtr CreateAnonymous(
        const std::string& tag = std::string());

    /// Returns true if this layer is an anonymous layer.
    bool IsAnonymous() const;

    /// Returns true if the \p identifier is an anonymous layer unique
    /// identifier.
    static bool IsAnonymousLayerIdentifier(const std::string& identifier);

    /// Returns the display name for the given \p identifier, using the same 
    /// rules as GetDisplayName.
    static std::string GetDisplayNameFromIdentifier(
        const std::string& identifier);

    /// @}
    /// \name File I/O
    /// @{

    /// Converts \e layerPath to a file system path.
    static std::string ComputeRealPath(const std::string &layerPath);

    /// Returns \c true if successful, \c false if an error occurred.
    /// Returns \c false if the layer has no remembered file name or the 
    /// layer type cannot be saved.
    bool Save() const;

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
    bool Export(const std::string& filename, 
                const std::string& comment = std::string(),
                const FileFormatArguments& args = FileFormatArguments()) const;

    /// Writes this layer to the given string.
    ///
    /// Returns \c true if successful and sets \p result, otherwise
    /// returns \c false.
    bool ExportToString(std::string* result) const;

    /// Reads this layer from the given string.
    ///
    /// Returns \c true if successful, otherwise returns \c false.
    bool ImportFromString(const std::string &string);

    /// Clears the layer of all content.
    ///
    /// This restores the layer to a state as if it had just been created
    /// with CreateNew().  This operation is Undo-able.
    ///
    /// The fileName and whether journaling is enabled are not affected
    /// by this method.
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
    /// mechanism is not used, and the layer is reloaded from disk.
    ///
    /// Passing true to the \p force parameter overrides this behavior,
    /// forcing the layer to be reloaded from disk regardless of whether
    /// it has changed.
    bool Reload(bool force = false);

    /// Reloads the specified layers.
    ///
    /// Returns \c false if one or more layers failed to reload.
    ///
    /// See \c Reload() for a description of the \p force flag.
    ///
    static bool ReloadLayers(const std::set<SdfLayerHandle>& layers,
                             bool force = false);

    /// Imports the content of the given layer path, replacing the content
    /// of the current layer.
    /// Note: If the layer path is the same as the current layer's real path,
    /// no action is taken (and a warning occurs). For this case use
    /// Reload().
    bool Import(const std::string &layerPath);

    /// @}
    /// \name External references
    /// @{

    /// Return paths of all external references of layer.
    std::set<std::string> GetExternalReferences();

    /// Updates the external references of the layer.
    ///
    /// If only the old asset path is given, this update works as delete, 
    /// removing any sublayers or prims referencing the pathtype using the
    /// old asset path as reference.
    /// 
    /// If new asset path is supplied, the update works as "rename", updating
    /// any occurence of the old reference to the new reference.
    bool UpdateExternalReference(
        const std::string &oldAssetPath,
        const std::string &newAssetPath=std::string());

    /// @}
    /// \name Identification
    ///
    /// A layer's identifier is a string that uniquely identifies a layer.
    /// At minimum, it is the string by which the layer was created, either
    /// via FindOrOpen or CreateNew. If additional arguments were passed
    /// to those functions, those arguments will be encoded in the identifier.
    /// 
    /// For example: 
    ///     FindOrOpen('foo.sdf', args={'a':'b', 'c':'d'}).identifier
    ///         => "foo.sdf?a=b&c=d"
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
    static bool SplitIdentifier(
        const std::string& identifier,
        std::string* layerPath,
        FileFormatArguments* arguments);

    /// Joins the given layer path and arguments into an identifier.
    static std::string CreateIdentifier(
        const std::string& layerPath,
        const FileFormatArguments& arguments);

    /// Returns the layer identifier.
    const std::string& GetIdentifier() const;

    /// Sets the layer identifier. 
    /// Note that the new identifier must have the same arguments (if any)
    /// as the old identifier.
    void SetIdentifier(const std::string& identifier);

    /// Update layer asset information. Calling this method re-resolves the
    /// layer identifier, which updates asset information such as the layer
    /// file revision, real path, and repository path. If \p fileVersion is
    /// supplied, it is used as the layer version if the identifier does not
    /// have a version or label specifier. This is typically used to tell Sd
    /// what the version of a layer is after submitting a new revision to the
    /// asset system.
    void UpdateAssetInfo(const std::string& fileVersion = std::string());

    /// Returns the layer's display name.
    ///
    /// The display name is the base filename of the identifier.
    std::string GetDisplayName() const;

    /// Returns the file system path where this layer exists or may exist
    /// after a call to Save.
    const std::string& GetRealPath() const;

    /// Returns the file extension to use for this layer.
    /// If this layer was loaded from disk, it should match the extension
    /// of the file format it was loaded as; if this is an anonymous
    /// in-memory layer it will be the default extension.
    std::string GetFileExtension() const;

    /// Returns the asset system version of this layer. If a layer is loaded
    /// from a location that is not version managed, or a configured asset
    /// system is not present when the layer is loaded or created, the version
    /// is empty. By default, asset version tracking is disabled; this method
    /// returns empty unless asset version tracking is enabled.
    const std::string& GetVersion() const;

    /// Returns the layer identifier in asset path form. In the presence of a
    /// properly configured path resolver, the asset path is a double-slash
    /// prefixed depot path. If the path resolver is not configured, the asset
    /// path of a layer is empty.
    const std::string& GetRepositoryPath() const;

    /// Returns the asset name associated with this layer.
    const std::string& GetAssetName() const;

    /// Returns resolve information from the last time the layer identifier
    /// was resolved.
    const VtValue& GetAssetInfo() const;

    /// Make the given \p relativePath absolute using the identifier of this
    /// layer.  If this layer does not have an identifier, or if the layer
    /// identifier is itself relative, \p relativePath is returned unmodified.
    std::string ComputeAbsolutePath(const std::string &relativePath);

    /// @}

    /// \name Fields
    ///
    /// All scene description for a given object is stored as a set of
    /// key/value pairs called fields. These methods provide direct access to
    /// those fields, though most clients should use the Spec API to ensure
    /// data consistency.
    ///
    /// These methods all take SdfAbstractDataSpecId to identify the spec
    /// being queried. Note that these are implicitly convertible from
    /// SdPath.
    ///
    /// @{

    /// Return the specifiers for \a id. This returns default constructed
    /// specifiers if no spec exists at \a id.
    SdfSpecType GetSpecType(const SdfAbstractDataSpecId& id) const;

    /// Return whether a spec exists at \a id.
    bool HasSpec(const SdfAbstractDataSpecId& id) const;

    /// Return the names of all the fields that are set at \p id.
    std::vector<TfToken> ListFields(const SdfAbstractDataSpecId& id) const;

    /// Return whether a value exists for the given \a id and \a fieldName.
    /// Optionally returns the value if it exists.
    bool HasField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
        VtValue *value=NULL) const;
    bool HasField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
        SdfAbstractDataValue *value) const;

    /// Returns \c true if the object has a non-empty value with name
    /// \p name and type \p T.  If value ptr is provided, returns the
    /// value found.
    template <class T>
    bool HasField(const SdfAbstractDataSpecId& id, const TfToken &name, 
        T* value) const
    {
        if (!value) {
            return HasField(id, name, static_cast<VtValue *>(NULL));
        }

        SdfAbstractDataTypedValue<T> outValue(value);
        const bool hasValue = HasField(
            id, name, static_cast<SdfAbstractDataValue *>(&outValue));

        if (std::is_same<T, SdfValueBlock>::value) {
            return hasValue && outValue.isValueBlock;
        }

        return hasValue && (!outValue.isValueBlock);
    }

    /// Return whether a value exists for the given \a id and \a fieldName and
    /// \a keyPath.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.  Optionally returns the value if it exists.
    bool HasFieldDictKey(const SdfAbstractDataSpecId& id,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         VtValue *value=NULL) const;
    bool HasFieldDictKey(const SdfAbstractDataSpecId& id,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         SdfAbstractDataValue *value) const;

    /// Returns \c true if the object has a non-empty value with name \p name
    /// and \p keyPath and type \p T.  If value ptr is provided, returns the
    /// value found.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.
    template <class T>
    bool HasFieldDictKey(const SdfAbstractDataSpecId& id, const TfToken &name,
                         const TfToken &keyPath, T* value) const
    {
        if (!value) {
            return HasFieldDictKey(id, name, keyPath,
                                   static_cast<VtValue *>(NULL));
        }

        SdfAbstractDataTypedValue<T> outValue(value);
        return HasFieldDictKey(id, name, keyPath,
                               static_cast<SdfAbstractDataValue *>(&outValue));
    }


    /// Return the value for the given \a id and \a fieldName. Returns an
    /// empty value if none is set.
    VtValue GetField(const SdfAbstractDataSpecId& id,
                     const TfToken& fieldName) const;

    /// Return the value for the given \a id and \a fieldName. Returns the
    /// provided \a defaultValue value if none is set.
    template <class T>
    inline T GetFieldAs(const SdfAbstractDataSpecId& id, 
        const TfToken& fieldName, const T& defaultValue = T()) const
    {
        return _data->GetAs<T>(id, fieldName, defaultValue);
    }

    /// Return the value for the given \a id and \a fieldName at \p
    /// keyPath. Returns an empty value if none is set.  The \p keyPath is a
    /// ':'-separated path addressing an element in sub-dictionaries.
    VtValue GetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken& fieldName,
                                   const TfToken& keyPath) const;

    /// Set the value of the given \a id and \a fieldName.
    void SetField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
        const VtValue& value);
    void SetField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
        const SdfAbstractDataConstValue& value);

    /// Set the value of the given \a id and \a fieldName.
    template <class T>
    void SetField(const SdfAbstractDataSpecId& id, const TfToken& fieldName, 
        const T& val) 
    {
        // Ideally, this would make use of the SdfAbstractDataConstValue
        // API to avoid unnecessarily copying the value into a VtValue.
        // However, Sdf needs to create a VtValue for change processing.
        // If the underlying SdAbstractData implementation also needs a 
        // VtValue, using the SdfAbstractDataConstValue API would cause
        // another copy to be made. So, it's more efficient to just create 
        // the VtValue once here and push that along.
        SetField(id, fieldName, VtValue(val));
    }

    /// Set the value of the given \a id and \a fieldName.  The \p keyPath is a
    /// ':'-separated path addressing an element in sub-dictionaries.
    void SetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                const VtValue& value);
    void SetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                const SdfAbstractDataConstValue& value);

    /// Set the value of the given \a id and \a fieldName.  The \p keyPath is a
    /// ':'-separated path addressing an element in sub-dictionaries.
    template <class T>
    void SetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
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
        SetFieldDictValueByKey(id, fieldName, keyPath, VtValue(val));
    }

    /// Remove the field at \p id and \p fieldName, if one exists.
    void EraseField(const SdfAbstractDataSpecId& id, const TfToken& fieldName);

    /// Remove the field at \p id and \p fieldName and \p keyPath, if one
    /// exists.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.
    void EraseFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                  const TfToken& fieldName,
                                  const TfToken& keyPath);

    /// Convenience API that takes an SdfPath instead of an
    /// SdfAbstractDataSpecId. See documentation above for details.
    SdfSpecType GetSpecType(const SdfPath& path) const;
    bool HasSpec(const SdfPath& path) const;

    std::vector<TfToken> ListFields(const SdfPath& path) const;

    /// Return a list of the keys of the (sub) dictionary identified by
    /// \p keyPath.  The \p keyPath is a ':'-separated path addressing an
    /// element in sub-dictionaries.
    std::vector<TfToken> ListFieldDictKeys(const SdfPath& path,
                                           const TfToken &keyPath) const;

    template <class T>
    bool HasField(const SdfPath& path,
                  const TfToken& fieldName, T* value) const;
    bool HasField(const SdfPath& path, const TfToken& fieldName) const;
    template <class T>
    bool HasFieldDictKey(const SdfPath& path, const TfToken& fieldName,
                         const TfToken &keyPath, T* value) const;
    bool HasFieldDictKey(const SdfPath& path, const TfToken& fieldName,
                         const TfToken &keyPath) const;

    VtValue GetField(const SdfPath& path, const TfToken& fieldName) const;
    VtValue GetFieldDictValueByKey(
        const SdfPath& path, const TfToken& fieldName,
        const TfToken &keyPath) const;

    template <class T>
    T GetFieldAs(const SdfPath& path, const TfToken& fieldName, 
                 const T& defaultValue = T()) const;

    template <class T>
    void SetField(const SdfPath& path, const TfToken& fieldName, const T& val);
    template <class T>
    void SetFieldDictValueByKey(const SdfPath& path, const TfToken& fieldName,
                                const TfToken &keyPath, const T& val);

    void EraseField(const SdfPath& path, const TfToken& fieldName);
    void EraseFieldDictValueByKey(const SdfPath& path, const TfToken& fieldName,
                                  const TfToken &keyPath);

    /// \name Traversal
    /// @{

    /// Callback function for Traverse. This callback will be invoked with
    /// the path of each spec that is visited.
    /// \sa Traverse
    typedef boost::function<void(const SdfPath&)> TraversalFunction;

    // Traverse will perform a traversal of the scene description hierarchy
    // rooted at \a path, calling \a func on each spec that it finds.
    void Traverse(const SdfPath& path, const TraversalFunction& func);

    /// @}

    /// \name Metadata
    /// @{

    /// Returns the comment string for this layer.
    ///
    /// The default value for comment is "".
    std::string GetComment() const;

    /// Sets the comment string for this layer.
    void SetComment(const std::string &comment);
    
    /// Return the defaultPrim metadata for this layer.  This field
    /// indicates the name of which root prim should be targeted by a reference
    /// or payload to this layer that doesn't specify a prim path.
    ///
    /// The default value is the empty token.
    TfToken GetDefaultPrim() const;

    /// Set the default prim metadata for this layer.  The root prim with this
    /// name will be targeted by a reference or a payload to this layer that
    /// doesn't specify a prim path.  Note that this must be a root prim
    /// <b>name</b> not a path.  E.g. "rootPrim" rather than "/rootPrim".  See
    /// GetDefaultPrim().
    void SetDefaultPrim(const TfToken &name);

    /// Clear the default prim metadata for this layer.  See GetDefaultPrim()
    /// and SetDefaultPrim().
    void ClearDefaultPrim();

    /// Return true if the default prim metadata is set in this layer.  See
    /// GetDefaultPrim() and SetDefaultPrim().
    bool HasDefaultPrim();

    /// Returns the documentation string for this layer.
    ///
    /// The default value for documentation is "".
    std::string GetDocumentation() const;

    /// Sets the documentation string for this layer.
    void SetDocumentation(const std::string &documentation);

    /// Returns the layer's start timeCode.
    ///
    /// The start and end timeCodes of a layer represent the suggested playback 
    /// range.  However, time-varying content is not limited to the timeCode range 
    /// of the layer.
    ///
    /// The default value for startTimeCode is 0.
    double GetStartTimeCode() const;

    /// Sets the layer's start timeCode.
    void SetStartTimeCode(double startTimecode);

    /// Returns true if the layer has a startTimeCode opinion.
    bool HasStartTimeCode() const;

    /// Clear the startTimeCode opinion.
    void ClearStartTimeCode();
    
    /// Returns the layer's end timeCode.
    /// The start and end timeCode of a layer represent a suggested playback range.  
    /// However, time-varying content is not limited to the timeCode range of the 
    /// layer.
    ///
    /// The default value for endTimeCode is 0.
    double GetEndTimeCode() const;

    /// Sets the layer's end timeCode.
    void SetEndTimeCode(double endTimeCode);

    /// Returns true if the layer has an endTimeCode opinion.
    bool HasEndTimeCode() const;

    /// Clear the endTimeCode opinion.
    void ClearEndTimeCode();
    
    /// Returns the layer's timeCodes per second.
    /// 
    /// Scales the time ordinate for samples contained in the file to seconds.  
    /// If timeCodesPerSecond is 24, then a sample at time ordinate 24 should 
    /// be viewed exactly one second after the sample at time ordinate 0. 
    /// 
    /// The default value of timeCodesPerSecond is 24.
    double GetTimeCodesPerSecond() const;

    /// Sets the layer's timeCodes per second
    void SetTimeCodesPerSecond(double timeCodesPerSecond);

    /// Returns true if the layer has a timeCodesPerSecond opinion.
    bool HasTimeCodesPerSecond() const;

    /// Clear the timeCodesPerSecond opinion.
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
    double GetFramesPerSecond() const;

    /// Sets the layer's frames per second
    void SetFramesPerSecond(double framesPerSecond);

    /// Returns true if the layer has a frames per second opinion.
    bool HasFramesPerSecond() const;

    /// Clear the framesPerSecond opinion.
    void ClearFramesPerSecond();
    
    /// Returns the layer's frame precision.
    int GetFramePrecision() const;

    /// Sets the layer's frame precision.
    void SetFramePrecision(int framePrecision);

    /// Returns true if the layer has a frames precision opinion.
    bool HasFramePrecision() const;

    /// Clear the framePrecision opinion.
    void ClearFramePrecision();

    /// Returns the layer's owner.
    std::string GetOwner() const;

    /// Sets the layer's owner.
    void SetOwner(const std::string& owner);

    /// Returns true if the layer has an owner opinion.
    bool HasOwner() const;

    /// Clear the owner opinion.
    void ClearOwner();

    /// Returns the layer's session owner.
    /// Note: This should only be used by session layers.
    std::string GetSessionOwner() const;

    /// Sets the layer's session owner.
    /// Note: This should only be used by session layers.
    void SetSessionOwner(const std::string& owner);

    /// Returns true if the layer has a session owner opinion.
    bool HasSessionOwner() const;

    // Clear the session owner opinion.
    void ClearSessionOwner();

    /// Returns true if the layer's sublayers are expected to have owners.
    bool GetHasOwnedSubLayers() const;

    /// Sets whether the layer's sublayers are expected to have owners.
    void SetHasOwnedSubLayers(bool);

    /// Returns the CustomLayerData dictionary associated with this layer.
    /// 
    /// This is a dictionary is custom metadata that is associated with
    /// this layer. It allows users to encode any set of information for
    /// human or program consumption.
    VtDictionary GetCustomLayerData() const;

    /// Sets the CustomLayerData dictionary associated with this layer.
    void SetCustomLayerData(const VtDictionary& value);

    /// Returns true if CustomLayerData is authored on the layer.
    bool HasCustomLayerData() const;

    /// Clears out the CustomLayerData dictionary associated with this layer.
    void ClearCustomLayerData();

    /// @}
    /// \name Prims
    /// @{

    // Type for root prims view.
    typedef SdfPrimSpecView RootPrimsView;

    /// Returns a vector of the layer's root prims
    RootPrimsView GetRootPrims() const;

    /// Sets a new vector of root prims.
    /// You can re-order, insert and remove prims but cannot 
    /// rename them this way.  If any of the listed prims have 
    /// an existing owner, they will be reparented.
    void SetRootPrims(const SdfPrimSpecHandleVector &rootPrims);

    /// Adds a new root prim at the given index.
    /// If the index is -1, the prim is inserted at the end.
    /// The layer will take ownership of the prim, via a TfRefPtr.
    /// Returns true if succesful, false if failed (for example,
    /// due to a duplicate name).
    bool InsertRootPrim(const SdfPrimSpecHandle &prim, int index = -1);

    /// Remove a root prim.
    void RemoveRootPrim(const SdfPrimSpecHandle &prim);

    /// Cause \p spec to be removed if it no longer affects the scene when the 
    /// last change block is closed, or now if there are no change blocks.
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
    void RemovePrimIfInert(SdfPrimSpecHandle prim);

    /// Removes prop if it has only required fields (i.e. is not 
    /// contributing any opinions to the scene other than property 
    /// instantiation).
    /// 
    /// The hierarchy \p prop is defined in will then be pruned up to the 
    /// layer root for each successive inert parent.
    void RemovePropertyIfHasOnlyRequiredFields(SdfPropertySpecHandle prop);

    /// Removes all scene description in this layer that does not affect the
    /// scene.
    ///
    /// This method walks the layer namespace hierarchy and removes any prims
    /// and that are not contributing any opinions.
    void RemoveInertSceneDescription();

    /// Returns the list of prim names for this layer's reorder rootPrims
    /// statement.
    ///
    /// See SetRootPrimOrder() for more info.
    SdfNameOrderProxy GetRootPrimOrder() const;
   
    /// Given a list of (possible sparse) prim names, authors a reorder
    /// rootPrims statement for this prim. 
    ///
    /// This reorder statement can modify the order of root prims that have 
    /// already been explicitly ordered with InsertRootPrim() or SetRootPrims();
    /// but only during composition.  Therefore, GetRootPrims(), 
    /// InsertRootPrim(), SetRootPrims(), etc. do not read, author, or pay any 
    /// attention to this statement.
    void SetRootPrimOrder(const std::vector<TfToken>& names);

    /// Adds a new root prim name in the root prim order.
    /// If the index is -1, the name is inserted at the end.
    void InsertInRootPrimOrder(const TfToken &name, int index = -1);

    /// Removes a root prim name from the root prim order.
    void RemoveFromRootPrimOrder(const TfToken & name);

    /// Removes a root prim name from the root prim order by index.
    void RemoveFromRootPrimOrderByIndex(int index);

    /// Reorders the given list of prim names according to the reorder rootPrims
    /// statement for this layer.
    ///
    /// This routine employs the standard list editing operations for ordered
    /// items in a ListEditor.
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
    SdfSubLayerProxy GetSubLayerPaths() const;

    /// Sets the paths of the layer's sublayers.
    void SetSubLayerPaths(const std::vector<std::string>& newPaths);

    /// Returns the number of sublayer paths (and offsets).
    size_t GetNumSubLayerPaths() const;

    /// Inserts new sublayer path at the given index.
    ///
    /// The default index of -1 means to insert at the end.
    void InsertSubLayerPath(const std::string& path, int index = -1);

    /// Removes sublayer path at the given index.
    void RemoveSubLayerPath(int index);

    /// Returns the layer offsets for all the subLayer paths.
    SdfLayerOffsetVector GetSubLayerOffsets() const;

    /// Returns the layer offset for the subLayer path at the given index.
    SdfLayerOffset GetSubLayerOffset(int index) const;

    /// Sets the layer offset for the subLayer path at the given index.
    void SetSubLayerOffset(const SdfLayerOffset& offset, int index);

    /// @}
    /// \name Muting
    /// @{

    /// Returns the set of muted layer paths.
    static std::set<std::string> GetMutedLayers();

    /// Returns \c true if the current layer is muted.
    bool IsMuted() const;

    /// Returns \c true if the specified layer path is muted.
    static bool IsMuted(const std::string &path);

    /// Mutes the current layer if \p muted is \c true, and unmutes it
    /// otherwise.
    void SetMuted(bool muted);

    /// Add the specified path to the muted layers set.
    static void AddToMutedLayers(const std::string &mutedPath);

    /// Remove the specified path from the muted layers set.
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
    SdfPrimSpecHandle GetPseudoRoot() const;

    /// Returns the object at the given \p path.
    ///
    /// There is no distinction between an absolute and relative path
    /// at the SdLayer level.
    ///
    /// Returns \c NULL if there is no object at \p path.
    SdfSpecHandle GetObjectAtPath(const SdfPath &path);

    /// Returns the prim at the given \p path.
    ///
    /// Returns \c NULL if there is no prim at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SdfPrimSpecHandle GetPrimAtPath(const SdfPath &path);

    /// Returns a property at the given \p path.
    ///
    /// Returns \c NULL if there is no property at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SdfPropertySpecHandle GetPropertyAtPath(const SdfPath &path);

    /// Returns an attribute at the given \p path.
    ///
    /// Returns \c NULL if there is no attribute at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SdfAttributeSpecHandle GetAttributeAtPath(const SdfPath &path);

    /// Returns a relationship at the given \p path.
    ///
    /// Returns \c NULL if there is no relationship at \p path.
    /// This is simply a more specifically typed version of
    /// \c GetObjectAtPath().
    SdfRelationshipSpecHandle GetRelationshipAtPath(const SdfPath &path);

    /// @}
    /// \name Permissions
    /// @{

    /// Returns true if the caller is allowed to modify the layer and 
    /// false otherwise.  A layer may have to perform some action to acquire 
    /// permission to be editted.
    bool PermissionToEdit() const;

    /// Returns true if the caller is allowed to save the layer to its 
    /// existing fileName and false otherwise.
    bool PermissionToSave() const;

    /// Sets permission to edit.
    void SetPermissionToEdit(bool allow);

    /// Sets permission to save.
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
    SdfNamespaceEditDetail::Result
    CanApply(const SdfBatchNamespaceEdit&,
             SdfNamespaceEditDetailVector* details = NULL) const;

    /// Performs a batch of namespace edits.  Returns \c true on success
    /// and \c false on failure.  On failure, no namespace edits will have
    /// occurred.
    bool Apply(const SdfBatchNamespaceEdit&);

    /// @}
    /// \name Layer state
    /// @{

    /// Returns the state delegate used to manage this layer's authoring
    /// state.
    SdfLayerStateDelegateBasePtr GetStateDelegate() const;

    /// Sets the state delegate used to manage this layer's authoring
    /// state. The 'dirty' state of this layer will be transferred to
    /// the new delegate.
    void SetStateDelegate(const SdfLayerStateDelegateBaseRefPtr& delegate);

    /// Returns \c true if the layer is dirty, i.e. has changed from
    /// its persistent representation.
    bool IsDirty() const;

    /// @}

    /// \name Time-sample API
    /// @{

    std::set<double> ListAllTimeSamples() const;
    
    std::set<double> 
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    bool GetBracketingTimeSamples(double time, double* tLower, double* tUpper);

    size_t GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    bool GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId& id, 
                                         double time,
                                         double* tLower, double* tUpper);

    bool QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                         VtValue *value=NULL) const;
    bool QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                         SdfAbstractDataValue *value) const;

    template <class T>
    bool QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                         T* data) const
    {
        if (!data) {
            return QueryTimeSample(id, time);
        }

        SdfAbstractDataTypedValue<T> outValue(data);
        const bool hasValue = QueryTimeSample(
            id, time, static_cast<SdfAbstractDataValue *>(&outValue));

        if (std::is_same<T, SdfValueBlock>::value) {
            return hasValue && outValue.isValueBlock;
        }

        return hasValue && (!outValue.isValueBlock);
    }

    void SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                       const VtValue & value);
    void SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                       const SdfAbstractDataConstValue& value);

    template <class T>
    void SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                       const T& value)
    {
        const SdfAbstractDataConstTypedValue<T> inValue(&value);
        const SdfAbstractDataConstValue& untypedInValue = inValue;
        return SetTimeSample(id, time, untypedInValue);
    }

    void EraseTimeSample(const SdfAbstractDataSpecId& id, double time);

    /// Convenience API that takes an SdfPath instead of an 
    /// SdfAbstractDataSpecId. See documentation above for details.
    size_t GetNumTimeSamplesForPath(const SdfPath& path) const;
    std::set<double> ListTimeSamplesForPath(const SdfPath& path) const;
    bool GetBracketingTimeSamplesForPath(const SdfPath& path, double time,
                                         double* tLower, double* tUpper);

    template <class T>
    bool QueryTimeSample(const SdfPath& path, double time, T* data) const;
    bool QueryTimeSample(const SdfPath& path, double time) const;

    template <class T>
    void SetTimeSample(const SdfPath& path, double time, const T& value);

    void EraseTimeSample(const SdfPath& path, double time);

    /// @}

    // Debugging
    // @{

    static void DumpLayerInfo();

    // Write this layer's SdfData to a file in a simple generic format.
    bool WriteDataFile(const std::string &filename);

    // @}

protected:
    static SdfLayerRefPtr _CreateAnonymousWithFormat(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string& tag = std::string());

    // Private constructor -- use New(), FindOrCreate(), etc.
    // Precondition: _layerRegistryMutex must be locked.
    SdfLayer(const SdfFileFormatConstPtr& fileFormat,
             const std::string &identifier,
             const std::string &realPath = std::string(),
             const ArAssetInfo& assetInfo = ArAssetInfo(),
             const FileFormatArguments &args = FileFormatArguments());

private:
    // Create a new layer.
    // Precondition: _layerRegistryMutex must be locked.
    static SdfLayerRefPtr _CreateNew(
        SdfFileFormatConstPtr fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo = ArAssetInfo(),
        const FileFormatArguments& args = FileFormatArguments());

    static SdfLayerRefPtr _CreateNewWithFormat(
        const SdfFileFormatConstPtr &fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo = ArAssetInfo(),
        const FileFormatArguments& args = FileFormatArguments());

    // Finish initializing this layer (which may have succeeded or not)
    // and publish the results to other threads by unlocking the mutex.
    // Sets _initializationWasSuccessful and unlocks _initializationMutex.
    void _FinishInitialization(bool success);

    // Layers retrieved from the layer registry may still be in the
    // process of having their contents initialized.  Other threads
    // retrieving layers from the registry must wait until initialization
    // is complete, using this method.  See _initializationMutex.
    // Returns _initializationWasSuccessful.
    //
    // Callers *must* be holding an SdfLayerRefPtr to this layer to
    // ensure that it is not deleted out from under them, in
    // case initialiation fails.  (This method cannot acquire the
    // reference itself internally without being susceptible to a race.)
    bool _WaitForInitializationAndCheckIfSuccessful();

    // Returns whether or not this menv layer should post change 
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

    /// Initialize layer internals that are based on it's id.
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
        _FindOrOpenLayerInfo* info);

    // Open a layer, adding an entry to the registry and releasing
    // the registry lock.
    // Precondition: _layerRegistryMutex must be locked.
    static SdfLayerRefPtr _OpenLayerAndUnlockRegistry(
        const _FindOrOpenLayerInfo& info,
        bool metadataOnly,
        std::string const &resolvedPath,
        const ArAssetInfo &assetInfo,
        bool isAnonymous);

    // Helper function to try to find the layer with \p identifier and
    // pre-resolved path \p resolvedPath in the registry.  Caller must hold
    // registry lock.  If layer found succesfully and returned, this function
    // unlocks the registry, otherwise the lock remains held.
    static SdfLayerRefPtr _TryToFindLayer(const std::string &identifier,
                                          const std::string &resolvedPath);

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
    bool _IsInertSubtree(const SdfPath &path);

    /// Cause \p spec to be removed if it does not affect the scene. This 
    /// removes any empty descendants before checking if \p spec itself is 
    /// inert. Property specs are always considered non-inert, so this will 
    /// remove them if they have only required fields (see 
    /// PropertySpec::HasOnlyRequiredFields). This also removes inert ancestors.
    void _RemoveIfInert(const SdfSpec& spec);

    /// Peforms a depth first search of the namespace hierarchy, beginning at
    /// \p prim, removing prims that do not affect the scene. The return value 
    /// indicates whether the prim passed in is now inert as a result of this 
    /// call, and can itself be removed.
    bool _RemoveInertDFS(SdfPrimSpecHandle prim);

    /// If \p prim is inert (has no affect on the scene), removes prim, then 
    /// prunes inert parent prims back to the root.
    void _RemoveInertToRootmost(SdfPrimSpecHandle prim);

    /// Returns the path used in the muted layers set.
    std::string _GetMutedPath() const;

    // If old and new asset path is given, rename all external prim
    // references referring to the old path.
    void _UpdateReferencePaths(const SdfPrimSpecHandle &parent,
                               const std::string &oldLayerPath,
                               const std::string &newLayerPath);

    // Set the clean state to the current state.
    void _MarkCurrentStateAsClean() const;

    // Return the field definition for \p fieldName if \p fieldName is a
    // required field for the spec type identified by \p id.
    inline SdfSchema::FieldDefinition const *
    _GetRequiredFieldDef(const SdfAbstractDataSpecId &id,
                         const TfToken &fieldName) const {
        SdfSchemaBase const &schema = GetSchema();
        if (ARCH_UNLIKELY(schema.IsRequiredFieldName(fieldName))) {
            // Get the spec definition.
            if (SdfSchema::SpecDefinition const *specDef =
                schema.GetSpecDefinition(GetSpecType(id))) {
                // If this field is required for this spec type, look up the
                // field definition.
                if (specDef->IsRequiredField(fieldName)) {
                    return schema.GetFieldDefinition(fieldName);
                }
            }
        }
        return NULL;
    }

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
               const std::string& resolvedPath, 
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
                      SdfFileFormatConstPtr fileFormat = TfNullPtr,
                      const FileFormatArguments& args = FileFormatArguments())
                      const;

    // Swap contents of _data and data. This operation does not register
    // inverses or emit change notification.
    void _SwapData(SdfAbstractDataRefPtr &data);

    // Set _data to match data, calling other primitive setter methods
    // to provide fine-grained inverses and notification.
    void _SetData(const SdfAbstractDataPtr &data);

    // Returns const handle to _data.
    SdfAbstractDataConstPtr _GetData() const;

    // Inverse primitive for setting a single field.
    template <class T>
    void _PrimSetField(const SdfAbstractDataSpecId& id, 
                       const TfToken& fieldName,
                       const T& value,
                       const VtValue *oldValue = NULL,
                       bool useDelegate = true);

    // Inverse primitive for setting a single key in a dict-valued field.
    template <class T>
    void _PrimSetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                     const TfToken& fieldName,
                                     const TfToken& keyPath,
                                     const T& value,
                                     const VtValue *oldValue = NULL,
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
    void _PrimSetTimeSample(const SdfAbstractDataSpecId& id, double time,
                            const T& value,
                            bool useDelegate = true);

    // Helper method for Traverse. Visits the children of \a path using the
    // specified \a ChildPolicy.
    template <typename ChildPolicy>
    void _TraverseChildren(const SdfPath &path, const TraversalFunction &func);

private:
    // Registry of Sdf Identities
    mutable Sdf_IdentityRegistry _idRegistry;

    // The underlying SdfData which stores all the data in the layer.
    SdfAbstractDataRefPtr _data;

    // The state delegate for this layer.
    SdfLayerStateDelegateBaseRefPtr _stateDelegate;

    // Mutex protecting layer initialization -- the interval between
    // adding a layer to the layer registry, and finishing the process
    // of initialization its contents, at which point we can truly publish
    // the layer. We add the layer to the registry before initialization
    // completes so that other threads can discover and block on the
    // same layer while it is being initalized.
    std::mutex _initializationMutex;

    // This is an optional<bool> that is only set once initialization
    // is complete, while _initializationMutex is locked.  If the
    // optional<bool> is unset, initialization is still underway.
    boost::optional<bool> _initializationWasSuccessful;

    // remembers the last 'IsDirty' state.
    mutable bool _lastDirtyState;

    // Asset information for this layer.
    boost::scoped_ptr<Sdf_AssetInfo> _assetInfo;

    // Modification timestamp of the backing file asset when last read.
    mutable VtValue _assetModificationTime;

    // Mutable revision number for cache invalidation.
    mutable size_t _mutedLayersRevisionCache;

    // Cache of whether or not this layer is muted.  Only valid if
    // _mutedLayersRevisionCache is up-to-date with the global revision number.
    mutable bool _isMutedCache;

    // Layer permission bits.
    bool _permissionToEdit;
    bool _permissionToSave;

    // Allow access to _FindLayerForData() and _IsInert().
    friend class SdfSpec;
    friend class SdfPropertySpec;

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

    // Give SdSpec access to _MoveSpec
    friend class SdSpec;
};

// Inlined implementations for convenience field and time sample API.
// These all simply wrap the given SdfPath in an SdfAbstractDataSpecId
// and forward to the corresponding function.

inline SdfSpecType
SdfLayer::GetSpecType(const SdfPath& path) const
{ 
    return GetSpecType(SdfAbstractDataSpecId(&path)); 
}

inline bool
SdfLayer::HasSpec(const SdfPath& path) const
{ 
    return HasSpec(SdfAbstractDataSpecId(&path)); 
}

inline std::vector<TfToken>
SdfLayer::ListFields(const SdfPath& path) const
{ 
    return ListFields(SdfAbstractDataSpecId(&path)); 
}

template <class T>
inline bool 
SdfLayer::HasField(const SdfPath& path, const TfToken &name, T* value) const
{ 
    return HasField(SdfAbstractDataSpecId(&path), name, value); 
}

inline bool 
SdfLayer::HasField(const SdfPath& path, const TfToken &name) const
{ 
    return HasField(SdfAbstractDataSpecId(&path), name); 
}

template <class T>
inline bool 
SdfLayer::HasFieldDictKey(const SdfPath& path, const TfToken &name,
                          const TfToken &keyPath, T* value) const
{ 
    return HasFieldDictKey(SdfAbstractDataSpecId(&path), name, keyPath, value);
}

inline bool 
SdfLayer::HasFieldDictKey(const SdfPath& path, const TfToken &name,
                          const TfToken &keyPath) const
{ 
    return HasFieldDictKey(SdfAbstractDataSpecId(&path), name, keyPath);
}

inline VtValue 
SdfLayer::GetField(const SdfPath& path, const TfToken& fieldName) const
{ 
    return GetField(SdfAbstractDataSpecId(&path), fieldName); 
}

inline VtValue
SdfLayer::GetFieldDictValueByKey(const SdfPath& path, const TfToken& fieldName,
                                 const TfToken &keyPath) const
{
    return GetFieldDictValueByKey(
        SdfAbstractDataSpecId(&path), fieldName, keyPath);
}

template <class T>
inline T 
SdfLayer::GetFieldAs(
    const SdfPath& path, const TfToken& fieldName, const T& defaultValue) const
{ 
    return GetFieldAs(SdfAbstractDataSpecId(&path), fieldName, defaultValue); 
}

template <class T>
inline void
SdfLayer::SetField(const SdfPath& path, const TfToken& fieldName, const T& val) 
{ 
    SetField(SdfAbstractDataSpecId(&path), fieldName, val); 
}

template <class T>
inline void
SdfLayer::SetFieldDictValueByKey(const SdfPath& path, const TfToken& fieldName,
                                 const TfToken &keyPath, const T& val)
{ 
    SetFieldDictValueByKey(
        SdfAbstractDataSpecId(&path), fieldName, keyPath, val);
}

inline void 
SdfLayer::EraseField(const SdfPath& path, const TfToken& fieldName)
{ 
    EraseField(SdfAbstractDataSpecId(&path), fieldName); 
}

inline void 
SdfLayer::EraseFieldDictValueByKey(const SdfPath& path,
                                   const TfToken& fieldName,
                                   const TfToken &keyPath)
{ 
    EraseFieldDictValueByKey(SdfAbstractDataSpecId(&path), fieldName, keyPath);
}

inline size_t
SdfLayer::GetNumTimeSamplesForPath(const SdfPath& path) const
{
    return GetNumTimeSamplesForPath(SdfAbstractDataSpecId(&path));
}

inline std::set<double> 
SdfLayer::ListTimeSamplesForPath(const SdfPath& path) const
{
    return ListTimeSamplesForPath(SdfAbstractDataSpecId(&path));
}

inline bool 
SdfLayer::GetBracketingTimeSamplesForPath(
    const SdfPath& path, double time, double* tLower, double* tUpper)
{
    return GetBracketingTimeSamplesForPath(
        SdfAbstractDataSpecId(&path), time, tLower, tUpper);
}

template <class T>
inline bool 
SdfLayer::QueryTimeSample(const SdfPath& path, double time, T* data) const
{
    return QueryTimeSample(SdfAbstractDataSpecId(&path), time, data);
}

inline bool
SdfLayer::QueryTimeSample(const SdfPath& path, double time) const
{
    return QueryTimeSample(SdfAbstractDataSpecId(&path), time);
}

template <class T>
inline void
SdfLayer::SetTimeSample(const SdfPath& path, double time, const T& value)
{
    SetTimeSample(SdfAbstractDataSpecId(&path), time, value);
}

inline void
SdfLayer::EraseTimeSample(const SdfPath& path, double time)
{
    EraseTimeSample(SdfAbstractDataSpecId(&path), time);
}

#endif // SDF_LAYER_H
