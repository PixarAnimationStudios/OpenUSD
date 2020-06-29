//
// Copyright 2020 benmalartre
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
#include "fileFormat.h"
#include "data.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/registryManager.h"
#include <iostream>
#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdAnimXFileFormatTokens, USD_ANIMX_FILE_FORMAT_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fCurveOp, "FCurveOp"))
    ((fCurve, "FCurve"))
    (attributeName)
    (dataType)
    (elementCount)
    (defaultValue)
    (keyframes)
);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdAnimXFileFormat, SdfTextFileFormat);
}

UsdAnimXFileFormat::UsdAnimXFileFormat()
    : SdfTextFileFormat(UsdAnimXFileFormatTokens->Id,
                        UsdAnimXFileFormatTokens->Version,
                        UsdUsdFileFormatTokens->Target)
{
    // Do Nothing.
}

UsdAnimXFileFormat::~UsdAnimXFileFormat()
{
    // Do Nothing.
}

SdfAbstractDataRefPtr
UsdAnimXFileFormat::InitData(
    const FileFormatArguments &args) const
{
    std::cout << "USD ANIM X INIT DATA !!!" << std::endl;
    return UsdAnimXData::New();
}

static 
void
_RecurseSdfChildren(SdfPrimSpecHandle& spec, UsdAnimXDataRefPtr animXData) 
{
    SdfPrimSpec::NameChildrenView childrens = spec->GetNameChildren();
    for(auto& child: childrens) {
      SdfPrimSpec spec = child.GetSpec();

      if(spec.GetTypeName() == _tokens->fCurveOp) {
          SdfPrimSpec::PropertySpecView props = spec.GetProperties();
          TfToken attributeName;
          TfToken dataType;
          VtValue defaultValue = VtValue();
          int elemCount = 1;
          
          for(auto& prop: props) {
            TfToken name = prop.GetSpec().GetNameToken();
            if(name == _tokens->attributeName) {
              attributeName =
                 prop.GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
            }
            else if(name == _tokens->dataType) {
             dataType = prop.GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
            }
            else if(name == _tokens->elementCount) {
              elemCount = prop.GetSpec().GetDefaultValue().Get<int>();
            }
            else if(name == _tokens->defaultValue) {
              defaultValue = prop.GetSpec().GetDefaultValue();
            }
          }

          animXData->AddOp(spec.GetNameParent().GetSpec().GetPath(),
              attributeName, dataType, elemCount, defaultValue);
      }
      else if(spec.GetTypeName() == _tokens->fCurve) {
          SdfPrimSpec::PropertySpecView props = spec.GetProperties();
          SdfPropertySpecHandle prop =  spec.GetPropertyAtPath(spec.GetPath()
              .AppendProperty(_tokens->keyframes));
          UsdAnimXCurve curve;
          SdfTimeSampleMap keyframesMap = prop.GetSpec().GetTimeSampleMap();
          for(auto& keyframe: keyframesMap) {
              adsk::Keyframe key = 
                  GetKeyframeFromVtValue( keyframe.first, keyframe.second, 
                      curve.keyframeCount());
              curve.addKeyframe(key);
          }

          SdfPrimSpec opSpec = spec.GetNameParent().GetSpec();
          SdfPath opPath = opSpec.GetPath();

          TfToken opName = opSpec.GetPropertyAtPath(opPath
              .AppendProperty(_tokens->attributeName))
                  .GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
          SdfPath primPath = opPath.GetParentPath();

          animXData->AddFCurve(primPath, opName, curve);
      }
      else {
        animXData->AddPrim(spec.GetPath());
      }

      _RecurseSdfChildren(SdfPrimSpecHandle(spec), animXData);
    }
}

bool 
UsdAnimXFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath,
        bool metadataOnly) const
{
  std::ifstream is(resolvedPath);

  if (is.is_open())
  {
      SdfLayerRefPtr buffer = SdfLayer::CreateAnonymous();
      const SdfFileFormatConstPtr fileFormat = 
          SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id);

      is.seekg(0, std::ios::end);
      size_t size = is.tellg();
      std::string content(size, ' ');
      is.seekg(0);
      is.read(&content[0], size); 
      content.replace(0, 10, "#usda 1.0");
      fileFormat->ReadFromString(&(*buffer), content);

      SdfAbstractDataRefPtr data = InitData(FileFormatArguments());
      UsdAnimXDataRefPtr animXData = TfStatic_cast<UsdAnimXDataRefPtr>(data);

      SdfPrimSpecHandle pseudoRoot = buffer->GetPseudoRoot();
      _RecurseSdfChildren(pseudoRoot, animXData);
      animXData->ComputeTimesSamples();
      _SetLayerData(layer, data);

      is.close();
      return true;
  }
  return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

