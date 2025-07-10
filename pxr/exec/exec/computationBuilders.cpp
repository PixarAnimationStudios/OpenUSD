//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationBuilders.h"

#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// Exec_ComputationBuilderValueSpecifierBase
//

// This struct exists just to keep Exec_InputKey private.
struct Exec_ComputationBuilderValueSpecifierBase::_Data
{
    // TODO: Remove when compiler bug is fixed and affected versions
    // are no longer supported.
    //
    // This helper only exists to avoid internal compiler errors
    // on Visual Studio 2022 17.13 (possibly back to 17.10) when
    // the _Data object is allocated in the member initializer
    // in Exec_ComputationBuilderValueSpecifierBase's c'tor.
    template <typename ...Args>
    static _Data* Create(Args&&... args)
    {
        return new _Data{std::forward<Args>(args)...};
    }

    Exec_InputKey inputKey;
};

Exec_ComputationBuilderValueSpecifierBase::
Exec_ComputationBuilderValueSpecifierBase(
    const TfToken &computationName,
    TfType resultType,
    ExecProviderResolution &&providerResolution,
    const TfToken &inputName,
    bool fallsBackToDispatched)
    : _data(
        _Data::Create(
            inputName,
            computationName,
            resultType,
            std::move(providerResolution),
            fallsBackToDispatched,
            /* optional */ true))
{}

Exec_ComputationBuilderValueSpecifierBase::
Exec_ComputationBuilderValueSpecifierBase(
    const Exec_ComputationBuilderValueSpecifierBase &o)
    : _data(std::make_unique<_Data>(*o._data))
{}

Exec_ComputationBuilderValueSpecifierBase::
~Exec_ComputationBuilderValueSpecifierBase() = default;

void
Exec_ComputationBuilderValueSpecifierBase::_SetInputName(
    const TfToken &inputName)
{
    _data->inputKey.inputName = inputName;
}

void
Exec_ComputationBuilderValueSpecifierBase::_SetOptional(
    const bool optional)
{
    _data->inputKey.optional = optional;
}

void
Exec_ComputationBuilderValueSpecifierBase::_SetFallsBackToDispatched(
    const bool fallsBackToDispatched)
{
    _data->inputKey.fallsBackToDispatched = true;
}

void
Exec_ComputationBuilderValueSpecifierBase::_GetInputKey(
    Exec_InputKey *const inputKey) const
{
    *inputKey = _data->inputKey;
}

//
// Exec_PrimComputationBuilder
//

struct Exec_PrimComputationBuilder::_Data
{
    _Data(
        const TfType schemaType_,
        const TfToken &computationName_,
        const bool dispatched_,
        ExecDispatchesOntoSchemas &&dispatchesOntoSchemas_)
    : schemaType(schemaType_)
    , computationName(computationName_)
    , dispatched(dispatched_)
    , dispatchesOntoSchemas(std::move(dispatchesOntoSchemas_))
    , inputKeys(Exec_InputKeyVector::MakeShared())
    {
    }

    const TfType schemaType;
    const TfToken computationName;
    const bool dispatched;
    ExecDispatchesOntoSchemas dispatchesOntoSchemas;
    TfType resultType;
    ExecCallbackFn callback;
    Exec_InputKeyVectorRefPtr inputKeys;
};

Exec_PrimComputationBuilder::Exec_PrimComputationBuilder(
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : _data(std::make_unique<_Data>(
                schemaType, computationName,
                dispatched, std::move(dispatchesOntoSchemas)))
{
}

Exec_PrimComputationBuilder::~Exec_PrimComputationBuilder()
{
    // A null pointer indicates the computation is not dispatched; otherwise,
    // the pointed-to vector contains the list of the schemas onto which the
    // dispatched computation dispatches, which can be empty, to indicate that
    // the computation dispatches onto all prims.
    std::unique_ptr<ExecDispatchesOntoSchemas> dispatchesOntoSchemas;
    if (_data->dispatched) {
        dispatchesOntoSchemas.reset(
            new ExecDispatchesOntoSchemas(
                std::move(_data->dispatchesOntoSchemas)));
    }

    Exec_DefinitionRegistry::ComputationBuilderAccess::_RegisterPrimComputation(
        _data->schemaType,
        _data->computationName,
        _data->resultType,
        std::move(_data->callback),
        std::move(_data->inputKeys),
        std::move(dispatchesOntoSchemas));
}

void
Exec_PrimComputationBuilder::_AddCallback(
    ExecCallbackFn &&callback, TfType resultType)
{
    _data->callback = std::move(callback);
    _data->resultType = resultType;
}

void
Exec_PrimComputationBuilder::_AddInputKey(
    const Exec_ComputationBuilderValueSpecifierBase *const valueSpecifier)
{
    _data->inputKeys->Get().push_back({});
    valueSpecifier->_GetInputKey(&_data->inputKeys->Get().back());
}

//
// Exec_ComputationBuilder
//

Exec_ComputationBuilder::Exec_ComputationBuilder(
    const TfType schemaType)
    : _schemaType(schemaType)
{
}

Exec_ComputationBuilder::~Exec_ComputationBuilder()
{
    Exec_DefinitionRegistry::ComputationBuilderAccess::
        _SetComputationRegistrationComplete(_schemaType);
}

Exec_PrimComputationBuilder 
Exec_ComputationBuilder::PrimComputation(
    const TfToken &computationName)
{
    return Exec_PrimComputationBuilder(_schemaType, computationName);
}

Exec_PrimComputationBuilder 
Exec_ComputationBuilder::DispatchedPrimComputation(
    const TfToken &computationName,
    ExecDispatchesOntoSchemas &&ontoSchemas)
{
    return Exec_PrimComputationBuilder(
        _schemaType,
        computationName,
        /* dispatched */ true,
        std::move(ontoSchemas));
}

PXR_NAMESPACE_CLOSE_SCOPE
