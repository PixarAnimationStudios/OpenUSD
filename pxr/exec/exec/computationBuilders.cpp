//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationBuilders.h"

#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/privateBuiltinComputations.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// Exec_ComputationBuilderCommonBase
//

template <Exec_ComputationBuilderProviderTypes allowed>
Exec_ComputationBuilderComputationValueSpecifier<allowed>
Exec_ComputationBuilderCommonBase::_GetMetadataValueSpecifier(
    const TfType resultType,
    const SdfPath &localTraversal,
    const TfToken &metadataKey)
{
    return Exec_ComputationBuilderComputationValueSpecifier<allowed>(
        Exec_PrivateBuiltinComputations->computeMetadata,
        resultType,
        {localTraversal, ExecProviderResolution::DynamicTraversal::Local},
        metadataKey)
        .InputName(metadataKey);
}

// Explicit template instantiations

template Exec_ComputationBuilderComputationValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Prim>
EXEC_API
Exec_ComputationBuilderCommonBase::_GetMetadataValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Prim>(
        TfType resultType,
        const SdfPath &localTraversal,
        const TfToken &metadataKey);

template Exec_ComputationBuilderComputationValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Attribute>
EXEC_API
Exec_ComputationBuilderCommonBase::_GetMetadataValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Attribute>(
        TfType resultType,
        const SdfPath &localTraversal,
        const TfToken &metadataKey);

template Exec_ComputationBuilderComputationValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Any>
EXEC_API
Exec_ComputationBuilderCommonBase::_GetMetadataValueSpecifier<
    Exec_ComputationBuilderProviderTypes::Any>(
        TfType resultType,
        const SdfPath &localTraversal,
        const TfToken &metadataKey);


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
    const TfToken &metadataKey)
    : _data(
        _Data::Create(
            inputName,
            computationName,
            resultType,
            metadataKey,
            std::move(providerResolution),
            /* fallsBackToDispatched */ false,
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
// Exec_ComputationBuilderBase
//

struct Exec_ComputationBuilderBase::_Data
{
    _Data(
        const TfToken &attributeName_,
        const TfType schemaType_,
        const TfToken &computationName_,
        const bool dispatched_,
        ExecDispatchesOntoSchemas &&dispatchesOntoSchemas_)
    : attributeName(attributeName_)
    , schemaType(schemaType_)
    , computationName(computationName_)
    , dispatched(dispatched_)
    , dispatchesOntoSchemas(std::move(dispatchesOntoSchemas_))
    , inputKeys(Exec_InputKeyVector::MakeShared())
    {
    }

    const TfToken attributeName;
    const TfType schemaType;
    const TfToken computationName;
    const bool dispatched;
    ExecDispatchesOntoSchemas dispatchesOntoSchemas;
    TfType resultType;
    ExecCallbackFn callback;
    Exec_InputKeyVectorRefPtr inputKeys;
};

Exec_ComputationBuilderBase::_Data &
Exec_ComputationBuilderBase::_GetData()
{
    if (TF_VERIFY(_data)) {
        return *_data;
    }

    static _Data empty({}, {}, {}, {}, {});
    return empty;
}

Exec_ComputationBuilderBase::Exec_ComputationBuilderBase(
    const TfToken &attributeName,
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : _data(std::make_unique<_Data>(
                attributeName, schemaType, computationName,
                dispatched, std::move(dispatchesOntoSchemas)))
{
}

Exec_ComputationBuilderBase::~Exec_ComputationBuilderBase() = default;

void
Exec_ComputationBuilderBase::_AddCallback(
    ExecCallbackFn &&callback, TfType resultType)
{
    _data->callback = std::move(callback);
    _data->resultType = resultType;
}

void
Exec_ComputationBuilderBase::_AddInputKey(
    const Exec_ComputationBuilderValueSpecifierBase *const valueSpecifier)
{
    _data->inputKeys->Get().push_back({});
    valueSpecifier->_GetInputKey(&_data->inputKeys->Get().back());
}

std::unique_ptr<ExecDispatchesOntoSchemas>
Exec_ComputationBuilderBase::_GetDispatchesOntoSchemas()
{
    // A null pointer indicates the computation is not dispatched; otherwise,
    // the pointed-to vector contains the list of the schemas onto which the
    // dispatched computation dispatches, which can be empty, to indicate that
    // the computation dispatches onto all prims, regardless of schemas.
    return _data->dispatched
        ? std::make_unique<ExecDispatchesOntoSchemas>(
            std::move(_data->dispatchesOntoSchemas))
        : nullptr;
}

//
// Exec_ComputationBuilderCRTPBase
//

template <typename Derived>
Exec_ComputationBuilderCRTPBase<Derived>::Exec_ComputationBuilderCRTPBase(
    const TfToken &attributeName,
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : Exec_ComputationBuilderBase(
        attributeName, schemaType, computationName,
        dispatched, std::move(dispatchesOntoSchemas))
{
}

template <typename Derived>
Exec_ComputationBuilderCRTPBase<Derived>::~Exec_ComputationBuilderCRTPBase()
= default;

// Explicit template instantiations.
template class Exec_ComputationBuilderCRTPBase<Exec_PrimComputationBuilder>;
template class Exec_ComputationBuilderCRTPBase<Exec_AttributeComputationBuilder>;

//
// Exec_PrimComputationBuilder
//

Exec_PrimComputationBuilder::Exec_PrimComputationBuilder(
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : Exec_ComputationBuilderCRTPBase<Exec_PrimComputationBuilder>(
        /* attributeName */ TfToken(),
        schemaType,
        computationName,
        dispatched,
        std::move(dispatchesOntoSchemas))
{
}

Exec_PrimComputationBuilder::~Exec_PrimComputationBuilder()
{
    _Data &data = _GetData();

    Exec_DefinitionRegistry::ComputationBuilderAccess::_RegisterPrimComputation(
        data.schemaType,
        data.computationName,
        data.resultType,
        std::move(data.callback),
        std::move(data.inputKeys),
        _GetDispatchesOntoSchemas());
}

//
// Exec_AttributeComputationBuilder
//

Exec_AttributeComputationBuilder::Exec_AttributeComputationBuilder(
    const TfToken &attributeName,
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : Exec_ComputationBuilderCRTPBase<Exec_AttributeComputationBuilder>(
        attributeName,
        schemaType,
        computationName,
        dispatched,
        std::move(dispatchesOntoSchemas))
{
}

Exec_AttributeComputationBuilder::~Exec_AttributeComputationBuilder()
{
    _Data &data = _GetData();

    Exec_DefinitionRegistry::ComputationBuilderAccess::
        _RegisterAttributeComputation(
            data.attributeName,
            data.schemaType,
            data.computationName,
            data.resultType,
            std::move(data.callback),
            std::move(data.inputKeys),
            _GetDispatchesOntoSchemas());
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

Exec_AttributeComputationBuilder 
Exec_ComputationBuilder::AttributeComputation(
    const TfToken &attributeName,
    const TfToken &computationName)
{
    return Exec_AttributeComputationBuilder(
        attributeName, _schemaType, computationName);
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

Exec_AttributeComputationBuilder 
Exec_ComputationBuilder::DispatchedAttributeComputation(
    const TfToken &computationName,
    ExecDispatchesOntoSchemas &&ontoSchemas)
{
    return Exec_AttributeComputationBuilder(
        /* attributeName */ TfToken(),
        _schemaType,
        computationName,
        /* dispatched */ true,
        std::move(ontoSchemas));
}

PXR_NAMESPACE_CLOSE_SCOPE
