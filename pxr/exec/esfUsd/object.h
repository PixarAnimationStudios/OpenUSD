//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_OBJECT_H
#define PXR_EXEC_ESF_USD_OBJECT_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/relationship.h"
#include "pxr/usd/usd/object.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// Common implementation of EsfObjectInterface.
///
/// This implementation wraps an instance of UsdObject or subclass of UsdObject.
/// The exact type is specified by the \p UsdObjectType template parameter.
///
/// The \p InterfaceType template parameter controls which interface this class
/// inherits from, which can be EsfObjectInterface or any other interface that
/// extends EsfObjectInterface.
///
/// ## EsfUsd Inheritance Structure
///
/// EsfUsd defines implementations of the Esf interface classes. Naturally,
/// EsfUsd_Object inherits from EsfObjectInterface, EsfUsd_Prim inherits from
/// EsfPrimInterface, etc.
///
/// However, while all prims are objects (i.e. EsfPrimInterface extends
/// EsfObjectInterface), EsfUsd_Prim does _not_ inherit from EsfUsd_Object.
/// This means EsfUsd_Prim needs to override and re-implement the virtual
/// methods of EsfObjectInterface in the same manner as EsfUsd_Object, but must
/// do so using a UsdPrim instead of a UsdObject.
///
/// We prevent code duplication by defining EsfUsd_XxxImpl class templates.
/// These templates provide a single implementation for virtual methods defined
/// by an Esf interface which can be "grafted" onto any subclass of that
/// interface while also operating on a generic USD object type.
///
/// For example, this is the full inheritance chain for EsfUsd_Attribute:
///
///  EsfObjectInterface
///    EsfPropertyInterface
///      EsfAttributeInterface
///        EsfUsd_ObjectImpl<EsfAttributeInterface, UsdAttribute>
///          EsfUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>
///            EsfUsd_Attribute
///
/// And here is the full inheritance chain for EsfUsd_Prim. Note that in this
/// diagram, EsfUsd_ObjectImpl inherits from EsfPrimInterface instead.
///
///  EsfObjectInterface
///    EsfPrimInterface
///      EsfUsd_ObjectImpl<EsfPrimInterface, UsdPrim>
///        EsfUsd_Prim
///
template <class InterfaceType, class UsdObjectType>
class EsfUsd_ObjectImpl : public InterfaceType
{
    static_assert(std::is_base_of_v<EsfObjectInterface, InterfaceType>);
    static_assert(std::is_base_of_v<UsdObject, UsdObjectType>);

public:
    ~EsfUsd_ObjectImpl() override;

    /// Copies the provided object into this instance.
    EsfUsd_ObjectImpl(const UsdObjectType &object)
        : InterfaceType(object.GetPath())
        , _object(object) {}

    /// Moves the provided object into this instance.
    EsfUsd_ObjectImpl(UsdObjectType &&object)
        : InterfaceType(object.GetPath())
        , _object(std::move(object)) {}

protected:
    // Accessors to the wrapped object are made available to all derived
    // classes.
    // 
    UsdObjectType &_GetWrapped() { return _object; }
    const UsdObjectType &_GetWrapped() const { return _object; }

private:
    // The wrapped native usd object.
    UsdObjectType _object;

    // EsfObjectInterface implementation.
    bool _IsValid() const final;
    TfToken _GetName() const final;
    EsfPrim _GetPrim() const final;
    EsfStage _GetStage() const final;
    EsfSchemaConfigKey _GetSchemaConfigKey() const final;
    bool IsPrim() const final;
    bool IsAttribute() const final;
    bool IsRelationship() const final;
    EsfObject AsObject() const final;
    EsfPrim AsPrim() const final;
    EsfAttribute AsAttribute() const final;
    EsfRelationship AsRelationship() const final;
};

/// Implementation of EsfObjectInterface that wraps a UsdObject.
using EsfUsd_Object = EsfUsd_ObjectImpl<EsfObjectInterface, UsdObject>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
