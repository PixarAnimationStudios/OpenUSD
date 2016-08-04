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
#ifndef USDSHADE_PARAMETER_H
#define USDSHADE_PARAMETER_H




#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/api.h"

#include <vector>

class UsdShadeShader;

/// \class UsdShadeParameter
/// \brief Schema wrapper for UsdAttribute for authoring and introspecting
/// shader parameters (which are attributes within a shading network).
class UsdShadeParameter
{
public:
    // Default constructor returns an invalid Parameter.  Exists for 
    // container classes
    UsdShadeParameter()
    {
        // nothing
    }

    /// \name Configuring the Parameter's Type
    /// @{
    
    /// \brief Return true if this Parameter is an array-type.
    ///
    /// As described in UsdShadeShader::CreateParameter(), the determination
    /// of array-ness is given by the SdfValueTypeNames \em typeName with
    /// which the Parameter was created.
    USDSHADE_API
    bool IsArray() const;
    
    /// \brief Set the value for the shade parameter.
    USDSHADE_API
    bool Set(const VtValue& value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \brief Specify an alternative, renderer-specific type to use when
    /// emitting/translating this parameter, rather than translating based
    /// on its UsdAttribute::GetTypeName()
    ///
    /// For example, we set the renderType to "struct" for parameters that
    /// are of renderman custom struct types.
    ///
    /// \return true on success
    USDSHADE_API
    bool SetRenderType(TfToken const& renderType) const;

    /// \brief Return this parameter's specialized renderType, or an empty
    /// token if none was authored.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    TfToken GetRenderType() const;

    /// \brief Return true if a renderType has been specified for this
    /// parameter.
    ///
    /// \sa SetRenderType()
    USDSHADE_API
    bool HasRenderType() const;

    /// @}
    
    /// \name Connections
    /// @{

    /// \brief Connect parameter to a named output on a given \p source.
    ///
    /// This action simply records an introspectable relationship:
    /// it implies no actual dataflow in USD, and makes no statement
    /// about what client behavior should be when a Parameter
    /// is determined to possess both an authored value and a connection
    /// to a value source - client renderers are required to impose their
    /// own, self-consistent rules.
    ///
    /// The only constraint imposed by the shading model is that Parameter
    /// connections can be only single-targetted; that is, any given scalar
    /// parameter can target at most a single source/outputName pair.
    ///
    /// If this Parameter is array-typed, then we assume (without validation)
    /// that the named output is also array-typed, and therefore we record
    /// (and will return) only a single connection for the Parameter, rather
    /// than one-per-element. If both a single-connection and per-element
    /// connections are recorded on the same Parameter, the single
    /// connection will win and be returned by GetConnectedSources().  If
    /// you wish to override a single-connection with per-element
    /// connections, you should first call DisconnectSources().
    ///
    /// \param source    the Shader object producing the value
    /// \param outputName  the particular computation or parameter we 
    ///        want to consume
    /// \param outputIsParameter outputs and parameters are namespaced 
    ///        differently on a shader prim, therefore we need to know
    ///        to which we are connecting.  By default we assume we are
    ///        connecting to a computational output, but you can specify
    ///        instead a parameter (assuming your renderer supports it)
    ///        with a value of \c true.
    /// \sa GetConnectedSource(), GetConnectedSources()
    USDSHADE_API
    bool ConnectToSource(
            UsdShadeShader const &source, 
            TfToken const &outputName,
            bool outputIsParameter=false) const;

    /// \brief Connect a single element of an array-typed Parameter to the
    /// given \p source.
    ///
    /// If called on a non-array Parameter, no action is taken, and we return
    /// \c false.
    ///
    /// The default size of a connected array Parameter (i.e. size of the
    /// vectors returned by GetConnectedSources()) is one greater than the
    /// maximum over all \p elementIndex for which this method has been
    /// called, operating on any layer of the composition (i.e. 1 +
    /// MAX(elementIndices) ).  Any \p elementIndex less than the MAX, for
    /// which this method has never been called, will be assumed to be
    /// unconnected, and therefore return empty/invalid entries when queried;
    /// exporters for renderers that do not allow a mix of connected and
    /// unconnected elements should take note and ensure all elements have
    /// been specified.
    ///
    /// The array size <b>can be explicitly overridden</b> using
    /// SetConnectedArraySize(),
    ///
    /// \sa ConnectToSource(), SetConnectedArraySize()
    USDSHADE_API
    bool ConnectElementToSource(
            size_t elementIndex, 
            UsdShadeShader const &source, 
            TfToken const &outputName,
            bool outputIsParameter=false) const;

    /// \brief Disconnect just the given \p elementIndex index of the array.
    ///
    /// \return false if an error occurred, or if this Parameter is not
    /// array-valued, true otherwise.
    ///
    /// \sa DisconnectSources(), ConnectElementToSource()
    USDSHADE_API
    bool DisconnectElement(size_t elementIndex) const;
    
    /// \brief Disconnects (all, for array parameters) sources for this 
    /// Parameter.
    ///
    /// This may author more scene description than you might expect - we define
    /// the behavior of disconnect to be that, even if a parameter becomes
    /// connected in a weaker layer than the current UsdEditTarget, the
    /// Parameter will \em still be disconnected in the composition, therefore
    /// we must "block" it (see for e.g. UsdRelationship::BlockTargets()) in
    /// the current UsdEditTarget.  For array Parameters, this means blocking
    /// each element, whether or not it has previously been connected.
    ///
    /// \sa ConnectToSource().
    USDSHADE_API
    bool DisconnectSources() const;

    /// \brief Clears (all, for array parameters) sources for this 
    /// Parameter in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSources()
    /// rather than this function.
    ///
    /// \sa DisconnectSources(), UsdRelationship::ClearTargets()
    USDSHADE_API
    bool ClearSources() const;

    /// \brief If this parameter is connected, retrieve the \p source prim
    /// and \p outputName to which it is connected. For array-valued parameters
    /// that are not connected-as-a-unit, this method returns nothing.
    ///
    /// We name the object a parameter is connected to a "source," as 
    /// the "source" produces or contains a value for the parameter.
    /// \return \c true if \p source is a defined prim on the stage, and 
    /// \p source has an attribute that is either a parameter or output;
    /// \c false if not connected to a defined prim.
    ///
    /// \note If the parameter is array-typed, and GetConnectedSources() and
    /// GetConnectedArraySize() indicate the size of the array is one, the
    /// \em only way to determine whether the parameter has an entire-array
    /// connection to another array, or whether it is simply an array of a
    /// single element is by calling this method.  If this method returns a
    /// connected source, then it is an entire-array connection; otherwise,
    /// it is an array of size one.
    ///
    /// \note The python wrapping for this method returns a 
    /// (source, ouputName) tuple if the parameter is singly-connected, else
    /// \c None
    USDSHADE_API
    bool GetConnectedSource(
            UsdShadeShader *source, 
            TfToken *outputName) const;

    /// \brief Return all connected sources for this Parameter. 
    ///
    /// For non-array parameters, or array parameters that are "connected as
    /// a unit", this method returns vectors of length 1. It is
    /// the client's responsibility to call IsArray() to determine whether
    /// the parameter is, in fact, array-valued.
    ///
    /// \return true if <b>any element</b> of an array parameter is connected.
    ///
    /// If some elements are connected and others aren't, \em sources[elt]
    /// will evaluate to false for any \em elt that is unconnected. By
    /// implication, for any array-type parameter, this method will always 
    /// return \p sources and \p outputNames of length GetConnectedArraySize(),
    /// even if none of the elements is currently connected.
    ///
    /// \note The python wrapping for this method returns a tuple of 
    /// (sources, outputNames) lists, or None if parameter is unconnected.
    USDSHADE_API
    bool GetConnectedSources(
            std::vector<UsdShadeShader> *sources, 
            std::vector<TfToken> *outputNames) const;

    /// Returns true if and only if the parameter is currently connected to the
    /// output of another \em defined shader object.
    ///
    /// If you will be calling GetConnectedSource() or GetConnectedSources()
    /// afterwards anyways, it will be \em much faster to instead guard like
    /// so:
    /// \code
    /// if (param.IsArray() and param.GetConnectedSources(&ps, &os)){
    ///      // process entirely or element-wise connected array
    /// } else if (param.GetConnectedSource(&p, &s)){
    ///      // process non-array, connected parameter
    /// } else {
    ///      // process unconnected parameter
    /// }
    /// \endcode
    USDSHADE_API
    bool IsConnected() const;

    /// \brief Explicitly state the number of connectable elements in an array
    /// type Parameter.
    ///
    /// This method need not generally be exercised, and exists primarily to
    /// allow a stronger layer to shrink the size of an array Parameter whose
    /// element connections have been defined in weaker layers (each
    /// connection is an independent property, and properties cannot be
    /// deactivated in stronger layers as prims can).  We cannot simply
    /// rely on the Parameter's authored array-value to infer the number
    /// of elements, principally because the shading model does not \em require
    /// that a Parameter's value be authored at all in order to connect it.
    ///
    /// If \p numElts is greater than the number of authored element 
    /// connections, the vectors returned by GetConnectedSources() will be
    /// padded out with empty/invalid connections.
    ///
    /// If this Parameter is not array-typed, nothing will be authored, and
    /// the method returns \c false.
    USDSHADE_API
    bool SetConnectedArraySize(size_t numElts) const;
    
    /// \brief Return the number of connectable array-elements present for
    /// this Parameter
    ///
    /// If the Parameter is not array-typed, returns zero.  Otherwise, if
    /// SetConnectedArraySize() has been exercised, the authored value will
    /// be returned, else we will determine the array size by the highest 
    /// element that has been connected via ConnectElementToSource(),
    /// \em unless the array has been connected as a unit to another array, in
    /// which case the returned size is one.
    ///
    /// \sa ConnectElementToSource(), GetConnectedSources()
    USDSHADE_API
    size_t GetConnectedArraySize() const;
    
    /// @}

    // TODO:
    /// \name Shader Parameter Values API
    /// @{

    /// This API is still in progress.  UsdLookInterfaceMap will likely be a
    /// map from ShaderParameters -> values.
    typedef bool UsdLookInterfaceMap;
    //void GetValue(UsdTimeCode time, UsdLookInterfaceMap const &interfaceValues) const;

    /// @}

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------

    typedef const UsdAttribute UsdShadeParameter::*_UnspecifiedBoolType;

    /// Speculative constructor that will produce a valid UsdShadeParameter when
    /// \p attr already represents an attribute that is Parameter, and
    /// produces an \em invalid Parameter otherwise (i.e. 
    /// \ref UsdShadeParameter_bool_type "unspecified-bool-type()" will return
    /// false).
    USDSHADE_API
    explicit UsdShadeParameter(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents a valid Parameter, which
    /// implies that creating a UsdShadeParameter from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    //static bool IsShaderParameter(const UsdAttribute &attr);
    
    /// Allow UsdShadeParameter to auto-convert to UsdAttribute, so you can
    /// pass a UsdShadeParameter to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }

    // TODO
    /// \brief Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a Parameter.
    bool IsDefined() const { return 
        _attr;
        //IsShaderParameter(_attr); 
    }

    /// \anchor UsdShadeParameter_bool_type
    /// Return true if this Primvar is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdShadeParameter::_attr : 0;
    }
#endif // doxygen

private:
    friend class UsdShadeShader;

    /// \brief infers whether parameter should be scalar or array from the
    /// provided typeName
    UsdShadeParameter(
            UsdPrim prim,
            TfToken const &name,
            SdfValueTypeName const &typeName);


    bool _Connect(UsdRelationship const &rel, UsdShadeShader const &source, 
                  TfToken const &outputName, bool outputIsParameter) const;
    
    UsdAttribute _attr;
};

#endif // USDSHADE_PARAMETER_H
