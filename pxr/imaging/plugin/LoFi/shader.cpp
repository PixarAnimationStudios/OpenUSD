#include <iostream>
#include <fstream>
#include <sstream>
#include "pxr/base/arch/hash.h"
#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

void LoFiGLSLShader::_ComputeHash()
{
  _hash = ArchHash(_code.c_str(), _code.size(), 0);;
}

void LoFiGLSLShader::OutputInfoLog()
{
  char buffer[512];
  glGetShaderInfoLog(_id, 512, NULL, &buffer[0]);
 
  std::cerr << "[LoFi][Compile GLSL shader] Info log : " << 
    (std::string)buffer << std::endl;
}

void LoFiGLSLShader::Load(const char* filename, GLenum type)
{
  std::ifstream file;
  file.open(filename, std::ios::in);
  if(file.is_open())
  {
    if(file.good())
    {
      file.seekg(0,std::ios::end);
      unsigned long len = file.tellg();
      file.seekg(std::ios::beg);

      if(len>0)
      {
        _code.assign((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
        _code += '\0';
      }
      else std::cerr << "[LoFi][Load shader from file] File is empty : " << 
        filename << std::endl;
    }
    else std::cerr << "[LoFi][Load shader from file] File is invalid : " <<
      filename << std::endl;
    
  }
  else std::cerr << "[LoFi][Load shader from file] Failure open file : " << 
    filename << std::endl;

  _type = type;
}

void LoFiGLSLShader::Set(const char* code, GLenum type)
{
  _code = code;
  _type = type;
}

void LoFiGLSLShader::Compile()
{
  _id = glCreateShader(_type);
  const char* code[1] = { _code.c_str() };
  glShaderSource(_id, 1, code, NULL);
  glCompileShader(_id);
  
  GLint status;
  glGetShaderiv(_id,GL_COMPILE_STATUS,&status);
  if(/*TfDebug::IsEnabled(LOFI_SHADER) && */status != GL_TRUE)
  {
    std::cerr << "[LoFi][Compile Shader] Fail compiling code: \n" << 
      _code << std::endl;

    OutputInfoLog();
  }
  _ComputeHash();
}

void LoFiGLSLProgram::_ComputeHash()
{
  _hash = 0;
  if(_vert)boost::hash_combine(_hash, _vert->Hash());
  if(_geom)boost::hash_combine(_hash, _geom->Hash());
  if(_frag)boost::hash_combine(_hash, _frag->Hash());
}

void LoFiGLSLProgram::_Build()
{  
  _pgm = glCreateProgram();
  
  if(_vert)
  {
    _vert->Compile();
    glAttachShader(_pgm,_vert->Get());
  }

  if(_geom)
  {
    _geom->Compile();
    glAttachShader(_pgm,_geom->Get());
  }
  
  if(_frag)
  {
    _frag->Compile();
    glAttachShader(_pgm,_frag->Get());
  }
  
  glBindAttribLocation(_pgm,CHANNEL_POSITION,"position");
  glBindAttribLocation(_pgm,CHANNEL_COLOR,"color");
  glLinkProgram(_pgm);  
  glUseProgram(_pgm);
  
  GLint status = 0;
  glGetProgramiv(_pgm, GL_LINK_STATUS, (int *)&status);
  if(status == GL_TRUE) 
  {
    std::cerr << "[LoFi][Build GLSL program] Success : " << _name <<std::endl;
  } 
  else 
  {
    glDeleteProgram(_pgm);
    std::cerr << "[LoFi][Build GLSL program] Fail : " << _name << std::endl;
    OutputInfoLog();
  }
  glUseProgram(0);
  _ComputeHash();
}

void LoFiGLSLProgram::Build(const char* name, const char* vertex, 
  const char* fragment)
{
  _name = name;
  LoFiGLSLShader vertShader;
  vertShader.Set(vertex, GL_VERTEX_SHADER);
  _vert = &vertShader;

  _geom = NULL;

  LoFiGLSLShader fragShader;
  fragShader.Set(fragment, GL_FRAGMENT_SHADER);
  _frag = &fragShader;

  _Build();
}

void LoFiGLSLProgram::Build(const char* name, const char* vertex, 
  const char* geom, const char* fragment)
{
  _name = name;
  LoFiGLSLShader vertShader;
  vertShader.Set(vertex, GL_VERTEX_SHADER);
  _vert = &vertShader;

  LoFiGLSLShader geomShader;
  geomShader.Set(geom, GL_GEOMETRY_SHADER);
  _geom = &geomShader;

  LoFiGLSLShader fragShader;
  fragShader.Set(fragment, GL_FRAGMENT_SHADER);
  _frag = &fragShader;

  _Build();
}

void LoFiGLSLProgram::Build(const char* name, LoFiGLSLShader* vertex, 
  LoFiGLSLShader* fragment)
{
  _name = name;
  _vert = vertex;
  _frag = fragment;

  _Build();
}

void LoFiGLSLProgram::Build(const char* name, LoFiGLSLShader* vertex, 
  LoFiGLSLShader* geom, LoFiGLSLShader* fragment)
{
  _name = name;
  _vert = vertex;
  _geom = geom;
  _frag = fragment;

  _Build();
}

void LoFiGLSLProgram::OutputInfoLog()
{
  char buffer[1024];
  GLsizei l;
  glGetProgramInfoLog(_pgm,1024,&l,&buffer[0]);
  std::cerr << "[LoFi][Build GLSL program] Info log : " << 
    (std::string)buffer << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
