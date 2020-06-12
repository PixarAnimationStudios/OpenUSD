//
// Copyright 2020 benmalartre
//
// unlicensed
//

#include "pxr/base/tf/token.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"
#include "pxr/imaging/plugin/LoFi/codeGen.h"
#include "pxr/imaging/hio/glslfx.h"

PXR_NAMESPACE_OPEN_SCOPE

static TfToken LoFiGetAttributeChannelName(LoFiAttributeChannel channel)
{
   switch(channel)
   {
    case CHANNEL_POSITION:
      return LoFiBufferTokens->position;
    case CHANNEL_NORMAL:
      return LoFiBufferTokens->normal;
    case CHANNEL_TANGENT:
      return LoFiBufferTokens->tangent;
    case CHANNEL_COLOR:
      return LoFiBufferTokens->color;
    case CHANNEL_UV:
      return LoFiBufferTokens->uv;
    case CHANNEL_WIDTH:
      return LoFiBufferTokens->width;
    case CHANNEL_ID:
      return LoFiBufferTokens->id;
    case CHANNEL_SCALE:
      return LoFiBufferTokens->scale;
    case CHANNEL_SHAPE_POSITION:
      return LoFiBufferTokens->shape_position;
    case CHANNEL_SHAPE_NORMAL:
      return LoFiBufferTokens->shape_normal;
    case CHANNEL_SHAPE_UV:
      return LoFiBufferTokens->shape_position;
    case CHANNEL_SHAPE_COLOR:
      return LoFiBufferTokens->shape_position;
    default:
      return TfToken();
   }
}

static TfToken LoFiGetAttributeChannelType(LoFiAttributeChannel channel)
{
   switch(channel)
   {
    case CHANNEL_POSITION:
    case CHANNEL_NORMAL:
    case CHANNEL_TANGENT:
    case CHANNEL_COLOR:
    case CHANNEL_SCALE:
    case CHANNEL_SHAPE_POSITION:
    case CHANNEL_SHAPE_NORMAL:
    case CHANNEL_SHAPE_COLOR:
      return LoFiGLTokens->vec3;
    case CHANNEL_UV:
    case CHANNEL_SHAPE_UV:
      return LoFiGLTokens->vec2;
    case CHANNEL_WIDTH:
      return LoFiGLTokens->_float;
    case CHANNEL_ID:
      return LoFiGLTokens->_int;
    default:
      return TfToken();
   }
}

static std::string _GetSwizzleString(TfToken const& type, 
                                     std::string const& swizzle=std::string())
{
    if (!swizzle.empty()) {
        return "." + swizzle;
    } 
    if (type == LoFiGLTokens->vec4 || type == LoFiGLTokens->ivec4) {
        return "";
    }
    if (type == LoFiGLTokens->vec3 || type == LoFiGLTokens->ivec3) {
        return ".xyz";
    }
    if (type == LoFiGLTokens->vec2 || type == LoFiGLTokens->ivec2) {
        return ".xy";
    }
    if (type == LoFiGLTokens->_float || type == LoFiGLTokens->_int) {
        return ".x";
    }

    return "";
}

static int _GetNumComponents(TfToken const& type)
{
    int numComponents = 1;
    if (type == LoFiGLTokens->vec2 || type == LoFiGLTokens->ivec2) {
        numComponents = 2;
    } else if (type == LoFiGLTokens->vec3 || type == LoFiGLTokens->ivec3) {
        numComponents = 3;
    } else if (type == LoFiGLTokens->vec4 || type == LoFiGLTokens->ivec4) {
        numComponents = 4;
    } else if (type == LoFiGLTokens->mat3) {
        numComponents = 9;
    } else if (type == LoFiGLTokens->mat4) {
        numComponents = 16;
    }

    return numComponents;
}

/// Constructor.
LoFiCodeGen::LoFiCodeGen(LoFiProgramType type, 
  const LoFiShaderCodeSharedPtr& shaderCode)
  : _type(type)
  , _shaderCode(shaderCode)
{
  const GlfContextCaps& caps = GlfContextCaps::GetInstance();
  _glslVersion = caps.glslVersion;
}

LoFiCodeGen::LoFiCodeGen(LoFiProgramType type, 
            const LoFiBindingList& uniformBindings,
            const LoFiBindingList& vertexBufferBindings,
            const LoFiShaderCodeSharedPtr& shaderCode)
  : _type(type)
  , _uniformBindings(uniformBindings)
  , _attributeBindings(vertexBufferBindings)
  , _shaderCode(shaderCode)
{
  const GlfContextCaps& caps = GlfContextCaps::GetInstance();
  _glslVersion = caps.glslVersion;
}

