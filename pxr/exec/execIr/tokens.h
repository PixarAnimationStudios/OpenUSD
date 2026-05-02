//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_TOKENS_H
#define PXR_EXEC_EXEC_IR_TOKENS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execIr/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define EXEC_IR_COMPUTATION_TOKENS              \
    (computeDesiredValue)                       \
    (explicitDesiredValue)                      \
                                                \
    (computeInvertedForwardValue)               \
    (forwardCompute)                            \
    (inverseCompute)

TF_DECLARE_PUBLIC_TOKENS(
    ExecIrComputationTokens, EXECIR_API, EXEC_IR_COMPUTATION_TOKENS);

#define EXEC_IR_ROTATION_ORDER_TOKENS                                           \
    (XYZ)                                                                       \
    (XZY)                                                                       \
    (YXZ)                                                                       \
    (YZX)                                                                       \
    (ZXY)                                                                       \
    (ZYX)

TF_DECLARE_PUBLIC_TOKENS(
    ExecIrRotationOrderTokens, EXECIR_API, EXEC_IR_ROTATION_ORDER_TOKENS);

#define EXEC_IR_FK_CONTROLLER_TOKENS                                            \
    ((parentInSpace, "parentIn:space"))                                         \
    ((parentInDefaultSpace, "parentIn:defaultSpace"))                           \
                                                                                \
    ((inDefaultSpace, "in:defaultSpace"))                                       \
                                                                                \
    ((inTx, "in:tx"))                                                           \
    ((inTy, "in:ty"))                                                           \
    ((inTz, "in:tz"))                                                           \
    ((inRx, "in:rx"))                                                           \
    ((inRy, "in:ry"))                                                           \
    ((inRz, "in:rz"))                                                           \
    ((inRspin, "in:rspin"))                                                     \
    ((inRotationOrder, "in:rotationOrder"))                                     \
                                                                                \
    ((outSpace, "out:space"))                                                   \
    ((outDefaultSpace, "out:defaultSpace"))

TF_DECLARE_PUBLIC_TOKENS(
    ExecIrFkControllerTokens, EXECIR_API, EXEC_IR_FK_CONTROLLER_TOKENS);

#define EXEC_IR_SWITCH_CONTROLLER_TOKENS                                        \
    (rig1)                                                                      \
    (rig2)                                                                      \
                                                                                \
    ((switchToken, "switch"))                                                   \
                                                                                \
    ((rig1Joint1Space, "rig1:joint1:space"))                                    \
    ((rig1Joint2Space, "rig1:joint2:space"))                                    \
                                                                                \
    ((rig2Joint1Space, "rig2:joint1:space"))                                    \
    ((rig2Joint2Space, "rig2:joint2:space"))                                    \
                                                                                \
    ((outJoint1Space, "out:joint1:space"))                                      \
    ((outJoint2Space, "out:joint2:space"))                                      \

TF_DECLARE_PUBLIC_TOKENS(
    ExecIrSwitchControllerTokens, EXECIR_API, EXEC_IR_SWITCH_CONTROLLER_TOKENS);

#define EXEC_IR_TRANSFORMABLE_TOKENS                                            \
    ((avarsTx, "avars:tx"))                                                     \
    ((avarsTy, "avars:ty"))                                                     \
    ((avarsTz, "avars:tz"))                                                     \
    ((avarsRx, "avars:rx"))                                                     \
    ((avarsRy, "avars:ry"))                                                     \
    ((avarsRz, "avars:rz"))                                                     \
    ((avarsRspin, "avars:rspin"))                                               \
    ((avarsRotationOrder, "avars:rotationOrder"))                               \
    ((avarsDefaultSpace, "avars:defaultSpace"))                                 \
    ((avarsUnitScaleFactor, "avars:unitScaleFactor"))                           \
                                                                                \
    ((restTx, "rest:tx"))                                                       \
    ((restTy, "rest:ty"))                                                       \
    ((restTz, "rest:tz"))                                                       \
    ((restRx, "rest:rx"))                                                       \
    ((restRy, "rest:ry"))                                                       \
    ((restRz, "rest:rz"))                                                       \
    ((restSpace, "rest:space"))                                                 \
                                                                                \
    ((defaultTx, "default:tx"))                                                 \
    ((defaultTy, "default:ty"))                                                 \
    ((defaultTz, "default:tz"))                                                 \
    ((defaultRx, "default:rx"))                                                 \
    ((defaultRy, "default:ry"))                                                 \
    ((defaultRz, "default:rz"))                                                 \
    ((defaultSpace, "default:space"))                                           \
                                                                                \
    ((posedSpace, "posed:space"))                                               \
    ((posedDefaultSpace, "posed:defaultSpace"))                                 \
                                                                                \
    ((parentSpace, "parent:Space"))                                             \
    ((parentDefaultSpace, "parent:defaultSpace"))                               \

TF_DECLARE_PUBLIC_TOKENS(
    ExecIrTransformableTokens, EXECIR_API, EXEC_IR_TRANSFORMABLE_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
