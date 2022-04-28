//
// Unlicensed 2022 benmalartre
//

#ifndef PXR_USD_USD_EXEC_OUTPUT_H
#define PXR_USD_USD_EXEC_OUTPUT_H

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/usd/usdExec/execTypes.h"
#include "pxr/usd/usdExec/execUtils.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/ndr/declare.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecConnectableAPI;
struct UsdExecConnectionSourceInfo;
class UsdExecInput;

/// \class UsdExecOutput
/// 
/// This class encapsulates a node or node-graph output, which is a 
/// connectable attribute representing a typed, externally computed value.
/// 
class UsdExecOutput
{
public:
    /// Default constructor returns an invalid Output.  Exists for 
    /// container classes
    UsdExecOutput()
    {
        // nothing
    }

    /// Get the name of the attribute associated with the output. 
    /// 
    TfToken const &GetFullName() const { 
        return _attr.GetName(); 
    }

    /// Returns the name of the output. 
    /// 
    /// We call this the base name since it strips off the "outputs:" namespace 
    /// prefix from the attribute name, and returns it.
    /// 
    USDEXEC_API
    TfToken GetBaseName() const;

    /// Get the prim that the output belongs to.
    UsdPrim GetPrim() const {
        return _attr.GetPrim();
    }

    /// Get the "scene description" value type name of the attribute associated 
    /// with the output.
    /// 
    USDEXEC_API
    SdfValueTypeName GetTypeName() const;
    
