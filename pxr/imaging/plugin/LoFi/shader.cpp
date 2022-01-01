#include <iostream>
#include <fstream>
#include <sstream>
#include "pxr/imaging/plugin/LoFi/shader.h"


PXR_NAMESPACE_OPEN_SCOPE

void GLSLShader::OutputInfoLog()
{
  char buffer[512];
  glGetShaderInfoLog(_id, 512, NULL, &buffer[0]);
  std::cout << "COMPILE : " << (std::string)buffer << std::endl;
}

void GLSLShader::Load(const char* filename, GLenum type)
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
      else std::cout << "[LoFi] File is empty : " << filename << std::endl;
    }
    else std::cout << "[LoFi] File is invalid : " << filename << std::endl;
    
  }
  else std::cout << "[LoFi] Fail open file : " << filename << std::endl;

  _type = type;
}

void GLSLShader::Set(const char* code, GLenum type)
{
  _code = code;
  _type = type;
}

void GLSLShader::Compile()
{
  std::cout << "COMPILE SHADER CODE FROM FILE " << std::endl;
  std::cout << _code << std::endl;
  std::cout << "-------------------------------" << std::endl;
  std::cout << std::flush;

  _id = glCreateShader(_type);
  const char* code[1] = { _code.c_str() };
  glShaderSource(_id, 1, code, NULL);
  glCompileShader(_id);
  
  GLint status;
  glGetShaderiv(_id,GL_COMPILE_STATUS,&status);
  if(status == GL_TRUE)
    std::cout << "[GLSLCreateShader] Success Compiling Shader !"<<std::endl;
  else
  {
    std::cout << "[GLSLCreateShader] Fail Compiling Shader !" <<std::endl;
    // Output Info Log
    OutputInfoLog();
  }
}

void GLSLProgram::_Build()
{  
  _pgm = glCreateProgram();
  
  if(_vert)
  {
    _vert->Compile();
    glAttachShader(_pgm,_vert->Get());
  }

  if(_geom && _geom->Get())
  {
    _geom->Compile();
    glAttachShader(_pgm,_geom->Get());
  }
  
  if(_frag)
  {
    _frag->Compile();
    glAttachShader(_pgm,_frag->Get());
  }
  
  glBindAttribLocation(_pgm,0,"position");
  glLinkProgram(_pgm);  
  glUseProgram(_pgm);
  
  GLint status = 0;
  glGetProgramiv(_pgm, GL_LINK_STATUS, (int *)&status);
  if(status == GL_TRUE) {
    std::cout << "[GLSLCreateShader] Success Build Program !"<<std::endl;
  } else {
    glDeleteProgram(_pgm);
    std::cout << "[GLSLCreateShader] Fail Build Program !" <<std::endl;
    OutputInfoLog();
  }
  glUseProgram(0);
}

void GLSLProgram::Build(const char* name, const char** vertex, const char** fragment)
{
  _name = name;
  GLSLShader vertShader;
  vertShader.Set(vertex[0], GL_VERTEX_SHADER);
  _vert = &vertShader;

  _geom = NULL;

  GLSLShader fragShader;
  fragShader.Set(fragment[0], GL_FRAGMENT_SHADER);
  _frag = &fragShader;

  _Build();
}

void GLSLProgram::Build(const char* name, const char** vertex, const char** geom, const char** fragment)
{
  _name = name;
  GLSLShader vertShader;
  vertShader.Set(vertex[0], GL_VERTEX_SHADER);
  _vert = &vertShader;

  GLSLShader geomShader;
  geomShader.Set(geom[0], GL_GEOMETRY_SHADER);
  _geom = &geomShader;

  GLSLShader fragShader;
  fragShader.Set(fragment[0], GL_FRAGMENT_SHADER);
  _frag = &fragShader;

  _Build();
}

void GLSLProgram::Build(const char* name, GLSLShader* vertex, GLSLShader* fragment)
{
  _name = name;
  _vert = vertex;
  _frag = fragment;

  _Build();
}

void GLSLProgram::Build(const char* name, GLSLShader* vertex, GLSLShader* geom, GLSLShader* fragment)
{
  _name = name;
  _vert = vertex;
  _geom = geom;
  _frag = fragment;

  _Build();
}

void GLSLProgram::OutputInfoLog()
{
  char buffer[1024];
  GLsizei l;
  glGetProgramInfoLog(_pgm,1024,&l,&buffer[0]);
  std::cout << _name << ":" << (std::string) buffer << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
