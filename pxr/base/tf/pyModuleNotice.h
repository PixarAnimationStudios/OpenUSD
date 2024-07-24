//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_MODULE_NOTICE_H
#define PXR_BASE_TF_PY_MODULE_NOTICE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/notice.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyModuleWasLoaded
///
/// A \a TfNotice that is sent when a script module is loaded.  Since many
/// modules may be loaded at once, listeners are encouraged to defer work
/// triggered by this notice to the end of an application iteration.  This, of
/// course, is good practice in general.
///
class TfPyModuleWasLoaded : public TfNotice {
public:
    explicit TfPyModuleWasLoaded(std::string const &name) : _name(name) {}

    TF_API
    virtual ~TfPyModuleWasLoaded();
    
    /// Return the name of the module that was loaded.
    std::string const &GetName() { return _name; }

private:
    std::string _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_MODULE_NOTICE_H
