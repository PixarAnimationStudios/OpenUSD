//
// Copyright 2020 benmalartre
//
// unlicensed
//

#include "pxr/base/tf/token.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/codeGen.h"

PXR_NAMESPACE_OPEN_SCOPE

/*
static const char *VERTEX_SHADER_330[1] = {
  "#version 330 core                                        \n" 
  "uniform mat4 model;                                      \n"
  "uniform mat4 view;                                       \n"
  "uniform mat4 projection;                                 \n"
  "                                                         \n"
  "in vec3 position;                                        \n"
  "in vec3 normal;                                          \n"
  "in vec3 color;                                           \n"
  "out vec3 vertex_color;                                   \n"
  "out vec3 vertex_normal;                                  \n"
  "void main(){                                             \n"
  "    vertex_normal = (model * vec4(normal, 0.0)).xyz;     \n"
  "    vertex_color = color;                                \n"
  "    vec3 p = vec3(view * model * vec4(position,1.0));    \n"
  "    gl_Position = projection * vec4(p,1.0);              \n"
  "}"
};

static const char *FRAGMENT_SHADER_120[1] = {
  "#version 120                                             \n"
  "varying vec3 vertex_normal;                              \n"
  "varying vec3 vertex_color;                               \n"
  "void main()                                              \n"
  "{                                                        \n"
  " vec3 color = vertex_normal * 0.5 + vertex_color * 0.5;  \n"
  "	gl_FragColor = vec4(color,1.0);                         \n"
  "}"
};

static const char *FRAGMENT_SHADER_330[1] = {
  "#version 330 core                                        \n"
  "in vec3 vertex_color;                                    \n"
  "in vec3 vertex_normal;                                   \n"
  "out vec4 outColor;                                       \n"
  "void main()                                              \n"
  "{                                                        \n"
  "	outColor = vec4(vertex_normal,1.0);                     \n"
  "}"
};
*/

static TfToken LoFiGetVertexBufferChannelName(LoFiVertexBufferChannel channel)
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

static TfToken LoFiGetVertexBufferChannelType(LoFiVertexBufferChannel channel)
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
LoFiCodeGen::LoFiCodeGen(LoFiGeometricProgramType type, 
  const LoFiVertexBufferChannelList& channels)
  : _type(type)
  , _channels(channels)
{
  const GlfContextCaps& caps = GlfContextCaps::GetInstance();
  _glslVersion = caps.glslVersion;
}

LoFiCodeGen::LoFiCodeGen(LoFiGeometricProgramType type, 
            const LoFiUniformBindingList& uniformBindings,
            const LoFiVertexBufferBindingList& vertexBufferBindings)
  : _type(type)
  , _uniformBindings(uniformBindings)
  , _vertexBufferBindings(vertexBufferBindings)
{
  const GlfContextCaps& caps = GlfContextCaps::GetInstance();
  _glslVersion = caps.glslVersion;
}

void LoFiCodeGen::_GenerateVersion(std::stringstream& ss)
{
  if(_glslVersion >= 330)
    ss << "#version 330 core \n";
  else
    ss << "#version 120 \n";
}

void LoFiCodeGen::_AddInputChannel(std::stringstream& ss, 
  LoFiVertexBufferChannel channel, size_t index, TfToken& type)
{
  TfToken channelName = LoFiGetVertexBufferChannelName(channel);
  ss << "#define LOFI_HAS_" << channelName << " 1\n";
  if(_glslVersion >= 330)
    ss << "layout(location=" << index <<") in " << type << " " << channelName << ";\n";
  else
    ss << "attribute " << type << " " << channelName << ";\n";;
}

void LoFiCodeGen::_AddOutputAttribute(std::stringstream& ss, 
  LoFiVertexBufferChannel channel, TfToken& type)
{
  TfToken name = LoFiGetVertexBufferChannelName(channel);
  if(_glslVersion >= 330)
    ss << "out " << type <<" vertex_" << name << ";\n";
  else
    ss << "varying " << type <<" vertex_" << name<< ";\n";
}

void LoFiCodeGen::_AddInputAttribute(std::stringstream& ss, 
  LoFiVertexBufferChannel channel, TfToken& type)
{
  TfToken name = LoFiGetVertexBufferChannelName(channel);
  if(_glslVersion >= 330)
    ss << "in " << type <<" vertex_" << name << ";\n";
  else
    ss << "varying " << type <<" vertex_" << name << ";\n";
}

void LoFiCodeGen::_AddUniform(std::stringstream& ss, const TfToken& name, 
  const TfToken& type)
{
  ss << "#define LOFI_HAS_" << name << " 1\n";
  ss << "uniform " << type << " " << name << ";\n";
}

