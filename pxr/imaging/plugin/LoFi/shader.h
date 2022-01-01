#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <pxr/imaging/glf/glew.h>


PXR_NAMESPACE_OPEN_SCOPE

// check opengl error
//----------------------------------------------------------------------------
static bool 
GLCheckError(const char* message)
{
  GLenum err = glGetError();
  if(err)
  {
    while(err != GL_NO_ERROR)
    {
      switch(err)
      {
        case GL_INVALID_OPERATION:
          std::cout << "[OpenGL Error] " << message 
            << " INVALID_OPERATION" << std::endl;
          break;
        case GL_INVALID_ENUM:
          std::cout << "[OpenGL Error] " << message 
            << " INVALID_ENUM" << std::endl;
          break;
        case GL_INVALID_VALUE:
          std::cout << "[OpenGL Error] " << message 
            << " INVALID_VALUE" << std::endl;
          break;
        case GL_OUT_OF_MEMORY:
          std::cout << "[OpenGL Error] " << message 
            << " OUT_OF_MEMORY" << std::endl;
          break;
        case GL_STACK_UNDERFLOW:
          std::cout << "[OpenGL Error] " << message 
            << "STACK_UNDERFLOW" <<std::endl;
          break;
        case GL_STACK_OVERFLOW:
          std::cout << "[OpenGL Error] " << message 
            << "STACK_OVERFLOW" << std::endl;
          break;
        default:
          std::cout << "[OpenGL Error] " << message 
            << " UNKNOWN_ERROR" << std::endl;
          break;
      }
      err = glGetError();
    }
    return true;
  }
  return false;
}

static void 
GLFlushError()
{
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR){};
}

enum GLSLShaderType
{
  SHADER_VERTEX,
  SHADER_GEOMETRY,
  SHADER_FRAGMENT
};
    
class GLSLShader{
public:
  GLSLShader():_id(0),_code(""){};
  GLSLShader(const GLSLShader& ) = delete;
  GLSLShader(GLSLShader&&) = delete;

  ~GLSLShader()
  {
    if(_id)glDeleteShader(_id);
  }
  void Load(const char* filename, GLenum t);
  void Compile();
  void OutputInfoLog();
  GLuint Get(){return _id;};
  void Set(const char* code, GLenum type);
private:
  std::string         _code;
  GLenum              _type;
  GLuint              _id;
};
    
class GLSLProgram
{
friend GLSLShader;
public:
  GLSLProgram():_vert(NULL),_geom(NULL),_frag(NULL),_pgm(0){};
  GLSLProgram(const GLSLProgram&) = delete;
  GLSLProgram(GLSLProgram&&) = delete;

  ~GLSLProgram()
  {
    if(_pgm)glDeleteProgram(_pgm);
  }
  void _Build();
  void Build(const char* name, const char** s_vert, const char** s_frag);
  void Build(const char* name, const char** s_vert, const char** s_geom, const char** s_frag);
  void Build(const char* name, GLSLShader* vertex, GLSLShader* fragment);
  void Build(const char* name, GLSLShader* vertex, GLSLShader* geom, GLSLShader* fragment);
  void OutputInfoLog();
  GLuint Get(){return _pgm;};
private:
  GLSLShader*         _vert;
  GLSLShader*         _geom;
  GLSLShader*         _frag;
  GLuint              _pgm;
  std::string         _name; 
};

PXR_NAMESPACE_CLOSE_SCOPE
