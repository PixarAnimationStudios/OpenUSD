#pragma once

#include <stdio.h>
#include <string.h>
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

enum GLSLShaderType
{
  SHADER_VERTEX,
  SHADER_GEOMETRY,
  SHADER_FRAGMENT
};
    
class GLSLShader{
public:
  GLSLShader():_shader(0),_code(NULL){};
  GLSLShader(const GLSLShader& ) = delete;
  GLSLShader(GLSLShader&&) = delete;

  ~GLSLShader()
  {
    if(_shader)glDeleteShader(_shader);
    if(_code)delete [] _code;
  }
  void Load(const char* filename);
  void Compile(const char* c, GLenum t);
  void Compile(GLenum t);
  void OutputInfoLog();
  GLuint Get(){return _shader;};
  void Set(const char* code);
private:
  std::string _path;
  GLchar* _code;
  GLenum _type;
  GLuint _shader;
};
    
class GLSLProgram
{
friend GLSLShader;
public:
  GLSLProgram():_vert(),_geom(),_frag(),_pgm(0),_ownVertexShader(false),
    _ownGeometryShader(false),_ownFragmentShader(false){};
  GLSLProgram(const GLSLProgram&) = delete;
  GLSLProgram(GLSLProgram&&) = delete;

  ~GLSLProgram()
  {
    if(_ownFragmentShader && _frag)delete _frag;
    if(_ownGeometryShader && _geom)delete _geom;
    if(_ownVertexShader && _vert)delete _vert;
  }
  void _Build();
  void Build(const char* name, const char* s_vert="", const char* s_frag="");
  void Build(const char* name, const char* s_vert="", const char* s_geom="", const char* s_frag="");
  void Build(const char* name, GLSLShader* vertex, GLSLShader* fragment);
  void Build(const char* name, GLSLShader* vertex, GLSLShader* geom, GLSLShader* fragment);
  void OutputInfoLog();
  GLuint Get(){return _pgm;};
private:
  GLSLShader* _vert;
  bool _ownVertexShader;
  GLSLShader* _geom;
  bool _ownGeometryShader;
  GLSLShader* _frag;
  bool _ownFragmentShader;
  GLuint _pgm;
  std::string _name; 
};

PXR_NAMESPACE_CLOSE_SCOPE
