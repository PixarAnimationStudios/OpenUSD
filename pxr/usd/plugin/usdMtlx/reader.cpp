//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/reader.h"
#include "pxr/usd/usdMtlx/utils.h"

#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdUI/nodeGraphNodeAPI.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/tokens.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/stringUtils.h"

#include <algorithm>

// If defined this macro causes default values on shader inputs
// (parameters and inputs in MaterialX-speak) to get written to each
// USD shader definition.  Note that this information is available in
// the corresponding NdrProperty for each input on the NdrNode for the
// shader.  There are pros and cons for including the defaults:
//   include:
//     + Final value visible in naive clients
//     - Redundant, could be out of sync
//     - Must compare value to detect default
//   exclude:
//     + Fewer opinions
//     + Default iff no opinion
//     - Naive clients (e.g. usdview) don't know default
#define ADD_NODE_INPUT_DEFAULTS_TO_USD

// MaterialX 1.35 lacks these.
namespace MaterialX {
using ConstParameterPtr = std::shared_ptr<const class Parameter>;
using ConstPortElementPtr = std::shared_ptr<const class PortElement>;
using ConstInputPtr = std::shared_ptr<const class Input>;
using ConstOutputPtr = std::shared_ptr<const class Output>;
using ConstInterfaceElementPtr = std::shared_ptr<const class InterfaceElement>;
using ConstShaderRefPtr = shared_ptr<const class ShaderRef>;
using ConstBindInputPtr = shared_ptr<const class BindInput>;
using ConstCollectionPtr = shared_ptr<const class Collection>;
using ConstGeomElementPtr = shared_ptr<const class GeomElement>;
using ConstLookPtr = shared_ptr<const class Look>;
using ConstMaterialAssignPtr = shared_ptr<const class MaterialAssign>;
}

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Attribute name tokens.
struct _AttributeNames {
    using Name = const std::string;

    Name bindtoken        {"bindtoken"};
    Name channels         {"channels"};
    Name cms              {"cms"};
    Name cmsconfig        {"cmsconfig"};
    Name collection       {"collection"};
    Name context          {"context"};
    Name default_         {"default"};
    Name doc              {"doc"};
    Name excludegeom      {"excludegeom"};
    Name geom             {"geom"};
    Name helptext         {"helptext"};
    Name includegeom      {"includegeom"};
    Name includecollection{"includecollection"};
    Name inherit          {"inherit"};
    Name interfacename    {"interfacename"};
    Name isdefaultversion {"isdefaultversion"};
    Name look             {"look"};
    Name material         {"material"};
    Name member           {"member"};
    Name nodedef          {"nodedef"};
    Name nodegraph        {"nodegraph"};
    Name nodename         {"nodename"};
    Name node             {"node"};
    Name output           {"output"};
    Name semantic         {"semantic"};
    Name shaderref        {"shaderref"};
    Name token            {"token"};
    Name type             {"type"};
    Name uicolor          {"uicolor"};
    Name uienum           {"uienum"};
    Name uienumvalues     {"uienumvalues"};
    Name uifolder         {"uifolder"};
    Name uimax            {"uimax"};
    Name uimin            {"uimin"};
    Name uiname           {"uiname"};
    Name value            {"value"};
    Name valuecurve       {"valuecurve"};
    Name valuerange       {"valuerange"};
    Name variant          {"variant"};
    Name variantassign    {"variantassign"};
    Name variantset       {"variantset"};
    Name version          {"version"};
    Name xpos             {"xpos"};
    Name ypos             {"ypos"};
};
static const _AttributeNames names;

TF_DEFINE_PRIVATE_TOKENS(
    tokens,

    ((defaultOutputName, "result"))
    ((light, "light"))
);

// Returns the name of an element.
template <typename T>
inline
static
const std::string&
_Name(const std::shared_ptr<T>& mtlx)
{
    return mtlx->getName();
}

// Returns the children of type T or any type derived from T.
template <typename T, typename U>
inline
static
std::vector<std::shared_ptr<T>>
_Children(const std::shared_ptr<U>& mtlx)
{
    std::vector<std::shared_ptr<T>> result;
    for (const mx::ElementPtr& child: mtlx->getChildren()) {
        if (auto typed = child->asA<T>()) {
            result.emplace_back(std::move(typed));
        }
    }
    return result;
}

// Returns the children of (exactly) the given category.
template <typename T>
inline
static
std::vector<mx::ElementPtr>
_Children(const std::shared_ptr<T>& mtlx, const std::string& category)
{
    std::vector<mx::ElementPtr> result;
    for (auto& child: mtlx->getChildren()) {
        if (child->getCategory() == category) {
            result.emplace_back(child);
        }
    }
    return result;
}

// A helper that wraps a MaterialX attribute value.  We don't usually
// care if an attribute exists, just that the value isn't empty.  (A
// non-existent attribute returns the empty string.)  A std::string
// has no operator bool or operator! so code would look this this:
//   auto& attr = elem->getAttribute("foo");
//   if (!attr.empty()) ...;
// With this helper we can do this:
//   if (auto& attr = _Attr(elem, "foo")) ...;
// This also allows us to add a check for existence in one place if
// that becomes important later.
class _Attr {
public:
    _Attr() = default;
    explicit _Attr(const std::string& value)
        : _value(value.empty() ? mx::EMPTY_STRING : value) { }
    template <typename T>
    _Attr(const std::shared_ptr<T>& element, const std::string& name)
        : _Attr(element->getAttribute(name)) { }
    _Attr(const _Attr&) = default;
    _Attr(_Attr&&) = default;
    _Attr& operator=(const _Attr&) = default;
    _Attr& operator=(_Attr&&) = default;
    ~_Attr() = default;

    explicit operator bool() const      { return !_value.empty(); }
    bool operator!() const              { return _value.empty(); }
    operator const std::string&() const { return _value; }
    const std::string& str() const      { return _value; }
    const char* c_str() const           { return _value.c_str(); }

    std::string::const_iterator begin() const { return _value.begin(); }
    std::string::const_iterator end()   const { return _value.end(); }

private:
    const std::string& _value = mx::EMPTY_STRING;
};

// Returns the type of a typed element.
template <typename T>
inline
static
const std::string&
_Type(const std::shared_ptr<T>& mtlx)
{
    return _Attr(mtlx, names.type);
}

// Returns the attribute named \p name on element \p mtlx as a T in \p value
// if possible and returns true, otherwise returns false.
template <typename T>
static
bool
_Value(T* value, const mx::ConstElementPtr& mtlx, const std::string& name)
{
    // Fail if the attribute doesn't exist.  This allows us to distinguish
    // an empty string from a missing string.
    if (!mtlx->hasAttribute(name)) {
        return false;
    }

    try {
        *value = mx::fromValueString<T>(_Attr(mtlx, name));
        return true;
    }
    catch (mx::ExceptionTypeError&) {
    }
    return false;
}

// Convert a MaterialX name into a valid USD name token.
static
TfToken
_MakeName(const std::string& mtlxName)
{
    // MaterialX names are valid USD names so we can use the name as is.
    return TfToken(mtlxName);
}

// Convert a MaterialX name into a valid USD name token.
static
TfToken
_MakeName(const mx::ConstElementPtr& mtlx)
{
    return mtlx ? _MakeName(_Name(mtlx)) : TfToken();
}

// Create a USD input on connectable that conforms to mtlx.
template <typename C>
static
UsdShadeInput
_MakeInput(C& connectable, const mx::ConstTypedElementPtr& mtlx)
{
    // Get the MaterialX type name.
    auto&& type = _Type(mtlx);
    if (type.empty()) {
        return UsdShadeInput();
    }

    // Get the Sdf type, if any.  If not then use token and we'll set
    // the render type later.
    TfToken renderType;
    auto converted = UsdMtlxGetUsdType(type).valueTypeName;
    if (!converted) {
        converted  = SdfValueTypeNames->Token;
        renderType = TfToken(type);
    }

    // Create the input.
    auto usdInput = connectable.CreateInput(_MakeName(mtlx), converted);

    // Set the render type if necessary.
    if (!renderType.IsEmpty()) {
        usdInput.SetRenderType(renderType);
    }

    return usdInput;
}

// Return the nodedef with node=family, that's type compatible with
// mtlxInterface, and has a compatible version.  If target isn't empty
// then it must also match.  Returns null if there's no such nodedef.
static
mx::ConstNodeDefPtr
_FindMatchingNodeDef(
    const mx::ConstInterfaceElementPtr& mtlxInterface,
    const std::string& family,
    const NdrVersion& version,
    const std::string& target)
{
    mx::ConstNodeDefPtr result = nullptr;

    for (auto&& mtlxNodeDef:
            mtlxInterface->getDocument()->getMatchingNodeDefs(family)) {
        // Filter by target.
        if (!mx::targetStringsMatch(target, mtlxNodeDef->getTarget())) {
            continue;
        }

        // Filter by types.
        if (!mtlxInterface->isTypeCompatible(mtlxNodeDef)) {
            continue;
        }

        // XXX -- We may want to cache nodedef version info.

        // Filter by version.
        bool implicitDefault;
        auto nodeDefVersion = UsdMtlxGetVersion(mtlxNodeDef,&implicitDefault);
        if (version.IsDefault()) {
            if (implicitDefault) {
                // This nodedef matches if no other nodedef is explicitly
                // the default so save it as the best match so far.
                result = mtlxNodeDef;
            }
            else if (nodeDefVersion.IsDefault()) {
                // The nodedef is explicitly the default and matches.
                result = mtlxNodeDef;
                break;
            }
        }
        else if (version == nodeDefVersion) {
            result = mtlxNodeDef;
            break;
        }
    }

    return result;
}

// Get the shader id for a MaterialX nodedef.
static
NdrIdentifier
_GetShaderId(const mx::ConstNodeDefPtr& mtlxNodeDef)
{
    return mtlxNodeDef ? NdrIdentifier(mtlxNodeDef->getName())
                       : NdrIdentifier();
}

// Get the shader id for a MaterialX node.
static
NdrIdentifier
_GetShaderId(const mx::ConstNodePtr& mtlxNode)
{
    return _GetShaderId(_FindMatchingNodeDef(mtlxNode,
                                             mtlxNode->getCategory(),
                                             UsdMtlxGetVersion(mtlxNode),
                                             mtlxNode->getTarget()));
}

// Copy the value from a Material value element to a UsdShadeInput with a
// Set() method taking any valid USD value type.
static
void
_CopyValue(const UsdShadeInput& usd, const mx::ConstValueElementPtr& mtlx)
{
    // Check for default value.
    auto value = UsdMtlxGetUsdValue(mtlx);
    if (!value.IsEmpty()) {
        usd.Set(value);
    }

    // Check for animated values.
    auto valuecurve = _Attr(mtlx, names.valuecurve);
    auto valuerange = _Attr(mtlx, names.valuerange);
    if (valuecurve && valuerange) {
        auto values = UsdMtlxGetPackedUsdValues(valuecurve,
                                                _Attr(mtlx, names.type));
        if (!values.empty()) {
            auto range = UsdMtlxGetPackedUsdValues(valuerange, "integer");
            if (range.size() == 2) {
                const int first = range[0].Get<int>();
                const int last  = range[1].Get<int>();
                if (last < first) {
                    TF_WARN(TfStringPrintf(
                        "Invalid valuerange [%d,%d] on '%s';  ignoring",
                        first, last, mtlx->getNamePath().c_str()));
                }
                else if (values.size() != size_t(last - first + 1)) {
                    TF_WARN(TfStringPrintf(
                        "valuerange [%d,%d] doesn't match valuecurve size "
                        "%zd on '%s';  ignoring",
                        first, last, values.size(),
                        mtlx->getNamePath().c_str()));
                }
                else {
                    auto frame = first;
                    for (auto&& value: values) {
                        usd.Set(value, UsdTimeCode(frame++));
                    }
                }
            }
            else {
                TF_WARN(TfStringPrintf(
                    "Malformed valuerange '%s' on '%s';  ignoring",
                    valuerange.c_str(), mtlx->getNamePath().c_str()));
            }
        }
        else {
            TF_WARN(TfStringPrintf(
                "Failed to parse valuecurve '%s' on '%s';  ignoring",
                valuecurve.c_str(), mtlx->getNamePath().c_str()));
        }
    }

    // Copy the active colorspace if it doesn't match the document and the
    // type supports it.
    auto&& colorspace = mtlx->getActiveColorSpace();
    if (!colorspace.empty() &&
            colorspace != mtlx->getDocument()->getActiveColorSpace()) {
        auto&& type = mtlx->getType();
        if (type.compare(0, 5, "color") == 0 || type == "filename") {
            usd.GetAttr().SetColorSpace(TfToken(colorspace));
        }
    }
}

