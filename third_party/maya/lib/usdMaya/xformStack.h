// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef PXRUSDMAYA_XFORM_STACK_H
#define PXRUSDMAYA_XFORM_STACK_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include <maya/MTransformationMatrix.h>

#include <limits>
#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// Note: pivotTranslate is currently not used in MayaXformStack,
// CommonXformStack, or MatrixStack, so it should never occur
// at present, but there was some support for reading it, thus
// why it's here

/// \hideinitializer
#define PXRUSDMAYA_XFORM_STACK_TOKENS \
    (translate) \
    (rotatePivotTranslate) \
    (rotatePivot) \
    (rotate) \
    (rotateAxis) \
    (scalePivotTranslate) \
    (scalePivot) \
    (shear) \
    (scale) \
    (pivot) \
    (pivotTranslate) \
    (transform)

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaXformStackTokens,
        PXRUSDMAYA_API,
        PXRUSDMAYA_XFORM_STACK_TOKENS);

class _PxrUsdMayaXformOpClassificationData;
class _PxrUsdMayaXformStackData;

/// \class PxrUsdMayaXformOpClassification
/// \brief Defines a named "class" of xform operation
///
/// Similar to UsdGeomXformOp, but without a specific attribute;
/// UsdGeomXformOps can be thought of as "instances" of a
/// PxrUsdMayaXformOpDefinition "type"
class PxrUsdMayaXformOpClassification
{
public:
    PxrUsdMayaXformOpClassification(const TfToken &name,
                                       UsdGeomXformOp::Type opType,
                                       bool isInvertedTwin=false);

    PxrUsdMayaXformOpClassification();

    static PxrUsdMayaXformOpClassification const & NullInstance();

    PXRUSDMAYA_API
    TfToken const &GetName() const;

    PXRUSDMAYA_API
    UsdGeomXformOp::Type GetOpType() const;

    PXRUSDMAYA_API
    bool IsInvertedTwin() const;;

    /// Return True if the given op type is compatible with this
    /// OpClassification (ie, is the same, or is, say rotateX, when
    /// this op type is rotateXYZ)
    PXRUSDMAYA_API
    bool IsCompatibleType(UsdGeomXformOp::Type otherType) const;

    PXRUSDMAYA_API
    bool operator ==(const PxrUsdMayaXformOpClassification& other) const;

    PXRUSDMAYA_API
    bool IsNull() const;

    PXRUSDMAYA_API
    std::vector<TfToken> CompatibleAttrNames() const;

private:
    // Because this is an immutable type, we keep a pointer to shared
    // data; this allows us to only have overhead associated with
    // a RefPtr, while having easy-python-wrapping (without overhead
    // of WeakPtr)
    typedef TfRefPtr<_PxrUsdMayaXformOpClassificationData> DataRefPtr;

    DataRefPtr _sharedData;
};

/// \class PxrUsdMayaXformStack
/// \brief Defines a standard list of xform operations.
///
/// Used to define the set and order of transforms that programs like
/// Maya use and understand.
class PxrUsdMayaXformStack
{
public:
    typedef PxrUsdMayaXformOpClassification OpClass;
    typedef std::vector<OpClass> OpClassList;
    typedef std::pair<const OpClass, const OpClass> OpClassPair;

    // Internally, we use indices, because position sometimes matters...
    // should be safe, since _ops is const.
    static constexpr size_t NO_INDEX = std::numeric_limits<size_t>::max();
    typedef std::pair<size_t, size_t> IndexPair;
    typedef std::unordered_map<TfToken, IndexPair, TfToken::HashFunctor>
            TokenIndexPairMap;
    typedef std::unordered_map<size_t, size_t> IndexMap;


    // Templated because we want it to work with both MEulerRotation::RotationOrder
    // and MTransformationMatrix::RotationOrder
    template<typename RotationOrder>
    static RotationOrder RotateOrderFromOpType(
            UsdGeomXformOp::Type opType,
            RotationOrder defaultRotOrder=RotationOrder::kXYZ)
    {
        switch(opType) {
            case UsdGeomXformOp::TypeRotateXYZ:
                return  RotationOrder::kXYZ;
            break;
            case UsdGeomXformOp::TypeRotateXZY:
                return  RotationOrder::kXZY;
            break;
            case UsdGeomXformOp::TypeRotateYXZ:
                return  RotationOrder::kYXZ;
            break;
            case UsdGeomXformOp::TypeRotateYZX:
                return  RotationOrder::kYZX;
            break;
            case UsdGeomXformOp::TypeRotateZXY:
                return  RotationOrder::kZXY;
            break;
            case UsdGeomXformOp::TypeRotateZYX:
                return  RotationOrder::kZYX;
            break;
            default:
                return defaultRotOrder;
            break;
        }
    }

    PxrUsdMayaXformStack(
            const OpClassList& ops,
            const std::vector<IndexPair>& inversionTwins,
            bool nameMatters=true);

