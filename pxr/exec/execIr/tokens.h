//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXECIR_TOKENS_H
#define EXECIR_TOKENS_H

/// \file execIr/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/exec/execIr/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class ExecIrTokensType
///
/// \link ExecIrTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// ExecIrTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use ExecIrTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(ExecIrTokens->avarsDefaultSpace);
/// \endcode
struct ExecIrTokensType {
    EXECIR_API ExecIrTokensType();
    /// \brief "avars:defaultSpace"
    /// 
    /// ExecIrXformable
    const TfToken avarsDefaultSpace;
    /// \brief "avars:rotationOrder"
    /// 
    /// ExecIrXformable
    const TfToken avarsRotationOrder;
    /// \brief "avars:rspin"
    /// 
    /// ExecIrXformable
    const TfToken avarsRspin;
    /// \brief "avars:rx"
    /// 
    /// ExecIrXformable
    const TfToken avarsRx;
    /// \brief "avars:ry"
    /// 
    /// ExecIrXformable
    const TfToken avarsRy;
    /// \brief "avars:rz"
    /// 
    /// ExecIrXformable
    const TfToken avarsRz;
    /// \brief "avars:tx"
    /// 
    /// ExecIrXformable
    const TfToken avarsTx;
    /// \brief "avars:ty"
    /// 
    /// ExecIrXformable
    const TfToken avarsTy;
    /// \brief "avars:tz"
    /// 
    /// ExecIrXformable
    const TfToken avarsTz;
    /// \brief "avars:unitScaleFactor"
    /// 
    /// ExecIrXformable
    const TfToken avarsUnitScaleFactor;
    /// \brief "default:rx"
    /// 
    /// ExecIrXformable
    const TfToken defaultRx;
    /// \brief "default:ry"
    /// 
    /// ExecIrXformable
    const TfToken defaultRy;
    /// \brief "default:rz"
    /// 
    /// ExecIrXformable
    const TfToken defaultRz;
    /// \brief "default:space"
    /// 
    /// ExecIrXformable
    const TfToken defaultSpace;
    /// \brief "default:tx"
    /// 
    /// ExecIrXformable
    const TfToken defaultTx;
    /// \brief "default:ty"
    /// 
    /// ExecIrXformable
    const TfToken defaultTy;
    /// \brief "default:tz"
    /// 
    /// ExecIrXformable
    const TfToken defaultTz;
    /// \brief "guide:displayColor"
    /// 
    /// ExecIrJointScope
    const TfToken guideDisplayColor;
    /// \brief "guide:displayOpacity"
    /// 
    /// ExecIrJointScope
    const TfToken guideDisplayOpacity;
    /// \brief "guide:length"
    /// 
    /// ExecIrJointScope
    const TfToken guideLength;
    /// \brief "in:defaultSpace"
    /// 
    /// ExecIrFkController
    const TfToken inDefaultSpace;
    /// \brief "in:rotationOrder"
    /// 
    /// ExecIrFkController
    const TfToken inRotationOrder;
    /// \brief "in:rspin"
    /// 
    /// ExecIrFkController
    const TfToken inRspin;
    /// \brief "in:rx"
    /// 
    /// ExecIrFkController
    const TfToken inRx;
    /// \brief "in:ry"
    /// 
    /// ExecIrFkController
    const TfToken inRy;
    /// \brief "in:rz"
    /// 
    /// ExecIrFkController
    const TfToken inRz;
    /// \brief "in:tx"
    /// 
    /// ExecIrFkController
    const TfToken inTx;
    /// \brief "in:ty"
    /// 
    /// ExecIrFkController
    const TfToken inTy;
    /// \brief "in:tz"
    /// 
    /// ExecIrFkController
    const TfToken inTz;
    /// \brief "out:defaultSpace"
    /// 
    /// ExecIrFkController
    const TfToken outDefaultSpace;
    /// \brief "out:space"
    /// 
    /// ExecIrFkController, ExecIrSwitchController
    const TfToken outSpace;
    /// \brief "parent:defaultSpace"
    /// 
    /// ExecIrXformable
    const TfToken parentDefaultSpace;
    /// \brief "parentIn:defaultSpace"
    /// 
    /// ExecIrFkController
    const TfToken parentInDefaultSpace;
    /// \brief "parentIn:space"
    /// 
    /// ExecIrFkController
    const TfToken parentInSpace;
    /// \brief "parent:space"
    /// 
    /// ExecIrXformable
    const TfToken parentSpace;
    /// \brief "posed:defaultSpace"
    /// 
    /// ExecIrXformable
    const TfToken posedDefaultSpace;
    /// \brief "posed:space"
    /// 
    /// ExecIrXformable
    const TfToken posedSpace;
    /// \brief "rest:rx"
    /// 
    /// ExecIrXformable
    const TfToken restRx;
    /// \brief "rest:ry"
    /// 
    /// ExecIrXformable
    const TfToken restRy;
    /// \brief "rest:rz"
    /// 
    /// ExecIrXformable
    const TfToken restRz;
    /// \brief "rest:space"
    /// 
    /// ExecIrXformable
    const TfToken restSpace;
    /// \brief "rest:tx"
    /// 
    /// ExecIrXformable
    const TfToken restTx;
    /// \brief "rest:ty"
    /// 
    /// ExecIrXformable
    const TfToken restTy;
    /// \brief "rest:tz"
    /// 
    /// ExecIrXformable
    const TfToken restTz;
    /// \brief "rig1"
    /// 
    /// Fallback value for ExecIrSwitchController::GetSwitchAttr()
    const TfToken rig1;
    /// \brief "rig1:space"
    /// 
    /// ExecIrSwitchController
    const TfToken rig1Space;
    /// \brief "rig2"
    /// 
    /// Possible value for ExecIrSwitchController::GetSwitchAttr()
    const TfToken rig2;
    /// \brief "rig2:space"
    /// 
    /// ExecIrSwitchController
    const TfToken rig2Space;
    /// \brief "switch"
    /// 
    /// ExecIrSwitchController
    const TfToken switch_;
    /// \brief "XYZ"
    /// 
    /// Fallback value for ExecIrFkController::GetInRotationOrderAttr(), Fallback value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken XYZ;
    /// \brief "XZY"
    /// 
    /// Possible value for ExecIrFkController::GetInRotationOrderAttr(), Possible value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken XZY;
    /// \brief "YXZ"
    /// 
    /// Possible value for ExecIrFkController::GetInRotationOrderAttr(), Possible value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken YXZ;
    /// \brief "YZX"
    /// 
    /// Possible value for ExecIrFkController::GetInRotationOrderAttr(), Possible value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken YZX;
    /// \brief "ZXY"
    /// 
    /// Possible value for ExecIrFkController::GetInRotationOrderAttr(), Possible value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken ZXY;
    /// \brief "ZYX"
    /// 
    /// Possible value for ExecIrFkController::GetInRotationOrderAttr(), Possible value for ExecIrXformable::GetAvarsRotationOrderAttr()
    const TfToken ZYX;
    /// \brief "IrController"
    /// 
    /// Schema identifer and family for ExecIrController
    const TfToken IrController;
    /// \brief "IrFkController"
    /// 
    /// Schema identifer and family for ExecIrFkController
    const TfToken IrFkController;
    /// \brief "IrJointScope"
    /// 
    /// Schema identifer and family for ExecIrJointScope
    const TfToken IrJointScope;
    /// \brief "IrSwitchController"
    /// 
    /// Schema identifer and family for ExecIrSwitchController
    const TfToken IrSwitchController;
    /// \brief "IrXformable"
    /// 
    /// Schema identifer and family for ExecIrXformable
    const TfToken IrXformable;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var ExecIrTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa ExecIrTokensType
extern EXECIR_API TfStaticData<ExecIrTokensType> ExecIrTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
