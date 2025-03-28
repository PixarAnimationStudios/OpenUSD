//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_INPUT_H
#define PXR_USD_USD_SHADE_INPUT_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/sdr/declare.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadeConnectableAPI;
struct UsdShadeConnectionSourceInfo;
class UsdShadeOutput;

/// \class UsdShadeInput
/// 
/// This class encapsulates a shader or node-graph input, which is a 
/// connectable attribute representing a typed value.
/// 
class UsdShadeInput
{
public:
    /// Default constructor returns an invalid Input.  Exists for the sake of
    /// container classes
    UsdShadeInput()
    {
        // nothing
    }

    /// Get the name of the attribute associated with the Input. 
    /// 
    TfToken const &GetFullName() const { 
        return _attr.GetName(); 
    }

    /// Returns the name of the input. 
    /// 
    /// We call this the base name since it strips off the "inputs:" namespace 
    /// prefix from the attribute name, and returns it.
    /// 
    USDSHADE_API
    TfToken GetBaseName() const;

    /// Get the "scene description" value type name of the attribute associated 
    /// with the Input.
    /// 
    USDSHADE_API
    SdfValueTypeName GetTypeName() const;
    
    /// Get the prim that the input belongs to.
    UsdPrim GetPrim() const {
        return _attr.GetPrim();
    }

    /// Convenience wrapper for the templated UsdAttribute::Get().
    template <typename T>
    bool Get(T* value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return GetAttr().Get(value, time);
    }

