//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pySafePython.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "pxr/exec/execIr/tokens.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nb::handle) { return ExecIrTokens->name.GetString(); });

void wrapExecIrTokens(nb::module_ &m)
{
    nb::class_<ExecIrTokensType> cls(m, "Tokens");
    _ADD_TOKEN(cls, avarsDefaultSpace);
    _ADD_TOKEN(cls, avarsRotationOrder);
    _ADD_TOKEN(cls, avarsRspin);
    _ADD_TOKEN(cls, avarsRx);
    _ADD_TOKEN(cls, avarsRy);
    _ADD_TOKEN(cls, avarsRz);
    _ADD_TOKEN(cls, avarsTx);
    _ADD_TOKEN(cls, avarsTy);
    _ADD_TOKEN(cls, avarsTz);
    _ADD_TOKEN(cls, avarsUnitScaleFactor);
    _ADD_TOKEN(cls, defaultRx);
    _ADD_TOKEN(cls, defaultRy);
    _ADD_TOKEN(cls, defaultRz);
    _ADD_TOKEN(cls, defaultSpace);
    _ADD_TOKEN(cls, defaultTx);
    _ADD_TOKEN(cls, defaultTy);
    _ADD_TOKEN(cls, defaultTz);
    _ADD_TOKEN(cls, guideDisplayColor);
    _ADD_TOKEN(cls, guideDisplayOpacity);
    _ADD_TOKEN(cls, guideLength);
    _ADD_TOKEN(cls, inDefaultSpace);
    _ADD_TOKEN(cls, inRotationOrder);
    _ADD_TOKEN(cls, inRspin);
    _ADD_TOKEN(cls, inRx);
    _ADD_TOKEN(cls, inRy);
    _ADD_TOKEN(cls, inRz);
    _ADD_TOKEN(cls, inTx);
    _ADD_TOKEN(cls, inTy);
    _ADD_TOKEN(cls, inTz);
    _ADD_TOKEN(cls, outDefaultSpace);
    _ADD_TOKEN(cls, outSpace);
    _ADD_TOKEN(cls, parentDefaultSpace);
    _ADD_TOKEN(cls, parentInDefaultSpace);
    _ADD_TOKEN(cls, parentInSpace);
    _ADD_TOKEN(cls, parentSpace);
    _ADD_TOKEN(cls, posedDefaultSpace);
    _ADD_TOKEN(cls, posedSpace);
    _ADD_TOKEN(cls, restRx);
    _ADD_TOKEN(cls, restRy);
    _ADD_TOKEN(cls, restRz);
    _ADD_TOKEN(cls, restSpace);
    _ADD_TOKEN(cls, restTx);
    _ADD_TOKEN(cls, restTy);
    _ADD_TOKEN(cls, restTz);
    _ADD_TOKEN(cls, rig1);
    _ADD_TOKEN(cls, rig1Space);
    _ADD_TOKEN(cls, rig2);
    _ADD_TOKEN(cls, rig2Space);
    _ADD_TOKEN(cls, switch_);
    _ADD_TOKEN(cls, XYZ);
    _ADD_TOKEN(cls, XZY);
    _ADD_TOKEN(cls, YXZ);
    _ADD_TOKEN(cls, YZX);
    _ADD_TOKEN(cls, ZXY);
    _ADD_TOKEN(cls, ZYX);
    _ADD_TOKEN(cls, IrController);
    _ADD_TOKEN(cls, IrFkController);
    _ADD_TOKEN(cls, IrJointScope);
    _ADD_TOKEN(cls, IrSwitchController);
    _ADD_TOKEN(cls, IrXformable);
}
