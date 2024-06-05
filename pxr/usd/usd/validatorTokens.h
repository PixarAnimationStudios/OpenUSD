//
// Copyright 2024 Pixar
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
#ifndef USD_VALIDATOR_TOKENS_H
#define USD_VALIDATOR_TOKENS_H

/// \file usd/validatorTokens.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_VALIDATOR_NAME_TOKENS                   \
    ((compositionErrorTest, "usd:CompositionErrorTest"))

#define USD_VALIDATOR_KEYWORD_TOKENS                \
    (UsdCoreValidators)

/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usd:, which is the name of
/// the usd plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorNameTokens, USD_API, 
                         USD_VALIDATOR_NAME_TOKENS);

/// Tokens representing keywords associated with any validator in the usd
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorKeywordTokens, USD_API, 
                         USD_VALIDATOR_KEYWORD_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
