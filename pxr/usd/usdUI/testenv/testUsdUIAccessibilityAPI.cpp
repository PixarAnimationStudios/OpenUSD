//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdUI/accessibilityAPI.h"
#include "pxr/usd/usdUI/tokens.h"
#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_USING_DIRECTIVE

int
main(int argc, char* argv[])
{
  auto stage = UsdStage::CreateInMemory();

  auto prim = stage->DefinePrim(SdfPath("/Root"));
  VtValue value;

  // Test Default
  auto defaultAPI = UsdUIAccessibilityAPI::ApplyDefaultAPI(prim);
  auto namedAPI = UsdUIAccessibilityAPI::Apply(prim, TfToken("foo"));

  // Make sure we create it with the right name
  std::string label = "The root prim";
  auto defaultLabelName = TfToken("accessibility:default:label");
  auto defaultLabel = defaultAPI.CreateLabelAttr(VtValue(label));
  TF_AXIOM(defaultLabel.GetName() == defaultLabelName);

  // Make sure we get the same name back and the values are correct
  defaultLabel = defaultAPI.GetLabelAttr();
  TF_AXIOM(defaultLabel.GetName() == defaultLabelName);

  defaultLabel.Get(&value);
  TF_AXIOM(value.IsHolding<std::string>());
  TF_AXIOM(value.Get<std::string>() == label);

  // Make sure we create the description with the right name
  std::string description = "The greatest prim of all time";
  auto defaultDescriptionName = TfToken("accessibility:default:description");
  auto defaultDescription = defaultAPI.CreateDescriptionAttr(VtValue(std::string(description)));
  TF_AXIOM(defaultDescription.GetName() == defaultDescriptionName);

  // Make sure we get the same name and value back
  defaultDescription = defaultAPI.GetDescriptionAttr();
  TF_AXIOM(defaultDescription.GetName() == defaultDescriptionName);
  defaultDescription.Get(&value);
  TF_AXIOM(value.IsHolding<std::string>());
  TF_AXIOM(value.Get<std::string>() == description);

  // Make sure we create the priority with the right name
  auto priority = UsdUITokens->high;
  auto defaultPriorityName = TfToken("accessibility:default:priority");
  auto defaultPriority = defaultAPI.CreatePriorityAttr(VtValue(priority));
  TF_AXIOM(defaultPriority.GetName() == defaultPriorityName);

  // Make sure we get the same name and value back
  defaultPriority = defaultAPI.GetPriorityAttr();
  TF_AXIOM(defaultPriority.GetName() == defaultPriorityName);
  defaultPriority.Get(&value);
  TF_AXIOM(value.IsHolding<TfToken>());
  TF_AXIOM(value.Get<TfToken>() == priority);

  // Check that the named APIs are created with the correct names and come back with the same name
  auto namedLabelName = TfToken("accessibility:foo:label");
  auto namedDescriptionName = TfToken("accessibility:foo:description");
  auto namedPriorityName = TfToken("accessibility:foo:priority");

  auto namedLabel = namedAPI.CreateLabelAttr(VtValue(label));
  TF_AXIOM(namedLabel.GetName() == namedLabelName);
  namedLabel = namedAPI.GetLabelAttr();
  TF_AXIOM(namedLabel.GetName() == namedLabelName);

  auto namedDescription = namedAPI.CreateDescriptionAttr(VtValue(description));
  TF_AXIOM(namedDescription.GetName() == namedDescriptionName);
  namedDescription = namedAPI.GetDescriptionAttr();
  TF_AXIOM(namedDescription.GetName() == namedDescriptionName);

  auto namedPriority = namedAPI.CreatePriorityAttr(VtValue(priority));
  TF_AXIOM(namedPriority.GetName() == namedPriorityName);
  namedPriority = namedAPI.GetPriorityAttr();
  TF_AXIOM(namedPriority.GetName() == namedPriorityName);

  return EXIT_SUCCESS;
}