// Copies common UI attributes available on any element from the element
// \p mtlx to the object \p usd.
static
void
_SetGlobalCoreUIAttributes(
    const UsdObject& usd, const mx::ConstElementPtr& mtlx)
{
    if (auto doc = _Attr(mtlx, names.doc)) {
        usd.SetDocumentation(doc);
    }
}

// Copies common UI attributes from the element \p mtlx to the object \p usd.
static
void
_SetCoreUIAttributes(const UsdObject& usd, const mx::ConstElementPtr& mtlx)
{
    _SetGlobalCoreUIAttributes(usd, mtlx);

    if (usd.Is<UsdPrim>()) {
        if (auto ui = UsdUINodeGraphNodeAPI(usd.GetPrim())) {
            float xpos, ypos;
            if (_Value(&xpos, mtlx, names.xpos) &&
                    _Value(&ypos, mtlx, names.ypos)) {
                ui.CreatePosAttr(VtValue(GfVec2f(xpos, ypos)));
            }

            mx::Vector3 color;
            if (_Value(&color, mtlx, names.uicolor)) {
                ui.CreateDisplayColorAttr(
                    VtValue(GfVec3f(color[0], color[1], color[2])));
            }
        }
    }
}

// Copies common UI attributes from the element \p mtlx to the object \p usd.
static
void
_SetUIAttributes(const UsdShadeInput& usd, const mx::ConstElementPtr& mtlx)
{
    if (auto helptext = _Attr(mtlx, names.helptext)) {
        usd.SetDocumentation(helptext);
    }

    mx::StringVec uienum;
    if (_Value(&uienum, mtlx, names.uienum) && !uienum.empty()) {
        // We can't write this directly via Usd API except through
        // SetMetadata() with a hard-coded key.  We'll use the Sdf
        // API instead.
        auto attr =
            TfStatic_cast<SdfAttributeSpecHandle>(
                usd.GetAttr().GetPropertyStack().front());
        VtTokenArray allowedTokens;
        allowedTokens.reserve(uienum.size());
        for (const auto& tokenString: uienum) {
            allowedTokens.push_back(TfToken(tokenString));
        }
        attr->SetAllowedTokens(allowedTokens);

        // XXX -- uienumvalues has no USD counterpart
    }

    // XXX -- uimin, uimax have no USD counterparts.

    if (auto uifolder = _Attr(mtlx, names.uifolder)) {
        // Translate '/' to ':'.
        std::string group = uifolder;
        std::replace(group.begin(), group.end(), '/', ':');
        usd.GetAttr().SetDisplayGroup(group);
    }
    if (auto uiname = _Attr(mtlx, names.uiname)) {
        usd.GetAttr().SetDisplayName(uiname);
    }

    _SetCoreUIAttributes(usd.GetAttr(), mtlx);
}

/// Returns an inheritance sequence with the most derived at the end
/// of the sequence.
template <typename T>
static
std::vector<std::shared_ptr<T>>
_GetInheritanceStack(const std::shared_ptr<T>& mtlxMostDerived)
{
    std::vector<std::shared_ptr<T>> result;

    // This is basically InheritanceIterator from 1.35.5 and up.
    std::set<std::shared_ptr<T>> visited;
    auto document = mtlxMostDerived->getDocument();
    for (auto mtlx = mtlxMostDerived; mtlx;
            mtlx = std::dynamic_pointer_cast<T>(
                document->getChild(_Attr(mtlx, names.inherit)))) {
        if (!visited.insert(mtlx).second) {
            throw mx::ExceptionFoundCycle(
                "Encountered cycle at element: " + mtlx->asString());
        }
        result.push_back(mtlx);
    }

    // We want more derived to the right.
    std::reverse(result.begin(), result.end());
    return result;
}

/// This class translates a MaterialX node graph into a USD node graph.
class _NodeGraphBuilder {
public:
    _NodeGraphBuilder() = default;
    _NodeGraphBuilder(const _NodeGraphBuilder&) = delete;
    _NodeGraphBuilder(_NodeGraphBuilder&&) = delete;
    _NodeGraphBuilder& operator=(const _NodeGraphBuilder&) = delete;
    _NodeGraphBuilder& operator=(_NodeGraphBuilder&&) = delete;
    ~_NodeGraphBuilder() = default;

    void SetInterface(const mx::ConstNodeDefPtr&);
    void SetContainer(const mx::ConstElementPtr&);
    void SetTarget(const UsdStagePtr&, const SdfPath& targetPath);
    void SetTarget(const UsdStagePtr&, const SdfPath& parentPath,
                   const mx::ConstElementPtr& childName);
    UsdShadeNodeGraph Build();

private:
    void _CreateInterface(const mx::ConstInterfaceElementPtr& iface,
                          const UsdShadeConnectableAPI& connectable);
    void _AddNode(const mx::ConstNodePtr& mtlxNode, const UsdPrim& usdParent);
    UsdShadeInput _AddParameter(const mx::ConstParameterPtr& mtlxParameter,
                                const UsdShadeConnectableAPI& connectable,
                                bool isInterface = false);
    UsdShadeInput _AddInput(const mx::ConstInputPtr& mtlxInput,
                            const UsdShadeConnectableAPI& connectable,
                            bool isInterface = false);
    UsdShadeInput _AddInputCommon(const mx::ConstValueElementPtr& mtlxValue,
                                  const UsdShadeConnectableAPI& connectable,
                                  bool isInterface);
    UsdShadeOutput _AddOutput(const mx::ConstTypedElementPtr& mtlxTyped,
                              const mx::ConstElementPtr& mtlxOwner,
                              const UsdShadeConnectableAPI& connectable,
                              bool shaderOnly = false);
    template <typename D>
    void _ConnectPorts(const mx::ConstPortElementPtr& mtlxDownstream,
                       const D& usdDownstream);
    template <typename U, typename D>
    void _ConnectPorts(const mx::ConstElementPtr& mtlxDownstream,
                       const U& usdUpstream, const D& usdDownstream);
    void _ConnectNodes();
    void _ConnectTerminals(const mx::ConstElementPtr& iface,
                           const UsdShadeConnectableAPI& connectable);

private:
    mx::ConstNodeDefPtr _mtlxNodeDef;
    mx::ConstElementPtr _mtlxContainer;
    UsdStagePtr _usdStage;
    SdfPath _usdPath;
    std::map<std::string, UsdShadeInput> _interfaceNames;
    std::map<mx::ConstInputPtr, UsdShadeInput> _inputs;
    std::map<std::string, UsdShadeOutput> _outputs;
};

void
_NodeGraphBuilder::SetInterface(const mx::ConstNodeDefPtr& mtlxNodeDef)
{
    _mtlxNodeDef = mtlxNodeDef;
}

void
_NodeGraphBuilder::SetContainer(const mx::ConstElementPtr& mtlxContainer)
{
    _mtlxContainer = mtlxContainer;
}

void
_NodeGraphBuilder::SetTarget(const UsdStagePtr& stage, const SdfPath& path)
{
    _usdStage = stage;
    _usdPath  = path;
}

void
_NodeGraphBuilder::SetTarget(
    const UsdStagePtr& stage,
    const SdfPath& parentPath,
    const mx::ConstElementPtr& childName)
{
    SetTarget(stage, parentPath.AppendChild(_MakeName(childName)));
}

UsdShadeNodeGraph
_NodeGraphBuilder::Build()
{
    if (!TF_VERIFY(_usdStage)) {
        return UsdShadeNodeGraph();
    }
    if (!TF_VERIFY(_usdPath.IsAbsolutePath() && _usdPath.IsPrimPath())) {
        return UsdShadeNodeGraph();
    }

    // Create the USD node graph.
    auto usdNodeGraph = UsdShadeNodeGraph::Define(_usdStage, _usdPath);
    if (!usdNodeGraph) {
        return UsdShadeNodeGraph();
    }
    if (_mtlxContainer->isA<mx::NodeGraph>()) {
        _SetCoreUIAttributes(usdNodeGraph.GetPrim(), _mtlxContainer);
    }

    // Create the interface.
    if (_mtlxNodeDef) {
        for (auto& i: _GetInheritanceStack(_mtlxNodeDef)) {
            _CreateInterface(i, usdNodeGraph);
        }
    }

    // Build the nodegraph.
    auto usdPrim = usdNodeGraph.GetPrim();
    // XXX -- File reading prior to 1.36 doesn't support nodes outside of
    //        a nodegraph so this will return an empty vector.
    for (auto& mtlxNode: _mtlxContainer->getChildrenOfType<mx::Node>()) {
        _AddNode(mtlxNode, usdPrim);
    }
    _ConnectNodes();
    _ConnectTerminals(_mtlxContainer, usdNodeGraph);

    return usdNodeGraph;
}

void
_NodeGraphBuilder::_CreateInterface(
    const mx::ConstInterfaceElementPtr& iface,
    const UsdShadeConnectableAPI& connectable)
{
    static constexpr bool isInterface = true;

    for (auto mtlxInput: iface->getParameters()) {
        _AddParameter(mtlxInput, connectable, isInterface);
    }
    for (auto mtlxInput: iface->getInputs()) {
        _AddInput(mtlxInput, connectable, isInterface);
    }
    // We deliberately ignore tokens here.
}

void
_NodeGraphBuilder::_AddNode(
    const mx::ConstNodePtr& mtlxNode,
    const UsdPrim& usdParent)
{
    // Create the shader.
    auto shaderId = _GetShaderId(mtlxNode);
    if (shaderId.IsEmpty()) {
        // If we don't have an interface then this is okay.
        if (_mtlxNodeDef) {
            return;
        }
    }
    auto stage      = usdParent.GetStage();
    auto shaderPath = usdParent.GetPath().AppendChild(_MakeName(mtlxNode));
    auto usdShader  = UsdShadeShader::Define(stage, shaderPath);
    if (!shaderId.IsEmpty()) {
        usdShader.CreateIdAttr(VtValue(TfToken(shaderId)));
    }
    auto connectable = usdShader.ConnectableAPI();
    _SetCoreUIAttributes(usdShader.GetPrim(), mtlxNode);

    // Add the parameters.
    for (auto mtlxInput: mtlxNode->getParameters()) {
        _AddParameter(mtlxInput, connectable);
    }

    // Add the inputs.
    for (auto mtlxInput: mtlxNode->getInputs()) {
        _AddInput(mtlxInput, connectable);
    }

    // We deliberately ignore tokens here.

    // Add the outputs.
    if (_Type(mtlxNode) == mx::MULTI_OUTPUT_TYPE_STRING) {
        if (auto mtlxNodeDef = mtlxNode->getNodeDef()) {
            for (auto i: _GetInheritanceStack(mtlxNodeDef)) {
                for (auto mtlxOutput: i->getOutputs()) {
                    _AddOutput(mtlxOutput, mtlxNode, connectable);
                }
            }
        }
    }
    else {
        // Default output.
        _AddOutput(mtlxNode, mtlxNode, connectable);
    }
}

