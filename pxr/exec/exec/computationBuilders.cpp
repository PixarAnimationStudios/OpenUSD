//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationBuilders.h"

#include "pxr/exec/exec/builtinComputationRegistry.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/privateBuiltinComputations.h"

#include "pxr/base/tf/diagnosticLite.h"

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
    const TfToken &disambiguatingId)
    : _data(
        _Data::Create(
            inputName,
            computationName,
            disambiguatingId,
            resultType,
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
    // Some built-in computations are not allowed to be consumed by user-
    // defined computations.
    if (Exec_BuiltinComputationRegistry::IsReservedName(
        _data->inputKey.computationName)) {
        const auto *traits = Exec_BuiltinComputationRegistry::GetInstance()
            .GetTraits(_data->inputKey.computationName);
        if (!traits || !traits->isInputConsumable) {
            TF_CODING_ERROR(
                "The builtin computation '%s' cannot be consumed by inputs to "
                "user-defined computations.",
                _data->inputKey.computationName.GetText());

            _data->inputKey.computationName = TfToken();
        }
    }

    *inputKey = _data->inputKey;
}

//
// Exec_ComputationBuilderConstantValueSpecifier
//

Exec_ComputationBuilderConstantValueSpecifier::
Exec_ComputationBuilderConstantValueSpecifier(
    const TfType resultType,
    const SdfPath &localTraversal,
    const TfToken &inputName,
    VtValue &&constantValue)
    : Exec_ComputationBuilderValueSpecifierBase(
        Exec_PrivateBuiltinComputations->computeConstant,
        resultType,
        {localTraversal, ExecProviderResolution::DynamicTraversal::Local},
        inputName,
        Exec_DefinitionRegistry::RegistrationAccess::
            _GetInstanceForRegistration().RegisterConstantValue(
                std::move(constantValue)))
{
}

//
// Exec_ComputationBuilderConstantAccessorBase
//

Exec_ComputationBuilderConstantAccessorBase::
Exec_ComputationBuilderConstantAccessorBase(
    VtValue &&constantValue,
    const TfType valueType)
    : Exec_ComputationBuilderAccessorBase(SdfPath::AbsoluteRootPath())
    , _constantValue(VtValue(std::move(constantValue)))
    , _valueType(valueType)
{
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
template class Exec_ComputationBuilderCRTPBase<ExecPrimComputationBuilder>;
template class Exec_ComputationBuilderCRTPBase<ExecAttributeComputationBuilder>;

//
// ExecPrimComputationBuilder
//

ExecPrimComputationBuilder::ExecPrimComputationBuilder(
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : Exec_ComputationBuilderCRTPBase<ExecPrimComputationBuilder>(
        /* attributeName */ TfToken(),
        schemaType,
        computationName,
        dispatched,
        std::move(dispatchesOntoSchemas))
{
}

ExecPrimComputationBuilder::~ExecPrimComputationBuilder()
{
    _Data &data = _GetData();

    Exec_DefinitionRegistry::RegistrationAccess::
        _GetInstanceForRegistration().RegisterPrimComputation(
            data.schemaType,
            data.computationName,
            data.resultType,
            std::move(data.callback),
            std::move(data.inputKeys),
            _GetDispatchesOntoSchemas());
}

//
// ExecAttributeComputationBuilder
//

ExecAttributeComputationBuilder::ExecAttributeComputationBuilder(
    const TfToken &attributeName,
    const TfType schemaType,
    const TfToken &computationName,
    const bool dispatched,
    ExecDispatchesOntoSchemas &&dispatchesOntoSchemas)
    : Exec_ComputationBuilderCRTPBase<ExecAttributeComputationBuilder>(
        attributeName,
        schemaType,
        computationName,
        dispatched,
        std::move(dispatchesOntoSchemas))
{
}

ExecAttributeComputationBuilder::~ExecAttributeComputationBuilder()
{
    _Data &data = _GetData();

    // TODO: If the expression has a registered inverse, we can use this
    // function to validate that the expression only uses inputs "approved" for
    // invertible expressions.

    Exec_DefinitionRegistry::RegistrationAccess::
        _GetInstanceForRegistration().RegisterAttributeComputation(
            data.attributeName,
            data.schemaType,
            data.computationName,
            data.resultType,
            std::move(data.callback),
            std::move(data.inputKeys),
            _GetDispatchesOntoSchemas());
}

//
// ExecAttributeExpressionBuilder
//

ExecAttributeExpressionBuilder::ExecAttributeExpressionBuilder(
    const TfToken &attributeName,
    const TfType schemaType)
    : Exec_ComputationBuilderCRTPBase<ExecAttributeExpressionBuilder>(
        attributeName,
        schemaType,
        Exec_PrivateBuiltinComputations->computeExpression,
        /* dispatched */ false,
        /* dispatchesOntoSchemas */ {})
{
}

ExecAttributeExpressionBuilder::~ExecAttributeExpressionBuilder()
{
    _Data &data = _GetData();

    Exec_DefinitionRegistry::RegistrationAccess::
        _GetInstanceForRegistration().RegisterAttributeComputation(
            data.attributeName,
            data.schemaType,
            data.computationName,
            data.resultType,
            std::move(data.callback),
            std::move(data.inputKeys),
            _GetDispatchesOntoSchemas());
}

//
// ExecComputationBuilder
//

ExecComputationBuilder::ExecComputationBuilder(
    const TfType schemaType)
    : _schemaType(schemaType)
{
}

ExecComputationBuilder::~ExecComputationBuilder()
{
    Exec_DefinitionRegistry::RegistrationAccess::
        _GetInstanceForRegistration().SetComputationRegistrationComplete(
            _schemaType);
}

ExecComputationBuilder
ExecComputationBuilder::ConstructionAccess::Construct(
    const std::string &schemaTypeName)
{
    const TfType schemaType(TfType::FindByName(schemaTypeName));
    if (schemaType.IsUnknown()) {
        TF_CODING_ERROR(
            "Attempt to build computations for unknown schema type name '%s'.",
            schemaTypeName.c_str());
    }
    return ExecComputationBuilder(schemaType);
}

ExecPrimComputationBuilder 
ExecComputationBuilder::PrimComputation(
    const TfToken &computationName)
{
    return ExecPrimComputationBuilder(_schemaType, computationName);
}

ExecAttributeComputationBuilder 
ExecComputationBuilder::AttributeComputation(
    const TfToken &attributeName,
    const TfToken &computationName)
{
    return ExecAttributeComputationBuilder(
        attributeName, _schemaType, computationName);
}

ExecAttributeExpressionBuilder
ExecComputationBuilder::AttributeExpression(const TfToken &attributeName)
{
    return ExecAttributeExpressionBuilder(attributeName, _schemaType);
}

ExecPrimComputationBuilder 
ExecComputationBuilder::DispatchedPrimComputation(
    const TfToken &computationName,
    ExecDispatchesOntoSchemas &&ontoSchemas)
{
    return ExecPrimComputationBuilder(
        _schemaType,
        computationName,
        /* dispatched */ true,
        std::move(ontoSchemas));
}

ExecAttributeComputationBuilder 
ExecComputationBuilder::DispatchedAttributeComputation(
    const TfToken &computationName,
    ExecDispatchesOntoSchemas &&ontoSchemas)
{
    return ExecAttributeComputationBuilder(
        /* attributeName */ TfToken(),
        _schemaType,
        computationName,
        /* dispatched */ true,
        std::move(ontoSchemas));
}

PXR_NAMESPACE_CLOSE_SCOPE
