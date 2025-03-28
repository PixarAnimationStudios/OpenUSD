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

/// Return a predicate library with a foundational set of predicate functions
/// to evaluate SdfPathExpressions on prims in a scene index.
///
/// To evaluate the path expression, an evaluator object is constructed with
/// the predicate library as an argument.
///
/// \sa HdCollectionExpressionEvaluator
/// \sa SdfPathExpression
///
/// ----------------------------------------------------------------------------
///
/// The library returned provides the following predicate functions:
///
/// hdType(string primType)
///     Returns true if the scene index prim's type is \p primType.
///
/// hdVisible(bool visibility = true)
///     Returns true if the scene index prim's visibility is \p visibility.
///
/// hdPurpose(string purpose)
///     Returns true if the scene index prim's purpose is \p purpose.
///
/// hdHasDataSource(string locatorStr)
///     Returns true if the scene index prim's container has a valid data source
///     at data source locator \p locatorStr.
///     Multiple locator tokens may be provided by using '.' as the delimiter.
///     e.g. "primvars.foo".
///     A locator token may contain a namespace prefix.
///     e.g. "primvars.ri:baz" is parsed as two tokens, "primvars" and "ri:baz".
///
/// hdHasPrimvar(string primvarName)
///     Returns true if the scene index prim has a primvar named \p primvarName.
///
/// hdHasMaterialBinding(string materialPath)
///     Returns true if the scene index prim's resolved (allPurpose) material
///     binding path contains the substring \p materialPath.
///
/// \deprecated
/// The following predicate functions are deprecated and will be removed in a
/// future release:
///
/// \li type
/// \li visible
/// \li purpose
/// \li hasDataSOurce
/// \li hasPrimvar
/// \li hasMaterialBinding
///
/// Any predicate functions in hd will use the 'hd' prefix henceforth to make
/// it clear to the author/reader that it is a (core) hydra predicate.
///
/// ----------------------------------------------------------------------------
///
/// Usage examples:
///
/// "/World//{hdType:basisCurves}" matches all descendant prims of /World that 
/// are basis curves.
///
/// "//{hdVisible:false}" matches all scene index prims that are invisible.
///
/// "//{hdPurpose:guide}" matches all scene index prims whose purpose is 
/// 'guide'.
///
/// "//Foo/{hdHasDataSource:"bar.baz"}" matches children of any prim named Foo 
/// that have a valid data source at bar.baz .
///
/// "/Foo//{hdHasPrimvar:baz}" matches all descendant prims of Foo that have a
/// primvar named "baz".
///
/// "//{hdHasMaterialBinding:"GlossyMat"}" matches all scene index prims
/// whose resolved (allPurpose) material binding path contains the string
/// "GlossyMat".
///
/// ----------------------------------------------------------------------------
///
HD_API
const HdCollectionPredicateLibrary &
HdGetCollectionPredicateLibrary();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_COLLECTION_PREDICATE_LIBRARY_H