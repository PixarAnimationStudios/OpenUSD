//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "usd/UsdPrimRegistry.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/primDefinition.h>
#include <pxr/usd/usd/schemaBase.h>
#include <pxr/usd/usd/schemaRegistry.h>

#include <algorithm>
#include <set>
#include <string_view>
#include <tuple>

namespace {
constexpr std::string_view kInputsPrefix = "inputs:";
constexpr std::string_view kOutputsPrefix = "outputs:";
} // namespace

// ---------------------------------------------------------------------------
// Schema discovery
// ---------------------------------------------------------------------------

void UsdPrimRegistry::_DiscoverTypes() const {
  std::call_once(_discoveryFlag, [this]() {
    const auto& schemaReg = UsdSchemaRegistry::GetInstance();
    std::set<TfType> allDerived;
    TfType::FindByName("UsdSchemaBase").GetAllDerivedTypes(&allDerived);

    auto& plugReg = PlugRegistry::GetInstance();

    // Discover every concrete schema type, grouped by its plugin family.
    // The generic UsdPrimLibrary renders all prim types now, so the create
    // menu offers every concrete type (abstract + API schemas are still
    // excluded by the IsConcrete check below) rather than a fixed allowlist.

    for (const auto& type : allDerived) {
      if (!schemaReg.IsConcrete(type)) {
        continue; // Excludes abstract + API schemas
      }

      std::string typeName = schemaReg.GetSchemaTypeName(type);
      if (typeName.empty()) {
        continue;
      }

      // Get family from plugin name (reliable across USD versions)
      std::string family = "Other";
      if (auto plugin = plugReg.GetPluginForType(type)) {
        family = plugin->GetName();
      }

      size_t idx = _allTypes.size();
      _allTypes.push_back({typeName, family});
      _familyIndex[family].push_back(idx);
      _typeNameSet.insert(typeName);
    }

    // Sort indices within each family alphabetically by typeName
    for (auto& [fam, indices] : _familyIndex) {
      std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        return _allTypes[a].typeName < _allTypes[b].typeName;
      });
    }
  });
}

std::vector<std::string> UsdPrimRegistry::GetFamilies() const {
  _DiscoverTypes();

  std::vector<std::string> families;
  families.reserve(_familyIndex.size());
  for (const auto& [fam, _] : _familyIndex) {
    families.push_back(fam);
  }
  std::sort(families.begin(), families.end());
  return families;
}

std::vector<UsdPrimRegistry::PrimTypeInfo> UsdPrimRegistry::GetPrimTypes(
    const std::string& family) const {
  _DiscoverTypes();

  auto it = _familyIndex.find(family);
  if (it == _familyIndex.end()) {
    return {};
  }

  std::vector<PrimTypeInfo> result;
  result.reserve(it->second.size());
  for (size_t idx : it->second) {
    result.push_back(_allTypes[idx]);
  }
  return result;
}

// ---------------------------------------------------------------------------
// Attribute introspection
// ---------------------------------------------------------------------------

UsdPrimRegistry::PrimDescriptor UsdPrimRegistry::GetDescriptorFromPrim(const UsdPrim& prim) const {
  PrimDescriptor desc;
  desc.primPath = prim.GetPath().GetString();

  std::unordered_set<std::string> inputSeen, outputSeen;
  for (const TfToken& nameTok : GetSchemaAwarePropertyNames(prim)) {
    const std::string& name = nameTok.GetString();
    const std::string typeName = GetAttributeTypeNameString(prim, nameTok);

    if (name.rfind(kInputsPrefix, 0) == 0) {
      std::string pinName = name.substr(kInputsPrefix.size());
      if (inputSeen.insert(pinName).second) {
        desc.inputPins.push_back(pinName);
      }
      if (!typeName.empty()) {
        desc.inputPinTypes[pinName] = typeName;
      }
    } else if (name.rfind(kOutputsPrefix, 0) == 0) {
      std::string pinName = name.substr(kOutputsPrefix.size());
      if (outputSeen.insert(pinName).second) {
        desc.outputPins.push_back(pinName);
      }
      if (!typeName.empty()) {
        desc.outputPinTypes[pinName] = typeName;
      }
    } else if (name.find(':') == std::string::npos) {
      if (inputSeen.insert(name).second) {
        desc.inputPins.push_back(name);
      }
      if (outputSeen.insert(name).second) {
        desc.outputPins.push_back(name);
      }
      if (!typeName.empty()) {
        desc.inputPinTypes[name] = typeName;
        desc.outputPinTypes[name] = typeName;
      }
    }
  }
  return desc;
}

// ---------------------------------------------------------------------------
// Schema-aware property helpers
// ---------------------------------------------------------------------------

