#include <iostream>
#include <fstream>
#include <sstream>
#include "pxr/base/arch/hash.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"


PXR_NAMESPACE_OPEN_SCOPE

LoFiShaderCode::LoFiShaderCode(const std::string& glslfxFilename, bool glslfx)
{
  std::stringstream ss(glslfxFilename);
  _glslfx.reset(new HioGlslfx(ss));
  boost::hash_combine(_hash, _glslfx->GetHash());
  //boost::hash_combine(_hash, cullingPass);
  //boost::hash_combine(_hash, primType);
}

void 
LoFiShaderCode::_AddSourceBlock(const TfToken& key, const std::string& source)
{
  if(!key.IsEmpty() && source.length())
    _sources[key] = source;
}

void 
LoFiShaderCode::_GetName()
{
  _name = _filename;
  const size_t lastSlashIndex = _name.find_last_of("\\/");
  if (std::string::npos != lastSlashIndex)
  {
      _name.erase(0, lastSlashIndex + 1);
  }

  const size_t periodIndex = _name.rfind('.');
  if (std::string::npos != periodIndex)
  {
      _name.erase(periodIndex);
  }
}

const std::string& 
LoFiShaderCode::GetSource(const TfToken& key)
{
  auto it = _sources.find(key);
  if(it != _sources.end())
    return it->second;
  else
  {
    TF_CODING_ERROR("[LoFiShaderCode] Can't find source for key %s", 
            key.GetText());
    return std::string();
  }
}

bool 
LoFiShaderCode::Parse()
{
  _GetName();
  std::ifstream file;
  file.open(_filename, std::ios::in);
  bool valid = false;
  if(file.is_open())
  {
    if(file.good())
    {
      file.seekg(0,std::ios::end);
      unsigned long len = file.tellg();
      file.seekg(std::ios::beg);

      if(len>0)
      {
        std::string line;
        std::string source;
        std::string key;
        while( std::getline( file, line ) ) 
        {
          // empty line/comment line, skip it
          if(!line.length() || line.rfind("//", 0, 2) == 0)continue;

          // new source , append previous one to the map
          if(line.rfind("=== ", 0, 4) == 0)
          {
            _AddSourceBlock(TfToken(key), source);
            source.clear();
          }
          // get source name
          else if(line.rfind("== ", 0, 3) == 0)
          {
            key = line.substr(3, line.length());
            size_t end = key.find_last_not_of(' ');
            if(end != std::string::npos)key = key.substr(0, end+1);
          }
          // append line to the source
          else source += line+'\n'; 

          valid = true;
        }
        _AddSourceBlock(TfToken(key), source);
        std::cout << "#######################################################" << std::endl;
        std::cout << " SHADER CODE : " << _name << std::endl;
        std::cout << "#######################################################" << std::endl;
        TF_FOR_ALL(it, _sources)
        {
          std::cout << "KEY : #" << it->first << "#" << std::endl;
          std::cout << it->second << std::endl;
          std::cout << "#######################################################" << std::endl;
        }
      }
      else TF_CODING_ERROR("[LoFiShaderCode] File is empty : %s\n", _filename.c_str());
    }
    else TF_CODING_ERROR("[LoFiShaderCode] File is invalid : %s\n", _filename.c_str());
  }
  else TF_CODING_ERROR("[LoFiShaderCode] Failure open file : %s\n", _filename.c_str());

  return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE
