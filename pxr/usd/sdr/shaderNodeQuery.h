//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_SHADER_NODE_QUERY_H
#define PXR_USD_SDR_SHADER_NODE_QUERY_H

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdr/shaderNode.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class SdrRegistry;
class SdrShaderNodeQueryResult;

/// SdrShaderNodeQuery is a constraint-based query builder object that
/// operates on all SdrShaderNodes contained in the SdrRegistry.
///
/// Queries can be used to get nodes associated with given constraints, or to
/// examine specific data from the nodes. For example, here's a query that
/// indicates it wants all the nodes satisfying some constraints:
/// \code
/// SdrShaderNodeQuery query = SdrShaderNodeQuery()
///     .NodeValueIs(TfToken("customMetadataItem"), "stage2")
///     .NodeValueIsNot(SdrNodeFieldKey->Identifier, "notthisone");
/// \endcode
///
/// For this query, SdrRegistry::RunQuery(query) returns a result containing
/// all the nodes associated with the given constraints.
///
/// Queries may additionally specify `SelectDistinct` to get aggregated data
/// from the nodes satisfying given constraints. For example:
/// \code
/// SdrShaderNodeQuery query2 = SdrShaderNodeQuery()
///     .SelectDistinct(SdrNodeFieldKey->Family)
///     .SelectDistinct(TfToken("customMetadataItem"))
///     .NodeHasValueFor(TfToken("customMetadataItem"))
/// \endcode
///
/// In this case, SdrRegistry::RunQuery(query2) would return a result
/// containing distinct combinations of the requested family field and
/// "customMetadataItem" values, along with the nodes satisfying said
/// combinations under the given constraint that the node must have
/// "customMetadataItem" defined in its metadata.
///
/// For more information on how query results are computed and represented,
/// see `SelectDistinct` and `SdrShaderNodeQueryResult`.
/// \sa SdrShaderNodeQuery::SelectDistinct
/// \sa SdrShaderNodeQueryResult
///
/// Note that SdrRegistry::RunQuery will cause all nodes in the registry
/// to be parsed in order to examine data on these nodes in their
/// final form.
class SdrShaderNodeQuery {
public:
    typedef std::function<bool (SdrShaderNodeConstPtr)> FilterFn;

    /// SelectDistinct asks for distinct information from SdrShaderNodes via
    /// the SdrShaderNode::GetDataForKey method.
    ///
    /// Any number of keys can be requested via the SelectDistinct API on a
    /// single query. If SelectDistinct is not called and therefore no keys
    /// are requested, the result of running this query will contain no data
    /// but will contain nodes that satisfy the query filter constraints.
    ///
    /// From each SdrShaderNode in the registry that satisfies the
    /// query filter constraints, this query will extract
    /// a "list of values" corresponding to the keys requested by
    /// SelectDistinct calls. SelectDistinct ensures that no duplicate
    /// "list of values" will appear in the SdrQueryResult. This echoes
    /// SQL's "SELECT DISTINCT" behavior. For example, these results are
    /// preserved as seen because they aren't duplicates of each other even
    /// though some entries are shared across them:
    ///    - math, abs
    ///    - math, add
    ///    - brep, add
    ///
    /// but the following duplicate results from different nodes would be
    /// coalesced into a single item instead.
    ///    - math, abs
    ///    - math, abs
    ///
    /// If a data item requested by `SelectDistinct` doesn't exist for a node
    /// that otherwise satisfies all constraints of this query, an empty
    /// VtValue is used to represent the nonexistent item.
    ///
    /// SdrShaderNodes from which these entries have been extracted will
    /// be returned alongside them in the query result.
    /// \sa SdrShaderNodeQueryResult 
    SDR_API
    SdrShaderNodeQuery& SelectDistinct(const TfToken& key);

    /// Specify multiple keys to query data for.
    /// \sa SelectDistinct(const TfToken& key)
    SDR_API
    SdrShaderNodeQuery& SelectDistinct(const std::vector<TfToken>& key);