UsdShadeInput
_NodeGraphBuilder::_AddParameter(
    const mx::ConstParameterPtr& mtlxParameter,
    const UsdShadeConnectableAPI& connectable,
    bool isInterface)
{
    auto result = _AddInputCommon(mtlxParameter, connectable, isInterface);
    result.SetConnectability(UsdShadeTokens->interfaceOnly);
    return result;
}

UsdShadeInput
_NodeGraphBuilder::_AddInput(
    const mx::ConstInputPtr& mtlxInput,
    const UsdShadeConnectableAPI& connectable,
    bool isInterface)
{
    return _inputs[mtlxInput] =
        _AddInputCommon(mtlxInput, connectable, isInterface);
}

UsdShadeInput
_NodeGraphBuilder::_AddInputCommon(
    const mx::ConstValueElementPtr& mtlxValue,
    const UsdShadeConnectableAPI& connectable,
    bool isInterface)
{
    auto usdInput = _MakeInput(connectable, mtlxValue);

    _CopyValue(usdInput, mtlxValue);
    _SetUIAttributes(usdInput, mtlxValue);

    // Add to the interface.
    if (isInterface) {
        _interfaceNames[_Name(mtlxValue)] = usdInput;
    }
    else {
        // See if this input is connected to the interface.
        if (auto name = _Attr(mtlxValue, names.interfacename)) {
            auto i = _interfaceNames.find(name);
            if (i != _interfaceNames.end()) {
                _ConnectPorts(mtlxValue, i->second, usdInput);
            }
            else {
                TF_WARN("No interface name '%s' for node '%s'",
                        name.c_str(), _Name(mtlxValue).c_str());
            }
        }
    }

    return usdInput;
}

UsdShadeOutput
_NodeGraphBuilder::_AddOutput(
    const mx::ConstTypedElementPtr& mtlxTyped,
    const mx::ConstElementPtr& mtlxOwner,
    const UsdShadeConnectableAPI& connectable,
    bool shaderOnly)
{
    auto& mtlxType = _Type(mtlxTyped);

    // Get the context, if any.
    std::string context;
    auto mtlxTypeDef = mtlxTyped->getDocument()->getTypeDef(mtlxType);
    if (mtlxTypeDef) {
        if (auto semantic = _Attr(mtlxTypeDef, names.semantic)) {
            if (semantic.str() == mx::SHADER_SEMANTIC) {
                context = _Attr(mtlxTypeDef, names.context);
            }
        }
    }

    // Choose the type.  USD uses Token for shader semantic types.
    TfToken renderType;
    SdfValueTypeName usdType;
    if (context == "surface" ||
            context == "displacement" ||
            context == "volume" ||
            context == "light" ||
            mtlxType == mx::SURFACE_SHADER_TYPE_STRING ||
            mtlxType == "displacementshader" ||
            mtlxType == mx::VOLUME_SHADER_TYPE_STRING ||
            mtlxType == "lightshader") {
        usdType = SdfValueTypeNames->Token;
    }
    else if (shaderOnly || !context.empty()) {
        // We don't know this shader semantic MaterialX type so use Token.
        usdType = SdfValueTypeNames->Token;
    }
    else {
        usdType = UsdMtlxGetUsdType(mtlxType).valueTypeName;
        if (!usdType) {
            usdType = SdfValueTypeNames->Token;
            renderType = TfToken(mtlxType);
        }
    }

    // Choose the output name.  If mtlxTyped is-a Output then we use the
    // output name, otherwise we use the default.
    const auto isAnOutput = mtlxTyped->isA<mx::Output>();
    const auto outputName =
        isAnOutput
            ? _MakeName(mtlxTyped)
            : tokens->defaultOutputName;

    // Get the node name.
    auto& nodeName = _Name(mtlxOwner);

    // Compute a key for finding this output.  Since we'll access this
    // table with the node name and optionally the output name for a
    // multioutput node, it's easiest to always have an output name
    // but make it empty for default outputs.
    auto key = nodeName + "." + (isAnOutput ? outputName.GetText() : "");

    auto result =
        _outputs[key] = connectable.CreateOutput(outputName, usdType);
    if (!renderType.IsEmpty()) {
        result.SetRenderType(renderType);
    }
    _SetCoreUIAttributes(result.GetAttr(), mtlxTyped);
    return result;
}

template <typename D>
void
_NodeGraphBuilder::_ConnectPorts(
    const mx::ConstPortElementPtr& mtlxDownstream,
    const D& usdDownstream)
{
    if (auto nodeName = _Attr(mtlxDownstream, names.nodename)) {
        auto i = _outputs.find(nodeName.str() + "." +
                               _Attr(mtlxDownstream, names.output).str());
        if (i == _outputs.end()) {
            TF_WARN("Output for <%s> missing",
                    usdDownstream.GetAttr().GetPath().GetText());
            return;
        }

        _ConnectPorts(mtlxDownstream, i->second, usdDownstream);
    }
}

template <typename U, typename D>
void
_NodeGraphBuilder::_ConnectPorts(
    const mx::ConstElementPtr& mtlxDownstream,
    const U& usdUpstream,
    const D& usdDownstream)
{
    if (mx::ConstInputPtr mtlxInput = mtlxDownstream->asA<mx::Input>()) {
        if (auto member = _Attr(mtlxInput, names.member)) {
            // XXX -- MaterialX member support.
            TF_WARN("Dropped member %s between <%s> -> <%s>",
                    member.c_str(),
                    usdUpstream.GetAttr().GetPath().GetText(),
                    usdDownstream.GetAttr().GetPath().GetText());
        }

        if (auto channels = _Attr(mtlxInput, names.channels)) {
            // XXX -- MaterialX swizzle support.
            TF_WARN("Dropped swizzle %s between <%s> -> <%s>",
                    channels.c_str(),
                    usdUpstream.GetAttr().GetPath().GetText(),
                    usdDownstream.GetAttr().GetPath().GetText());
        }
    }

    // Connect.
    if (!usdDownstream.ConnectToSource(usdUpstream)) {
        TF_WARN("Failed to connect <%s> -> <%s>",
                usdUpstream.GetAttr().GetPath().GetText(),
                usdDownstream.GetAttr().GetPath().GetText());
    }
}

void
_NodeGraphBuilder::_ConnectNodes()
{
    for (auto& i: _inputs) {
        _ConnectPorts(i.first, i.second);
    }
}

void
_NodeGraphBuilder::_ConnectTerminals(
    const mx::ConstElementPtr& iface,
    const UsdShadeConnectableAPI& connectable)
{
    for (auto& mtlxOutput: iface->getChildrenOfType<mx::Output>()) {
        _ConnectPorts(mtlxOutput, _AddOutput(mtlxOutput, iface, connectable));
    }
}

/// This wraps a UsdNodeGraph to allow referencing.
class _NodeGraph {
public:
    _NodeGraph();

    explicit operator bool() const { return bool(_usdNodeGraph); }

    void SetImplementation(_NodeGraphBuilder&);

    _NodeGraph AddReference(const SdfPath& referencingPath) const;

    UsdShadeNodeGraph GetNodeGraph() const { return _usdNodeGraph; }
    UsdShadeOutput GetOutputByName(const std::string& name) const;

private:
    _NodeGraph(const _NodeGraph&, const UsdPrim& referencer);

private:
    UsdShadeNodeGraph _usdNodeGraph;
    SdfPath _referencer;
};

_NodeGraph::_NodeGraph()
{
    // Do nothing
}

_NodeGraph::_NodeGraph(const _NodeGraph& other, const UsdPrim& referencer)
    : _usdNodeGraph(other._usdNodeGraph)
    , _referencer(referencer.GetPath())
{
    // Do nothing
}

void
_NodeGraph::SetImplementation(_NodeGraphBuilder& builder)
{
    if (auto usdNodeGraph = builder.Build()) {
        // Success.  Cut over.
        _usdNodeGraph = usdNodeGraph;
        _referencer   = SdfPath();
    }
}

UsdShadeOutput
_NodeGraph::GetOutputByName(const std::string& name) const
{
    auto nodeGraph =
        _referencer.IsEmpty()
            ? _usdNodeGraph
            : UsdShadeNodeGraph::Get(_usdNodeGraph.GetPrim().GetStage(),
                                     _referencer);
    if (nodeGraph) {
        return nodeGraph.GetOutput(TfToken(name));
    }
    else {
        return UsdShadeOutput();
    }
}

_NodeGraph
_NodeGraph::AddReference(const SdfPath& referencingPath) const
{
    if (!_usdNodeGraph) {
        return *this;
    }

    auto stage = _usdNodeGraph.GetPrim().GetStage();
    if (auto prim = stage->GetPrimAtPath(referencingPath)) {
        if (UsdShadeNodeGraph(prim)) {
            // A node graph already exists -- reuse it.
            return _NodeGraph(*this, prim);
        }

        // Something other than a node graph already exists.
        TF_WARN("Can't create node graph at <%s>; a '%s' already exists",
                referencingPath.GetText(), prim.GetTypeName().GetText());
        return _NodeGraph();
    }

    // Create a new prim referencing the node graph.
    auto referencer = stage->DefinePrim(referencingPath);
    _NodeGraph result(*this, referencer);
    referencer.GetReferences().AddInternalReference(_usdNodeGraph.GetPath());
    return result;
}

/// This class maintains significant state about the USD stage and
/// provides methods to translate MaterialX elements to USD objects.
/// It also provides enough accessors to implement the reader.
class _Context {
public:
    using VariantName = std::string;
    using VariantSetName = std::string;
    using VariantSetOrder = std::vector<VariantSetName>;
    using VariantShaderSet = std::vector<std::string>;

    _Context(const UsdStagePtr& stage, const SdfPath& internalPath);

    void AddVariants(const mx::ConstElementPtr& mtlx);
    _NodeGraph AddNodeGraph(const mx::ConstNodeGraphPtr& mtlxNodeGraph);
    _NodeGraph AddImplicitNodeGraph(const mx::ConstDocumentPtr& mtlxDocument);
    _NodeGraph AddNodeGraphWithDef(const mx::ConstNodeGraphPtr& mtlxNodeGraph);
    UsdShadeMaterial BeginMaterial(const mx::ConstMaterialPtr& mtlxMaterial);
    void EndMaterial();
    UsdShadeShader AddShaderRef(const mx::ConstShaderRefPtr& mtlxShaderRef);
    void AddMaterialVariant(const std::string& mtlxMaterialName,
                            const VariantSetName& variantSetName,
                            const VariantName& variantName,
                            const VariantName& uniqueVariantName,
                            const VariantShaderSet* shaders = nullptr) const;
    UsdCollectionAPI AddCollection(const mx::ConstCollectionPtr&);
    UsdCollectionAPI AddGeometryReference(const mx::ConstGeomElementPtr&);

    const VariantSetOrder& GetVariantSetOrder() const;
    std::set<VariantName> GetVariants(const VariantSetName&) const;
    UsdShadeMaterial GetMaterial(const std::string& mtlxMaterialName) const;
    SdfPath GetCollectionsPath() const;
    UsdCollectionAPI GetCollection(const mx::ConstGeomElementPtr&,
                                   const UsdPrim& prim = UsdPrim()) const;

private:
    using Variant = std::map<std::string, mx::ConstValueElementPtr>;
    using VariantSet = std::map<VariantName, Variant>;
    using VariantSetsByName = std::map<VariantSetName, VariantSet>;

