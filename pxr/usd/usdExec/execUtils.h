//
// Unlicensed 2022 benmalartre
//

#ifndef PXR_USD_EXEC_UTILS_H
#define PXR_USD_EXEC_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/usd/usdExec/execTypes.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecInput;
class UsdExecOutput;

/// \class UsdExecUtils
///
/// This class contains a set of utility functions used when authoring and 
/// querying exec networks.
///
class UsdExecUtils {
public:
    /// Returns the namespace prefix of the USD attribute associated with the
    /// given shading attribute type.
    USDEXEC_API
    static std::string GetPrefixForAttributeType(
        UsdExecAttributeType sourceType);

    /// Given the full name of a shading attribute, returns it's base name and
    /// shading attribute type.
    USDEXEC_API
    static std::pair<TfToken, UsdExecAttributeType> 
        GetBaseNameAndType(const TfToken &fullName);

    /// Given the full name of a shading attribute, returns its shading
    /// attribute type.
    USDEXEC_API
    static UsdExecAttributeType GetType(const TfToken &fullName);

    /// Returns the full shading attribute name given the basename and the
    /// shading attribute type. \p baseName is the name of the input or output
    /// on the shading node. \p type is the \ref UsdExecAttributeType of the
    /// shading attribute.
    USDEXEC_API
    static TfToken GetFullName(const TfToken &baseName, 
                               const UsdExecAttributeType type);

    /// \brief Find what is connected to an Input or Output recursively
    ///
    /// GetValueProducingAttributes implements the UsdExec connectivity rules
    /// described in \ref UsdExecAttributeResolution .
    ///
    /// When tracing connections within networks that contain containers like
    /// UsdExecNodeGraph nodes, the actual output(s) or value(s) at the end of
    /// an input or output might be multiple connections removed. The methods
    /// below resolves this across multiple physical connections.
    ///
    /// An UsdExecInput is getting its value from one of these sources:
    /// - If the input is not connected the UsdAttribute for this input is
    /// returned, but only if it has an authored value. The input attribute
    /// itself carries the value for this input.
    /// - If the input is connected we follow the connection(s) until we reach
    /// a valid output of a UsdExecNode node or if we reach a valid
    /// UsdExecInput attribute of a UsdExecNodeGraph or UsdExecMaterial that
    /// has an authored value.
    ///
    /// An UsdExecOutput on a container can get its value from the same
    /// type of sources as a UsdExecInput on either a UsdExecNode or
    /// UsdExecNodeGraph. Outputs on non-containers (UsdExecNodes) cannot be
    /// connected.
    ///
    /// This function returns a vector of UsdAttributes. The vector is empty if
    /// no valid attribute was found. The type of each attribute can be
    /// determined with the \p UsdExecUtils::GetType function.
    ///
    /// If \p outputsOnly is true, it will only report attributes that are
    /// outputs of non-containers (UsdExecNodes). This is a bit faster and
    /// what is need when determining the connections for Material terminals.
    ///
    /// \note This will return the last attribute along the connection chain
    /// that has an authored value, which might not be the last attribute in the
    /// chain itself.
    /// \note When the network contains multi-connections, this function can
    /// return multiple attributes for a single input or output. The list of
    /// attributes is build by a depth-first search, following the underlying
    /// connection paths in order. The list can contain both UsdExecOutput and
    /// UsdExecInput attributes. It is up to the caller to decide how to
    /// process such a mixture.
    USDEXEC_API
    static UsdExecAttributeVector GetValueProducingAttributes(
        UsdExecInput const &input,
        bool outputsOnly = false);
    /// \overload
    USDEXEC_API
    static UsdExecAttributeVector GetValueProducingAttributes(
        UsdExecOutput const &output,
        bool outputsOnly = false);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
