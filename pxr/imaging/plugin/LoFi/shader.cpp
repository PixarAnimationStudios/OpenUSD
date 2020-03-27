#include <iostream>
#include <fstream>
#include "pxr/imaging/plugin/LoFi/shader.h"


PXR_NAMESPACE_OPEN_SCOPE

void GLSLShader::OutputInfoLog()
{
  char buffer[512];
  glGetShaderInfoLog(_shader, 512, NULL, &buffer[0]);
  std::cout << "COMPILE : " << (std::string)buffer << std::endl;
}

void GLSLShader::Load(const char* filename)
{
  std::fstream file;
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
        std::string code;
        _code = (GLchar*) new char[len+1];
        code.assign((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
        memcpy(_code, code.c_str(), len * sizeof(char));
        
        _code[len] = 0;
      }
      else std::cout << "[LoFi] File is empty : " << filename << std::endl;
    }
    else std::cout << "[LoFi] File is invalid : " << filename << std::endl;
    file.close();   
  }
  else std::cout << "[LoFi] Fail open file : " << filename << std::endl;
}

void GLSLShader::Set(const char* code)
{
  unsigned long len = strlen(code);
  _code = (GLchar*) new char[len+1];
  _code[len] = 0;
  memcpy(_code, code, len);
}

void GLSLShader::Compile(const char* code, GLenum type)
{
  _shader = glCreateShader(type);

  glShaderSource(_shader,1,(GLchar**)code,NULL);
  glCompileShader(_shader);
  
  GLint status;
  glGetShaderiv(_shader,GL_COMPILE_STATUS,&status);
  if(status)
    std::cout << "[GLSLCreateShader] Success Compiling Shader !"<<std::endl;
  else
  {
    std::cout << "[GLSLCreateShader] Fail Compiling Shader !" <<std::endl;
  
    // Output Info Log
    OutputInfoLog();
  }
}

void GLSLShader::Compile(GLenum type)
{
  _shader = glCreateShader(type);
  
  glShaderSource(_shader,1,(GLchar**)&_code,NULL);
  glCompileShader(_shader);
  
  GLint status;
  glGetShaderiv(_shader,GL_COMPILE_STATUS,&status);
  if(status)
    std::cout << "[GLSLCreateShader] Success Compiling Shader !"<<std::endl;
  else
  {
    std::cout << "[GLSLCreateShader] Fail Compiling Shader !"<<std::endl;
    OutputInfoLog();
  }
}

void GLSLProgram::_Build()
{  
  _pgm = glCreateProgram();
  GLCheckError("Create Program : ");
  
  if(_vert)
  {
    glAttachShader(_pgm,_vert->Get());
    GLCheckError("Attach Vertex Shader ");
  }

  if(_geom && _geom->Get())
  {
    glAttachShader(_pgm,_geom->Get());
    GLCheckError("Attach Geometry Shader ");
  }
  
  if(_frag)
  {
    glAttachShader(_pgm,_frag->Get());
    GLCheckError("Attach Fragment Shader ");
  }
  
  glBindAttribLocation(_pgm,0,"position");
  
  glLinkProgram(_pgm);
  GLCheckError("Link Program : ");
  
  glUseProgram(_pgm);
  GLCheckError("Use Program : ");
  
  OutputInfoLog();

}

void GLSLProgram::Build(const char* name, const char* vertex, const char* fragment)
{
  _name = name;
  _vert = new GLSLShader();
  _vert->Set(vertex);
  _vert->Compile(GL_VERTEX_SHADER);
  _ownVertexShader = true;
  GLCheckError("Compile Vertex Shader : ");

  _frag = new GLSLShader();
  _frag->Set(fragment);
  _frag->Compile(GL_FRAGMENT_SHADER);
  _ownFragmentShader = true;
  GLCheckError("Compile Fragment Shader : ");

  _Build();
}

void GLSLProgram::Build(const char* name, const char* vertex, const char* geom, const char* fragment)
{
  _name = name;
  _vert = new GLSLShader();
  _vert->Set(vertex);
  _vert->Compile(GL_VERTEX_SHADER);
  _ownVertexShader = true;
  GLCheckError("Compile Vertex Shader : ");

  _geom = new GLSLShader();
  _geom->Set(geom);
  _geom->Compile(GL_GEOMETRY_SHADER);
  _ownGeometryShader = true;
  GLCheckError("Compile Geometry Shader : ");

  _frag = new GLSLShader();
  _frag->Set(fragment);
  _frag->Compile(GL_FRAGMENT_SHADER);
  _ownFragmentShader = true;
  GLCheckError("Compile Fragment Shader : ");

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