    // A 'collection' attribute key is the collection name.
    using _CollectionKey = std::string;

    // A 'geom' attribute key is the (massaged) geom expressions.
    using _GeomKey = std::string;

    _NodeGraph _AddNodeGraph(const mx::ConstNodeGraphPtr& mtlxNodeGraph,
                             const mx::ConstDocumentPtr& mtlxDocument);
    void _BindNodeGraph(const mx::ConstBindInputPtr& mtlxBindInput,
                        const UsdShadeConnectableAPI& connectable,
                        const _NodeGraph& usdNodeGraph);
    bool _AddShaderVariant(const std::string& mtlxMaterialName,
                           const std::string& mtlxShaderRefName,
                           const Variant& variant);
    static
    UsdShadeInput _AddInput(const mx::ConstValueElementPtr& mtlxValue,
                            const UsdShadeConnectableAPI& connectable);
    static
    UsdShadeInput _AddInputWithValue(const mx::ConstValueElementPtr& mtlxValue,
                                     const UsdShadeConnectableAPI& connectable);
    static
    UsdShadeOutput _AddShaderOutput(const mx::ConstTypedElementPtr& mtlxTyped,
                                    const UsdShadeConnectableAPI& connectable);
    UsdCollectionAPI _AddCollection(const mx::ConstCollectionPtr& mtlxCollection,
                                    std::set<mx::ConstCollectionPtr>* visited);
    UsdCollectionAPI _AddGeomExpr(const mx::ConstGeomElementPtr& mtlxGeomElement);
    void _AddGeom(const UsdRelationship& rel,
                  const std::string& pathString) const;

    const Variant*
    _GetVariant(const VariantSetName&, const VariantName&) const;
    void _CopyVariant(const UsdShadeConnectableAPI&, const Variant&) const;

private:
    UsdStagePtr _stage;
    SdfPath _collectionsPath;
    SdfPath _looksPath;
    SdfPath _materialsPath;
    SdfPath _nodeGraphsPath;
    SdfPath _shadersPath;

    // Global state.
    VariantSetsByName _variantSets;
    VariantSetOrder _variantSetGlobalOrder;
    std::map<mx::ConstNodeGraphPtr, _NodeGraph> _nodeGraphs;
    std::map<std::string, UsdShadeMaterial> _materials;
    std::map<_CollectionKey, UsdCollectionAPI> _collections;
    std::map<_GeomKey, UsdCollectionAPI> _geomSets;
    std::map<mx::ConstGeomElementPtr, UsdCollectionAPI> _collectionMapping;
    // Mapping of MaterialX material name to mapping of shaderref name to
    // the corresponding UsdShadeShader.  If the shaderref name is empty
    // this maps to the UsdShadeMaterial.
    std::map<std::string,
             std::map<std::string, UsdShadeConnectableAPI>> _shaders;
    int _nextGeomIndex = 1;

    // Active state.
    mx::ConstMaterialPtr _mtlxMaterial;
    UsdShadeMaterial _usdMaterial;
};

_Context::_Context(const UsdStagePtr& stage, const SdfPath& internalPath)
    : _stage(stage)
    , _collectionsPath(internalPath.AppendChild(TfToken("Collections")))
    , _looksPath(internalPath.AppendChild(TfToken("Looks")))
    , _materialsPath(internalPath.AppendChild(TfToken("Materials")))
    , _nodeGraphsPath(internalPath.AppendChild(TfToken("NodeGraphs")))
    , _shadersPath(internalPath.AppendChild(TfToken("Shaders")))
{
    // Do nothing
}

void
_Context::AddVariants(const mx::ConstElementPtr& mtlx)
{
    // Collect all of the MaterialX variants.
    for (auto& mtlxVariantSet: _Children(mtlx, names.variantset)) {
        VariantSet variantSet;

        // Over all variants.
        for (auto& mtlxVariant: _Children(mtlxVariantSet, names.variant)) {
            Variant variant;

            // Over all values in the variant.
            for (auto& mtlxValue: _Children<mx::ValueElement>(mtlxVariant)) {
                variant[_Name(mtlxValue)] = mtlxValue;
            }

            // Keep the variant iff there was something in it.
            if (!variant.empty()) {
                variantSet[_Name(mtlxVariant)] = std::move(variant);
            }
        }

        // Keep the variant set iff there was something in it.
        if (!variantSet.empty()) {
            auto& variantSetName = _Name(mtlxVariantSet);
            _variantSets[variantSetName] = std::move(variantSet);
            _variantSetGlobalOrder.push_back(variantSetName);
        }
    }
}

_NodeGraph
_Context::AddNodeGraph(const mx::ConstNodeGraphPtr& mtlxNodeGraph)
{
    return _AddNodeGraph(mtlxNodeGraph, mtlxNodeGraph->getDocument());
}

_NodeGraph
_Context::AddImplicitNodeGraph(const mx::ConstDocumentPtr& mtlxDocument)
{
    return _AddNodeGraph(nullptr, mtlxDocument);
}

_NodeGraph
_Context::_AddNodeGraph(
    const mx::ConstNodeGraphPtr& mtlxNodeGraph,
    const mx::ConstDocumentPtr& mtlxDocument)
{
    auto& nodeGraph = _nodeGraphs[mtlxNodeGraph];
    if (!nodeGraph) {
        _NodeGraphBuilder builder;

        // Choose USD parent path.  If mtlxNodeGraph exists then use
        // its name as the USD nodegraph's name, otherwise we're
        // making a nodegraph out of the nodes and outputs at the
        // document scope and we need a unique name.
        if (mtlxNodeGraph) {
            builder.SetContainer(mtlxNodeGraph);
            builder.SetTarget(_stage, _nodeGraphsPath, mtlxNodeGraph);
        }
        else {
            // XXX -- Cast away const because createValidChildName() is
            //        not const but should be.
            auto uniqueName =
                std::const_pointer_cast<mx::Document>(mtlxDocument)
                    ->createValidChildName("adhoc");
            auto parentPath =
                _nodeGraphsPath.AppendChild(_MakeName(uniqueName));
            builder.SetContainer(mtlxDocument);
            builder.SetTarget(_stage, parentPath);
        }

        nodeGraph.SetImplementation(builder);
    }
    return nodeGraph;
}

_NodeGraph
_Context::AddNodeGraphWithDef(const mx::ConstNodeGraphPtr& mtlxNodeGraph)
{
    auto& nodeGraph = _nodeGraphs[mtlxNodeGraph];
    if (!nodeGraph && mtlxNodeGraph) {
        if (auto mtlxNodeDef = mtlxNodeGraph->getNodeDef()) {
            _NodeGraphBuilder builder;
            builder.SetInterface(mtlxNodeDef);
            builder.SetContainer(mtlxNodeGraph);
            builder.SetTarget(_stage, _nodeGraphsPath, mtlxNodeDef);
            nodeGraph.SetImplementation(builder);
        }
    }
    return nodeGraph;
}

UsdShadeMaterial
_Context::BeginMaterial(const mx::ConstMaterialPtr& mtlxMaterial)
{
    if (TF_VERIFY(!_usdMaterial)) {
        auto materialPath =
            _materialsPath.AppendChild(_MakeName(mtlxMaterial));
        if (auto usdMaterial = UsdShadeMaterial::Define(_stage, materialPath)) {
            _SetCoreUIAttributes(usdMaterial.GetPrim(), mtlxMaterial);

            // Record the material for later variants.
            _shaders[_Name(mtlxMaterial)][""] = usdMaterial;

            // Cut over.
            _mtlxMaterial = mtlxMaterial;
            _usdMaterial  = usdMaterial;
        }
    }
    return _usdMaterial;
}

void
_Context::EndMaterial()
{
    if (!TF_VERIFY(_usdMaterial)) {
        return;
    }

    _materials[_Name(_mtlxMaterial)] = _usdMaterial;
    _mtlxMaterial = nullptr;
    _usdMaterial  = UsdShadeMaterial();
}