void LoFiCodeGen::_EmitDeclaration(std::stringstream &ss,
                                    TfToken const &name,
                                    TfToken const &type,
                                    LoFiBinding const &binding,
                                    size_t arraySize)
{
    /*
    /// GLSL 120
      [vertex attribute]
         attribute <type> <name>;
      [uniform]
         uniform <type> <name>;
    
    /// GLSL 330
       [vertex attribute]
         layout (location = <location>) in <type> <name>;
      [uniform]
         uniform <type> <name>;
     */
    LoFiBinding::Type bindingType = binding.GetType();

    if (!TF_VERIFY(!name.IsEmpty())) return;
    if (!TF_VERIFY(!type.IsEmpty(),
                      "Unknown dataType for %s",
                      name.GetText())) return;

    if (arraySize > 0) 
    {
        if (!TF_VERIFY(bindingType == LoFiBinding::UNIFORM_ARRAY))
            return;
    }

    switch (bindingType) 
    {
    case LoFiBinding::VERTEX_ATTR:
      if(_glslVersion >= 330)
      {ss << "in " << type << " " << name << ";\n";
        break;}
      else
      {ss << "attribute " << type << " " << name << ";\n";}
    case LoFiBinding::UNIFORM:
        ss << "uniform " << type << " " << name << ";\n";
        break;
    case LoFiBinding::UNIFORM_ARRAY:
        ss << "uniform " << type << " " << name
            << "[" << arraySize << "];\n";
        break;
    default:
        TF_CODING_ERROR("Unknown binding type %d, for %s\n",
                        binding.GetType(), name.GetText());
        break;
    }
}

void LoFiCodeGen::_EmitAccessor(std::stringstream &ss,
                                TfToken const &name,
                                TfToken const &type,
                                LoFiBinding const &binding,
                                const char *index)
{
    if (index) 
    {
        ss << type << " LoFiGet_" << name << "(int localIndex) {\n"
            << "  int index = " << index << ";\n";
        if (binding.GetType() == LoFiBinding::TBO) {
            ss << "  return "
                << type
                << "(texelFetch(" << name << ", index)"
                << _GetSwizzleString(type) << ");\n}\n";
        } else {
            ss << "  return " << name << "[index];\n}\n";
        }
    } 
    else 
    {
        // non-indexed, only makes sense for uniform or vertex.
        if (binding.GetType() == LoFiBinding::UNIFORM || 
            binding.GetType() == LoFiBinding::VERTEX_ATTR) 
        {
            ss << type
                << " LoFiGet_" << name << "(int localIndex) { ";
            ss << "return " << name << ";}\n";
        }
    }

    ss << type << " LoFiGet_" << name << "()"
        << " { return LoFiGet_" << name << "(0); }\n";
}

void LoFiCodeGen::_EmitStructAccessor(std::stringstream &ss,
                                      TfToken const &structName,
                                      TfToken const &name,
                                      TfToken const &type,
                                      int arraySize,
                                      const char *index)
{ 
    if (index) 
    {
        if (arraySize > 1) 
        {
            ss << type << " LoFiGet_" << name
                << "(int arrayIndex, int localIndex) {\n"
                << "  int index = " << index << ";\n"
                << "  return "
                << structName << "[index]." << name << "[arrayIndex];\n}\n";
        } 
        else 
        {
            ss << type << " LoFiGet_" << name
                << "(int localIndex) {\n"
                << "  int index = " << index << ";\n"
                << "  return "
                << structName << "[index]." << name << ";\n}\n";
        }
    }
    else 
    {
        if (arraySize > 1) 
        {
            ss << type << " LoFiGet_" << name
                << "(int arrayIndex, int localIndex) { return "
                << structName << "." << name << "[arrayIndex];}\n";
        } 
        else 
        {
            ss << type << " LoFiGet_" << name
                << "(int localIndex) { return "
                << structName << "." << name << ";}\n";
        }
    }

    if (arraySize > 1) 
    {
        ss << type << " LoFiGet_" << name
            << "(int arrayIndex)"
            << " { return LoFiGet_" << name << "(arrayIndex, 0); }\n";
    } 
    else 
    {
        ss << type << " LoFiGet_" << name << "()"
            << " { return LoFiGet_" << name << "(0); }\n";
    }
}