    /// \name Node Filtering
    /// 
    /// NodeValueIs, NodeValueIsIn, NodeHasValueFor:
    /// These inclusion constraints are joined with each other by boolean "and"
    /// semantics, so only SdrShaderNodes that satisfy *all* of these inclusion
    /// constraints are kept.
    ///
    /// NodeValueIsNot, NodeValueIsNotIn, NodeHasNoValueFor:
    /// These exclusion constraints are joined with each other by boolean "or"
    /// semantics, so only SdrShaderNodes that satisfy *none* of these
    /// exclusion constraints are kept.
    ///
    /// \note nonexistence and empty VtValue are considered equivalent states.
    /// VtValue containing an empty item (e.g. empty string) is not equivalent
    /// to the former states. For example: node A has metadata 
    /// {"foo": VtValue()} and node B has metadata {}. The metadata item
    /// keyed by "foo" does not exist on A; nor does it exist on B. As a
    /// consequence, NodeValueIs(key, VtValue()) is equivalent to
    /// NodeHasNoValueFor(key); and NodeHasValueFor(key) is equivalent to
    /// NodeValueIsNot(key, VtValue()).
    ///
    /// {@

    /// Only keep SdrShaderNodes whose value returned from
    /// SdrShaderNode::GetDataForKey(key) matches the given `value`.
    SDR_API
    SdrShaderNodeQuery& NodeValueIs(const TfToken& key, const VtValue& value);

    template <typename T>
    SdrShaderNodeQuery& NodeValueIs(const TfToken& key, const T& value) {
        return NodeValueIs(key, VtValue(value));
    }

    /// Only keep SdrShaderNodes whose value returned from
    /// SdrShaderNode::GetDataForKey(key) matches any of the given `values`.
    SDR_API
    SdrShaderNodeQuery& NodeValueIsIn(const TfToken& key,
                                       const std::vector<VtValue>& values);

    /// Only keep SdrShaderNodes that have an existing value for
    /// for the given key.
    SDR_API
    SdrShaderNodeQuery& NodeHasValueFor(const TfToken& key);

    /// Only keep SdrShaderNodes whose value returned from
    /// SdrShaderNode::GetDataForKey(key) doesn't match the given 'value'.
    SDR_API
    SdrShaderNodeQuery& NodeValueIsNot(const TfToken& key, const VtValue& value);

    template <typename T>
    SdrShaderNodeQuery& NodeValueIsNot(const TfToken& key, const T& value) {
        return NodeValueIsNot(key, VtValue(value));
    }

    /// Only keep SdrShaderNodes whose value returned from
    /// SdrShaderNode::GetDataForKey(key) doesn't match any of the given
    /// `values`.
    SDR_API
    SdrShaderNodeQuery& NodeValueIsNotIn(const TfToken& key,
                                         const std::vector<VtValue>& values);

    /// Only keep SdrShaderNodes that don't have an existing value for
    /// for the given key. Empty values are considered "existing".
    SDR_API
    SdrShaderNodeQuery& NodeHasNoValueFor(const TfToken& key);

    /// @}

    /// Supply a custom filter to this query. This custom filter function will
    /// run on every considered SdrShaderNode. When this function evaluates to
    /// true, the node will be kept for further consideration. When the
    /// function evaluates to false, the node will be discarded from further
    /// consideration.
    SDR_API
    SdrShaderNodeQuery& CustomFilter(FilterFn fn);

    /// Convenience to run this query on the SdrRegistry.
    ///
    /// Equivalent to SdrRegistry::RunQuery(query)
    SDR_API
    SdrShaderNodeQueryResult Run();

private:
    friend class SdrRegistry;

