//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdUI/localizationAPI.h"
#include "pxr/usd/usdUI/tokens.h"
#include "pxr/base/tf/diagnostic.h"
#include <iostream>
PXR_NAMESPACE_USING_DIRECTIVE

int
main(int argc, char* argv[])
{
  auto en_us = TfToken("en_US");
  auto fr_ca = TfToken("fr_CA");
  auto hi_in = TfToken("hi_IN");

  auto stage = UsdStage::CreateInMemory();
  auto prim = stage->DefinePrim(SdfPath("/Root"));

  auto api = UsdUILocalizationAPI::Apply(prim);
  auto frAPI = UsdUILocalizationAPI::Apply(prim, fr_ca);

  auto language = api.CreateLanguageAttr(VtValue(en_us));
  TF_AXIOM(language.GetName() == UsdUITokens->languageAttribute);


  // Test Default localization
  auto baseName = TfToken("foo");
  auto baseAttr = prim.CreateAttribute(baseName, SdfValueTypeNames->String);
  baseAttr.Set(VtValue("Hello"));
  TF_AXIOM(baseAttr.GetPath() == UsdUILocalizationAPI::GetDefaultProperty(baseAttr).GetPath());

  auto frAttrName = TfToken("foo:lang:fr_CA");
  auto frAttr = frAPI.CreateLocalizedAttribute(baseAttr, VtValue("Bonjour"));
  TF_AXIOM(frAttr.GetName() == frAttrName);
  TF_AXIOM(UsdUILocalizationAPI::GetDefaultProperty(frAttr).GetPath() == baseAttr.GetPath());
  TF_AXIOM(UsdUILocalizationAPI::GetPropertyLanguage(frAttr) == fr_ca);
  TF_AXIOM(UsdUILocalizationAPI::GetLocalizedProperty(baseAttr, fr_ca).GetPath() == frAttr.GetPath());
  TF_AXIOM(frAttr.IsCustom() == baseAttr.IsCustom());
  TF_AXIOM(frAttr.GetTypeName() == baseAttr.GetTypeName());

  auto hiAttrName = TfToken("foo:lang:hi_IN");
  auto hiAttr = UsdUILocalizationAPI::CreateLocalizedAttribute(baseAttr, hi_in, VtValue("नमस्ते"));
  TF_AXIOM(hiAttr.GetName() == hiAttrName);
  TF_AXIOM(UsdUILocalizationAPI::GetLocalizedProperty(baseAttr, hi_in).GetPath() == hiAttr.GetPath());

  std::map<TfToken, UsdProperty> properties;
  auto defaultAttr = UsdUILocalizationAPI::GetAppliedPropertyLocalizations(baseAttr, properties);
  TF_AXIOM(properties.size() == 1);
  TF_AXIOM(properties.count(fr_ca) == 1);
  TF_AXIOM(defaultAttr.GetPath() == baseAttr.GetPath());

  properties.clear();
  defaultAttr = UsdUILocalizationAPI::GetAllPropertyLocalizations(baseAttr, properties);
  TF_AXIOM(properties.size() == 2);
  TF_AXIOM(properties.count(fr_ca) == 1);
  TF_AXIOM(properties.count(hi_in) == 1);
  TF_AXIOM(defaultAttr.GetPath() == baseAttr.GetPath());

  auto baseRelName = TfToken("spam");
  auto baseRel = prim.CreateRelationship(baseRelName, false);
  auto frRel = frAPI.CreateLocalizedRelationship(baseRel);
  TF_AXIOM(UsdUILocalizationAPI::GetLocalizedProperty(baseRel, fr_ca).GetPath() == frRel.GetPath());

  return EXIT_SUCCESS;
}