void LoFiCodeGen::_EmitDeclaration(std::stringstream &ss,
                                    TfToken const &name,
                                    TfToken const &type,
                                    LoFiBinding const &binding,
                                    size_t arraySize)
{
  LoFiBindingType bindingType = binding.type;

  if (!TF_VERIFY(!name.IsEmpty())) return;
  if (!TF_VERIFY(!type.IsEmpty(),
                    "Unknown dataType for %s",
                    name.GetText())) return;

  if (arraySize > 0) 
  {
    if (!TF_VERIFY(bindingType == LoFiBindingType::UNIFORM_ARRAY))
        return;
  }

  switch (bindingType) 
  {
  case LoFiBindingType::VERTEX:
    if(_glslVersion >= 330)
    {
      ss  << "layout (location = " << binding.location << ") in "
          << type << " " << name << ";\n";
      break;
    }
    else
    {
      ss << "attribute " << type << " " << name << ";\n";
      break;
    }
  case LoFiBindingType::UNIFORM:
      ss << "uniform " << type << " " << name << ";\n";
      break;
  case LoFiBindingType::UNIFORM_ARRAY:
      ss << "uniform " << type << " " << name
          << "[" << arraySize << "];\n";
      break;
  default:
      TF_CODING_ERROR("Unknown binding type %d, for %s\n",
                      binding.type, name.GetText());
      break;
  }
}

void LoFiCodeGen::_EmitAccessor(std::stringstream &ss,
                                TfToken const &name,
                                TfToken const &type,
                                LoFiBinding const &binding)
{
  ss << type << " LOFI_GET_" << name << "()"
      << " { return " << name << "; }\n";
}

void LoFiCodeGen::_EmitStageAccessor(std::stringstream &ss,
                                      TfToken const &stage,
                                      TfToken const &name,
                                      TfToken const &type,
                                      int arraySize,
                                      int* index)
{ 
  if(index!=NULL)
  {
    if (arraySize > 1) 
    {
      ss << type << " LOFI_GET_" << name << "(int localIndex, int arrayIndex)"
          << " { return " << stage << "_" << name << "[" << *index << "][arrayIndex]; }\n";
    } 
    else 
    {
      ss << type << " LOFI_GET_" << name << "(int localIndex)"
          << " { return " << stage << "_" << name << "[" << *index << "]; }\n";
    }
  }
  else
  {
    if (arraySize > 1) 
    {
      ss << type << " LOFI_GET_" << name << "(int index)"
          << " { return " << stage << "_" << name << "[index]; }\n";
    } 
    else 
    {
      ss << type << " LOFI_GET_" << name << "()"
          << " { return " << stage << "_" << name << "; }\n";
    }
  }
  
  
}

void LoFiCodeGen::_EmitStageEmittor(std::stringstream &ss,
                                    TfToken const &stage,
                                    TfToken const &name,
                                    TfToken const &type,
                                    int arraySize, 
                                    int* index)
{ 
  if(stage == LoFiShaderTokens->fragment)
  {
    if(_glslVersion>=330)
    {
      ss << "out " << type << " " << name << ";\n";
      ss << "void  LOFI_SET_" << name << "(" << type << " value)"
          << " { " << name << " = value; }\n";
    }
    else
    {
      std::string substName = "gl_FragColor";
      if(index!=NULL)
      {
        substName = "gl_FragData["+TfStringify(*index)+"]";
      }
      ss << "void  LOFI_SET_" << name << "(" << type << " value)"
          << " { " << substName << " = value; }\n";
    }
  }
  else
  {
    if (arraySize > 1) 
    {
      ss << "void LOFI_SET_" << name << "(int index, " << type << " value)"
          << " { " << stage << "_" << name << "[index] = value; }\n";
    } 
    else 
    {
      ss << "void  LOFI_SET_" << name << "(" << type << " value)"
          << " { " << stage << "_" << name << " = value; }\n";
    }
  }
}

void 
LoFiCodeGen::_GenerateVersion()
{
  if(_glslVersion >= 330)
  {
    _genCommon << "#version 330 core \n";
    _genCommon << "#define LOFI_GLSL_330 1\n";
  }
  else
  {
    _genCommon << "#version 120 \n";
  }
}