void
LoFiCodeGen::_GeneratePrimvar(bool hasGeometryShader)
{
  
    // Vertex and FVar primvar flow into the fragment shader as per-fragment
    // attribute data that has been interpolated by the rasterizer, and hence
    // have similarities for code gen.
    // While vertex primvar are authored per vertex and require plumbing
    // through all shader stages, fVar is emitted only in the GS stage.
    
    //  // --------- vertex data declaration (VS) ----------
    //  layout (location = 0) in vec3 normals;
    //  layout (location = 1) in vec3 points;
//
    //  out Primvars {
    //      vec3 normals;
    //      vec3 points;
    //  } outPrimvars;
//
    //  void ProcessPrimvars() {
    //      outPrimvars.normals = normals;
    //      outPrimvars.points = points;
    //  }
//
    //  // --------- geometry stage plumbing -------
    //  in Primvars {
    //      vec3 normals;
    //      vec3 points;
    //  } inPrimvars[];
    //  out Primvars {
    //      vec3 normals;
    //      vec3 points;
    //  } outPrimvars;
//
    //  void ProcessPrimvars(int index) {
    //      outPrimvars = inPrimvars[index];
    //  }
//
    //  // --------- vertex data accessors (used in geometry/fragment shader) ---
    //  in Primvars {
    //      vec3 normals;
    //      vec3 points;
    //  } inPrimvars;
    //  vec3 HdGet_normals(int localIndex=0) {
    //      return inPrimvars.normals;
    //  }
    //

    std::stringstream vertexInputs;
    std::stringstream interstageVertexData;
    std::stringstream accessorsVS, accessorsGS, accessorsFS;

    // vertex varying
    TF_FOR_ALL (it, _vertexBufferBindings) {
        TfToken const &name = it->GetName();
        TfToken const &dataType = it->GetType();

        _EmitDeclaration(vertexInputs, name, dataType, *it);

        interstageVertexData << "  " << dataType
                             << " " << name << ";\n";

        // primvar accessors
        _EmitAccessor(accessorsVS, name, dataType, *it);

        _EmitStructAccessor(accessorsGS,  LoFiBufferTokens->inPrimvars,
                            name, dataType, 1, "localIndex");

        _EmitStructAccessor(accessorsFS,  LoFiBufferTokens->inPrimvars,
                            name, dataType, 1);

        // interstage plumbing
        _procVS << "  " << LoFiBufferTokens->outPrimvars << "." << name
                << " = " << name << ";\n";

        _procGS  << "  " << LoFiBufferTokens->outPrimvars << "." << name
                 << " = " << LoFiBufferTokens->inPrimvars << "[index]." << name << ";\n";
    }

    std::cout << "PROC VERTEX SHADER : " <<std::endl;
    std::cout << _procVS.str() <<std::endl;
    std::cout << "PROC GEOMETRY SHADER : " <<std::endl;
    std::cout << _procVS.str() <<std::endl;

      //// --------- facevarying data declaration ----------------
      //layout (std430, binding=?) buffer buffer0 {
      //    vec2 map1[];
      //};
      //layout (std430, binding=?) buffer buffer1 {
      //    float map2_u[];
      //};
//
      //// --------- geometry stage plumbing -------
      //out Primvars {
      //    ...
      //    vec2 map1;
      //    float map2_u;
      //} outPrimvars;
//
      //void ProcessPrimvars(int index) {
      //    outPrimvars.map1 = HdGet_map1(index);
      //    outPrimvars.map2_u = HdGet_map2_u(index);
      //}
//
      //// --------- fragment stage plumbing -------
      //in Primvars {
      //    ...
      //    vec2 map1;
      //    float map2_u;
      //} inPrimvars;
//
      //// --------- facevarying data accessors ----------
      //// in geometry shader (internal accessor)
      //vec2 HdGet_map1_Coarse(int localIndex) {
      //    int fvarIndex = GetFVarIndex(localIndex);
      //    return vec2(map1[fvarIndex]);
      //}
      //// in geometry shader (public accessor)
      //vec2 HdGet_map1(int localIndex) {
      //    int fvarIndex = GetFVarIndex(localIndex);
      //    return (HdGet_map1_Coarse(0) * ...);
      //}
      //// in fragment shader
      //vec2 HdGet_map1() {
      //    return inPrimvars.map1;
      //}


    // face varying
    std::stringstream fvarDeclarations;
    std::stringstream interstageFVarData;

/*
     if (hasGS) {
        // FVar primvars are emitted only by the GS.
        // If the GS isn't active, we can skip processing them.
        TF_FOR_ALL (it, _metaData.fvarData) {
            HdBinding binding = it->first;
            TfToken const &name = it->second.name;
            TfToken const &dataType = it->second.dataType;

            _EmitDeclaration(fvarDeclarations, name, dataType, binding);

            interstageFVarData << "  " << _GetPackedType(dataType, false)
                               << " " << name << ";\n";

            // primvar accessors (only in GS and FS)
            _EmitFVarGSAccessor(accessorsGS, name, dataType, binding,
                                _geometricShader->GetPrimitiveType());
            _EmitStructAccessor(accessorsFS, _tokens->inPrimvars, name, dataType,
                                1, NULL);

            _procGS << "  outPrimvars." << name 
                                        <<" = HdGet_" << name << "(index);\n";
        }
    }
    */
    if (!interstageVertexData.str().empty()) 
    {
        _genVS  << vertexInputs.str()
                << "out Primvars {\n"
                << interstageVertexData.str()
                << "} outPrimvars;\n"
                << accessorsVS.str();

        _genGS  << fvarDeclarations.str()
                << "in Primvars {\n"
                << interstageVertexData.str()
                << "} inPrimvars[HD_NUM_PRIMITIVE_VERTS];\n"
                << "out Primvars {\n"
                << interstageVertexData.str()
                << interstageFVarData.str()
                << "} outPrimvars;\n"
                << accessorsGS.str();

        _genFS  << "in Primvars {\n"
                << interstageVertexData.str()
                << interstageFVarData.str()
                << "} inPrimvars;\n"
                << accessorsFS.str();
    }

    std::cout << "GENERATE VERTEX SHADER CODE: " <<std::endl;
    std::cout << _genVS.str() <<std::endl;
    std::cout << "GENERATE GEOMETRY SHADER CODE: " <<std::endl;
    std::cout << _genGS.str() <<std::endl;
    std::cout << "GENERATE FRAGMENT SHADER CODE: " <<std::endl;
    std::cout << _genFS.str() <<std::endl;
    /*
    // ---------
    _genFS << "vec4 GetPatchCoord(int index);\n";
    _genFS << "vec4 GetPatchCoord() { return GetPatchCoord(0); }\n";

    _genGS << "vec4 GetPatchCoord(int localIndex);\n";

    // VS specific accessor for the "vertex drawing coordinate"
    // Even though we currently always plumb vertexCoord as part of the drawing
    // coordinate, we expect clients to use this accessor when querying the base
    // vertex offset for a draw call.
    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    _genVS << "int GetBaseVertexOffset() {\n";
    if (caps.shaderDrawParametersEnabled) {
        if (caps.glslVersion < 460) { // use ARB extension
            _genVS << "  return gl_BaseVertexARB;\n";
        } else {
            _genVS << "  return gl_BaseVertex;\n";
        }
    } else {
        _genVS << "  return GetDrawingCoord().vertexCoord;\n";
    }
    _genVS << "}\n";
    */
}