    /// Set a value for the output.
    /// 
    /// It's unusual to be setting a value on an output since it represents 
    /// an externally computed value. The Set API is provided here just for the 
    /// sake of completeness and uniformity with other property schema.
    /// 
    USDEXEC_API
    bool Set(const VtValue& value, 
             UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Set the attribute value of the Output at \p time 
    /// 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        if (UsdAttribute attr = GetAttr()) {
            return attr.Set(value, time);
        }
        return false;
    }

    /// \name Configuring the Output's Type
    /// @{

    /// Specify an alternative, renderer-specific type to use when
    /// emitting/translating this output, rather than translating based
    /// on its GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for outputs that
    /// are of renderman custom struct types.
    ///
    /// \return true on success
    USDEXEC_API
    bool SetRenderType(TfToken const& renderType) const;

    /// Return this output's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    USDEXEC_API
    TfToken GetRenderType() const;

    /// Return true if a renderType has been specified for this
    /// output.
    ///
    /// \sa SetRenderType()
    USDEXEC_API
    bool HasRenderType() const;

    /// @}

    /// \name API to author and query an Output's execMetadata
    /// 
    /// This section provides API for authoring and querying node registry
    /// metadata on an Output. When the owning node prim is providing a node 
    /// definition, the authored "execMetadata" dictionary value provides 
    /// metadata needed to populate the Output correctly in the node registry. 
    /// 
    /// We expect the keys in execMetadata to correspond to the keys 
    /// in \ref SdrPropertyMetadata. However, this is not strictly enforced by
    /// the API. The only allowed value type in the "execMetadata" dictionary is 
    /// a std::string since it needs to be converted into a NdrTokenMap, which 
    /// Sdr will parse using the utilities available in \ref ExecMetadataHelpers.
    /// 
    /// @{

    /// Returns this Output's composed "execMetadata" dictionary as a 
    /// NdrTokenMap.
    USDEXEC_API
    NdrTokenMap GetExecMetadata() const;
    
    /// Returns the value corresponding to \p key in the composed 
    /// <b>execMetadata</b> dictionary.
    USDEXEC_API
    std::string GetExecMetadataByKey(const TfToken &key) const;
        
    /// Authors the given \p execMetadata value on this Output at the current 
    /// EditTarget.
    USDEXEC_API
    void SetExecMetadata(const NdrTokenMap &execMetadata) const;

    /// Sets the value corresponding to \p key to the given string \p value, in 
    /// the Output's "execMetadata" dictionary at the current EditTarget.
    USDEXEC_API
    void SetExecMetadataByKey(
        const TfToken &key, 
        const std::string &value) const;

    /// Returns true if the Output has a non-empty composed "execMetadata" 
    /// dictionary value.
    USDEXEC_API
    bool HasExecMetadata() const;

    /// Returns true if there is a value corresponding to the given \p key in 
    /// the composed "execMetadata" dictionary.
    USDEXEC_API
    bool HasExecMetadataByKey(const TfToken &key) const;

    /// Clears any "execMetadata" value authored on the Output in the current 
    /// EditTarget.
    USDEXEC_API
    void ClearExecMetadata() const;

    /// Clears the entry corresponding to the given \p key in the 
    /// "execMetadata" dictionary authored in the current EditTarget.
    USDEXEC_API
    void ClearExecMetadataByKey(const TfToken &key) const;

    /// @}

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    /// @{

    /// Speculative constructor that will produce a valid UsdExecOutput when
    /// \p attr already represents a node Output, and produces an \em invalid 
    /// UsdExecOutput otherwise (i.e. the explicit bool conversion operator 
    /// will return false).
    USDEXEC_API
    explicit UsdExecOutput(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Output, which
    /// implies that creating a UsdExecOutput from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    USDEXEC_API
    static bool IsOutput(const UsdAttribute &attr);

    /// Explicit UsdAttribute extractor.
    UsdAttribute GetAttr() const { return _attr; }
    
    /// Allow UsdExecOutput to auto-convert to UsdAttribute, so you can
    /// pass a UsdExecOutput to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute () const { return GetAttr(); }

    /// Return true if the wrapped UsdAttribute is defined, and in
    /// addition the attribute is identified as an output.
    ///
    bool IsDefined() const {
        return IsOutput(_attr);
    }

    /// @}

    // -------------------------------------------------------------------------
    /// \name Connections API
    // -------------------------------------------------------------------------
    /// @{

    /// Determines whether this Output can be connected to the given 
    /// source attribute, which can be an input or an output.
    /// 
    /// An output is considered to be connectable only if it belongs to a 
    /// node-graph. Node outputs are not connectable.
    /// 
    /// \sa UsdExecConnectableAPI::CanConnect
    USDEXEC_API
    bool CanConnect(const UsdAttribute &source) const;

    /// \overload
    USDEXEC_API
    bool CanConnect(const UsdExecInput &sourceInput) const;

    /// \overload
    USDEXEC_API
    bool CanConnect(const UsdExecOutput &sourceOutput) const;

    using ConnectionModification = UsdExecConnectionModification;

    /// Authors a connection for this Output
    /// 
    /// \p source is a struct that describes the upstream source attribute
    /// with all the information necessary to make a connection. See the
    /// documentation for UsdExecConnectionSourceInfo.
    /// \p mod describes the operation that should be applied to the list of
    /// connections. By default the new connection will replace any existing
    /// connections, but it can add to the list of connections to represent
    /// multiple input connections.
    /// 
    /// \return
    /// \c true if a connection was created successfully.
    /// \c false if \p shadingAttr or \p source is invalid.
    /// 
    /// \note This method does not verify the connectability of the shading
    /// attribute to the source. Clients must invoke CanConnect() themselves
    /// to ensure compatibility.
    /// \note The source shading attribute is created if it doesn't exist
    /// already.
    /// 
    /// \sa UsdExecConnectableAPI::ConnectToSource
    ///
    USDEXEC_API
    bool ConnectToSource(
        UsdExecConnectionSourceInfo const &source,
        ConnectionModification const mod =
            ConnectionModification::Replace) const;

    /// \deprecated
    /// \overload
    USDEXEC_API
    bool ConnectToSource(
        UsdExecConnectableAPI const &source, 
        TfToken const &sourceName, 
        UsdExecAttributeType const sourceType=UsdExecAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName()) const;

    /// Authors a connection for this Output to the source at the given path.
    /// 
    /// \sa UsdExecConnectableAPI::ConnectToSource
    ///
    USDEXEC_API
    bool ConnectToSource(SdfPath const &sourcePath) const;

    /// Connects this Output to the given input, \p sourceInput.
    /// 
    /// \sa UsdExecConnectableAPI::ConnectToSource
    ///
    USDEXEC_API
    bool ConnectToSource(UsdExecInput const &sourceInput) const;

    /// Connects this Output to the given output, \p sourceOutput.
    /// 
    /// \sa UsdExecConnectableAPI::ConnectToSource
    ///
    USDEXEC_API
    bool ConnectToSource(UsdExecOutput const &sourceOutput) const;

    /// Connects this Output to the given sources, \p sourceInfos
    /// 
    /// \sa UsdExecConnectableAPI::SetConnectedSources
    ///
    USDEXEC_API
    bool SetConnectedSources(
        std::vector<UsdExecConnectionSourceInfo> const &sourceInfos) const;

    // XXX move to new header
    using SourceInfoVector = TfSmallVector<UsdExecConnectionSourceInfo, 2>;

    /// Finds the valid sources of connections for the Output.
    /// 
    /// \p invalidSourcePaths is an optional output parameter to collect the
    /// invalid source paths that have not been reported in the returned vector.
    /// 
    /// Returns a vector of \p UsdExecConnectionSourceInfo structs with
    /// information about each upsteam attribute. If the vector is empty, there
    /// have been no valid connections.
    /// 
    /// \note A valid connection requires the existence of the source attribute
    /// and also requires that the source prim is UsdExecConnectableAPI
    /// compatible.
    /// \note The python wrapping returns a tuple with the valid connections
    /// first, followed by the invalid source paths.
    /// 
    /// \sa UsdExecConnectableAPI::GetConnectedSources
    ///
    USDEXEC_API
    SourceInfoVector GetConnectedSources(
        SdfPathVector *invalidSourcePaths = nullptr) const;

    /// \deprecated Please use GetConnectedSources instead
    USDEXEC_API
    bool GetConnectedSource(UsdExecConnectableAPI *source, 
                            TfToken *sourceName,
                            UsdExecAttributeType *sourceType) const;

    /// \deprecated
    /// Returns the "raw" (authored) connected source paths for this Output.
    /// 
    /// \sa UsdExecConnectableAPI::GetRawConnectedSourcePaths
    ///
    USDEXEC_API
    bool GetRawConnectedSourcePaths(SdfPathVector *sourcePaths) const;

    /// Returns true if and only if this Output is currently connected to a 
    /// valid (defined) source. 
    ///
    /// \sa UsdExecConnectableAPI::HasConnectedSource
    /// 
    USDEXEC_API
    bool HasConnectedSource() const;

    /// Disconnect source for this Output. If \p sourceAttr is valid, only a
    /// connection to the specified attribute is disconnected, otherwise all
    /// connections are removed.
    /// 
    /// \sa UsdExecConnectableAPI::DisconnectSource
    ///
    USDEXEC_API
    bool DisconnectSource(UsdAttribute const &sourceAttr = UsdAttribute()) const;

    /// Clears sources for this Output in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa UsdExecConnectableAPI::ClearSources
    ///
    USDEXEC_API
    bool ClearSources() const;

    /// \deprecated
    USDEXEC_API
    bool ClearSource() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Connected Value API
    // -------------------------------------------------------------------------
    /// @{

    /// \brief Find what is connected to this Output recursively
    ///
    /// \sa UsdExecUtils::GetValueProducingAttributes
    USDEXEC_API
    UsdExecAttributeVector GetValueProducingAttributes(
        bool outputsOnly = false) const;

    /// @}

    /// Return true if this Output is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
    explicit operator bool() const { 
        return IsDefined(); 
    }

    /// Equality comparison. Returns true if \a lhs and \a rhs represent the 
    /// same UsdExecOutput, false otherwise.
    friend bool operator==(const UsdExecOutput &lhs, const UsdExecOutput &rhs) {
        return lhs.GetAttr() == rhs.GetAttr();
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdExecOutput, true otherwise.
    friend bool operator!=(const UsdExecOutput &lhs, const UsdExecOutput &rhs) {
        return !(lhs == rhs);
    }

private:
    friend class UsdExecConnectableAPI;

    // Constructor that creates a UsdExecOutput with the given name on the 
    // given prim.
    // \p name here is the unnamespaced name of the output.
    UsdExecOutput(UsdPrim prim,
                   TfToken const &name,
                   SdfValueTypeName const &typeName);

    UsdAttribute _attr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_EXEC_OUTPUT_H
