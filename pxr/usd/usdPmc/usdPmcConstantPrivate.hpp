//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcConstantPrivate.hpp

#ifndef USD_PMC_CONSTANT_PRIVATE_H
#define USD_PMC_CONSTANT_PRIVATE_H

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Main USD section identifier (optional)
const std::string kUSDJsonMainKey = "usd";

/// Type name field within usd section (optional string)
const std::string kUSDJsonTypeNameKey = "tn";

/// Submesh names array within usd section (optional list)
const std::string kUSDJsonSubmeshNamesKey = "ss";

PXR_NAMESPACE_CLOSE_SCOPE

#endif
