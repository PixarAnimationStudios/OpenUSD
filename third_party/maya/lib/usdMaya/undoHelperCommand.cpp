//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "usdMaya/undoHelperCommand.h"

#include "pxr/base/tf/errorMark.h"

#include <maya/MSyntax.h>

PXR_NAMESPACE_OPEN_SCOPE

const UsdMayaUndoHelperCommand::UndoableFunction*
UsdMayaUndoHelperCommand::_dgModifierFunc = nullptr;

UsdMayaUndoHelperCommand::UsdMayaUndoHelperCommand() : _undoable(false)
{
}

UsdMayaUndoHelperCommand::~UsdMayaUndoHelperCommand()
{
}

MStatus
UsdMayaUndoHelperCommand::doIt(const MArgList& /*args*/)
{
    if (!_dgModifierFunc) {
        _undoable = false;
        return MS::kFailure;
    }

    TfErrorMark errorMark;
    errorMark.SetMark();
    (*_dgModifierFunc)(_modifier);
    _dgModifierFunc = nullptr;
    _undoable = errorMark.IsClean();
    return MS::kSuccess;
}

MStatus
UsdMayaUndoHelperCommand::redoIt()
{
    return _modifier.doIt();
}

MStatus
UsdMayaUndoHelperCommand::undoIt()
{
    return _modifier.undoIt();
}

bool
UsdMayaUndoHelperCommand::isUndoable() const {
    return _undoable;
};

MSyntax
UsdMayaUndoHelperCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

void*
UsdMayaUndoHelperCommand::creator()
{
    return new UsdMayaUndoHelperCommand();
}

/* static */
void
UsdMayaUndoHelperCommand::ExecuteWithUndo(const UndoableFunction& func)
{
    int cmdExists = 0;
    MGlobal::executeCommand("exists usdUndoHelperCmd", cmdExists);
    if (!cmdExists) {
        TF_WARN("usdUndoHelperCmd is unavailable; "
                "function will run without undo support");
        MDGModifier modifier;
        func(modifier);
        return;
    }

    // Run function through command if it is available to get undo support!
    _dgModifierFunc = &func;
    MGlobal::executeCommand("usdUndoHelperCmd", false, true);
}

PXR_NAMESPACE_CLOSE_SCOPE
