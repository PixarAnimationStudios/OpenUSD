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
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"

#include <iostream>
#include <fstream>

#include "animx.h"
#include "fileFormat.h"
#include "data.h"
#include "reader.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdAnimXFileFormatTokens, USD_ANIMX_FILE_FORMAT_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (prim)
    (op)
    (curve)
    (target)
    (dataType)
    (defaultValue)
);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdAnimXFileFormat, SdfFileFormat);
}

UsdAnimXFileFormat::UsdAnimXFileFormat()
    : SdfFileFormat(
        UsdAnimXFileFormatTokens->Id,
        UsdAnimXFileFormatTokens->Version,
        UsdAnimXFileFormatTokens->Target,
        UsdAnimXFileFormatTokens->Extension)
{
}

UsdAnimXFileFormat::~UsdAnimXFileFormat()
{
}

SdfAbstractDataRefPtr
UsdAnimXFileFormat::InitData(
    const FileFormatArguments &args) const
{
    return UsdAnimXData::New();
}

/*
static 
void
_RecurseSdfChildren(SdfPrimSpecHandle& spec, UsdAnimXDataRefPtr animXData) 
{
    SdfPrimSpec::NameChildrenView childrens = spec->GetNameChildren();
    for(auto& child: childrens) {
      SdfPrimSpec childSpec = child.GetSpec();
      if(childSpec.GetTypeName() == _tokens->fCurveOp) {
          SdfPrimSpec::PropertySpecView props = childSpec.GetProperties();
          TfToken attributeName;
          TfToken dataType;
          VtValue defaultValue = VtValue();
          
          for(auto& prop: props) {
            TfToken name = prop.GetSpec().GetNameToken();
            if(name == _tokens->attributeName) {
                attributeName =
                    prop.GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
            }
            else if(name == _tokens->dataType) {
                dataType = prop.GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
            }
            else if(name == _tokens->defaultValue) {
                defaultValue = prop.GetSpec().GetDefaultValue();
            }
          }
          animXData->AddOp(spec->GetPath(), attributeName, dataType, defaultValue);
      } else if(childSpec.GetTypeName() == _tokens->fCurve) {
          SdfPrimSpec::PropertySpecView props = childSpec.GetProperties();
          SdfPropertySpecHandle prop =  
              childSpec.GetPropertyAtPath(childSpec.GetPath()
                  .AppendProperty(_tokens->keyframes));
          UsdAnimXCurve curve;
          SdfTimeSampleMap keyframesMap = prop.GetSpec().GetTimeSampleMap();
          for(auto& keyframe: keyframesMap) {
              adsk::Keyframe key = 
                  GetKeyframeFromVtValue( keyframe.first, keyframe.second, 
                      curve.keyframeCount());
              curve.addKeyframe(key);
          }
          SdfPrimSpec opSpec = spec.GetSpec();
          SdfPath opPath = opSpec.GetPath();

          TfToken opName = opSpec.GetPropertyAtPath(opPath
              .AppendProperty(_tokens->attributeName))
                  .GetSpec().GetDefaultValue().GetWithDefault<TfToken>();
          SdfPath primPath = opPath.GetParentPath();

          animXData->AddFCurve(primPath, opName, curve);
      } else {
          animXData->AddPrim(childSpec.GetPath());
      }

      _RecurseSdfChildren(SdfPrimSpecHandle(childSpec), animXData);
    }
}
*/
static
bool
_CanRead(const std::shared_ptr<ArAsset>& asset,
             const std::string& cookie)
{
    TfErrorMark mark;

    char aLine[512];

    size_t numToRead = std::min(sizeof(aLine), cookie.length());
    if (asset->Read(aLine, numToRead, /* offset = */ 0) != numToRead) {
        return false;
    }

    aLine[numToRead] = '\0';

    // Don't allow errors to escape this function, since this function is
    // just trying to answer whether the asset can be read.
    return !mark.Clear() && TfStringStartsWith(aLine, cookie);
}

