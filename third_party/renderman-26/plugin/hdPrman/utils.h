//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/imaging/hd/dataSource.h"

#include "RiTypesHelper.h" // for RtParamList

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class SdfAssetPath;
class VtValue;

namespace HdPrman_Utils {

/// Adds (or updates) a VtValue parameter to \p params and returns true if
/// the parameter was set.
bool
SetParamFromVtValue(
    RtUString const& name,
    VtValue const& val,
    TfToken const& role,
    RtParamList *params);

/// Similar to the function above, with the addition of \p detail, which 
/// specifies how array values should be handled across topology.
bool
SetPrimVarFromVtValue(
    RtUString const& name,
    VtValue const& val,
    RtDetailType const& detail,
    TfToken const& role,
    RtPrimVarList *params);

/// Helper to convert matrix types, handling double->float conversion.
inline RtMatrix4x4
GfMatrixToRtMatrix(const GfMatrix4d &m)
{
    const double *d = m.GetArray();
    return RtMatrix4x4(
        d[0], d[1], d[2], d[3],
        d[4], d[5], d[6], d[7],
        d[8], d[9], d[10], d[11],
        d[12], d[13], d[14], d[15]);
}

/// Helper to convert matrix types, handling float->double conversion.
inline GfMatrix4d
RtMatrixToGfMatrix(const RtMatrix4x4 &m)
{
    return GfMatrix4d(
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}

/// Attempt to extract a useful texture identifier from the given \p asset.
/// If \p asset is determined to not be a .tex file, attempt to use the Hio
/// based Rtx plugin to load the texture.  If \p asset is non-empty, we will
/// always return _something_
RtUString
ResolveAssetToRtUString(
    SdfAssetPath const &asset,
    bool flipTexture = true,
    bool writeAsset = false,
    char const *debugNodeType = nullptr);

/// Some quantites previously given as options now need to be provided
/// through different Riley APIs. This method returns a pruned
/// copy of the options, to be provided to SetOptions().
RtParamList
PruneDeprecatedOptions(
    const RtParamList &options);

/// Some options, such as exitat, should only apply to a batch-mode render.
/// This method returns a pruned copy of the options, removing those
/// that should only be used in batch rendering.
RtParamList
PruneBatchOnlyOptions(
    const RtParamList &options);

/// Returns a small set of options for default path tracer configuration.
RtParamList
GetDefaultRileyOptions();

/// Returns the options driven by environment variables.
RtParamList
GetRileyOptionsFromEnvironment();

/// Missing from std until C++20
template<typename T>
struct remove_cvref
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

/// Missing from std until C++20
template< class T >
using remove_cvref_t = typename remove_cvref<T>::type;

/// Return a new ParamList (or PrimVarList) by composing together all the given
/// \p args, which must be of the same type. Where a given param or primvar is
/// present in multiple lists in \p args, the opinion from the earliest such
/// list wins. Each input list in \p args must be passable by reference and may
/// be const or non-const.
template <
    typename ...Ts,
    typename T = remove_cvref_t<std::tuple_element_t<0,std::tuple<Ts...>>>>
T
Compose(Ts&& ...args)
{
    static_assert(
        std::conjunction<
            std::disjunction<
                std::is_same<RtParamList, T>, std::is_same<RtPrimVarList, T>>,
            std::is_reference<Ts>...,
            std::is_same<T, remove_cvref_t<Ts>>...>::value,
        "Arguments must be all RtParamList& or all RtPrimVarList&");
    const size_t n = sizeof...(Ts);
    T result;
    if (n == 0) {
        return result;
    }
    std::vector<std::reference_wrapper<const T>> list
        { std::forward<Ts>(args)... };
    std::reverse(list.begin(), list.end());
    for (const T& b : list) {
        if (b.GetNumParams() != 0) {
            result.Update(b);
        }
    }
    return result;
}

} // namespace HdPrman_Utils

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H
