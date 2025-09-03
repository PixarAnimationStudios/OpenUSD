//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/request.h"

#include "pxr/exec/execUsd/requestImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

ExecUsdRequest::ExecUsdRequest(std::unique_ptr<ExecUsd_RequestImpl> &&impl)
    : _impl(std::move(impl))
{}

ExecUsdRequest::ExecUsdRequest(ExecUsdRequest &&) = default;

ExecUsdRequest& ExecUsdRequest::operator=(ExecUsdRequest &&) = default;

ExecUsdRequest::~ExecUsdRequest() = default;

bool
ExecUsdRequest::IsValid() const
{
    return _impl && !_impl->GetExpiredIndices().IsAnySet();
}

PXR_NAMESPACE_CLOSE_SCOPE
