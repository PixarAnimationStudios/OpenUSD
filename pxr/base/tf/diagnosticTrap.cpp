//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticMgr.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TfDiagnosticTrap::TfDiagnosticTrap()
    : _trapKey(TfDiagnosticMgr::GetInstance()._PushTrap(this))
{
}

TfDiagnosticTrap::~TfDiagnosticTrap()
{
    Dismiss();
}

void
TfDiagnosticTrap::Dismiss()
{
    if (_trapKey) {
        TfDiagnosticMgr::GetInstance()._PopTrap(this, _trapKey);
        _trapKey = nullptr;
        _container.Post();
    }
}

void
TfDiagnosticTrap::Clear()
{
    EraseMatching([](TfDiagnosticBase const &) { return true; });
}

void
TfDiagnosticTrap::ClearErrors()
{
    EraseMatching([](TfError const &) { return true; });
}

void
TfDiagnosticTrap::ClearWarnings()
{
    EraseMatching([](TfWarning const &) { return true; });
}

void
TfDiagnosticTrap::ClearStatuses()
{
    EraseMatching([](TfStatus const &) { return true; });
}

bool
TfDiagnosticTrap::IsClean() const {
    return _container.IsEmpty();
}

bool
TfDiagnosticTrap::HasErrors() const
{
    return HasAnyMatching([](TfError const &) { return true; });
}

bool
TfDiagnosticTrap::HasWarnings() const
{
    return HasAnyMatching([](TfWarning const &) { return true; });
}

bool
TfDiagnosticTrap::HasStatuses() const
{
    return HasAnyMatching([](TfStatus const &) { return true; });
}

std::vector<TfError> const &
TfDiagnosticTrap::GetErrors() const
{
    return _container.GetErrors();
}

std::vector<TfWarning> const &
TfDiagnosticTrap::GetWarnings() const
{
    return _container.GetWarnings();
}

std::vector<TfStatus> const &
TfDiagnosticTrap::GetStatuses() const
{
    return _container.GetStatuses();
}

TfDiagnosticTransport
TfDiagnosticTrap::Transport()
{
    TfDiagnosticTransport transport;
    if (IsClean()) {
        return transport;
    }
    TfDiagnosticMgr::_LogTextPin pin =
        TfDiagnosticMgr::GetInstance()._PinDiagnosticsLogText(_container);
    transport = TfDiagnosticTransport {
        std::move(_container),
        std::move(pin)
    };
    _container.Clear();
    _OnContentsChanged();
    return transport;
}

void
TfDiagnosticTrap::_OnContentsChanged() const
{
    TfDiagnosticMgr::GetInstance()._RebuildTrappedDiagnosticsLogText(_logStart);
}

PXR_NAMESPACE_CLOSE_SCOPE
