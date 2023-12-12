//
// Copyright 2016 Pixar
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
#ifndef PXR_H
#define PXR_H

/// \file pxr/pxr.h

#define PXR_MAJOR_VERSION 0
#define PXR_MINOR_VERSION 23
#define PXR_PATCH_VERSION 11

#define PXR_VERSION 2311

#define PXR_USE_NAMESPACES 1

#if PXR_USE_NAMESPACES

#define PXR_NS pxr
#define PXR_INTERNAL_NS Pixar
#define PXR_NS_GLOBAL ::PXR_NS

// A doc for the purposes of root level namespace documentation in Swift.
// NOTE: PXR_INTERNAL_NS has been unwrapped to Pixar on L.67
// to help the linter out.

/**
  # ``Pixar``

  ## Overview

  The **Pixar** namespace is the top level namespace for all **USD** modules.
  While you *can* use the **Pixar** namespace directly, it is recommended that you
  use the **Pixar** namespace as a **container** for these other Pixar modules, such
  as **Pixar.Tf** and **Pixar.Gf**, as these inner Pixar modules contain the swift
  wrappers for the c++ classes and functions, providing a more **swifty** interface
  to the underlying c++ code, which is far safer and more convenient to use than the
  c++ code directly. 

  - **Notice**: This is analogous to the **pxr** namespace in c++. In swift, an enum
  cannot "use" another enum to globally inject it into it's scope, so the longer form
  of the internal namespace `pxrInternal_v0_23__pxrReserved__` becomes the **pxr**
  namespace. Due to a Swift compiler bug when attempting to create a typealias to a
  c++ namespace (swift enum), Wabi renamed this namespace to **Pixar** for developer
  convenience, else users would have to use the longer form of the namespace in swift
  code.
 */
namespace Pixar { }

// The root level namespace for all source in the USD distribution.
namespace PXR_NS {
    using namespace PXR_INTERNAL_NS;
}

#define PXR_NAMESPACE_OPEN_SCOPE   namespace PXR_INTERNAL_NS {
#define PXR_NAMESPACE_CLOSE_SCOPE  }  
#define PXR_NAMESPACE_USING_DIRECTIVE using namespace PXR_NS;

#else

#define PXR_NS 
#define PXR_NS_GLOBAL 
#define PXR_NAMESPACE_OPEN_SCOPE   
#define PXR_NAMESPACE_CLOSE_SCOPE 
#define PXR_NAMESPACE_USING_DIRECTIVE

#endif // PXR_USE_NAMESPACES

#if 1
#define PXR_PYTHON_SUPPORT_ENABLED 1
#endif

#if 1
#define PXR_PREFER_SAFETY_OVER_SPEED 1
#endif

#endif //PXR_H