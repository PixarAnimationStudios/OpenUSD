#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/wrapPathJs.h"

#include <iostream>
#include <typeinfo>

#include <emscripten/bind.h>
using namespace emscripten;

PXR_NAMESPACE_USING_DIRECTIVE

EMSCRIPTEN_REGISTER_SMART_PTR(SdfLayer)
EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfPrimSpec)
EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfPropertySpec)
EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfAttributeSpec)
EMSCRIPTEN_REGISTER_SDF_HANDLE(SdfRelationshipSpec)
EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(SdfLayer)

std::string ExportToString(pxr::SdfLayer& self)
{
    std::string output;
    self.ExportToString(&output);
    return output;
}

void traverse(pxr::SdfLayer& self, const pxr::SdfPath& path, val jsFunc) {
  pxr::SdfLayer::TraversalFunction func = [=] (const pxr::SdfPath& path){
    jsFunc(path);
  };

  self.Traverse(path, func);
}

bool SdfFileFormatArgumentsFromJS(
    const emscripten::val& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg)
{
    SdfLayer::FileFormatArguments argsMap;
    typedef SdfLayer::FileFormatArguments::key_type ArgKeyType;
    typedef SdfLayer::FileFormatArguments::mapped_type ArgValueType;

    emscripten::val keys = emscripten::val::global("Object").call<emscripten::val>("keys", dict);
    int length = keys["length"].as<int>();
    for (int i = 0; i < length; ++i) {
      try {
        std::string key = keys[i].as<std::string>();
        std::string value = dict[key].as<std::string>();

        argsMap[key] = value;
      } catch(std::exception e) {
        *errMsg = "All file format argument keys must be strings";
        return false;
      }
    }

    args->swap(argsMap);
    return true;
}

static bool
_ExtractFileFormatArguments(
    const emscripten::val& dict,
    SdfLayer::FileFormatArguments* args)
{
    std::string errMsg;
    if (!SdfFileFormatArgumentsFromJS(dict, args, &errMsg)) {
        TF_CODING_ERROR("%s", errMsg.c_str());
        return false;
    }
    return true;
}

// Should this return a ref pointer instead?
// The layer handle is a weak pointer, so a layer could
// get destroyed while a reference is still kept to it
// Is this intended? Note, the wrapLayer bindings for Python
// also return a layer handle
static SdfLayerHandle
_Find(
    const std::string& identifier,
    const emscripten::val& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerHandle();
    }

    return SdfLayer::Find(identifier, args);
}

static SdfLayerHandle
_Find(
    const std::string& identifier)
{
  return _Find(identifier, emscripten::val::object());
}

static SdfLayerRefPtr
_CreateAnonymous(
    const std::string& tag,
    const emscripten::val& dict)
{
    SdfLayer::FileFormatArguments args;
    if (!_ExtractFileFormatArguments(dict, &args)) {
        return SdfLayerRefPtr();
    }

    return SdfLayer::CreateAnonymous(tag, args);
}
static SdfLayerRefPtr
_CreateAnonymous(
    const std::string& tag)
{
  return _CreateAnonymous(tag, emscripten::val::object());
}

typedef std::function<pxr::VtValue (const emscripten::val& jsVal)> SdfToVtValueFunc;
SdfToVtValueFunc* UsdJsToSdfType(pxr::SdfValueTypeName const &targetType);

static void
_SetTimeSample(const SdfLayerHandle& layer, const SdfPath& path,
               double time, const val& value)
{
    auto property = layer->GetPropertyAtPath(path);

    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(property->GetTypeName());

    bool result = false;
    if (sdfToValue != NULL) {
        pxr::VtValue vtValue = (*sdfToValue)(value);
        layer->SetTimeSample(path, time, vtValue);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for " << property->GetTypeName() << std::endl;
    }
}

static val _GetPropertyAtPath(const SdfLayerHandle& layer, const SdfPath& path) {
    // The Spec class is not polymorphic. We therefore have to manually cast here and
    // return the correct type
    auto attribute = layer->GetAttributeAtPath(path);
    if (attribute) {
        //result = attribute;
        return val(attribute);
    }
    auto relationship = layer->GetRelationshipAtPath(path);
    if (relationship) {
        return val(relationship);
    }
    auto property = layer->GetPropertyAtPath(path);
    if (property) {
        return val(property);
    }

    return val::undefined();
}

EMSCRIPTEN_BINDINGS(SdfLayer) {
  class_<pxr::SdfLayer>("SdfLayer")
    .function("ExportToString", &ExportToString)
    .function("GetDisplayName", &pxr::SdfLayer::GetDisplayName)
    .function("GetPrimAtPath", &pxr::SdfLayer::GetPrimAtPath)
    .function("GetPropertyAtPath", /*&pxr::SdfLayer::GetPropertyAtPath*/ &_GetPropertyAtPath, allow_raw_pointer<ret_val>())
    .function("SetTimeSample", &_SetTimeSample)
    .function("Traverse", &traverse)
    .class_function("Find", emscripten::select_overload<SdfLayerHandle(const std::string&)>(&_Find))
    .class_function("Find", emscripten::select_overload<SdfLayerHandle(const std::string&, const emscripten::val&)>(&_Find))
    .class_function("CreateAnonymous", emscripten::select_overload<SdfLayerRefPtr(const std::string&)>(&_CreateAnonymous))
    .class_function("CreateAnonymous", emscripten::select_overload<SdfLayerRefPtr(const std::string&, const emscripten::val&)>(&_CreateAnonymous))
    .property("identifier", &pxr::SdfLayer::GetIdentifier, &pxr::SdfLayer::SetIdentifier)
    .smart_ptr<pxr::SdfLayerRefPtr>("EmUsdSdfLayerRefPtr")
    ;
}
