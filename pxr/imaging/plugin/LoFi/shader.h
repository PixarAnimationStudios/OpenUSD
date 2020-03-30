#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <boost/functional/hash.hpp>

#include "pxr/imaging/plugin/LoFi/debugCodes.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/glf/glew.h"


PXR_NAMESPACE_OPEN_SCOPE


enum LoFiGLSLShaderType
{
  SHADER_VERTEX,
  SHADER_GEOMETRY,
  SHADER_FRAGMENT
};
    
class LoFiGLSLShader{
public:
  LoFiGLSLShader():_id(0),_code(""){};
  LoFiGLSLShader(const LoFiGLSLShader& ) = delete;
  LoFiGLSLShader(LoFiGLSLShader&&) = delete;

  ~LoFiGLSLShader()
  {
    if(_id)glDeleteShader(_id);
  }
  void Load(const char* filename, GLenum t);
  void Compile();
  void OutputInfoLog();
  GLuint Get(){return _id;};
  void Set(const char* code, GLenum type);
  void _ComputeHash();
  size_t Hash(){return _hash;};

private:
  std::string         _code;
  GLenum              _type;
  GLuint              _id;
  size_t              _hash;
};
    
class LoFiGLSLProgram
{
friend LoFiGLSLShader;
public:
  // constructor (empty program)
  LoFiGLSLProgram():_vert(NULL),_geom(NULL),_frag(NULL),_pgm(0){};
  
  // destructor
  ~LoFiGLSLProgram()
  {
    if(_pgm)glDeleteProgram(_pgm);
  }

  /// internal build of the glsl program
  void _Build();

  /// build glsl program from vertex and fragment code
  void Build(const char* name, const char** s_vert, const char** s_frag);

  /// build glsl program from vertex, geometry and fragment code
  void Build(const char* name, const char** s_vert, const char** s_geom, 
    const char** s_frag);

  /// build glsl program from vertex and fragment LoFiGLSLShader objects
  void Build(const char* name, LoFiGLSLShader* vertex, 
    LoFiGLSLShader* fragment);

  /// build glsl program from vertex, geometry and fragment LoFiGLSLShader 
  /// objects
  void Build(const char* name, LoFiGLSLShader* vertex,
    LoFiGLSLShader* geom, LoFiGLSLShader* fragment);

  /// output build program info log
  void OutputInfoLog();

  /// compute hash
  void _ComputeHash();
  size_t Hash(){return _hash;};

  /// get GL program id
  GLuint Get(){return _pgm;};

  /// this object is non-copyable
  //LoFiGLSLProgram(const LoFiGLSLProgram&) = delete;
  //LoFiGLSLProgram(LoFiGLSLProgram&&) = delete;

private:
  LoFiGLSLShader*         _vert;
  LoFiGLSLShader*         _geom;
  LoFiGLSLShader*         _frag;
  GLuint                  _pgm;
  std::string             _name; 
  size_t                  _hash;
};

PXR_NAMESPACE_CLOSE_SCOPE
