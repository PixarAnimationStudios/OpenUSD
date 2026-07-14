//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "usd/UsdPrimRegistry.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/pxr.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/class.hpp"
  #include "pxr/external/boost/python/noncopyable.hpp"
  #include "pxr/external/boost/python/suite/indexing/map_indexing_suite.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;

namespace {

// -- PrimTypeInfo helpers --

std::string _PrimTypeInfo_getTypeName(const UsdPrimRegistry::PrimTypeInfo& i) {
  return i.typeName;
}
std::string _PrimTypeInfo_getFamily(const UsdPrimRegistry::PrimTypeInfo& i) {
  return i.family;
}

// -- PrimDescriptor helpers --

std::string _PrimDescriptor_getPrimPath(const UsdPrimRegistry::PrimDescriptor& d) {
  return d.primPath;
}
list _PrimDescriptor_getInputPins(const UsdPrimRegistry::PrimDescriptor& d) {
  list result;
  for (const auto& s : d.inputPins) {
    result.append(s);
  }
  return result;
}
list _PrimDescriptor_getOutputPins(const UsdPrimRegistry::PrimDescriptor& d) {
  list result;
  for (const auto& s : d.outputPins) {
    result.append(s);
  }
  return result;
}
dict _PrimDescriptor_getInputPinTypes(const UsdPrimRegistry::PrimDescriptor& d) {
  dict result;
  for (const auto& [k, v] : d.inputPinTypes) {
    result[k] = v;
  }
  return result;
}
dict _PrimDescriptor_getOutputPinTypes(const UsdPrimRegistry::PrimDescriptor& d) {
  dict result;
  for (const auto& [k, v] : d.outputPinTypes) {
    result[k] = v;
  }
  return result;
}

// -- LinkInfo helpers --

std::string _LinkInfo_getSourceNodeId(const UsdPrimRegistry::LinkInfo& l) {
  return l.sourceNodeId;
}
std::string _LinkInfo_getSourcePort(const UsdPrimRegistry::LinkInfo& l) {
  return l.sourcePort;
}
std::string _LinkInfo_getTargetNodeId(const UsdPrimRegistry::LinkInfo& l) {
  return l.targetNodeId;
}
std::string _LinkInfo_getTargetPort(const UsdPrimRegistry::LinkInfo& l) {
  return l.targetPort;
}
bool _LinkInfo_getIsInputLink(const UsdPrimRegistry::LinkInfo& l) {
  return l.isInputLink;
}

// -- UsdPrimRegistry method wrappers returning Python lists --

list _GetFamilies(const UsdPrimRegistry& reg) {
  list result;
  for (const auto& s : reg.GetFamilies()) {
    result.append(s);
  }
  return result;
}

list _GetPrimTypes(const UsdPrimRegistry& reg, const std::string& family) {
  list result;
  for (const auto& t : reg.GetPrimTypes(family)) {
    result.append(t);
  }
  return result;
}

list _GetLinksForPrim(const UsdPrimRegistry& reg, const UsdPrim& prim) {
  list result;
  for (const auto& l : reg.GetLinksForPrim(prim)) {
    result.append(l);
  }
  return result;
}

} // namespace

void wrapUsdPrimRegistry() {
  class_<UsdPrimRegistry::PrimTypeInfo>("PrimTypeInfo", no_init)
      .add_property("typeName", &_PrimTypeInfo_getTypeName)
      .add_property("family", &_PrimTypeInfo_getFamily);

  class_<UsdPrimRegistry::PrimDescriptor>("PrimDescriptor", no_init)
      .add_property("primPath", &_PrimDescriptor_getPrimPath)
      .add_property("inputPins", &_PrimDescriptor_getInputPins)
      .add_property("outputPins", &_PrimDescriptor_getOutputPins)
      .add_property("inputPinTypes", &_PrimDescriptor_getInputPinTypes)
      .add_property("outputPinTypes", &_PrimDescriptor_getOutputPinTypes);

  class_<UsdPrimRegistry::LinkInfo>("LinkInfo", no_init)
      .add_property("sourceNodeId", &_LinkInfo_getSourceNodeId)
      .add_property("sourcePort", &_LinkInfo_getSourcePort)
      .add_property("targetNodeId", &_LinkInfo_getTargetNodeId)
      .add_property("targetPort", &_LinkInfo_getTargetPort)
      .add_property("isInputLink", &_LinkInfo_getIsInputLink);

  class_<UsdPrimRegistry, noncopyable>("UsdPrimRegistry")
      .def("GetFamilies", &_GetFamilies)
      .def("GetPrimTypes", &_GetPrimTypes)
      .def("GetDescriptorFromPrim", &UsdPrimRegistry::GetDescriptorFromPrim)
      .def("CanHandlePrim", &UsdPrimRegistry::CanHandlePrim)
      .def("GetLinksForPrim", &_GetLinksForPrim);
}
