//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_USD_ABC_ALEMBIC_TEST_H
#define PXR_USD_PLUGIN_USD_ABC_ALEMBIC_TEST_H

#include "pxr/pxr.h"
#include "pxr/usd/plugin/usdAbc/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// Test Alembic conversion.
USDABC_API
bool UsdAbc_TestAlembic(const std::string& pathname);

/// Read Usd file from \p srcPathname and write as Alembic to \p dstPathname.
USDABC_API
bool UsdAbc_WriteAlembic(const std::string& srcPathname,
                         const std::string& dstPathname);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ABC_ALEMBIC_TEST_H
