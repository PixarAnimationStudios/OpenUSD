#include <iostream>
#include <fstream>
#include <sstream>
#include "pxr/base/arch/hash.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/plugin/LoFi/shaderCode.h"


PXR_NAMESPACE_OPEN_SCOPE


LoFiShaderCode::LoFiShaderCode(const TfToken& filename)
: _filename(filename)
{
  _glslfx.reset(new HioGlslfx(_filename));
  boost::hash_combine(_hash, _glslfx->GetHash());

  std::string reason;
  if(!_glslfx->IsValid(&reason))
  {
    std::cout << _glslfx->GetSource(LoFiShaderTokens->vertex) << std::endl;
    std::cout << _glslfx->GetSource(LoFiShaderTokens->geometry) << std::endl;
    std::cout << _glslfx->GetSource(LoFiShaderTokens->fragment) << std::endl;
  }
}

std::string
LoFiShaderCode::GetSource(const TfToken& key)
{
  return _glslfx->GetSource(key);
}


PXR_NAMESPACE_CLOSE_SCOPE