UsdShadeShader
_Context::AddShaderRef(const mx::ConstShaderRefPtr& mtlxShaderRef)
{
    if (!TF_VERIFY(_usdMaterial)) {
        return UsdShadeShader();
    }

    // Get the nodeDef for this shaderRef.
    mx::ConstNodeDefPtr mtlxNodeDef = mtlxShaderRef->getNodeDef();
    if (mtlxShaderRef->getNodeDefString().empty()) {
        // The shaderref specified a node instead of a nodeDef. Find
        // the best matching nodedef since the MaterialX API doesn't.
        mtlxNodeDef =
            _FindMatchingNodeDef(mtlxNodeDef,
                                 mtlxShaderRef->getNodeString(),
                                 UsdMtlxGetVersion(mtlxShaderRef),
                                 mtlxShaderRef->getTarget());
    }
    auto shaderId = _GetShaderId(mtlxNodeDef);
    if (shaderId.IsEmpty()) {
        return UsdShadeShader();
    }

    // XXX -- If the node def is implemented by a nodegraph we may need
    //        to reference that node graph instead of creating a
    //        UsdShadeShader.  That will require other USD support for
    //        inline shaders.

    // XXX -- At the moment it's not clear how we'll handle a nodedef
    //        backed by a nodegraph.  Will it be opaque with the
    //        implementation known only to the shader registry?  Or will
    //        using that shader cause the nodegraph to be added to the
    //        stage?  In that case do we make a separate copy of the
    //        nodegraph for each use or reference a single instantation?
    //
    //        Note that we don't have a efficient way to get the nodegraph
    //        that implements the nodedef here.

    // Choose the name of the shader.  In MaterialX this is just
    // mtlxShaderRef->getName() and has no meaning other than to uniquely
    // identify the shader.  In USD to support materialinherit we must
    // ensure that shaders have the same name if one should compose over
    // the other.  MaterialX composes over if a shaderref refers to the
    // same nodedef so in USD we use the nodedef's name.  This name isn't
    // ideal since it's just an arbitrary unique name;  the nodedef's
    // node name is more meaningful.  But the MaterialX spec says that
    // composing over happens if the shaderrefs refer to the same
    // nodedef element, not the same nodedef node name, and more than one
    // nodedef can overload a node name.
    const auto name = _MakeName(mtlxNodeDef);

    // Create the shader if it doesn't exist and copy node def values.
    auto shaderImplPath = _shadersPath.AppendChild(name);
    if (auto usdShaderImpl = UsdShadeShader::Get(_stage, shaderImplPath)) {
        // Do nothing
    }
    else if (usdShaderImpl = UsdShadeShader::Define(_stage, shaderImplPath)) {
        usdShaderImpl.CreateIdAttr(VtValue(TfToken(shaderId)));
        auto connectable = usdShaderImpl.ConnectableAPI();
        _SetCoreUIAttributes(usdShaderImpl.GetPrim(), mtlxShaderRef);

        for (auto& i: _GetInheritanceStack(mtlxNodeDef)) {
#ifdef ADD_NODE_INPUT_DEFAULTS_TO_USD
            // Copy the nodedef parameters/inputs.
            for (auto mtlxValue: i->getParameters()) {
                _CopyValue(_MakeInput(usdShaderImpl, mtlxValue), mtlxValue);
            }
            for (auto mtlxValue: i->getInputs()) {
                _CopyValue(_MakeInput(usdShaderImpl, mtlxValue), mtlxValue);
            }
            // We deliberately ignore tokens here.
#endif

            // Create USD output(s) for each MaterialX output with
            // semantic="shader".
            if (_Type(mtlxNodeDef) == mx::MULTI_OUTPUT_TYPE_STRING) {
                for (auto mtlxOutput: i->getOutputs()) {
                    _AddShaderOutput(mtlxOutput, connectable);
                }
            }
            else {
                _AddShaderOutput(i, connectable);
            }
        }
    }

    // Reference the shader under the material.  We need to reference it
    // so variants will be stronger, in case we have any variants.
    auto shaderPath = _usdMaterial.GetPath().AppendChild(name);
    auto usdShader = UsdShadeShader::Define(_stage, shaderPath);
    usdShader.GetPrim().GetReferences().AddInternalReference(shaderImplPath);

    // Record the referencing shader for later variants.
    _shaders[_Name(_mtlxMaterial)][_Name(mtlxShaderRef)] = usdShader;

    // Connect to material interface.
    for (auto& i: _GetInheritanceStack(mtlxNodeDef)) {
        for (auto mtlxValue: i->getParameters()) {
            auto shaderInput   = _MakeInput(usdShader, mtlxValue);
            auto materialInput = _MakeInput(_usdMaterial, mtlxValue);
            shaderInput.SetConnectability(UsdShadeTokens->interfaceOnly);
            materialInput.SetConnectability(UsdShadeTokens->interfaceOnly);
            shaderInput.ConnectToSource(materialInput);
        }
        for (auto mtlxValue: i->getInputs()) {
            auto shaderInput   = _MakeInput(usdShader, mtlxValue);
            auto materialInput = _MakeInput(_usdMaterial, mtlxValue);
            shaderInput.ConnectToSource(materialInput);
        }
        // We deliberately ignore tokens here.
    }

    // Translate bindings.
    for (auto mtlxParam: mtlxShaderRef->getBindParams()) {
        if (auto input = _AddInputWithValue(mtlxParam, _usdMaterial)) {
            input.SetConnectability(UsdShadeTokens->interfaceOnly);
        }
    }
    for (auto mtlxInput: mtlxShaderRef->getBindInputs()) {
        // Simple binding.
        _AddInputWithValue(mtlxInput, _usdMaterial);

        // Check if this input references an output.
        if (auto outputName = _Attr(mtlxInput, names.output)) {
            // The "nodegraph" attribute is optional.  If missing then
            // we create a USD nodegraph from the nodes and outputs on
            // the document and use that.
            auto mtlxNodeGraph =
                mtlxInput->getDocument()->
                    getNodeGraph(_Attr(mtlxInput, names.nodegraph).str());
            if (auto usdNodeGraph =
                    mtlxNodeGraph
                        ? AddNodeGraph(mtlxNodeGraph)
                        : AddImplicitNodeGraph(mtlxInput->getDocument())) {
                _BindNodeGraph(mtlxInput, _usdMaterial, usdNodeGraph);
            }
        }
    }
    if (auto primvars = UsdGeomPrimvarsAPI(_usdMaterial)) {
        for (auto mtlxToken: mtlxShaderRef->getChildren()) {
            if (mtlxToken->getCategory() == names.bindtoken) {
                // Always use the string type for MaterialX tokens.
                auto primvar =
                    UsdGeomPrimvarsAPI(_usdMaterial)
                        .CreatePrimvar(_MakeName(mtlxToken),
                                       SdfValueTypeNames->String);
                primvar.Set(VtValue(_Attr(mtlxToken, names.value).str()));
            }
        }
    }

    // Connect the shader's outputs to the material.
    if (auto output = usdShader.GetOutput(UsdShadeTokens->surface)) {
        UsdShadeConnectableAPI::ConnectToSource(
            _usdMaterial.CreateSurfaceOutput(),
            output);
    }
    if (auto output = usdShader.GetOutput(UsdShadeTokens->displacement)) {
        UsdShadeConnectableAPI::ConnectToSource(
            _usdMaterial.CreateDisplacementOutput(),
            output);
    }
    if (auto output = usdShader.GetOutput(UsdShadeTokens->volume)) {
        UsdShadeConnectableAPI::ConnectToSource(
            _usdMaterial.CreateVolumeOutput(),
            output);
    }
    if (auto output = usdShader.GetOutput(tokens->light)) {
        // USD doesn't support this type.
        UsdShadeConnectableAPI::ConnectToSource(
            _usdMaterial.CreateOutput(tokens->light, SdfValueTypeNames->Token),
            output);
    }

    // Connect other semantic shader outputs.
    for (auto output: usdShader.GetOutputs()) {
        auto name = output.GetBaseName();
        if (name != UsdShadeTokens->surface &&
            name != UsdShadeTokens->displacement &&
            name != UsdShadeTokens->volume &&
            name != tokens->light) {
            UsdShadeConnectableAPI::ConnectToSource(
                _usdMaterial.CreateOutput(name, SdfValueTypeNames->Token),
                output);
        }
    }

    return usdShader;
}

void
_Context::AddMaterialVariant(
    const std::string& mtlxMaterialName,
    const VariantSetName& variantSetName,
    const VariantName& variantName,
    const VariantName& uniqueVariantName,
    const VariantShaderSet* shaders) const
{
    auto i = _shaders.find(mtlxMaterialName);
    if (i == _shaders.end()) {
        // Unknown material.
        return;
    }
    const auto* variant = _GetVariant(variantSetName, variantName);
    if (!variant) {
        // Unknown variant.
        return;
    }

    // Create the variant set on the material.
    auto usdMaterial = GetMaterial(mtlxMaterialName);
    auto usdVariantSet =
        usdMaterial.GetPrim().GetVariantSet(variantSetName);

    // Create the variant on the material.
    if (!usdVariantSet.AddVariant(uniqueVariantName)) {
        TF_CODING_ERROR("Failed to author material variant '%s' "
                        "in variant set '%s' on <%s>",
                        uniqueVariantName.c_str(),
                        variantSetName.c_str(),
                        usdMaterial.GetPath().GetText());
        return;
    }

    usdVariantSet.SetVariantSelection(uniqueVariantName);
    {
        UsdEditContext ctx(usdVariantSet.GetVariantEditContext());
        if (shaders) {
            // Copy to given shaderrefs.
            for (auto& mtlxShaderRefName: *shaders) {
                auto j = i->second.find(mtlxShaderRefName);
                if (j != i->second.end()) {
                    _CopyVariant(j->second, *variant);
                }
            }
        }
        else {
            // Copy to the material.
            auto j = i->second.find("");
            if (j != i->second.end()) {
                _CopyVariant(j->second, *variant);
            }
        }
    }
    usdVariantSet.ClearVariantSelection();
}

bool
_Context::_AddShaderVariant(
    const std::string& mtlxMaterialName,
    const std::string& mtlxShaderRefName,
    const Variant& variant)
{
    // Find the USD shader.
    auto usdShader = _shaders[mtlxMaterialName][mtlxShaderRefName];
    if (!usdShader) {
        // Unknown shader.
        return false;
    }

    // Copy the values.
    for (auto& nameAndValue: variant) {
        auto& mtlxValue = nameAndValue.second;
        _CopyValue(_MakeInput(usdShader, mtlxValue), mtlxValue);
    }
    return true;
}

UsdCollectionAPI
_Context::AddCollection(const mx::ConstCollectionPtr& mtlxCollection)
{
    // Add the collection and any referenced collection.
    std::set<mx::ConstCollectionPtr> visited;
    return _AddCollection(mtlxCollection, &visited);
}

UsdCollectionAPI
_Context::AddGeometryReference(const mx::ConstGeomElementPtr& mtlxGeomElement)
{
    // Get the MaterialX collection.
    UsdCollectionAPI result;
    if (auto mtlxCollection = _Attr(mtlxGeomElement, names.collection)) {
        auto i = _collections.find(mtlxCollection);
        if (i != _collections.end()) {
            result = i->second;
        }
        else {
            TF_WARN("Unknown collection '%s' in %s",
                    mtlxCollection.c_str(),
                    mtlxGeomElement->getNamePath().c_str());
        }
    }

    // If there's a 'geom' attribute then use that instead.
    else if (auto collection = _AddGeomExpr(mtlxGeomElement)) {
        result = collection;
    }

    // Remember the collection for this geom element.
    return _collectionMapping[mtlxGeomElement] = result;
}

UsdCollectionAPI
_Context::_AddCollection(
    const mx::ConstCollectionPtr& mtlxCollection,
    std::set<mx::ConstCollectionPtr>* visited)
{
    if (!visited->insert(mtlxCollection).second) {
        TF_WARN("Found a collection cycle at '%s'",
                _Name(mtlxCollection).c_str());
        return UsdCollectionAPI();
    }

    // Create the prim.
    auto usdPrim = _stage->DefinePrim(_collectionsPath);

    // Create the collection.
    auto& usdCollection =
        _collections[_Name(mtlxCollection)] =
            UsdCollectionAPI::ApplyCollection(usdPrim,
                                    _MakeName(mtlxCollection));
    _SetCoreUIAttributes(usdCollection.CreateIncludesRel(), mtlxCollection);

    // Add the included collections (recursively creating them if necessary)
    // and the included and excluded geometry.
    if (auto inclcol = _Attr(mtlxCollection, names.includecollection)) {
        for (auto& collectionName: UsdMtlxSplitStringArray(inclcol.str())) {
            if (auto mtlxChildCollection =
                    mtlxCollection->getDocument()
                        ->getCollection(collectionName)) {
                if (auto usdChildCollection =
                        _AddCollection(mtlxChildCollection, visited)) {
                    usdCollection.IncludePath(
                        usdChildCollection.GetCollectionPath());
                }
            }
        }
    }
    auto& geomprefix = mtlxCollection->getActiveGeomPrefix();
    if (auto inclgeom = _Attr(mtlxCollection, names.includegeom)) {
        for (auto& path: UsdMtlxSplitStringArray(inclgeom.str())) {
            _AddGeom(usdCollection.CreateIncludesRel(), geomprefix + path);
        }
    }
    if (auto exclgeom = _Attr(mtlxCollection, names.excludegeom)) {
        for (auto& path: UsdMtlxSplitStringArray(exclgeom.str())) {
            _AddGeom(usdCollection.CreateExcludesRel(), geomprefix + path);
        }
    }
    return usdCollection;
}

UsdCollectionAPI
_Context::_AddGeomExpr(const mx::ConstGeomElementPtr& mtlxGeomElement)
{
    // Check if the 'geom' attribute exists.
    auto geom = _Attr(mtlxGeomElement, names.geom);
    if (!geom) {
        // No 'geom' attribute so give up.
        return UsdCollectionAPI();
    }

    // Since a geom attribute can only add geometry it doesn't matter
    // what order it's in.  So we split, sort, discard duplicates
    // and join to make a key.
    auto geomexprArray = UsdMtlxSplitStringArray(geom.str());
    std::sort(geomexprArray.begin(), geomexprArray.end());
    geomexprArray.erase(std::unique(geomexprArray.begin(),
                                    geomexprArray.end()),
                        geomexprArray.end());
    _GeomKey key = TfStringJoin(geomexprArray, ",");

    // See if this key exists.
    auto i = _geomSets.emplace(std::move(key), UsdCollectionAPI());
    if (!i.second) {
        // Yep, we have this collection already.
        return i.first->second;
    }

    // Nope, new collection.  Make a unique name for it.
    int& k = _nextGeomIndex;
    auto name = "geom_";
    auto usdPrim = _stage->DefinePrim(_collectionsPath);
    while (UsdCollectionAPI(usdPrim, TfToken(name + std::to_string(k)))) {
        ++k;
    }

    // Create the collection.
    auto& usdCollection =
        i.first->second =
            UsdCollectionAPI::ApplyCollection(usdPrim,
                                    TfToken(name + std::to_string(k)));

    // Add the geometry expressions.
    auto& geomprefix = mtlxGeomElement->getActiveGeomPrefix();
    for (auto& path: geomexprArray) {
        _AddGeom(usdCollection.CreateIncludesRel(), geomprefix + path);
    }

    return usdCollection;
}

