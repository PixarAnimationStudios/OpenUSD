//
// Copyright 2017 Pixar
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

#include "pxr/imaging/hd/extComputationContextInternal.h"

PXR_NAMESPACE_OPEN_SCOPE

Hd_ExtComputationContextInternal::Hd_ExtComputationContextInternal()
 : HdExtComputationContext()
 , m_inputs()
 , m_outputs()
 , m_compuationError(false){
}

Hd_ExtComputationContextInternal::~Hd_ExtComputationContextInternal()
{
}


void
Hd_ExtComputationContextInternal::SetInputValue(const TfToken &name,
                                                 const VtValue &input)
{
    m_inputs.emplace(name, input);
}

bool
Hd_ExtComputationContextInternal::GetOutputValue(const TfToken &name,
                                                 VtValue *output) const
{
    ValueMap::const_iterator it = m_outputs.find(name);

    if (it == m_outputs.end()) {
       TF_WARN("output not specified: %s", name.GetText());

       return false;
    }

    *output = it->second;
    return true;
}

bool
Hd_ExtComputationContextInternal::HasComputationError()
{
    return m_compuationError;
}


void
Hd_ExtComputationContextInternal::RaiseComputationError()
{
    m_compuationError = true;
}


const VtValue &
Hd_ExtComputationContextInternal::GetInputValue(const TfToken &name) const
{
    ValueMap::const_iterator it = m_inputs.find(name);

    if (it == m_inputs.end()) {
       TF_CODING_ERROR("Asking for invalid input %s", name.GetText());
       static VtValue ERROR_VT_VALUE;

       return ERROR_VT_VALUE;
    }

    return it->second;
}

const VtValue *
Hd_ExtComputationContextInternal::GetOptionalInputValuePtr(
                                                      const TfToken &name) const
{
    ValueMap::const_iterator it = m_inputs.find(name);

    if (it == m_inputs.end()) {
        return nullptr;
    }

    return &(it->second);
}

void
Hd_ExtComputationContextInternal::SetOutputValue(const TfToken &name,
                                        const VtValue &input)
{
    m_outputs.emplace(name, input);
}

PXR_NAMESPACE_CLOSE_SCOPE