TfTokenVector UsdPrimRegistry::GetSchemaAwarePropertyNames(const UsdPrim& prim) {
  // TfToken dedup: its HashFunctor hashes the interned pointer, avoiding a
  // std::string allocation per token compared to std::unordered_set<string>.
  std::unordered_set<TfToken, TfToken::HashFunctor> seen;
  TfTokenVector ordered;

  // UsdPrim::GetPrimDefinition() returns the per-prim composed definition,
  // which includes concrete type attrs, built-in API schemas, and any
  // runtime-applied API schemas on this specific prim.
  const UsdPrimDefinition& primDef = prim.GetPrimDefinition();
  for (const TfToken& name : primDef.GetPropertyNames()) {
    // Skip relationships — only attributes become pins.
    if (!primDef.GetSchemaAttributeSpec(name)) {
      continue;
    }
    if (seen.insert(name).second) {
      ordered.push_back(name);
    }
  }

  for (const UsdAttribute& attr : prim.GetAuthoredAttributes()) {
    const TfToken& name = attr.GetName();
    if (seen.insert(name).second) {
      ordered.push_back(name);
    }
  }
  return ordered;
}

std::string UsdPrimRegistry::GetAttributeTypeNameString(
    const UsdPrim& prim,
    const TfToken& attrName) {
  if (UsdAttribute attr = prim.GetAttribute(attrName); attr && attr.IsValid()) {
    if (SdfValueTypeName t = attr.GetTypeName()) {
      return t.GetAsToken().GetString();
    }
  }

  const UsdPrimDefinition& primDef = prim.GetPrimDefinition();
  if (SdfAttributeSpecHandle spec = primDef.GetSchemaAttributeSpec(attrName); spec) {
    return spec->GetTypeName().GetAsToken().GetString();
  }
  return {};
}

// ---------------------------------------------------------------------------
// Prim type check
// ---------------------------------------------------------------------------

bool UsdPrimRegistry::CanHandlePrim(const UsdPrim& prim) const {
  _DiscoverTypes();

  std::string typeName = prim.GetTypeName();
  if (typeName.empty()) {
    return false;
  }

  return _typeNameSet.count(typeName) > 0;
}

// ---------------------------------------------------------------------------
// Connection reading
// ---------------------------------------------------------------------------

std::vector<UsdPrimRegistry::LinkInfo> UsdPrimRegistry::GetLinksForPrim(const UsdPrim& prim) const {
  std::vector<LinkInfo> links;
  // Bare attributes can produce duplicates when both endpoints are bare
  // and the connection is discovered from both sides. Dedup inline so we
  // never carry duplicates in `links` to begin with.
  std::set<std::tuple<std::string, std::string, std::string, std::string>> seen;

  for (const auto& attr : prim.GetAttributes()) {
    std::string attrName = attr.GetName();

    bool isInput = false;
    std::string pinName;

    bool isBare = false;
    if (attrName.rfind(kInputsPrefix, 0) == 0) {
      isInput = true;
      pinName = attrName.substr(kInputsPrefix.size());
    } else if (attrName.rfind(kOutputsPrefix, 0) == 0) {
      isInput = false;
      pinName = attrName.substr(kOutputsPrefix.size());
    } else if (attrName.find(':') == std::string::npos) {
      // Bare attribute - dual pin, direction inferred per-connection
      isBare = true;
      isInput = true; // placeholder
      pinName = attrName;
    } else {
      continue;
    }

    SdfPathVector connections;
    attr.GetConnections(&connections);

    for (const auto& connPath : connections) {
      // Use SdfPath API to split into prim path and property name
      if (!connPath.IsPrimPropertyPath()) {
        continue;
      }

      std::string targetPrimPath = connPath.GetPrimPath().GetString();
      std::string targetPropName = connPath.GetNameToken().GetString();

      // Extract target pin name
      std::string targetPinName;
      if (targetPropName.rfind(kInputsPrefix, 0) == 0) {
        targetPinName = targetPropName.substr(kInputsPrefix.size());
      } else if (targetPropName.rfind(kOutputsPrefix, 0) == 0) {
        targetPinName = targetPropName.substr(kOutputsPrefix.size());
      } else {
        targetPinName = targetPropName;
      }

      // For bare attributes, infer direction from target property prefix.
      // Target inputs:* -> bare is the output (source) side.
      // Target outputs:* or bare -> bare is the input (target) side
      // (UsdShade convention).
      if (isBare) {
        isInput = targetPropName.rfind(kInputsPrefix, 0) != 0;
      }

      LinkInfo link;
      if (isInput) {
        // Connection on input side: this node is target, connection points to
        // source
        link.sourceNodeId = targetPrimPath;
        link.sourcePort = targetPinName;
        link.targetNodeId = prim.GetPath().GetString();
        link.targetPort = pinName;
        link.isInputLink = true;
      } else {
        // Connection on output side: this node is source, connection points to
        // target
        link.sourceNodeId = prim.GetPath().GetString();
        link.sourcePort = pinName;
        link.targetNodeId = targetPrimPath;
        link.targetPort = targetPinName;
        link.isInputLink = false;
      }

      auto key =
          std::make_tuple(link.sourceNodeId, link.sourcePort, link.targetNodeId, link.targetPort);
      if (seen.insert(key).second) {
        links.push_back(link);
      }
    }
  }

  return links;
}