    std::vector<std::pair<TfToken, VtValue>> _hasValues;
    std::vector<std::pair<TfToken, std::vector<VtValue>>> _hasOneOfValues;
    std::vector<std::pair<TfToken, VtValue>> _lacksValues;
    std::vector<std::pair<TfToken, std::vector<VtValue>>> _lacksAllOfValues;
    std::vector<TfToken> _selectKeys;
    std::vector<FilterFn> _customFilters;
};

/// SdrShaderNodeQueryResult stores the results of an SdrShaderNodeQuery.
class SdrShaderNodeQueryResult {
public:
    /// Returns keys requested by SelectDistinct calls on SdrShaderNodeQuery
    /// in the order they were added to the query.
    ///
    /// If the query had no calls to SelectDistinct, returns an empty vector.
    const TfTokenVector& GetKeys() const & { return _keys; }

    /// Overload for rvalues, move out the keys.
    TfTokenVector GetKeys() && { return std::move(_keys); }

    /// Returns distinct "list of values" extracted from SdrShaderNodes
    /// corresponding to keys requested by SelectDistinct calls on
    /// SdrShaderNodeQuery. The result is an (N x M) container of VtValues,
    /// where M is the number of keys and N is the number of distinct
    /// "list of values" (containing M number of VtValues)
    /// that correspond to the keys.
    ///
    /// Non-existent values are represented by empty VtValues.
    ///
    /// \sa SdrShaderNodeQuery::SelectDistinct
    ///
    /// If the query had no calls to SelectDistinct, returns an empty vector.
    const std::vector<std::vector<VtValue>>& GetValues() const & { return _values; }

    /// Overload for rvalues, move out the values.
    std::vector<std::vector<VtValue>> GetValues() && {
        return std::move(_values);
    }

    /// Convenience to convert all values to T.
    ///
    /// If the query had no calls to SelectDistinct, returns an empty vector.
    ///
    /// If any value fails conversion, an empty vector is returned, and
    /// a TF_CODING_ERROR is issued. Empty VtValues will always fail
    /// conversion. 
    template <class T>
    std::vector<std::vector<T>> GetValuesAs() const {
        std::vector<std::vector<T>> result;
        result.reserve(_values.size());
        for (const std::vector<VtValue>& row : _values) {
            std::vector<T> convertedRows;
            convertedRows.reserve(row.size());
            for (const VtValue& value : row) {
                const VtValue convertedValue = VtValue::Cast<T>(value);
                if (convertedValue.IsEmpty()) {
                    TF_CODING_ERROR(
                        "Failed to get value of type %s from value '%s'",
                        typeid(T).name(), TfStringify(value).c_str());
                    return {};
                } else {
                    convertedRows.push_back(convertedValue);
                }
            result.push_back(convertedRows);
            }
        }
        return result;
    }

    /// Get string representations of all values, as provided by TfStringify
    SDR_API
    std::vector<std::vector<std::string>> GetStringifiedValues() const;

    /// Gets shader nodes, grouped by value rows.
    /// 
    /// The result is an (N x S) container of shader nodes, where S is the
    /// number of shader nodes that have the key-value characteristics
    /// represented by the "nth" row of the returned structure of GetValues.
    /// S is not constant, and may vary from row to row.
    ///
    /// Each SdrShaderNodePtrVec is sorted alphabetically by identifier,
    /// then sourceType.
    ///
    /// If the query had no calls to SelectDistinct, returns an empty vector.
    SDR_API
    std::vector<SdrShaderNodePtrVec> GetShaderNodesByValues() const;

    /// Returns all shader nodes that match the constraints of the query.
    ///
    /// The resulting SdrShaderNodePtrVec is sorted alphabetically by identifier,
    /// then sourceType.
    SDR_API
    SdrShaderNodePtrVec GetAllShaderNodes() const;

private:
    friend class SdrRegistry;

    /// Returns true if the contents of this result are well-formed.
    bool _IsValid() const;

    std::vector<TfToken> _keys;
    std::vector<std::vector<VtValue>> _values;
    std::vector<SdrShaderNodePtrVec> _nodes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SHADER_NODE_QUERY_H
