//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/controllerBuilder.h"

PXR_NAMESPACE_OPEN_SCOPE

// ExecIr_ControllerBuilderBase

ExecIr_ControllerBuilderBase::ExecIr_ControllerBuilderBase(
    ExecComputationBuilder &self)
    : _self(self)
{
}

ExecIr_ControllerBuilderBase::~ExecIr_ControllerBuilderBase() = default;

// ExecIrControllerBuilder

ExecIrControllerBuilder::ExecIrControllerBuilder(
    ExecComputationBuilder &self,
    Callback forwardCallback,
    Callback inverseCallback)
    : ExecIr_ControllerBuilderBase(self)
    , _forwardComputeReg(
        self.PrimComputation(_privateComputations->forwardCompute))
    , _inverseComputeReg(
        self.PrimComputation(_privateComputations->inverseCompute))
    , _inverseCallback(inverseCallback)
{
    _forwardComputeReg.Callback<ExecIrResult>(forwardCallback);
}

ExecIrControllerBuilder::~ExecIrControllerBuilder()
{
    // Wrap the inverse callback in a lambda that checks if we have input values
    // for all invertible output attributes and returns an empty ExecIrResult if
    // any are missing.
    //
    // TODO: The Presto implementation calls the inverseCallback if we have
    // input values for *any* invertible output attributes, instead of only
    // doing so if we have values for *all* of them. We may ultimately want to
    // change this code to follow that precedent, but for now we are sticking
    // with this implementation because it means client callbacks don't need to
    // test if values are available.
    //
    _inverseComputeReg.Callback<ExecIrResult>(
        [inverseCallback = _inverseCallback,
         invertibleOutputAttributeNames =
             std::move(_invertibleOutputAttributeNames)]
        (const VdfContext &ctx) -> void {
            for (const TfToken &name : invertibleOutputAttributeNames) {
                if (!ctx.HasInputValue(name)) {
                    ctx.SetOutput(ExecIrResult{});
                    return;
                }
            }

            ctx.SetOutput(inverseCallback(ctx));
        });
}

ExecIrControllerBuilder::_PrivateComputationsType::_PrivateComputationsType()
    : computeInvertedForwardValue(
        "computeInvertedForwardValue", TfToken::Immortal)
    , forwardCompute("forwardCompute", TfToken::Immortal)
    , inverseCompute("inverseCompute", TfToken::Immortal)
{}

TfStaticData<ExecIrControllerBuilder::_PrivateComputationsType>
    ExecIrControllerBuilder::_privateComputations;

PXR_NAMESPACE_CLOSE_SCOPE
