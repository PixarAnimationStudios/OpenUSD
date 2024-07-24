//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_CONNECTABLE_BEHAVIOR_H
#define PXR_USD_USD_SHADE_CONNECTABLE_BEHAVIOR_H

/// \file usdShade/connectableAPIBehavior.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;
class UsdShadeInput;
class UsdShadeOutput;

/// UsdShadeConnectableAPIBehavior defines the compatibilty and behavior
/// UsdShadeConnectableAPIof when applied to a particular prim type.
///
/// This enables schema libraries to enable UsdShadeConnectableAPI for
/// their prim types and define its behavior.
class UsdShadeConnectableAPIBehavior
{
public:

    /// An enum describing the types of connectable nodes which will govern what
    /// connectibility rule is invoked for these.
    enum ConnectableNodeTypes
    {
        BasicNodes, // Shader, NodeGraph
        DerivedContainerNodes, // Material, etc
    };

    // By default we want a connectableBehavior to not exhibit a container like
    // behavior. And we want encapsulation behavior enabled by default.
    USDSHADE_API
    UsdShadeConnectableAPIBehavior() 
        : _isContainer(false), _requiresEncapsulation(true) {}

    USDSHADE_API
    UsdShadeConnectableAPIBehavior(const bool isContainer, 
            const bool requiresEncapsulation)
        : _isContainer(isContainer), 
        _requiresEncapsulation(requiresEncapsulation) {}

    USDSHADE_API
    virtual ~UsdShadeConnectableAPIBehavior();

    /// The prim owning the input is guaranteed to be of the type this
    /// behavior was registered with. The function must be thread-safe.
    ///
    /// It should return true if the connection is allowed, false
    /// otherwise. If the connection is prohibited and \p reason is
    /// non-NULL, it should be set to a user-facing description of the
    /// reason the connection is prohibited.
    ///
    /// The base implementation checks that the input is defined; that
    /// the source attribute exists; and that the connectability metadata
    /// on the input allows a connection from the attribute -- see
    /// UsdShadeInput::GetConnectability().
    ///
    USDSHADE_API
    virtual bool
    CanConnectInputToSource(const UsdShadeInput &,
                            const UsdAttribute &,
                            std::string *reason) const;

    /// The prim owning the output is guaranteed to be of the type this
    /// behavior was registered with. The function must be thread-safe.
    ///
    /// It should return true if the connection is allowed, false
    /// otherwise. If the connection is prohibited and \p reason is
    /// non-NULL, it should be set to a user-facing description of the
    /// reason the connection is prohibited.
    ///
    /// The base implementation returns false. Outputs of most prim
    /// types will be defined by the underlying node definition (see
    /// UsdShadeNodeDefAPI), not a connection.
    ///
    USDSHADE_API
    virtual bool
    CanConnectOutputToSource(const UsdShadeOutput &,
                             const UsdAttribute &,
                             std::string *reason) const;

    /// The function must be thread-safe.
    ///
    /// It should return true if the associated prim type is considered
    /// a "container" for connected nodes. 
    /// Returns the value set for _isContainer.
    USDSHADE_API
    virtual bool 
    IsContainer() const final;

    /// The function must be thread-safe.
    ///
    /// Determines if the behavior should respect container encapsulation rules
    /// (\ref UsdShadeConnectability), when evaluating CanConnectInputToSource 
    /// or CanConnectOutputToSource. This should return true if the container 
    /// encapsulation rules need to be respected, false otherwise. 
    //
    /// Returns the value set for _requiresEncapsulation.
    ///
    /// \sa IsContainer()
    /// 
    USDSHADE_API
    virtual bool
    RequiresEncapsulation() const final;

protected:
    /// Helper function to separate and share special connectivity logic for 
    /// specialized, NodeGraph-derived nodes, like Material (and other in other 
    /// domains) that allow their inputs to be connected to an output of a 
    /// source that they directly contain/encapsulate. The default behavior is 
    /// for Shader Nodes or NodeGraphs which allow their input connections to 
    /// output of a sibling source, both encapsulated by the same container 
    /// node.
    USDSHADE_API
    bool _CanConnectInputToSource(const UsdShadeInput&, const UsdAttribute&,
                                  std::string *reason, 
                                  ConnectableNodeTypes nodeType = 
                                    ConnectableNodeTypes::BasicNodes) const;

    USDSHADE_API
    bool _CanConnectOutputToSource(const UsdShadeOutput&, const UsdAttribute&,
                                   std::string *reason,
                                   ConnectableNodeTypes nodeType =
                                     ConnectableNodeTypes::BasicNodes) const;
private:
    bool _isContainer;
    bool _requiresEncapsulation;
};

/// Registers \p behavior to define connectability of attributes for \p PrimType.
///
/// Plugins should call this function in a TF_REGISTRY_FUNCTION.  For example:
/// 
/// \code
/// class MyBehavior : public UsdShadeConnectableAPIBehavior { ... }
/// 
/// TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
/// {
///     UsdShadeRegisterConnectableAPIBehavior<MyPrim, MyBehavior>();
/// }
/// \endcode
///
/// Plugins must also note that UsdShadeConnectableAPI behavior is implemented
/// for a prim type in that type's schema definnition.  For example: 
///
/// \code
/// class "MyPrim" (
///     ...
///     customData = {
///         dictionary extraPlugInfo = {
///             bool providesUsdShadeConnectableAPIBehavior = true
///         }
///     }
///     ...
/// )
/// { ... }
/// \endcode
///
/// This allows the plugin system to discover this behavior dynamically
/// and load the plugin if needed.
/// 
/// In addition to Typed schemas, single apply API schemas can also include
/// **providesUsdShadeConnectableAPIBehavior** in their **extraPlugInfo** and 
/// hence impart connectableAPIBehavior to the prim definition in which they 
/// are participating. Additionally a schema can include metadata in their 
/// extraPlugInfo fields to override isContainer and requiresEncapsulation 
/// properties by specifying bool values for **isUsdShadeContainer** and
/// **requiresUsdShadeEncapsulation**. This can be especially useful for 
/// \ref codeless_schema that cannot provide a C++ derivation of
/// UsdShadeConnectableAPIBehavior.
///
/// \ref UsdShadeConnectableAPIBehavior_ResolutionOrder defines the
/// resolution order when multiple types and apiSchemas provide a 
/// UsdShadeConnectableAPIBehavior.
/// 
template <class PrimType, class BehaviorType = UsdShadeConnectableAPIBehavior>
inline void 
UsdShadeRegisterConnectableAPIBehavior()
{
    UsdShadeRegisterConnectableAPIBehavior(
        TfType::Find<PrimType>(),
        std::shared_ptr<UsdShadeConnectableAPIBehavior>(new BehaviorType));
}

/// Registers \p behavior to define connectability of attributes for
/// \p PrimType.
USDSHADE_API
void 
UsdShadeRegisterConnectableAPIBehavior(
    const TfType& connectablePrimType,
    const std::shared_ptr<UsdShadeConnectableAPIBehavior>& behavior);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_SHADE_CONNECTABLE_BEHAVIOR_H
