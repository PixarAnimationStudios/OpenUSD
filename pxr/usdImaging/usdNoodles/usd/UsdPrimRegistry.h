//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#ifndef NOODLES_USD_PRIM_REGISTRY_H
#define NOODLES_USD_PRIM_REGISTRY_H

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primDefinition.h>
#include <pxr/usd/usd/schemaRegistry.h>

#include "core/api.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

/// Registry of concrete USD prim types discovered from UsdSchemaRegistry.
///
/// Provides schema discovery, attribute introspection, and connection reading
/// for generic USD typed prims.  Thread-safe; discovery is performed once on
/// first access and cached.
class NOODLES_API UsdPrimRegistry {
 public:
  struct PrimTypeInfo {
    std::string typeName; // e.g. "Mesh", "DistantLight"
    std::string family; // e.g. "usdGeom", "usdLux" (plugin name)
  };

  struct PrimDescriptor {
    std::string primPath;
    std::vector<std::string> inputPins;
    std::vector<std::string> outputPins;
    std::unordered_map<std::string, std::string> inputPinTypes;
    std::unordered_map<std::string, std::string> outputPinTypes;
  };

  struct LinkInfo {
    std::string sourceNodeId;
    std::string sourcePort;
    std::string targetNodeId;
    std::string targetPort;
    bool isInputLink{};
  };

  /// Return sorted list of family names (plugin names like "usdGeom").
  std::vector<std::string> GetFamilies() const;

  /// Return all concrete prim types belonging to \p family.
  std::vector<PrimTypeInfo> GetPrimTypes(const std::string& family) const;

  /// Build a descriptor from an existing prim's connectable attributes.
  PrimDescriptor GetDescriptorFromPrim(const UsdPrim& prim) const;

  /// Return true if \p prim has a concrete typed schema known to the registry.
  bool CanHandlePrim(const UsdPrim& prim) const;

  /// Read authored connections on \p prim and return normalised link data.
  std::vector<LinkInfo> GetLinksForPrim(const UsdPrim& prim) const;

  /// Return deduplicated attribute names ordered schema-first, authored-second.
  ///
  /// Schema-declared names are obtained from prim.GetPrimDefinition(), which
  /// includes properties from all apiSchemas (built-in and runtime-applied).
  /// Relationships are excluded — only attributes become pins.
  /// Authored attribute names not already covered by the schema are appended.
  static TfTokenVector GetSchemaAwarePropertyNames(const UsdPrim& prim);

  /// Return the attribute's SdfValueTypeName as a string.
  ///
  /// Prefers the runtime attribute when valid; otherwise falls back to the
  /// schema's attribute spec for the prim's type. Empty string if neither
  /// yields a type.
  static std::string GetAttributeTypeNameString(const UsdPrim& prim, const TfToken& attrName);

 private:
  mutable std::vector<PrimTypeInfo> _allTypes;
  mutable std::unordered_set<std::string> _typeNameSet;
  mutable std::unordered_map<std::string, std::vector<size_t>> _familyIndex;
  mutable std::once_flag _discoveryFlag;

  void _DiscoverTypes() const;
};

#endif // NOODLES_USD_PRIM_REGISTRY_H
