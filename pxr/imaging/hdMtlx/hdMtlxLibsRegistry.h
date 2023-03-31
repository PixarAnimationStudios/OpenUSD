//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HD_MTLX_HDMTLXLIBSREGISTRY_H
#define PXR_IMAGING_HD_MTLX_HDMTLXLIBSREGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include <boost/noncopyable.hpp>

#include "pxr/imaging/hdMtlx/api.h"

#include <MaterialXCore/Library.h>
#include <MaterialXFormat/File.h>
#include <cstddef>

MATERIALX_NAMESPACE_BEGIN
    class FileSearchPath;
    using DocumentPtr = std::shared_ptr<class Document>;
MATERIALX_NAMESPACE_END

PXR_NAMESPACE_OPEN_SCOPE
namespace mx = MaterialX;
class MtlxLibsRegistry : boost::noncopyable
{
	public:
                 HDMTLX_API
		 static MtlxLibsRegistry& GetInstance() { 
			 return TfSingleton<MtlxLibsRegistry>::GetInstance();
		 };
	 
                 HDMTLX_API
		 MaterialX::DocumentPtr stdLibraries() const { return _stdLibraries; };
		 HDMTLX_API
		 const MaterialX::FileSearchPath& searchPaths() const {return _searchPaths;};
	 private:
		  
		 MtlxLibsRegistry();
		 ~MtlxLibsRegistry();
		 friend class TfSingleton<MtlxLibsRegistry>;
	private:
		 MaterialX::FileSearchPath _searchPaths;
		 MaterialX::DocumentPtr _stdLibraries = NULL;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
