//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_COLLECTION_PREDICATE_LIBRARY_H
#define PXR_IMAGING_HD_COLLECTION_PREDICATE_LIBRARY_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/usd/sdf/predicateLibrary.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdSceneIndexPrim;

using HdCollectionPredicateLibrary =
    SdfPredicateLibrary<const HdSceneIndexPrim &>;

///
/// \defgroup group_hd_collectionPredicates Hydra Collection Predicate API
/// Functions for use in path expressions that are evaluated on prims in a
/// scene index.
/// @{
/// The library returned by \ref HdGetCollectionPredicateLibrary provides the 
/// following predicate functions:
///
/// \li \c hdType(string primType)
///     Returns true if the scene index prim's type is \p primType.
///
/// \li \c hdVisible(bool visibility = true)
///     Returns true if the scene index prim's visibility is \p visibility.
///
/// \li \c hdPurpose(string purpose)
///     Returns true if the scene index prim's purpose is \p purpose.
///
/// \li \c hdHasDataSource(string locatorStr)
///     Returns true if the scene index prim's container has a valid data source
///     at data source locator \p locatorStr.
///     Multiple locator tokens may be provided by using '.' as the delimiter.
///     e.g. "primvars.foo".
///     A locator token may contain a namespace prefix.
///     e.g. "primvars.ri:baz" is parsed as two tokens, "primvars" and "ri:baz".
///
/// \li \c hdHasPrimvar(string primvarName)
///     Returns true if the scene index prim has a primvar named \p primvarName.
///
/// \li \c hdHasMaterialBinding(string materialPath)
///     Returns true if the scene index prim's resolved (allPurpose) material
///     binding path contains the substring \p materialPath.
///
/// \deprecated
/// The following predicate functions are deprecated and will be removed in a
/// future release:
///
/// \li \c type
/// \li \c visible
/// \li \c purpose
/// \li \c hasDataSOurce
/// \li \c hasPrimvar
/// \li \c hasMaterialBinding
///
/// Any predicate functions in hd will use the 'hd' prefix henceforth to make
/// it clear to the author/reader that it is a (core) hydra predicate.
///
/// \section hd_predicate_usage Usage Examples
/// \ingroup group_hd_collectionPredicates
///
/// This section provides usage examples for the Hydra Collection Predicate API.
///
/// \c "/World//{hdType:basisCurves}" matches all descendant prims of /World that 
/// are basis curves.
///
/// \c "//{hdVisible:false}" matches all scene index prims that are invisible.
///
/// \c "//{hdPurpose:guide}" matches all scene index prims whose purpose is 
/// 'guide'.
///
/// \c "//Foo/{hdHasDataSource:"bar.baz"}" matches children of any prim named Foo 
/// that have a valid data source at bar.baz .
///
/// \c "/Foo//{hdHasPrimvar:baz}" matches all descendant prims of Foo that have a
/// primvar named "baz".
///
/// \c "//{hdHasMaterialBinding:"GlossyMat"}" matches all scene index prims
/// whose resolved (allPurpose) material binding path contains the string
/// "GlossyMat".
///

/// \brief Return a predicate library with a foundational set of predicate 
/// functions to evaluate SdfPathExpressions on prims in a scene index.
///
/// To evaluate the path expression, an evaluator object is constructed with
/// the predicate library as an argument.
///
/// \sa HdCollectionExpressionEvaluator
/// \sa SdfPathExpression
/// 
HD_API
const HdCollectionPredicateLibrary &
HdGetCollectionPredicateLibrary();

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_COLLECTION_PREDICATE_LIBRARY_H