void
_Context::_AddGeom(
    const UsdRelationship& rel, const std::string& pathString) const
{
    std::string errMsg;
    if (SdfPath::IsValidPathString(pathString, &errMsg)) {
        rel.AddTarget(
            SdfPath(pathString).ReplacePrefix(SdfPath::AbsoluteRootPath(),
                                              _collectionsPath));
    }
    else {
        TF_WARN("Ignored non-path '%s' on collection relationship <%s>",
                pathString.c_str(), rel.GetPath().GetText());
    }
}

const _Context::VariantSetOrder&
_Context::GetVariantSetOrder() const
{
    return _variantSetGlobalOrder;
}

std::set<_Context::VariantName>
_Context::GetVariants(const VariantSetName& variantSetName) const
{
    std::set<VariantName> result;
    auto i = _variantSets.find(variantSetName);
    if (i != _variantSets.end()) {
        for (auto& j: i->second) {
            result.insert(j.first);
        }
    }
    return result;
}

UsdShadeMaterial
_Context::GetMaterial(const std::string& mtlxMaterialName) const
{
    auto i = _materials.find(mtlxMaterialName);
    return i == _materials.end() ? UsdShadeMaterial() : i->second;
}

SdfPath
_Context::GetCollectionsPath() const
{
    return _collectionsPath;
}

UsdCollectionAPI
_Context::GetCollection(
    const mx::ConstGeomElementPtr& mtlxGeomElement,
    const UsdPrim& prim) const
{
    auto i = _collectionMapping.find(mtlxGeomElement);
    if (i == _collectionMapping.end()) {
        return UsdCollectionAPI();
    }
    if (!prim) {
        return i->second;
    }

    // Remap the collection to prim.
    auto orig = i->second.GetCollectionPath();
    auto path = orig.ReplacePrefix(orig.GetPrimPath(), prim.GetPath());
    if (path.IsEmpty()) {
        return UsdCollectionAPI();
    }
    return UsdCollectionAPI::GetCollection(prim.GetStage(), path);
}

void
_Context::_BindNodeGraph(
    const mx::ConstBindInputPtr& mtlxBindInput,
    const UsdShadeConnectableAPI& connectable,
    const _NodeGraph& usdNodeGraph)
{
    // Reference the instantiation.
    auto referencingPath =
        connectable.GetPath().AppendChild(
            usdNodeGraph.GetNodeGraph().GetPath().GetNameToken());
    auto refNodeGraph = usdNodeGraph.AddReference(referencingPath);
    if (!refNodeGraph) {
        return;
    }

    // Connect the input to the nodegraph's output.
    if (auto output =
            refNodeGraph.GetOutputByName(_Attr(mtlxBindInput, names.output))) {
        UsdShadeConnectableAPI::ConnectToSource(
            _AddInput(mtlxBindInput, connectable),
            output);
    }
    else {
        TF_WARN("No output \"%s\" for input \"%s\" on <%s>",
                _Attr(mtlxBindInput, names.output).c_str(),
                _Name(mtlxBindInput).c_str(),
                connectable.GetPath().GetText());
    }
}

UsdShadeInput
_Context::_AddInput(
    const mx::ConstValueElementPtr& mtlxValue,
    const UsdShadeConnectableAPI& connectable)
{
    auto usdInput = _MakeInput(connectable, mtlxValue);
    _SetCoreUIAttributes(usdInput.GetAttr(), mtlxValue);
    return usdInput;
}

UsdShadeInput
_Context::_AddInputWithValue(
    const mx::ConstValueElementPtr& mtlxValue,
    const UsdShadeConnectableAPI& connectable)
{
    if (auto usdInput = _AddInput(mtlxValue, connectable)) {
        _CopyValue(usdInput, mtlxValue);
        return usdInput;
    }
    return UsdShadeInput();
}

UsdShadeOutput
_Context::_AddShaderOutput(
    const mx::ConstTypedElementPtr& mtlxTyped,
    const UsdShadeConnectableAPI& connectable)
{
    auto& type = _Type(mtlxTyped);

    std::string context;
    auto mtlxTypeDef = mtlxTyped->getDocument()->getTypeDef(type);
    if (mtlxTypeDef) {
        if (auto semantic = _Attr(mtlxTypeDef, names.semantic)) {
            if (semantic.str() == mx::SHADER_SEMANTIC) {
                context = _Attr(mtlxTypeDef, names.context);
            }
        }
    }
    if (context == "surface" || type == mx::SURFACE_SHADER_TYPE_STRING) {
        return connectable.CreateOutput(UsdShadeTokens->surface,
                                        SdfValueTypeNames->Token);
    }
    else if (context == "displacement" || type == "displacementshader") {
        return connectable.CreateOutput(UsdShadeTokens->displacement,
                                        SdfValueTypeNames->Token);
    }
    else if (context == "volume" || type == mx::VOLUME_SHADER_TYPE_STRING) {
        return connectable.CreateOutput(UsdShadeTokens->volume,
                                        SdfValueTypeNames->Token);
    }
    else if (context == "light" || type == "lightshader") {
        // USD doesn't support this.
        return connectable.CreateOutput(tokens->light,
                                        SdfValueTypeNames->Token);
    }
    else if (!context.empty()) {
        // We don't know this type so use the MaterialX type name as-is.
        return connectable.CreateOutput(TfToken(type),
                                        SdfValueTypeNames->Token);
    }
    return UsdShadeOutput();
}

const _Context::Variant*
_Context::_GetVariant(
    const VariantSetName& variantSetName,
    const VariantName& variantName) const
{
    auto i = _variantSets.find(variantSetName);
    if (i != _variantSets.end()) {
        auto j = i->second.find(variantName);
        if (j != i->second.end()) {
            return &j->second;
        }
    }
    return nullptr;
}

void
_Context::_CopyVariant(
    const UsdShadeConnectableAPI& connectable,
    const Variant& variant) const
{
    for (auto& nameAndValue: variant) {
        auto& mtlxValue = nameAndValue.second;
        _CopyValue(_MakeInput(connectable, mtlxValue), mtlxValue);
    }
}

/// This class tracks variant selections on materialassigns and any
/// shaderrefs the variant selection is limited to.  Objects are
/// created using the VariantAssignmentsBuilder helper.
class VariantAssignments {
public:
    using VariantName = _Context::VariantName;
    using VariantSetName = _Context::VariantSetName;
    using VariantSetOrder = _Context::VariantSetOrder;
    using VariantSelection = std::pair<VariantSetName, VariantName>;
    using VariantSelectionSet = std::set<VariantSelection>;
    using MaterialAssignPtr = mx::ConstMaterialAssignPtr;
    using MaterialAssigns = std::vector<MaterialAssignPtr>;

    using ShaderName = std::string;
    using VariantShaderSet = std::vector<ShaderName>;
    struct VariantAndShaders {
        VariantAndShaders(VariantName&& originalName,
                          VariantName&& uniqueName,
                          VariantShaderSet&& shaderRefSet)
            : originalName(std::move(originalName))
            , uniqueName(std::move(uniqueName))
            , shaderRefSet(std::move(shaderRefSet)) { }

        VariantName originalName;
        VariantName uniqueName;
        VariantShaderSet shaderRefSet;
    };
    using VariantAndShadersBag = std::vector<VariantAndShaders>;

    /// Returns all material assigns.
    const MaterialAssigns& GetMaterialAssigns() const;

    /// Returns the variant set order for the material assign.
    VariantSetOrder GetVariantSetOrder(const MaterialAssignPtr&) const;

    /// Returns the variants for the given variant set on the given
    /// material assign.  Each variant is accompanied by the shaderrefs
    /// that it applies to.
    const VariantAndShadersBag&
    GetVariants(const MaterialAssignPtr&, const VariantSetName&) const;

    /// Returns the variant selections on the given material assign.
    const VariantSelectionSet&
    GetVariantSelections(const MaterialAssignPtr&) const;

private:
    VariantAssignments() = default;

private:
    VariantSetOrder _globalVariantSetOrder;
    MaterialAssigns _materialAssigns;
    std::map<MaterialAssignPtr,
             std::map<VariantSetName,
                      VariantAndShadersBag>> _materialInfo;
    std::map<MaterialAssignPtr, VariantSelectionSet> _selections;

    friend class VariantAssignmentsBuilder;
};

const VariantAssignments::MaterialAssigns&
VariantAssignments::GetMaterialAssigns() const
{
    return _materialAssigns;
}

VariantAssignments::VariantSetOrder
VariantAssignments::GetVariantSetOrder(
    const MaterialAssignPtr& mtlxMaterialAssign) const
{
    // We could compute and store an order per material assign instead.
    return _globalVariantSetOrder;
}

const VariantAssignments::VariantAndShadersBag&
VariantAssignments::GetVariants(
    const MaterialAssignPtr& mtlxMaterialAssign,
    const VariantSetName& variantSetName) const
{
    auto i = _materialInfo.find(mtlxMaterialAssign);
    if (i != _materialInfo.end()) {
        auto j = i->second.find(variantSetName);
        if (j != i->second.end()) {
            return j->second;
        }
    }
    static const VariantAndShadersBag empty;
    return empty;
}

const VariantAssignments::VariantSelectionSet&
VariantAssignments::GetVariantSelections(
    const MaterialAssignPtr& mtlxMaterialAssign) const
{
    auto i = _selections.find(mtlxMaterialAssign);
    if (i != _selections.end()) {
        return i->second;
    }
    static const VariantSelectionSet empty;
    return empty;
}

/// This class collects variant assignments and their associated shaderrefs.
class ShadersForVariantAssignments {
public:
    using VariantName      = VariantAssignments::VariantName;
    using VariantSetName   = VariantAssignments::VariantSetName;
    using VariantShaderSet = VariantAssignments::VariantShaderSet;

    struct ShadersForVariantAssignment {
        ShadersForVariantAssignment(const VariantSetName& variantSetName,
                                    const VariantName& variantName,
                                    VariantShaderSet&& shaderSet)
            : variantSetName(variantSetName)
            , variantName(variantName)
            , shaderSet(std::move(shaderSet)) { }

        VariantSetName   variantSetName;
        VariantName      variantName;
        VariantShaderSet shaderSet;
    };
    using iterator = std::vector<ShadersForVariantAssignment>::iterator;

    /// Add the variant assignments from \p mtlx to this object.
    void Add(const mx::ConstElementPtr& mtlx);

    /// Add the variant assignments from \p mtlxLook and all inherited
    /// looks to this object.
    void AddInherited(const mx::ConstLookPtr& mtlxLook);

    /// Compose variant assignments in this object over assignments in
    /// \p weaker and store the result in this object.
    void Compose(const ShadersForVariantAssignments& weaker);

    iterator begin() { return _assignments.begin(); }
    iterator end() { return _assignments.end(); }

private:
    using _Assignments = std::vector<ShadersForVariantAssignment>;

    VariantShaderSet _GetShaders(const mx::ConstElementPtr& mtlx);
    _Assignments _Get(const mx::ConstElementPtr& mtlx);
    void _Compose(const _Assignments& weaker);

private:
    _Assignments _assignments;

    // Variant sets that have been handled already.
    std::set<VariantSetName> _seen;
};

