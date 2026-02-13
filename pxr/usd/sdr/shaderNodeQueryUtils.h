//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_SHADER_NODE_QUERY_UTILS_H
#define PXR_USD_SDR_SHADER_NODE_QUERY_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/sdr/shaderNodeQuery.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace SdrShaderNodeQueryUtils {

/// Return shader node query results in a nested tree form.
///
/// For example, if a query result contains:
/// - values
///     ["context1", "id1"]
///     ["context1", "id2"]
/// - one shader node corresponding to the first value row
/// - two shader nodes corresponding to the second value row
/// the grouped result will be a VtDictionary of the form
///     {
///       "context1": {
///           "id1": [<SdrShaderNodeConstPtr>]
///           "id2": [<SdrShaderNodeConstPtr>, <SdrShaderNodeConstPtr>]
///        }
///     }
/// where the SdrShaderNodeConstPtr container is a VtValue holding
/// std::vector<SdrShaderNodeConstPtr> and is sorted alphabetically by
/// node identifier, then sourceType.
///
/// Values are stringified by SdrShaderNodeQueryResult::GetStringifiedValues.
///
/// Note that this utility doesn't help collapse empty values, but will
/// rather preserve empty values as keys. For example, a query result
/// containing values from a single node might have values:
///     [["a", "", "b"]]
/// For this above example, this function would return
///     {"a": {"": {"b": [<SdrShaderNodeConstPtr>] }}}
///
/// Note that innermost nodes are always contained in a list structure.
///
/// Returns an empty VtDictionary if the given query result has no keys or
/// if the result has no nodes.
SDR_API
VtDictionary GroupQueryResults(const SdrShaderNodeQueryResult& result);

} // end namespace SdrShaderNodeQueryUtils

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SHADER_NODE_QUERY_UTILS_H
