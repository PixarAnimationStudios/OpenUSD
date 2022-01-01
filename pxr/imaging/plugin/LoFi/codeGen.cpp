//
// Copyright 2020 benmalartre
//
// unlicensed
//

#include "pxr/base/tf/token.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/plugin/LoFi/codeGen.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_double, "double"))
    ((_float, "float"))
    ((_int, "int"))
    (ivec2)
    (ivec3)
    (ivec4)
    (vec2)
    (vec3)
    (vec4)
    (dvec2)
    (dvec3)
    (dvec4)
    (mat3)
    (mat4)
    (dmat3)
    (dmat4)
    (isamplerBuffer)
    (samplerBuffer)
);

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

static std::string LoFiGetVertexBufferChannelName(LoFiVertexBufferChannel channel)
{
   switch(channel)
   {
    case CHANNEL_POSITION:
      return "position";
    case CHANNEL_NORMAL:
      return "normal";
    case CHANNEL_TANGENT:
      return "tangent";
    case CHANNEL_COLOR:
      return "color";
    case CHANNEL_UV:
      return "uvs";
    case CHANNEL_WIDTH:
      return "width";
    case CHANNEL_ID:
      return "id";
    case CHANNEL_SCALE:
      return "scale";
    case CHANNEL_SHAPE_POSITION:
      return "shape_position";
    case CHANNEL_SHAPE_NORMAL:
      return "shape_normal";
    case CHANNEL_SHAPE_UV:
      return "shape_position";
    case CHANNEL_SHAPE_COLOR:
      return "shape_position";
    default:
      return "channel_unknown";
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
      return _tokens->vec3;
    case CHANNEL_UV:
    case CHANNEL_SHAPE_UV:
      return _tokens->vec2;
    case CHANNEL_WIDTH:
      return _tokens->_float;
    case CHANNEL_ID:
      return _tokens->_int;
    default:
      return _tokens->vec3;
   }
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
  if(_glslVersion >= 330)
    ss << "layout(location=" << index <<") in " << type.GetText() << " " <<
      LoFiGetVertexBufferChannelName(channel) << ";\n";
  else
    ss << "attribute " << type.GetText() << " " <<
      LoFiGetVertexBufferChannelName(channel) << ";\n";;
}

void LoFiCodeGen::_AddInputUniform(std::stringstream& ss, const std::string& name,
  TfToken& type)
{
  ss << "uniform " << type.GetText() << " " << name << ";\n";
}

void LoFiCodeGen::_AddOutputAttribute(std::stringstream& ss, 
  LoFiVertexBufferChannel channel, TfToken& type)
{
  std::string name = "vertex_"+LoFiGetVertexBufferChannelName(channel);
  if(_glslVersion >= 330)
    ss << "out " << type.GetText() <<" " << name << ";\n";
  else
    ss << "varying " << type.GetText() <<" " << name << ";\n";
}

void LoFiCodeGen::_AddInputAttribute(std::stringstream& ss, 
  LoFiVertexBufferChannel channel, TfToken& type)
{
  std::string name = "vertex_"+LoFiGetVertexBufferChannelName(channel);
  if(_glslVersion >= 330)
    ss << "in " << type.GetText() <<" " << name << ";\n";
  else
    ss << "varying " << type.GetText() <<" " << name << ";\n";
}

void LoFiCodeGen::GenerateMeshCode()
{
  // vertex shader code
  _GenerateVersion(_vertex);
  _GenerateVersion(_fragment);

  // add input channels
  for(int i=0;i<_channels.size();++i)
  {
    TfToken channelType = LoFiGetVertexBufferChannelType(_channels[i]);
    _AddInputChannel(_vertex, _channels[i], i, channelType);
    _AddInputAttribute(_fragment, _channels[i], channelType);
  }

  // add output attributes
  for(int i=0;i<_channels.size();++i)
  {
    TfToken channelType = LoFiGetVertexBufferChannelType(_channels[i]);
    _AddOutputAttribute(_vertex, _channels[i], channelType);
  }

  std::cout << "VERTEX SHADER : " << std::endl;
  std::cout << _vertex.str() << std::endl;

   std::cout << "FRAGMENT SHADER : " << std::endl;
  std::cout << _fragment.str() << std::endl;
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