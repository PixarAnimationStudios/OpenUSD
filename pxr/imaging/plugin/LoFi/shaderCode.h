#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/plugin/LoFi/debugCodes.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/utils.h"


PXR_NAMESPACE_OPEN_SCOPE

class HioGlslfx;

class LoFiShaderCode {
public:
  LoFiShaderCode(const std::string& glslfxFile, bool glslfx);
  LoFiShaderCode(const std::string& filename):_filename(filename){};
  bool Parse();
  const std::string& GetSource(const TfToken& key);

  // non copyable
  LoFiShaderCode(const LoFiShaderCode& ) = delete;
  LoFiShaderCode(LoFiShaderCode&&) = delete;

  ~LoFiShaderCode(){};

private:
  void _GetName();
  void _AddSourceBlock(const TfToken& key, const std::string& block);
  std::string                       _filename;
  std::string                       _name;
  std::map<TfToken, std::string>    _sources;

  std::unique_ptr<HioGlslfx>        _glslfx;
  size_t                            _hash;

};

typedef std::shared_ptr<LoFiShaderCode> LoFiShaderCodeSharedPtr;
typedef std::vector<LoFiShaderCodeSharedPtr> LoFiShaderCodeSharedPtrList;
  

PXR_NAMESPACE_CLOSE_SCOPE