void
ShadersForVariantAssignments::Add(const mx::ConstElementPtr& mtlx)
{
    auto&& assignments = _Get(mtlx);
    _assignments.insert(_assignments.end(),
                        std::make_move_iterator(assignments.begin()),
                        std::make_move_iterator(assignments.end()));
}

void
ShadersForVariantAssignments::AddInherited(const mx::ConstLookPtr& mtlxLook)
{
    // Compose the look's variant assignments as weaker.
    _Compose(_Get(mtlxLook));

    // Compose inherited assignments as weaker.
    if (auto inherited = mtlxLook->getInheritsFrom()) {
        if (auto inheritedLook = inherited->asA<mx::Look>()) {
            AddInherited(inheritedLook);
        }
    }
}

void
ShadersForVariantAssignments::Compose(
    const ShadersForVariantAssignments& weaker)
{
    _Compose(weaker._assignments);
}

ShadersForVariantAssignments::VariantShaderSet
ShadersForVariantAssignments::_GetShaders(const mx::ConstElementPtr& mtlx)
{
    VariantShaderSet shaders;
    if (_Value(&shaders, mtlx, names.shaderref)) {
        std::sort(shaders.begin(), shaders.end());
    }
    return shaders;
}

ShadersForVariantAssignments::_Assignments
ShadersForVariantAssignments::_Get(const mx::ConstElementPtr& mtlx)
{
    _Assignments result;

    // Last assignment wins for any given variant set.  If we wanted
    // the first to win then we wouldn't reverse.
    auto mtlxVariantAssigns = _Children(mtlx, names.variantassign);
    std::reverse(mtlxVariantAssigns.begin(), mtlxVariantAssigns.end());

    // Collect the ordered variant selections.
    for (auto& mtlxVariantAssign: mtlxVariantAssigns) {
        _Attr variantset(mtlxVariantAssign, names.variantset);
        _Attr variant(mtlxVariantAssign, names.variant);
        // Ignore assignments to a variant set we've already seen.
        if (_seen.insert(variantset).second) {
            result.emplace_back(variantset, variant,
                                _GetShaders(mtlxVariantAssign));
        }
    }

    // Reverse the result since we reversed the iteration.
    std::reverse(result.begin(), result.end());

    return result;
}

void
ShadersForVariantAssignments::_Compose(const _Assignments& weaker)
{
    // Apply weaker to stronger.  That means we ignore any variantsets
    // already in stronger.
    for (const auto& assignment: weaker) {
        if (_seen.insert(assignment.variantSetName).second) {
            _assignments.emplace_back(assignment);
        }
    }
}

/// Helper class to build \c VariantAssignments.
class VariantAssignmentsBuilder {
public:
    using MaterialAssignPtr = VariantAssignments::MaterialAssignPtr;

    /// Add variant assignments (with associated shaders) on a material
    /// assign to the builder.
    void Add(const MaterialAssignPtr&, ShadersForVariantAssignments&&);

    /// Build and return a VariantAssignments using the added data.
    /// This also resets the builder.
    VariantAssignments Build(const _Context&);

private:
    std::map<MaterialAssignPtr, ShadersForVariantAssignments> _data;
};

void
VariantAssignmentsBuilder::Add(
    const MaterialAssignPtr& mtlxMaterialAssign,
    ShadersForVariantAssignments&& selection)
{
    // We don't expect duplicate keys but we use the last data added.
    _data[mtlxMaterialAssign] = std::move(selection);
}

VariantAssignments
VariantAssignmentsBuilder::Build(const _Context& context)
{
    VariantAssignments result;

    // Just tuck this away.
    result._globalVariantSetOrder = context.GetVariantSetOrder();

    // We could scan for and discard variant assignments that don't
    // affect their material here.

    // We should expand empty shaderref sets into the full set of
    // shaderrefs on that material or replace full sets with the
    // empty string so that they compare as identical, otherwise
    // we'll get different variants with identical opinions for them.
    // XXX

    // Reorganize data into result, finding variants that must be made
    // unique.  This is somewhat complicated.  A material M's variants
    // are those assigned to it over all looks.  Since each variant is
    // in a variantset this also determines the variantsets.  However,
    // a variant can also have a shaderref string array which causes
    // the variant to be applied to a subset of the material's
    // shaderrefs.  In USD to apply a variant to different shaderref
    // sets necessitates using different variants.  That means making
    // up and using a new variant name.
    // 
    // visitedNames maps shaderref sets to unique variant names per
    // (material,variantset,original variant name).  knownNames is
    // used to construct unique variant names, mapping a (material,
    // variantset) to a suffix and known variant names.  The suffix
    // is an integer used to create unique names.
    //
    // While making variants unique we also record in the result all
    // of the material assignments and the variant info and selection
    // for each (materialassign,variantset).
    //
    using VariantName = VariantAssignments::VariantName;
    using VariantSetName = VariantAssignments::VariantSetName;
    using VariantShaderSet = VariantAssignments::VariantShaderSet;

    std::map<std::tuple<std::string, VariantSetName, VariantName>,
             std::map<VariantShaderSet, VariantName>> visitedNames;
    std::map<std::pair<std::string, VariantSetName>,
             std::pair<int, std::set<VariantName>>> knownNames;
    for (auto& i: _data) {
        auto& mtlxMaterialAssign         = i.first;
        auto& variantSelectionAndShaders = i.second;
        auto& materialInfo = result._materialInfo[mtlxMaterialAssign];
        auto& selections   = result._selections[mtlxMaterialAssign];
        auto materialName  = _Attr(mtlxMaterialAssign, names.material).str();

        // Record all material assigns.
        result._materialAssigns.emplace_back(mtlxMaterialAssign);

        // Process all variants.
        for (auto& j: variantSelectionAndShaders) {
            auto& variantSetName    = j.variantSetName;
            auto& variantName       = j.variantName;
            auto& shaderSet         = j.shaderSet;
            auto& visitedShaderSets =
                visitedNames[std::make_tuple(materialName,
                                             variantSetName,
                                             variantName)];

            // Look up this variantset/variant.
            std::string uniqueVariantName;
            if (!visitedShaderSets.empty()) {
                auto k = visitedShaderSets.find(shaderSet);
                if (k != visitedShaderSets.end()) {
                    // We've seen this shader set before.
                    uniqueVariantName = k->second;
                }
                else {
                    // This variant must be made unique.
                    auto& newVariantName = visitedShaderSets[shaderSet];

                    // Get the known names, including ones we created.  If
                    // there are no names yet then populate with the names
                    // from the context.
                    auto& m = 
                        knownNames[std::make_pair(materialName,
                                                  variantSetName)];
                    auto& variantSetKnownNames = m.second;
                    if (variantSetKnownNames.empty()) {
                        variantSetKnownNames =
                            context.GetVariants(variantSetName);
                    }

                    // Choose and save a unique variant name.
                    int& n = m.first;
                    auto base = variantName + "_";
                    do {
                        newVariantName = base + std::to_string(++n);
                    } while (!variantSetKnownNames
                                .emplace(newVariantName).second);

                    uniqueVariantName = newVariantName;
                }
            }
            else {
                // New variantset/variant for the material.
                uniqueVariantName =
                visitedShaderSets[shaderSet] = variantName;
            }

            // Note the variant selection.
            selections.emplace(variantSetName, uniqueVariantName);

            // Add the variant.
            materialInfo[std::move(variantSetName)]
                .emplace_back(std::move(variantName),
                              std::move(uniqueVariantName),
                              std::move(shaderSet));
        }
    }

    // Discard remaining data.
    _data.clear();

    return result;
}

// Convert MaterialX nodegraphs with nodedef attributes to UsdShadeNodeGraphs.
// This is basically a one-to-one translation of nodes to UsdShadeShaders,
// parameters and inputs to UsdShadeInputs, outputs (include default
// outputs) to UsdShadeOutputs, and input connections using the nodename
// attribute to USD connections.
static
void
ReadNodeGraphsWithDefs(mx::ConstDocumentPtr mtlx, _Context& context)
{
    // Translate nodegraphs with nodedefs.
    for (auto& mtlxNodeGraph: mtlx->getNodeGraphs()) {
        context.AddNodeGraphWithDef(mtlxNodeGraph);
    }
}

// Convert MaterialX nodegraphs w/out nodedef attributes to UsdShadeNodeGraphs.
// This is basically a one-to-one translation of nodes to UsdShadeShaders,
// parameters and inputs to UsdShadeInputs, outputs (include default
// outputs) to UsdShadeOutputs, and input connections using the nodename
// attribute to USD connections.
static
void
ReadNodeGraphsWithoutDefs(mx::ConstDocumentPtr mtlx, _Context& context)
{
    // Translate nodegraphs with nodedefs.
    for (auto& mtlxNodeGraph: mtlx->getNodeGraphs()) {
        if (!mtlxNodeGraph->getNodeDef()) {
            context.AddNodeGraph(mtlxNodeGraph);
        }
    }
}

// Convert MaterialX materials to USD materials.  Each USD material has
// child shader prims for each shaderref in the MaterialX material.  In
// addition, all of the child shader inputs and outputs are connected to
// a synthesized material interface that's the union of all of those
// inputs and outputs.  The child shader prims reference shader prims
// that encapsulate the nodedef for the shader.  This necessary to
// ensure that variants opinions are stronger than the nodedef opinions
// but it also makes for a clean separation and allows sharing nodedefs
// across materials.  Material inherits are added at the end via
// specializes arcs.
static
void
ReadMaterials(mx::ConstDocumentPtr mtlx, _Context& context)
{
    for (auto& mtlxMaterial: mtlx->getMaterials()) {
        // Translate material.
        if (auto usdMaterial = context.BeginMaterial(mtlxMaterial)) {
            // Translate all shader references.
            for (auto mtlxShaderRef: mtlxMaterial->getShaderRefs()) {
                // Translate shader reference.
                if (auto usdShader = context.AddShaderRef(mtlxShaderRef)) {
                    // Do nothing.
                }
                else {
                    if (auto nodedef = _Attr(mtlxShaderRef, names.nodedef)) {
                        TF_WARN("Failed to create shaderref '%s' "
                                "to nodedef '%s'",
                                _Name(mtlxShaderRef).c_str(),
                                nodedef.c_str());
                    }
                    else if (auto node = _Attr(mtlxShaderRef, names.node)) {
                        TF_WARN("Failed to create shaderref '%s' "
                                "to node '%s'",
                                _Name(mtlxShaderRef).c_str(),
                                node.c_str());
                    }
                    else {
                        // Ignore -- no node was specified.
                    }
                }
            }
            context.EndMaterial();
        }
        else {
            TF_WARN("Failed to create material '%s'",
                    _Name(mtlxMaterial).c_str());
        }
    }

    // Add material inherits.  We wait until now so we can be sure all
    // the materials exist.
    for (auto& mtlxMaterial: mtlx->getMaterials()) {
        if (auto usdMaterial = context.GetMaterial(_Name(mtlxMaterial))) {
            if (auto name = _Attr(mtlxMaterial, names.inherit)) {
                if (auto usdInherited = context.GetMaterial(name)) {
                    usdMaterial.GetPrim().GetSpecializes()
                        .AddSpecialize(usdInherited.GetPath());
                }
                else {
                    TF_WARN("Material '%s' attempted to inherit from "
                            "unknown material '%s'",
                            _Name(mtlxMaterial).c_str(), name.c_str());
                }
            }
        }
    }
}

