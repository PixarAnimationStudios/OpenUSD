#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/glslfxConfig.h"
#include "pxr/imaging/plugin/LoFi/debugCodes.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/utils.h"


PXR_NAMESPACE_OPEN_SCOPE

class HioGlslfx;

class LoFiShaderCode {
public:
  LoFiShaderCode(const TfToken& filename);

  std::string
  LoFiShaderCode::GetSource(const TfToken& key);

  // non copyable
  LoFiShaderCode(const LoFiShaderCode& ) = delete;
  LoFiShaderCode(LoFiShaderCode&&) = delete;

  ~LoFiShaderCode(){};

private:
  TfToken                           _filename;
  std::string                       _name;

  std::unique_ptr<HioGlslfx>        _glslfx;
  size_t                            _hash;

};

typedef std::shared_ptr<LoFiShaderCode> LoFiShaderCodeSharedPtr;
typedef std::vector<LoFiShaderCodeSharedPtr> LoFiShaderCodeSharedPtrList;
  

PXR_NAMESPACE_CLOSE_SCOPE