void
LoFiCodeGen::_GeneratePrimvars(bool hasGeometryShader, size_t numVertexPerPrimitive)
{
  std::stringstream vertexInputs;
  std::vector<std::string> vertexDatas, geometryDatas;
  std::stringstream streamVS, streamGS, streamFS;

  // parse attributes
  TF_FOR_ALL (it, _attributeBindings) 
  {
    TfToken const &name = it->name;
    TfToken const &dataType = it->dataType;

    _EmitDeclaration(vertexInputs, name, dataType, *it);

    std::stringstream vertexData;
    vertexData  << "  " << dataType << " " 
                << LoFiShaderTokens->vertex <<"_" << name;
    vertexDatas.push_back(vertexData.str());
    
    std::stringstream geometryData;
    geometryData  << "  " << dataType << " " 
                  << LoFiShaderTokens->geometry <<"_" << name;
    geometryDatas.push_back(geometryData.str());

    // primvars
    _EmitAccessor(streamVS, name, dataType, *it);
    _EmitStageEmittor(streamVS, LoFiShaderTokens->vertex,
                    name, dataType, 1);

    if(hasGeometryShader)
    {
      _EmitStageAccessor(streamGS,  LoFiShaderTokens->vertex,
                        name, dataType, 6);
      _EmitStageEmittor(streamGS, LoFiShaderTokens->geometry, name, dataType, 1);
      _EmitStageAccessor(streamFS,  LoFiShaderTokens->geometry,
                        name, dataType, 1);
    }
    else
    {
      _EmitStageAccessor(streamFS,  LoFiShaderTokens->vertex,
                        name, dataType, 1);
    }
  }

  // vertex shader code
  _genVS  << vertexInputs.str();
  TF_FOR_ALL(it, vertexDatas)
  {
    if(_glslVersion>=330)
      _genVS << "out " << it->c_str() << ";\n";
    else
      _genVS << "varying " << it->c_str() << ";\n";
  }
  _genVS << streamVS.str();

  
  if(hasGeometryShader)
  {
    _genGS << "#define LOFI_NUM_PRIMITIVE_VERTS ";
    _genGS << numVertexPerPrimitive << "\n";
    // geometry shader code
    TF_FOR_ALL(it, vertexDatas)
    {
      if(_glslVersion>=330)
        _genGS << "in " << it->c_str() << "[LOFI_NUM_PRIMITIVE_VERTS];\n";
      else
        _genGS << "varying " << it->c_str() << "[LOFI_NUM_PRIMITIVE_VERTS];\n";
    }
    TF_FOR_ALL(it, geometryDatas)
    {
      if(_glslVersion>=330)
        _genGS << "out " << it->c_str() << ";\n";
      else
        _genGS << "varying " << it->c_str() << ";\n";
    }
    
    _genGS<< streamGS.str();

    // fragment shader code
    TF_FOR_ALL(it, geometryDatas)
    {
      if(_glslVersion>=330)
        _genFS << "in " << it->c_str() << ";\n";
      else
        _genFS << "varying " << it->c_str() << ";\n";
    }
    _genFS << streamFS.str();
  }
  // fragment shader code
  else
  {
    TF_FOR_ALL(it, vertexDatas)
    {
      if(_glslVersion>=330)
        _genFS << "in " << it->c_str() << ";\n";
      else
        _genFS << "varying " << it->c_str() << ";\n";
    }
    _genFS << streamFS.str();
  }
}

void
LoFiCodeGen::_GenerateUniforms()
{
  std::stringstream uniformInputs;
  std::stringstream accessorsCommon;

  // vertex varying
  TF_FOR_ALL (it, _uniformBindings) 
  {
    TfToken const &name = it->name;
    TfToken const &dataType = it->dataType;

    _EmitDeclaration(uniformInputs, name, dataType, *it);
    // uniform accessors
    _EmitAccessor(accessorsCommon, name, dataType, *it);
  }

  _genVS  << uniformInputs.str()
          << accessorsCommon.str();

  _genGS  << uniformInputs.str()
          << accessorsCommon.str();

  _genFS  << uniformInputs.str()
          << accessorsCommon.str();
}

void LoFiCodeGen::_GenerateResults()
{
  _EmitStageEmittor(_genFS,  LoFiShaderTokens->fragment,
                    TfToken("result"), LoFiGLTokens->vec4, 1);
}

void LoFiCodeGen::GenerateProgramCode(bool hasGeometryShader, size_t numVertexPerPrimitive)
{
  // initialize source buckets
  _genCommon.str(""), _genVS.str(""), _genGS.str(""), _genFS.str("");

  _GenerateVersion();
  
  TF_FOR_ALL (it, _attributeBindings) {
    _genCommon << "#define LOFI_HAS_" << it->name << " 1\n";
  }

  TF_FOR_ALL (it, _uniformBindings) {
    _genCommon << "#define LOFI_HAS_" << it->name << " 1\n";
  }

  _genVS << _genCommon.str();
  _genGS << _genCommon.str();
  _genFS << _genCommon.str();

  _GenerateUniforms();
  _GeneratePrimvars(hasGeometryShader, numVertexPerPrimitive);
  _GenerateResults();

  // shader sources which own main()
  _genVS << _shaderCode->GetSource(LoFiShaderTokens->vertex);
  _genGS << _shaderCode->GetSource(LoFiShaderTokens->geometry);
  _genFS << _shaderCode->GetSource(LoFiShaderTokens->fragment);

  // cache strings
  _vertexCode = _genVS.str();
  _geometryCode = _genGS.str();
  _fragmentCode = _genFS.str();


  std::cout << "######################## VERTEX ########################" << std::endl;
  std::cout << _vertexCode << std::endl;
  std::cout << "######################## GEOMETRY ########################" << std::endl;
  std::cout << _geometryCode << std::endl;
  std::cout << "######################## FRAGMENT ########################" << std::endl;
  std::cout << _fragmentCode << std::endl;

}

PXR_NAMESPACE_CLOSE_SCOPE