// Convert MaterialX collections and geom attributes on material assigns
// to USD collections.  All collections go onto a single prim in USD.
// All paths are absolutized and MaterialX paths that require geomexpr
// are discarded with a warning (since USD only supports simple absolute
// paths in collections).  geom attributes are converted to collections
// because USD material binding requires a UsdCollectionAPI.  geomprefix
// is baked into the paths during this conversion.  Equal collections
// are shared;  we note the source MaterialX element and the resulting
// USD collection here so we can bind it later.
static
bool
ReadCollections(mx::ConstDocumentPtr mtlx, _Context& context)
{
    bool hasAny = false;

    // Translate all collections.
    for (auto& mtlxCollection: mtlx->getCollections()) {
        context.AddCollection(mtlxCollection);
        hasAny = true;
    }

    // Make a note of the geometry on each material assignment.
    for (auto& mtlxLook: mtlx->getLooks()) {
        for (auto& mtlxMaterialAssign: mtlxLook->getMaterialAssigns()) {
            context.AddGeometryReference(mtlxMaterialAssign);
        }
    }

    return hasAny;
}

// Creates the variants bound to a MaterialX materialassign on the USD
// Material and/or its shader children.  The variant opinions go on the
// Material unless MaterialX variantassign uses the shaderref attribute
// to apply to only certain shaders.
static
void
AddMaterialVariants(
    const mx::ConstMaterialAssignPtr& mtlxMaterialAssign,
    const _Context& context,
    const VariantAssignments& assignments)
{
    std::string materialName = _Attr(mtlxMaterialAssign, names.material);

    // Process variant sets in the appropriate order.
    for (const auto& variantSetName:
            assignments.GetVariantSetOrder(mtlxMaterialAssign)) {
        // Loop over all variants in the variant set on the material.
        for (const auto& variantAndShaders:
                assignments.GetVariants(mtlxMaterialAssign,
                                        variantSetName)) {
            // Add the variant to all shaderrefs in shaders or, if shaders
            // is empty, to the material.
            context.AddMaterialVariant(materialName, variantSetName,
                                       variantAndShaders.originalName,
                                       variantAndShaders.uniqueName,
                                       variantAndShaders.shaderRefSet.empty()
                                           ? nullptr
                                           : &variantAndShaders.shaderRefSet);
        }
    }
}

// Converts a MaterialX look to a USD prim.  This prim references the
// collections so it can use them in any material binding.  It has a
// UsdShadeMaterialBindingAPI and a Material child prim under a
// "Materials" scope for each materialassign.  The Material prims
// will use variant selections for each MaterialX variantassign and
// will reference the materials created by ReadMaterials().
//
// If the look has a inherit then the USD will reference the corresponding
// USD prim.
static
void
ReadLook(
    const mx::ConstLookPtr& mtlxLook,
    const UsdPrim& root,
    _Context& context,
    const VariantAssignments& assignments,
    bool hasCollections)
{
    static const TfToken materials("Materials");

    _SetCoreUIAttributes(root, mtlxLook);

    // Add a reference for the inherit, if any.
    if (auto inherit = _Attr(mtlxLook, names.inherit)) {
        auto path =
            root.GetPath().GetParentPath().AppendChild(_MakeName(inherit));
        root.GetReferences().AddInternalReference(path);
    }

    // Add a reference to the collections in each look so they can use
    // them in bindings.  Inheriting looks will get the collections
    // directly and via the inherited look.  USD will collapse these
    // into a single reference.
    if (hasCollections) {
        root.GetReferences().AddInternalReference(
            context.GetCollectionsPath());
    }

    // Make a prim for all of the materials.
    auto lookMaterialsPrim =
        root.GetStage()->DefinePrim(root.GetPath().AppendChild(materials));

    // Collect all of the material assign names and whether the name
    // has been used yet.
    std::map<TfToken, int> materialNames;
    for (auto& mtlxMaterialAssign: mtlxLook->getMaterialAssigns()) {
        materialNames[_MakeName(mtlxMaterialAssign)] = 0;
    }
    for (auto&& child: lookMaterialsPrim.GetAllChildren()) {
        // Inherited.
        materialNames[child.GetName()] = 1;
    }

    // Make an object for binding materials.
    auto binding = UsdShadeMaterialBindingAPI(root);

    // Get the current (inherited) property order.
    const auto inheritedOrder = root.GetPropertyOrder();

    // Add each material assign and record the order of material bindings.
    TfTokenVector order;
    for (auto& mtlxMaterialAssign: mtlxLook->getMaterialAssigns()) {
        // Get the USD material.
        auto usdMaterial =
            context.GetMaterial(_Attr(mtlxMaterialAssign, names.material));
        if (!usdMaterial) {
            // Unknown material.
            continue;
        }

        // Make a unique material name.  If possible use the name of
        // the materialassign.
        auto materialName = _MakeName(mtlxMaterialAssign);
        int& n = materialNames[materialName];
        if (n) {
            // Make a unique name.
            auto stage   = lookMaterialsPrim.GetStage();
            auto base    = lookMaterialsPrim.GetPath();
            auto prefix  = materialName.GetString() + "_";
            do {
                materialName = TfToken(prefix + std::to_string(n++));
            } while (stage->GetPrimAtPath(base.AppendChild(materialName)));
        }
        else {
            // We've used the name now.
            n = 1;
        }

        // Make a material prim.  This has the MaterialX name of the
        // material assign since we can assign the same material
        // multiple times with different variants to different
        // collections (so we can't use the material name itself).
        auto lookMaterialPrim =
            lookMaterialsPrim.GetStage()->DefinePrim(
                lookMaterialsPrim.GetPath().AppendChild(materialName));
        _SetGlobalCoreUIAttributes(lookMaterialPrim, mtlxMaterialAssign);

        // Reference the original material.
        lookMaterialPrim.GetReferences()
            .AddInternalReference(usdMaterial.GetPath());

        // Set the variant selections.
        for (const auto& i:
                assignments.GetVariantSelections(mtlxMaterialAssign)) {
            lookMaterialPrim.GetVariantSet(i.first)
                .SetVariantSelection(i.second);
        }

        // Find the collection.
        if (auto collection =
                context.GetCollection(mtlxMaterialAssign, root)) {
            // Bind material to a collection.
            if (binding.Bind(collection, UsdShadeMaterial(lookMaterialPrim),
                             materialName)) {
                // Record the binding.
                order.push_back(
                    binding.GetCollectionBindingRel(materialName).GetName());
            }
        }
        else {
            // Bind material to the prim.
            if (binding.Bind(UsdShadeMaterial(lookMaterialPrim))) {
                // Record the binding.
                order.push_back(binding.GetDirectBindingRel().GetName());
            }
        }
    }

    // Ensure our local material bindings are strongest and in the
    // right order.
    if (!order.empty()) {
        order.insert(order.end(),
                     inheritedOrder.begin(), inheritedOrder.end());
        root.SetPropertyOrder(order);
    }
}

} // anonymous namespace

void
UsdMtlxRead(
    const MaterialX::ConstDocumentPtr& mtlx,
    const UsdStagePtr& stage,
    const SdfPath& internalPath,
    const SdfPath& externalPath)
{
    if (!mtlx) {
        TF_CODING_ERROR("Invalid MaterialX document");
        return;
    }
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return;
    }
    if (!internalPath.IsPrimPath()) {
        TF_CODING_ERROR("Invalid internal prim path");
        return;
    }
    if (!externalPath.IsPrimPath()) {
        TF_CODING_ERROR("Invalid external prim path");
        return;
    }

    _Context context(stage, internalPath);

    // Color management.
    if (auto cms = _Attr(mtlx, names.cms)) {
        stage->SetColorManagementSystem(TfToken(cms));
    }
    if (auto cmsconfig = _Attr(mtlx, names.cmsconfig)) {
        // XXX -- Is it okay to use the URI as is?
        stage->SetColorConfiguration(SdfAssetPath(cmsconfig));
    }
    auto&& colorspace = mtlx->getActiveColorSpace();
    if (!colorspace.empty()) {
        VtDictionary dict;
        dict[SdfFieldKeys->ColorSpace.GetString()] = VtValue(colorspace);
        stage->SetMetadata(SdfFieldKeys->CustomLayerData, dict);
    }

    // Translate all materials.
    ReadMaterials(mtlx, context);

    // If there are no looks then we're done.
    if (mtlx->getLooks().empty()) {
        return;
    }

    // Collect the MaterialX variants.
    context.AddVariants(mtlx);

    // Translate all collections.
    auto hasCollections = ReadCollections(mtlx, context);

    // Collect all of the material/variant assignments.
    VariantAssignmentsBuilder materialVariantAssignmentsBuilder;
    for (auto& mtlxLook: mtlx->getLooks()) {
        // Get the variant assigns for the look and (recursively) its
        // inherited looks.
        ShadersForVariantAssignments lookVariantAssigns;
        lookVariantAssigns.AddInherited(mtlxLook);

        for (auto& mtlxMaterialAssign: mtlxLook->getMaterialAssigns()) {
            // Get the material assign's variant assigns.
            ShadersForVariantAssignments variantAssigns;
            variantAssigns.Add(mtlxMaterialAssign);

            // Compose variantAssigns over lookVariantAssigns.
            variantAssigns.Compose(lookVariantAssigns);

            // Note all of the assigned variants.
            materialVariantAssignmentsBuilder
                .Add(mtlxMaterialAssign, std::move(variantAssigns));
        }
    }

    // Build the variant assignments object.
    auto assignments =
        materialVariantAssignmentsBuilder.Build(context);

    // Create the variants on each material.
    for (const auto& mtlxMaterialAssign: assignments.GetMaterialAssigns()) {
        AddMaterialVariants(mtlxMaterialAssign, context, assignments);
    }

    // Make an internal path for looks.
    auto looksPath = internalPath.AppendChild(TfToken("Looks"));

    // Create the external root prim.
    auto root = stage->DefinePrim(externalPath);

    // Create each look as a variant.
    auto lookVariantSet =
        root.GetVariantSets().AddVariantSet("LookVariant");
    for (auto& mtlxMostDerivedLook: mtlx->getLooks()) {
        // We rely on inherited looks to exist in USD so we do
        // those first.
        for (auto& mtlxLook: _GetInheritanceStack(mtlxMostDerivedLook)) {
            auto lookName = _Name(mtlxLook);

            // Add the look prim.  If it already exists (because it was
            // inherited by a previously handled look) then skip it.
            auto usdLook =
                stage->DefinePrim(looksPath.AppendChild(TfToken(lookName)));
            if (usdLook.HasAuthoredReferences()) {
                continue;
            }

            // Read the look.
            ReadLook(mtlxLook, usdLook, context, assignments, hasCollections);

            // Create a variant for this look in the external root.
            if (lookVariantSet.AddVariant(lookName)) {
                lookVariantSet.SetVariantSelection(lookName);
                UsdEditContext ctx(lookVariantSet.GetVariantEditContext());
                root.GetReferences().AddInternalReference(usdLook.GetPath());
            }
            else {
                TF_CODING_ERROR("Failed to author look variant '%s' "
                                "in variant set '%s' on <%s>",
                                lookName.c_str(),
                                lookVariantSet.GetName().c_str(),
                                root.GetPath().GetText());
            }
        }
    }
    lookVariantSet.ClearVariantSelection();
}

void
UsdMtlxReadNodeGraphs(
    const MaterialX::ConstDocumentPtr& mtlx,
    const UsdStagePtr& stage,
    const SdfPath& internalPath)
{
    if (!mtlx) {
        TF_CODING_ERROR("Invalid MaterialX document");
        return;
    }
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return;
    }
    if (!internalPath.IsPrimPath()) {
        TF_CODING_ERROR("Invalid internal prim path");
        return;
    }

    _Context context(stage, internalPath);

    ReadNodeGraphsWithDefs(mtlx, context);
    ReadNodeGraphsWithoutDefs(mtlx, context);
}

PXR_NAMESPACE_CLOSE_SCOPE