void LoFiCodeGen::GenerateMeshCode()
{
  _GeneratePrimvar(true);
  // vertex shader code
  _GenerateVersion(_genVS);
  _GenerateVersion(_genFS);

  // add input channels
  for(int i=0;i<_channels.size();++i)
  {
    TfToken channelType = LoFiGetVertexBufferChannelType(_channels[i]);
    _AddInputChannel(_genVS, _channels[i], i, channelType);
    _AddInputAttribute(_genFS, _channels[i], channelType);
  }
  
  // uniforms
  _AddUniform(_genVS, LoFiUniformTokens->model, LoFiGLTokens->mat4);
  _AddUniform(_genVS, LoFiUniformTokens->view, LoFiGLTokens->mat4);
  _AddUniform(_genVS, LoFiUniformTokens->projection, LoFiGLTokens->mat4);

  _AddUniform(_genFS, LoFiUniformTokens->model, LoFiGLTokens->mat4);
  _AddUniform(_genFS, LoFiUniformTokens->view, LoFiGLTokens->mat4);
  _AddUniform(_genFS, LoFiUniformTokens->projection, LoFiGLTokens->mat4);

  // add output attributes
  for(int i=0;i<_channels.size();++i)
  {
    TfToken channelType = LoFiGetVertexBufferChannelType(_channels[i]);
    _AddOutputAttribute(_genVS, _channels[i], channelType);
  }

  std::cout << "VERTEX SHADER : " << std::endl;
  std::cout << _genGS.str() << std::endl;

   std::cout << "FRAGMENT SHADER : " << std::endl;
  std::cout << _genFS.str() << std::endl;
}

/*
  "varying vec3 vertex_color;                               \n"
  "void main(){                                             \n"
  "    vertex_normal = (model * vec4(normal, 0.0)).xyz;     \n"
  "    vertex_color = color;                                \n"
  "    vec3 p = vec3(view * model * vec4(position,1.0));    \n"
  "    gl_Position = projection * vec4(p,1.0);              \n"
  "}"
};
*/

PXR_NAMESPACE_CLOSE_SCOPE