/*
static void PrintReadState(size_t state){
    switch(state) {
        case READ_NONE:
            std::cout << "READ STATE : NONE" << std::endl;
            break;
        case READ_PRIM_ENTER:
            std::cout << "READ STATE : PRIM_ENTER" << std::endl;
            break;
        case READ_PRIM:
            std::cout << "READ STATE : PRIM" << std::endl;
            break;
        case READ_OP_ENTER:
            std::cout << "READ STATE : OP_ENTER" << std::endl;
            break;
        case READ_OP:
            std::cout << "READ STATE : OP" << std::endl;
            break;
        case READ_CURVE_ENTER:
            std::cout << "READ STATE : CURVE_ENTER" << std::endl;
            break;
        case READ_CURVE:
            std::cout << "READ STATE : CURVE" << std::endl;
            break;
        default:
            std::cout << "READ STATE : OUT OF BOUND!!!" << std::endl;
    }
}
*/
bool 
UsdAnimXFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath,
        bool metadataOnly) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(resolvedPath);
    if (!asset) {
        return false;
    }

    // Quick check to see if the file has the magic cookie before spinning up
    // the parser.
    if (!_CanRead(asset, GetFileCookie())) {
        TF_RUNTIME_ERROR("<%s> is not a valid %s layer",
                         resolvedPath.c_str(),
                         GetFormatId().GetText());
        return false;
    }


    SdfAbstractDataRefPtr data = InitData(layer->GetFileFormatArguments());
    UsdAnimXReader reader;
    reader.Open(asset);
    /*
    std::ifstream file(resolvedPath);
    std::string line;

    UsdAnimXPrimDesc primDesc;
    UsdAnimXOpDesc opDesc;
    UsdAnimXCurveDesc curveDesc;
    UsdAnimXKeyframeDesc keyframeDesc;

    UsdAnimXPrimDesc* currentPrim = NULL;
    UsdAnimXOpDesc* currentOp = NULL;
    UsdAnimXCurveDesc* currentCurve = NULL;

    _readState = READ_NONE;
    _primDepth = 0;
    size_t start_p, end_p;
    while (std::getline(file, line)) {
        if(line.empty())continue;
        PrintReadState(_readState);
        std::cout << "PRIM DEPTH : " << _primDepth << std::endl;
        line = _Trim(line);
        if(_IsPrim(line, &primDesc))
        {
            _primDepth++;
            if(currentPrim){
                primDesc.parent = currentPrim;
                currentPrim->children.push_back(primDesc);
                currentPrim = &currentPrim->children.back();
            }
            else {
                primDesc.parent = NULL;
                _rootPrims.push_back(primDesc);
                currentPrim = &_rootPrims.back();
            }
            _readState = READ_PRIM_ENTER;
        }
        else if(_IsOp(line, &opDesc))
        {
          _readState = READ_OP_ENTER;
        }
        else if(_IsCurve(line, &curveDesc))
        {
          _readState = READ_CURVE_ENTER;
        }
        if(_HasOpeningBrace(line, &start_p)) {
            _readState++;
        }
        else if(_HasClosingBrace(line, &end_p)) {
            switch(_readState) {
                case READ_CURVE:
                    _readState = READ_OP;
                    break;
                case READ_OP:
                    _readState = READ_PRIM;
                    break;
                case READ_PRIM:
                    _primDepth--;
                    if(_primDepth == 0)_readState = READ_NONE;
                    break;
            }
        }
    }
    file.close();
    */
    /*
    if (!Sdf_ParseMenva(
            resolvedPath, asset, GetFormatId(), GetVersionString(), 
            metadataOnly, TfDynamic_cast<SdfDataRefPtr>(data))) {
        return false;
    }
    */

    _SetLayerData(layer, data);
    return true;
  /*
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

      SdfPrimSpecHandle srcRoot = buffer->GetPseudoRoot();

      _RecurseSdfChildren(srcRoot, animXData);
      animXData->ComputeTimesSamples();
      _SetLayerData(layer, data);

      is.close();
      return true;
  }

  return false;
  */
/*
  SdfAbstractDataRefPtr data = InitData(FileFormatArguments());
  UsdAnimXDataRefPtr animXData = TfStatic_cast<UsdAnimXDataRefPtr>(data);

  SdfLayerRefPtr genLayer(layer);


  SdfPath rootPath("/Animation");

  animXData->AddPrim(rootPath);
  SdfPath cubePath = rootPath.AppendChild(TfToken("cube"));
  animXData->AddPrim(cubePath);

  VtArray<TfToken> xformOpOrder = {TfToken("xformOp:translate")};
  animXData->AddOp(cubePath, TfToken("xformOpOrder"),
      TfToken("token[]"),VtValue(xformOpOrder));

  animXData->AddOp(cubePath, TfToken("xformOp:translate"),
      TfToken("double3"), VtValue(GfVec3d(0,10,0)));

  animXData->AddOp(cubePath, TfToken("size"),
      TfToken("double"), VtValue((double)7.0));

  UsdAnimXCurve x, y, z;
  
  adsk::Keyframe key;
  key.time = 0;
  key.value = 0;
  key.index = 0;
  key.tanIn = {adsk::TangentType::Auto, (adsk::seconds)1, (adsk::seconds)1};
  key.tanOut = {adsk::TangentType::Auto, (adsk::seconds)1, (adsk::seconds)1};
  key.linearInterpolation = false;
  key.quaternionW = 1.0;
  x.addKeyframe(key);

  key.time = 100;
  key.value = 10;
  key.index = 1;
  x.addKeyframe(key);
  
  animXData->AddFCurve(cubePath, TfToken("xformOp:translate"), x);

  key.time = 0;
  key.value = 0;
  key.index = 0;
  y.addKeyframe(key);

  key.time = 50;
  key.value = 10;
  key.index = 1;
  y.addKeyframe(key);

  key.time = 100;
  key.value = 0;
  key.index = 2;
  y.addKeyframe(key);

  animXData->AddFCurve(cubePath, TfToken("xformOp:translate"), y);

  key.time = 0;
  key.value = 0;
  key.index = 0;
  z.addKeyframe(key);

  animXData->AddFCurve(cubePath, TfToken("xformOp:translate"), z);

  animXData->ComputeTimesSamples();
  _SetLayerData(layer, data);
*/
  return true;
}

bool
UsdAnimXFileFormat::CanRead(const std::string& filePath) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(filePath);
    return asset && _CanRead(asset, GetFileCookie());
}

PXR_NAMESPACE_CLOSE_SCOPE

