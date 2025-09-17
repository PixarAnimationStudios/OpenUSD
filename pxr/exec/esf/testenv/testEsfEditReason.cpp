//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/esf/editReason.h"

#include "pxr/base/tf/diagnosticLite.h"

PXR_NAMESPACE_USING_DIRECTIVE;

constexpr EsfEditReason none = EsfEditReason::None;
constexpr EsfEditReason resyncedObject = EsfEditReason::ResyncedObject;
constexpr EsfEditReason changedPropertyList = EsfEditReason::ChangedPropertyList;
constexpr EsfEditReason many = resyncedObject | changedPropertyList;

static void
TestBitwiseLogic()
{
    TF_AXIOM(!none);
    TF_AXIOM(resyncedObject);
    TF_AXIOM((resyncedObject | none) == resyncedObject);
    TF_AXIOM((resyncedObject & none) == none);
    TF_AXIOM(many & resyncedObject);
    TF_AXIOM(many & changedPropertyList);
    TF_AXIOM(many.Contains(resyncedObject));
    TF_AXIOM(!resyncedObject.Contains(many));
}

static void
TestGetDescription()
{
    TF_AXIOM(none.GetDescription() == "None");
    TF_AXIOM(resyncedObject.GetDescription() == "ResyncedObject");
    TF_AXIOM(changedPropertyList.GetDescription() == "ChangedPropertyList");
    TF_AXIOM(many.GetDescription() == "ResyncedObject, ChangedPropertyList");
}

int main()
{
    TestBitwiseLogic();
    TestGetDescription();
}
