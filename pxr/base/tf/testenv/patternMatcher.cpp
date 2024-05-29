//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/patternMatcher.h"
#include "pxr/base/tf/regTest.h"

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfPatternMatcher()
{
    bool status = true;
    std::string toast = "i like toast";
    std::string toast2 = "i like ToaST";
    TfPatternMatcher pm;
    pm.SetPattern("oast");
    pm.SetIsGlobPattern(true);
    pm.SetIsCaseSensitive(true);
    std::string err = "foo";
    status &= pm.Match(toast, &err);
    err = "foo";
    status &= !pm.Match(toast2, &err);
    pm.SetPattern("oast\\");
    status &= !pm.Match(toast, &err);

    // A loose date/time match regexp.
    TfPatternMatcher dt(
        "^[0-9]{4}/[0-9]{2}/[0-9]{2}(:[0-9]{2}:[0-9]{2}:[0-9]{2})?");
    status &= dt.Match("2009/01/01");
    status &= dt.Match("2009/01/01:12:34:56");
    status &= !dt.Match("01/01/2009");

    return status;
}

TF_ADD_REGTEST(TfPatternMatcher);
