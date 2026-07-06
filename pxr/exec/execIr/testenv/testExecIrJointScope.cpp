//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xformable.h"

#include <iostream>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_CLOSE(expr, expected)                                           \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (!GfIsClose(expr_, expected, 1e-6)) {                               \
            std::cout << std::flush;                                           \
            std::cerr << std::flush;                                           \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

struct _EnsureNoErrors {
    ~_EnsureNoErrors() {
        TF_VERIFY(mark.IsClean());
    }

    TfErrorMark mark;
};

static void
Test_JointScopeBasic()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Scope "Root" {
            def IrJointScope "Joint" {
                double avars:tx =   1.0
                double avars:ty =   2.0
                double avars:tz =   3.0
                double avars:rx =  90.0
                double avars:ry = -90.0
                double avars:rz =  90.0
                matrix4d posed:space.connect = [
                    </Root/FkController.out:space>
                ]
            }

            def IrFkController "FkController" {
                double in:tx.connect = </Root/Joint.avars:tx>
                double in:ty.connect = </Root/Joint.avars:ty>
                double in:tz.connect = </Root/Joint.avars:tz>
                double in:rx.connect = </Root/Joint.avars:rx>
                double in:ry.connect = </Root/Joint.avars:ry>
                double in:rz.connect = </Root/Joint.avars:rz>
                double in:rspin.connect = </Root/Joint.avars:rspin>
                token in:rotationOrder.connect = [
                    </Root/Joint.avars:rotationOrder>
                ]
                matrix4d in:defaultSpace.connect = [
                    </Root/Joint.avars:defaultSpace>
                ]
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim joint = usdStage->GetPrimAtPath(SdfPath("/Root/Joint"));
    TF_AXIOM(joint);
    const UsdAttribute posedSpace =
        joint.GetAttribute(ExecIrTokens->posedSpace);
    TF_AXIOM(posedSpace);

    const std::vector<UsdAttribute> inputAttributes = {
        joint.GetAttribute(ExecIrTokens->avarsRx),
        joint.GetAttribute(ExecIrTokens->avarsRy),
        joint.GetAttribute(ExecIrTokens->avarsRz),
        joint.GetAttribute(ExecIrTokens->avarsRspin),
        joint.GetAttribute(ExecIrTokens->avarsTx),
        joint.GetAttribute(ExecIrTokens->avarsTy),
        joint.GetAttribute(ExecIrTokens->avarsTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    ExecUsdSystem execSystem(usdStage);

    // Compute forward to get the output value produced by the authored
    // scene.
    {
        const ExecUsdRequest request = execSystem.BuildRequest({
            ExecUsdValueKey{posedSpace, ExecBuiltinComputations->computeValue}
        });
        TF_AXIOM(request.IsValid());

        {
            ExecUsdCacheView cache = execSystem.Compute(request);
            ASSERT_CLOSE(
                cache.Get(0).Get<GfMatrix4d>(),
                GfMatrix4d(0,  0, 1, 0,
                           0, -1, 0, 0,
                           1,  0, 0, 0,
                           1,  2, 3, 1));
        }

        // Translate the rest space and compute again.
        UsdAttribute restTz =
            joint.GetAttribute(ExecIrTokens->restTz);
        TF_AXIOM(restTz);
        restTz.Set(10.0);

        {
            ExecUsdCacheView cache = execSystem.Compute(request);
            ASSERT_CLOSE(
                cache.Get(0).Get<GfMatrix4d>(),
                GfMatrix4d(0,  0,  1, 0,
                           0, -1,  0, 0,
                           1,  0,  0, 0,
                           1,  2, 13, 1));
        }
    }

    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &attr : inputAttributes) {
            valueKeys.emplace_back(
                attr, ExecIrComputations->computeDesiredValue);
        }
        const ExecUsdRequest request =
            execSystem.BuildRequest(std::move(valueKeys));
        TF_AXIOM(request.IsValid());

        // Perform an inverse compute, to get the values of the avars that
        // produce the desired value for the posed space.
        {
            const GfMatrix4d desiredPosedSpaceValue(0,  0,  1, 0,
                                                    0, -1,  0, 0,
                                                    1,  0,  0, 0,
                                                    6,  7, 18, 1);
            ExecUsdValueOverrideVector overrides {
                {{posedSpace, ExecIrComputations->explicitDesiredValue},
                 VtValue(desiredPosedSpaceValue)}
            };
            ExecUsdCacheView cache =
                execSystem.ComputeWithOverrides(request, std::move(overrides));

            // Expected input values in the same order as the value keys in the
            // request.
            const std::vector<double> expectedInputValues{
                90.0, -90.0, 90.0, 0.0,    6.0, 7.0, 8.0,
            };
            for (unsigned int index=0;
                 index<expectedInputValues.size(); ++index) {
                ASSERT_CLOSE(
                    cache.Get(index).Get<double>(),
                    expectedInputValues[index]);
            }
        }
    }
}

static void
Test_JointScopeRestSpace()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Xform "Root" {
            def IrJointScope "Joint" {
                double avars:ty = 10.0
                matrix4d posed:space.connect = [
                    </Root/FkController.out:space>
                ]
            }

            def IrFkController "FkController" {
                double in:tx.connect = </Root/Joint.avars:tx>
                double in:ty.connect = </Root/Joint.avars:ty>
                double in:tz.connect = </Root/Joint.avars:tz>
                double in:rx.connect = </Root/Joint.avars:rx>
                double in:ry.connect = </Root/Joint.avars:ry>
                double in:rz.connect = </Root/Joint.avars:rz>
                double in:rspin.connect = </Root/Joint.avars:rspin>
                token in:rotationOrder.connect = [
                    </Root/Joint.avars:rotationOrder>
                ]
                matrix4d in:defaultSpace.connect = [
                    </Root/Joint.avars:defaultSpace>
                ]
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim joint = usdStage->GetPrimAtPath(SdfPath("/Root/Joint"));
    TF_AXIOM(joint);
    const UsdAttribute posedSpace =
        joint.GetAttribute(ExecIrTokens->posedSpace);
    TF_AXIOM(posedSpace);

    const std::vector<UsdAttribute> inputAttributes = {
        joint.GetAttribute(ExecIrTokens->avarsRx),
        joint.GetAttribute(ExecIrTokens->avarsRy),
        joint.GetAttribute(ExecIrTokens->avarsRz),
        joint.GetAttribute(ExecIrTokens->avarsRspin),
        joint.GetAttribute(ExecIrTokens->avarsTx),
        joint.GetAttribute(ExecIrTokens->avarsTy),
        joint.GetAttribute(ExecIrTokens->avarsTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
            ExecUsdValueKey{posedSpace, ExecBuiltinComputations->computeValue}
        });
    TF_AXIOM(request.IsValid());

    // Compute forward to get the output value produced by the authored
    // scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(request);
        ASSERT_CLOSE(
            cache.Get(0).Get<GfMatrix4d>(),
            GfMatrix4d(1,  0, 0, 0,
                       0,  1, 0, 0,
                       0,  0, 1, 0,
                       0, 10, 0, 1));
    }

    // Rotate the posed space around the X axis.
    {
        const UsdAttribute avarsRx =
            joint.GetAttribute(ExecIrTokens->avarsRx);
        TF_AXIOM(avarsRx);
        avarsRx.Set(90.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        ASSERT_CLOSE(
            cache.Get(0).Get<GfMatrix4d>(),
            GfMatrix4d(1,  0, 0, 0,
                       0,  0, 1, 0,
                       0, -1, 0, 0,
                       0, 10, 0, 1));
    }

    // Translate the rest space along the Z axis.
    {
        const UsdAttribute restTz =
            joint.GetAttribute(ExecIrTokens->restTz);
        TF_AXIOM(restTz);
        restTz.Set(5.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        ASSERT_CLOSE(
            cache.Get(0).Get<GfMatrix4d>(),
            GfMatrix4d(1,  0, 0, 0,
                       0,  0, 1, 0,
                       0, -1, 0, 0,
                       0, 10, 5, 1));
    }
}

static void
Test_JointScopeParentChild()
{
    _EnsureNoErrors mark;

    // Create a scene with two fkControllers, where the child controller's
    // parent space is connected to the output of the parent controller.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Xform "Root" {
            def IrJointScope "ParentJoint" {
                double avars:rx = 90.0
                matrix4d posed:space.connect = [
                    </Root/ParentController.out:space>
                ]

                def IrJointScope "ChildJoint" {
                    double avars:tx =  1.0
                    double avars:ty =  2.0
                    double avars:tz =  3.0
                    double rest:tx  = 10.0
                    matrix4d posed:space.connect = [
                        </Root/ChildController.out:space>
                    ]
                }
            }

            def IrFkController "ParentController" {
                double in:tx.connect = </Root/ParentJoint.avars:tx>
                double in:ty.connect = </Root/ParentJoint.avars:ty>
                double in:tz.connect = </Root/ParentJoint.avars:tz>
                double in:rx.connect = </Root/ParentJoint.avars:rx>
                double in:ry.connect = </Root/ParentJoint.avars:ry>
                double in:rz.connect = </Root/ParentJoint.avars:rz>
                double in:rspin.connect = </Root/ParentJoint.avars:rspin>
                token in:rotationOrder.connect = [
                    </Root/ParentJoint.avars:rotationOrder>
                ]
                matrix4d in:defaultSpace.connect = [
                    </Root/ParentJoint.avars:defaultSpace>
                ]
            }

            def IrFkController "ChildController" {
                double in:tx.connect = </Root/ParentJoint/ChildJoint.avars:tx>
                double in:ty.connect = </Root/ParentJoint/ChildJoint.avars:ty>
                double in:tz.connect = </Root/ParentJoint/ChildJoint.avars:tz>
                double in:rx.connect = </Root/ParentJoint/ChildJoint.avars:rx>
                double in:ry.connect = </Root/ParentJoint/ChildJoint.avars:ry>
                double in:rz.connect = </Root/ParentJoint/ChildJoint.avars:rz>
                double in:rspin.connect = [
                    </Root/ParentJoint/ChildJoint.avars:rspin>
                ]
                token in:rotationOrder.connect = [
                    </Root/ParentJoint/ChildJoint.avars:rotationOrder>
                ]
                matrix4d in:defaultSpace.connect = [
                    </Root/ParentJoint/ChildJoint.avars:defaultSpace>
                ]
                matrix4d parentIn:defaultSpace.connect = [
                    </Root/ParentController.out:defaultSpace>
                ]
                matrix4d parentIn:space.connect = [
                    </Root/ParentController.out:space>
                ]
            }
        }
        )usda");

    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim parent =
        usdStage->GetPrimAtPath(SdfPath("/Root/ParentJoint"));
    const UsdPrim child =
        usdStage->GetPrimAtPath(SdfPath("/Root/ParentJoint/ChildJoint"));
    TF_AXIOM(parent && child);

    const UsdAttribute parentPosedSpace =
        parent.GetAttribute(ExecIrTokens->posedSpace);
    const UsdAttribute childPosedSpace =
        child.GetAttribute(ExecIrTokens->posedSpace);
    TF_AXIOM(parentPosedSpace && childPosedSpace);

    const std::vector<UsdAttribute> inputAttributes = {
        parent.GetAttribute(ExecIrTokens->avarsRx),
        parent.GetAttribute(ExecIrTokens->avarsRy),
        parent.GetAttribute(ExecIrTokens->avarsRz),
        parent.GetAttribute(ExecIrTokens->avarsRspin),
        parent.GetAttribute(ExecIrTokens->avarsTx),
        parent.GetAttribute(ExecIrTokens->avarsTy),
        parent.GetAttribute(ExecIrTokens->avarsTz),

        child.GetAttribute(ExecIrTokens->avarsRx),
        child.GetAttribute(ExecIrTokens->avarsRy),
        child.GetAttribute(ExecIrTokens->avarsRz),
        child.GetAttribute(ExecIrTokens->avarsRspin),
        child.GetAttribute(ExecIrTokens->avarsTx),
        child.GetAttribute(ExecIrTokens->avarsTy),
        child.GetAttribute(ExecIrTokens->avarsTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    ExecUsdSystem execSystem(usdStage);

    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
            ExecUsdValueKey{
                parentPosedSpace, ExecBuiltinComputations->computeValue},
            ExecUsdValueKey{
                childPosedSpace, ExecBuiltinComputations->computeValue},
        });
    TF_AXIOM(outputRequest.IsValid());

    // Matrices representing rotations about the X and Y axes by 90 degrees.
    static const GfMatrix4d rotateX90(GfRotation({1, 0, 0}, 90), {0, 0, 0});
    static const GfMatrix4d rotateY90(GfRotation({0, 1, 0}, 90), {0, 0, 0});

    // Compute in the forward direction.
    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_CLOSE(cache.Get(0).Get<GfMatrix4d>(), rotateX90);
        ASSERT_CLOSE(cache.Get(1).Get<GfMatrix4d>(),
                     GfMatrix4d().SetTranslate({11, 2, 3}) * rotateX90);
    }

    std::vector<ExecUsdValueKey> valueKeys;
    for (const UsdAttribute &attr : inputAttributes) {
        valueKeys.emplace_back(
            attr, ExecIrComputations->computeDesiredValue);
    }
    const ExecUsdRequest inputRequest =
        execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(inputRequest.IsValid());

    const GfMatrix4d desiredParentPosedSpaceValue(rotateY90);
    GfMatrix4d desiredChildPosedSpaceValue = desiredParentPosedSpaceValue;
    desiredChildPosedSpaceValue.SetTranslateOnly({15, 20, 0});

    // The expected results are:
    // - The parent posed space is rotated by 90 degrees about the Y axis.
    // - The child posed space is in a space that receives the parent rotation,
    //   which results in:
    //   o X is -10 (the Z goal rotated to the X axis, offsetting rest space)
    //   o Y is 20 (directly from the goal, no rotation since we rotate about Y)
    //   o Z is 15 (from the goal, rotated from X to Z)
    const std::vector<double> expectedInputValues{
        0.0, 90.0, 0.0, 0.0,      0.0,  0.0,  0.0,
        0.0,  0.0, 0.0, 0.0,    -10.0, 20.0, 15.0,
    };

    {
        ExecUsdValueOverrideVector overrides {
            {{parentPosedSpace, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredParentPosedSpaceValue)},
            {{childPosedSpace, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredChildPosedSpaceValue)},
        };
        ExecUsdCacheView cache =
            execSystem.ComputeWithOverrides(inputRequest, std::move(overrides));

        for (unsigned int index=0; index<expectedInputValues.size(); ++index) {
            ASSERT_CLOSE(cache.Get(index).Get<double>(),
                         expectedInputValues[index]);
        }
    }

    // Author the expected input values and confirm we get the desired output
    // values.
    {
        int index = 0;
        for (const UsdAttribute &attr : inputAttributes) {
            attr.Set(expectedInputValues[index++]);
        }

        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_CLOSE(cache.Get(0).Get<GfMatrix4d>(),
                     desiredParentPosedSpaceValue);
        ASSERT_CLOSE(cache.Get(1).Get<GfMatrix4d>(),
                     desiredChildPosedSpaceValue);
    }

    // Add a transform on the root Xform and test that it is inherited by the
    // JointScope.
    {
        TF_AXIOM(usdStage->GetPrimAtPath(SdfPath("/Root")));
        const UsdGeomXformable xformable(
            usdStage->GetPrimAtPath(SdfPath("/Root")));
        TF_AXIOM(xformable);

        GfMatrix4d rootTransform;
        rootTransform.SetTranslate({1.1, 2.2, 3.3});

        UsdGeomXformOp transformOp =
            xformable.AddXformOp(UsdGeomXformOp::TypeTransform);
        transformOp.Set(VtValue(rootTransform));

        ExecUsdCacheView cache = execSystem.Compute(outputRequest);
        ASSERT_CLOSE(cache.Get(0).Get<GfMatrix4d>(),
                     desiredParentPosedSpaceValue * rootTransform);
        ASSERT_CLOSE(cache.Get(1).Get<GfMatrix4d>(),
                     desiredChildPosedSpaceValue * rootTransform);
    }
}

int main(int argc, char **argv)
{
    Test_JointScopeBasic();
    Test_JointScopeRestSpace();
    Test_JointScopeParentChild();
}
