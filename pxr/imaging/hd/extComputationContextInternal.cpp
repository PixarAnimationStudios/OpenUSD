//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/extComputationContextInternal.h"

PXR_NAMESPACE_OPEN_SCOPE

HdExtComputationContextInternal::HdExtComputationContextInternal()
 : HdExtComputationContext()
 , m_inputs()
 , m_outputs()
 , m_compuationError(false){
}

HdExtComputationContextInternal::~HdExtComputationContextInternal()
{
}


void
HdExtComputationContextInternal::SetInputValue(const TfToken &name,
                                                 const VtValue &input)
{
    m_inputs.emplace(name, input);
}

bool
HdExtComputationContextInternal::GetOutputValue(const TfToken &name,
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
HdExtComputationContextInternal::HasComputationError()
{
    return m_compuationError;
}


void
HdExtComputationContextInternal::RaiseComputationError()
{
    m_compuationError = true;
}


const VtValue &
HdExtComputationContextInternal::GetInputValue(const TfToken &name) const
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
HdExtComputationContextInternal::GetOptionalInputValuePtr(
                                                      const TfToken &name) const
{
    ValueMap::const_iterator it = m_inputs.find(name);

    if (it == m_inputs.end()) {
        return nullptr;
    }

    return &(it->second);
}

void
HdExtComputationContextInternal::SetOutputValue(const TfToken &name,
                                        const VtValue &input)
{
    m_outputs.emplace(name, input);
}

PXR_NAMESPACE_CLOSE_SCOPE
