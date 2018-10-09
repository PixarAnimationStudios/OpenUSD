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
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <boost/make_shared.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdRenderDelegate>();
}

//
// WORKAROUND: As these classes are pure interface classes, they do not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
HdRenderParam::~HdRenderParam() = default;
HdRenderDelegate::~HdRenderDelegate() = default;

HdRenderPassStateSharedPtr
HdRenderDelegate::CreateRenderPassState() const
{
    return boost::make_shared<HdRenderPassState>();
}


TfToken
HdRenderDelegate::GetMaterialBindingPurpose() const
{
    return HdTokens->preview;
}

TfTokenVector 
HdRenderDelegate::GetShaderSourceTypes() const
{
    return TfTokenVector();
}

TfToken 
HdRenderDelegate::GetMaterialNetworkSelector() const
{
    return TfToken();
}

HdAovDescriptor
HdRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    return HdAovDescriptor();
}

PXR_NAMESPACE_CLOSE_SCOPE