    // Don't want to accidentally make a copy, since the only instances are supposed
    // to be static!
    explicit PxrUsdMayaXformStack(const PxrUsdMayaXformStack& other) = default;
    explicit PxrUsdMayaXformStack(PxrUsdMayaXformStack&& other) = default;

    PXRUSDMAYA_API
    OpClassList const & GetOps() const;

    PXRUSDMAYA_API
    const std::vector<IndexPair>& GetInversionTwins() const;

    PXRUSDMAYA_API
    bool GetNameMatters() const;

    PXRUSDMAYA_API
    PxrUsdMayaXformOpClassification const & operator[] (const size_t index) const;

    PXRUSDMAYA_API
    size_t GetSize() const;


    /// \brief  Finds the index of the Op Classification with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// \param  isInvertedTwin the returned op classification object must match
    ///         this param for it's IsInvertedTwin() - if an op is found that matches
    ///         the name, but has the wrong invertedTwin status, NO_INDEX is returned
    /// return  Index to the op classification object with the given name (and
    ///         inverted twin state); will be NO_INDEX if no match could be found.
    PXRUSDMAYA_API
    size_t FindOpIndex(const TfToken& opName, bool isInvertedTwin=false) const;

    /// \brief  Finds the Op Classification with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// \param  isInvertedTwin the returned op classification object must match
    ///         this param for it's IsInvertedTwin() - if an op is found that matches
    ///         the name, but has the wrong invertedTwin status, OpClass::NullInstance
    ///         is returned
    /// return  Reference to the op classification object with the given name (and
    ///         inverted twin state); will be a reference to OpClass::NullInstance
    ///         if no match could be found.
    PXRUSDMAYA_API
    const OpClass& FindOp(const TfToken& opName, bool isInvertedTwin=false) const;

    /// \brief  Finds the indices of Op Classification(s) with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// return  A pair of indices to op classification objects with the given name;
    ///         if the objects are part of an inverted twin pair, then both are
    ///         returned (in the order they appear in this stack). If found, but
    ///         not as part of an inverted twin pair, the first result will point
    ///         to the found classification, and the second will be NO_INDEX.  If
    ///         no matches are found, both will be NO_INDEX.
    PXRUSDMAYA_API
    const IndexPair& FindOpIndexPair(const TfToken& opName) const;

    /// \brief  Finds the Op Classification(s) with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// return  A pair classification objects with the given name; if the objects
    ///         are part of an inverted twin pair, then both are returned (in the
    ///         order they appear in this stack). If found, but not as part of an
    ///         inverted twin pair, the first result will point to the found
    ///         classification, and the second will be OpClass::NullInstance.  If
    ///         no matches are found, both results will be OpClass::NullInstance.
    PXRUSDMAYA_API
    const OpClassPair FindOpPair(const TfToken& opName) const;

    /// \brief Returns a list of matching XformOpDefinitions for this stack
    ///
    /// For each xformop, we want to find the corresponding op within this
    /// stack that it matches.  There are 3 requirements:
    ///  - to be considered a match, the name and type must match an op in this stack
    ///  - the matches for each xformop must have increasing indexes in the stack
    ///  - inversionTwins must either both be matched or neither matched.
    ///
    /// This returns a vector of the matching XformOpDefinitions in this stack. The
    /// size of this vector will be 0 if no complete match is found, or xformops.size()
    /// if a complete match is found.
    PXRUSDMAYA_API
    OpClassList
    MatchingSubstack(
            const std::vector<UsdGeomXformOp>& xformops) const;

    /// \brief The standard Maya xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    rotatePivotTranslate
    ///    rotatePivot
    ///    rotate
    ///    rotateAxis
    ///    rotatePivot^-1 (inverted twin)
    ///    scalePivotTranslate
    ///    scalePivot
    ///    shear
    ///    scale
    ///    scalePivot^-1 (inverted twin)
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& MayaStack();


    /// \brief The Common API xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    pivot
    ///    rotate
    ///    scale
    ///    pivot^-1 (inverted twin)
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& CommonStack();

    /// \brief xform "stack" consisting of only a single matrix xform
    ///
    /// This stack will match any list of xform ops that consist of a single
    /// matrix "transform" op, regardless of name.
    /// Consists of these xform operators:
    ///    transform
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& MatrixStack();

    /// \brief Runs MatchingSubstack against the given list of stacks
    ///
    /// Returns the first non-empty result it finds; if all stacks
    /// return an empty vector, an empty vector is returned.
    PXRUSDMAYA_API
    static OpClassList
    FirstMatchingSubstack(
            const std::vector<PxrUsdMayaXformStack const *>& stacks,
            const std::vector<UsdGeomXformOp>& xformops);

private:
    // Because this is an immutable type, we keep a pointer to shared
    // data; this allows us to only have overhead associated with
    // a RefPtr, while having easy-python-wrapping (without overhead
    // of WeakPtr)
    typedef TfRefPtr<_PxrUsdMayaXformStackData> DataRefPtr;

    DataRefPtr _sharedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_XFORM_STACK_H