    /// Convenience wrapper for VtValue version of UsdAttribute::Get().
    USDSHADE_API
    bool Get(VtValue* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Set a value for the Input at \p time.
    /// 
    USDSHADE_API
    bool Set(const VtValue& value, 
             UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Set a value of the Input at \p time.
    /// 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    /// Hash functor.
    struct Hash {
        inline size_t operator()(const UsdShadeInput &input) const {
            return hash_value(input._attr);
        }
    };

    /// \name Configuring the Input's Type
    /// @{

    /// Specify an alternative, renderer-specific type to use when
    /// emitting/translating this Input, rather than translating based
    /// on its GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for Inputs that
    /// are of renderman custom struct types.
    ///
    /// \return true on success.
    ///
    USDSHADE_API
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this Input's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this Input.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    bool HasRenderType() const;

    /// @}

    /// \name API to author and query an Input's sdrMetadata
    /// 
    /// This section provides API for authoring and querying shader registry
    /// metadata on an Input. When the owning shader prim is providing a shader 
    /// definition, the authored "sdrMetadata" dictionary value provides 
    /// metadata needed to populate the Input correctly in the shader registry. 
    /// 
    /// We expect the keys in sdrMetadata to correspond to the keys 
    /// in \ref SdrPropertyMetadata. However, this is not strictly enforced by
    /// the API. The only allowed value type in the "sdrMetadata" dictionary is 
    /// a std::string since it needs to be converted into a SdrTokenMap, which 
    /// Sdr will parse using the utilities available in \ref SdrMetadataHelpers.
    /// 
    /// @{

    /// Returns this Input's composed "sdrMetadata" dictionary as a 
    /// SdrTokenMap.
    USDSHADE_API
    SdrTokenMap GetSdrMetadata() const;
    
    /// Returns the value corresponding to \p key in the composed 
    /// <b>sdrMetadata</b> dictionary.
    USDSHADE_API
    std::string GetSdrMetadataByKey(const TfToken &key) const;
        
    /// Authors the given \p sdrMetadata value on this Input at the current 
    /// EditTarget.
    USDSHADE_API
    void SetSdrMetadata(const SdrTokenMap &sdrMetadata) const;

    /// Sets the value corresponding to \p key to the given string \p value, in 
    /// the Input's "sdrMetadata" dictionary at the current EditTarget.
    USDSHADE_API
    void SetSdrMetadataByKey(
        const TfToken &key, 
        const std::string &value) const;

    /// Returns true if the Input has a non-empty composed "sdrMetadata" 
    /// dictionary value.
    USDSHADE_API
    bool HasSdrMetadata() const;

    /// Returns true if there is a value corresponding to the given \p key in 
    /// the composed "sdrMetadata" dictionary.
    USDSHADE_API
    bool HasSdrMetadataByKey(const TfToken &key) const;

    /// Clears any "sdrMetadata" value authored on the Input in the current 
    /// EditTarget.
    USDSHADE_API
    void ClearSdrMetadata() const;

    /// Clears the entry corresponding to the given \p key in the 
    /// "sdrMetadata" dictionary authored in the current EditTarget.
    USDSHADE_API
    void ClearSdrMetadataByKey(const TfToken &key) const;

    /// @}

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    /// Speculative constructor that will produce a valid UsdShadeInput when
    /// \p attr already represents a shade Input, and produces an \em invalid 
    /// UsdShadeInput otherwise (i.e. the explicit bool conversion operator will 
    /// return false).
    USDSHADE_API
    explicit UsdShadeInput(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Input, which
    /// implies that creating a UsdShadeInput from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    USDSHADE_API
    static bool IsInput(const UsdAttribute &attr);

    /// Test if this name has a namespace that indicates it could be an
    /// input.
    USDSHADE_API
    static bool IsInterfaceInputName(const std::string & name);

    /// Explicit UsdAttribute extractor.
    const UsdAttribute &GetAttr() const { return _attr; }

    /// Allow UsdShadeInput to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeInput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator const UsdAttribute & () const { return GetAttr(); }

    /// Return true if the wrapped UsdAttribute is defined, and in addition the 
    /// attribute is identified as an input.
    bool IsDefined() const {
        return _attr && IsInput(_attr);
    }

    /// Set documentation string for this Input.
    /// \sa UsdObject::SetDocumentation()
    USDSHADE_API
    bool SetDocumentation(const std::string& docs) const;

    /// Get documentation string for this Input.
    /// \sa UsdObject::GetDocumentation()
    USDSHADE_API
    std::string GetDocumentation() const;

    /// Set the displayGroup metadata for this Input,  i.e. hinting for the
    /// location and nesting of the attribute.
    ///
    /// Note for an input representing a nested SdrShaderProperty, its expected
    /// to have the scope delimited by a ":".
    /// \sa UsdProperty::SetDisplayGroup(), UsdProperty::SetNestedDisplayGroup()
    /// \sa SdrShaderProperty::GetPage()
    USDSHADE_API
    bool SetDisplayGroup(const std::string& displayGroup) const;

    /// Get the displayGroup metadata for this Input, i.e. hint for the location 
    /// and nesting of the attribute.
    /// \sa UsdProperty::GetDisplayGroup(), UsdProperty::GetNestedDisplayGroup()
    USDSHADE_API
    std::string GetDisplayGroup() const;

    /// @}

    /// Return true if this Input is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
    explicit operator bool() const { 
        return IsDefined(); 
    }

    /// Equality comparison. Returns true if \a lhs and \a rhs represent the 
    /// same UsdShadeInput, false otherwise.
    friend bool operator==(const UsdShadeInput &lhs, const UsdShadeInput &rhs) {
        return lhs.GetAttr() == rhs.GetAttr();
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdShadeInput, true otherwise.
    friend bool operator!=(const UsdShadeInput &lhs, const UsdShadeInput &rhs) {
        return !(lhs == rhs);
    }
    
    // -------------------------------------------------------------------------
    /// \name Connections API
    // -------------------------------------------------------------------------
    /// @{

    /// Determines whether this Input can be connected to the given 
    /// source attribute, which can be an input or an output.
    /// 
    /// \sa UsdShadeConnectableAPI::CanConnect
    USDSHADE_API
    bool CanConnect(const UsdAttribute &source) const;

    /// \overload
    USDSHADE_API
    bool CanConnect(const UsdShadeInput &sourceInput) const;

    /// \overload
    USDSHADE_API
    bool CanConnect(const UsdShadeOutput &sourceOutput) const;

    using ConnectionModification = UsdShadeConnectionModification;

    /// Authors a connection for this Input
    /// 
    /// \p source is a struct that describes the upstream source attribute
    /// with all the information necessary to make a connection. See the
    /// documentation for UsdShadeConnectionSourceInfo.
    /// \p mod describes the operation that should be applied to the list of
    /// connections. By default the new connection will replace any existing
    /// connections, but it can add to the list of connections to represent
    /// multiple input connections.
    /// 
    /// \return
    /// \c true if a connection was created successfully.
    /// \c false if this input or \p source is invalid.
    /// 
    /// \note This method does not verify the connectability of the shading
    /// attribute to the source. Clients must invoke CanConnect() themselves
    /// to ensure compatibility.
    /// \note The source shading attribute is created if it doesn't exist
    /// already.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(
        UsdShadeConnectionSourceInfo const &source,
        ConnectionModification const mod =
            ConnectionModification::Replace) const;

    /// \deprecated
    /// \overload
    USDSHADE_API
    bool ConnectToSource(
        UsdShadeConnectableAPI const &source,
        TfToken const &sourceName,
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName()) const;

    /// Authors a connection for this Input to the source at the given path.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(SdfPath const &sourcePath) const;

    /// Connects this Input to the given input, \p sourceInput.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(UsdShadeInput const &sourceInput) const;

    /// Connects this Input to the given output, \p sourceOutput.
    /// 
    /// \sa UsdShadeConnectableAPI::ConnectToSource
    ///
    USDSHADE_API
    bool ConnectToSource(UsdShadeOutput const &sourceOutput) const;

    /// Connects this Input to the given sources, \p sourceInfos
    /// 
    /// \sa UsdShadeConnectableAPI::SetConnectedSources
    ///
    USDSHADE_API
    bool SetConnectedSources(
        std::vector<UsdShadeConnectionSourceInfo> const &sourceInfos) const;

    using SourceInfoVector = TfSmallVector<UsdShadeConnectionSourceInfo, 1>;

    /// Finds the valid sources of connections for the Input.
    /// 
    /// \p invalidSourcePaths is an optional output parameter to collect the
    /// invalid source paths that have not been reported in the returned vector.
    /// 
    /// Returns a vector of \p UsdShadeConnectionSourceInfo structs with
    /// information about each upsteam attribute. If the vector is empty, there
    /// have been no valid connections.
    /// 
    /// \note A valid connection requires the existence of the source attribute
    /// and also requires that the source prim is UsdShadeConnectableAPI
    /// compatible.
    /// \note The python wrapping returns a tuple with the valid connections
    /// first, followed by the invalid source paths.
    /// 
    /// \sa UsdShadeConnectableAPI::GetConnectedSources
    ///
    USDSHADE_API
    SourceInfoVector GetConnectedSources(
        SdfPathVector *invalidSourcePaths = nullptr) const;

    /// \deprecated
    USDSHADE_API
    bool GetConnectedSource(UsdShadeConnectableAPI *source,
                            TfToken *sourceName,
                            UsdShadeAttributeType *sourceType) const;

    /// \deprecated
    /// Returns the "raw" (authored) connected source paths for this Input.
    /// 
    /// \sa UsdShadeConnectableAPI::GetRawConnectedSourcePaths
    ///
    USDSHADE_API
    bool GetRawConnectedSourcePaths(SdfPathVector *sourcePaths) const;

    /// Returns true if and only if this Input is currently connected to a 
    /// valid (defined) source. 
    ///
    /// \sa UsdShadeConnectableAPI::HasConnectedSource
    /// 
    USDSHADE_API
    bool HasConnectedSource() const;

    /// Returns true if the connection to this Input's source, as returned by 
    /// GetConnectedSource(), is authored across a specializes arc, which is 
    /// used to denote a base material.
    /// 
    /// \sa UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial
    ///
    USDSHADE_API
    bool IsSourceConnectionFromBaseMaterial() const;

    /// Disconnect source for this Input. If \p sourceAttr is valid, only a
    /// connection to the specified attribute is disconnected, otherwise all
    /// connections are removed.
    /// 
    /// \sa UsdShadeConnectableAPI::DisconnectSource
    ///
    USDSHADE_API
    bool DisconnectSource(UsdAttribute const &sourceAttr = UsdAttribute()) const;

    /// Clears sources for this Input in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa UsdShadeConnectableAPI::ClearSources
    ///
    USDSHADE_API
    bool ClearSources() const;

    /// \deprecated
    USDSHADE_API
    bool ClearSource() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Connectability API
    // -------------------------------------------------------------------------
    /// @{
        
    /// \brief Set the connectability of the Input. 
    /// 
    /// In certain shading data models, there is a need to distinguish which 
    /// inputs <b>can</b> vary over a surface from those that must be 
    /// <b>uniform</b>. This is accomplished in UsdShade by limiting the 
    /// connectability of the input. This is done by setting the 
    /// "connectability" metadata on the associated attribute.
    /// 
    /// Connectability of an Input can be set to UsdShadeTokens->full or 
    /// UsdShadeTokens->interfaceOnly. 
    /// 
    /// \li <b>full</b> implies that  the Input can be connected to any other 
    /// Input or Output.  
    /// \li <b>interfaceOnly</b> implies that the Input can only be connected to 
    /// a NodeGraph Input (which represents an interface override, not a 
    /// render-time dataflow connection), or another Input whose connectability 
    /// is also "interfaceOnly".
    /// 
    /// The default connectability of an input is UsdShadeTokens->full.
    /// 
    /// \sa SetConnectability()
    USDSHADE_API
    bool SetConnectability(const TfToken &connectability) const;

    /// \brief Returns the connectability of the Input.
    /// 
    /// \sa SetConnectability()
    USDSHADE_API
    TfToken GetConnectability() const;

    /// \brief Clears any authored connectability on the Input.
    /// 
    USDSHADE_API
    bool ClearConnectability() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Connected Value API
    // -------------------------------------------------------------------------
    /// @{

    /// \brief Find what is connected to this Input recursively
    ///
    /// \sa UsdShadeUtils::GetValueProducingAttributes
    USDSHADE_API
    UsdShadeAttributeVector GetValueProducingAttributes(
        bool shaderOutputsOnly = false) const;

    /// \deprecated in favor of calling GetValueProducingAttributes
    USDSHADE_API
    UsdAttribute GetValueProducingAttribute(
        UsdShadeAttributeType* attrType) const;

    /// @}

private:
    friend class UsdShadeConnectableAPI;

    // Constructor that creates a UsdShadeInput with the given name on the 
    // given prim.
    // \p name here is the unnamespaced name of the input.
    UsdShadeInput(UsdPrim prim,
                  TfToken const &name,
                  SdfValueTypeName const &typeName);
    
    UsdAttribute _attr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_SHADE_INPUT_H
