//
// Copyright 2016 Pixar
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
#ifndef USDGEOM_GENERATED_XFORMABLE_H
#define USDGEOM_GENERATED_XFORMABLE_H

/// \file usdGeom/xformable.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdGeom/xformOp.h" 
#include <vector> 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// XFORMABLE                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdGeomXformable
///
/// Base class for all transformable prims, which allows arbitrary
/// sequences of component affine transformations to be encoded.
/// 
/// \note 
/// You may find it useful to review \ref UsdGeom_LinAlgBasics while reading
/// this class description.
/// 
/// <b>Supported Component Transformation Operations</b>
/// 
/// UsdGeomXformable currently supports arbitrary sequences of the following
/// operations, each of which can be encoded in an attribute of the proper
/// shape in any supported precision:
/// \li translate - 3D
/// \li scale     - 3D
/// \li rotateX   - 1D angle in degrees
/// \li rotateY   - 1D angle in degrees
/// \li rotateZ   - 1D angle in degrees
/// \li rotateABC - 3D where ABC can be any combination of the six principle
/// Euler Angle sets: XYZ, XZY, YXZ, YZX, ZXY, ZYX.  See
/// \ref usdGeom_rotationPackingOrder "note on rotation packing order"
/// \li orient    - 4D (quaternion)
/// \li transform - 4x4D 
/// 
/// <b>Creating a Component Transformation</b>
/// 
/// To add components to a UsdGeomXformable prim, simply call AddXformOp()
/// with the desired op type, as enumerated in \ref UsdGeomXformOp::Type,
/// and the desired precision, which is one of \ref UsdGeomXformOp::Precision.
/// Optionally, you can also provide an "op suffix" for the operator that 
/// disambiguates it from other components of the same type on the same prim.  
/// Application-specific transform schemas can use the suffixes to fill a role 
/// similar to that played by AbcGeom::XformOp's "Hint" enums for their own 
/// round-tripping logic.
/// 
/// We also provide specific "Add" API for each type, for clarity and 
/// conciseness, e.g. AddTranslateOp(), AddRotateXYZOp() etc.
/// 
/// AddXformOp() will return a UsdGeomXformOp object, which is a schema on a 
/// newly created UsdAttribute that provides convenience API for authoring
/// and computing the component transformations.  The UsdGeomXformOp can then
/// be used to author any number of timesamples and default for the op.
/// 
/// Each successive call to AddXformOp() adds an operator that will be applied
/// "more locally" than the preceding operator, just as if we were pushing
/// transforms onto a transformation stack - which is precisely what should
/// happen when the operators are consumed by a reader.
/// 
/// \note
/// If you can, please try to use the UsdGeomXformCommonAPI, which wraps
/// the UsdGeomXformable with an interface in which Op creation is taken
/// care of for you, and there is a much higher chance that the data you
/// author will be importable without flattening into other DCC's, as it
/// conforms to a fixed set of Scale-Rotate-Translate Ops.
/// 
/// \sa \ref usdGeom_xformableExamples "Using the Authoring API"
/// 
/// <b>Data Encoding and Op Ordering</b>
/// 
/// Because there is no "fixed schema" of operations, all of the attributes
/// that encode transform operations are dynamic, and are scoped in 
/// the namespace "xformOp". The second component of an attribute's name provides
/// the \em type of operation, as listed above.  An "xformOp" attribute can 
/// have additional namespace components derived from the \em opSuffix argument 
/// to the AddXformOp() suite of methods, which provides a preferred way of 
/// naming the ops such that we can have multiple "translate" ops with unique
/// attribute names. For example, in the attribute named 
/// "xformOp:translate:maya:pivot", "translate" is the type of operation and
/// "maya:pivot" is the suffix.
/// 
/// The following ordered list of attribute declarations in usda
/// define a basic Scale-Rotate-Translate with XYZ Euler angles, wherein the
/// translation is double-precision, and the remainder of the ops are single,
/// in which we will:
/// 
/// <ol>
/// <li> Scale by 2.0 in each dimension
/// <li> Rotate about the X, Y, and Z axes by 30, 60, and 90 degrees, respectively
/// <li> Translate by 100 units in the Y direction
/// </ol>
/// 
/// \code
/// float3 xformOp:rotateXYZ = (30, 60, 90)
/// float3 xformOp:scale = (2, 2, 2)
/// double3 xformOp:translate = (0, 100, 0)
/// uniform token[] xformOpOrder = [ "xformOp:translate", "xformOp:rotateXYZ", "xformOp:scale" ]
/// \endcode
/// 
/// The attributes appear in the dictionary order in which USD, by default,
/// sorts them.  To ensure the ops are recovered and evaluated in the correct
/// order, the schema introduces the **xformOpOrder** attribute, which
/// contains the names of the op attributes, in the precise sequence in which
/// they should be pushed onto a transform stack. **Note** that the order is
/// opposite to what you might expect, given the matrix algebra described in
/// \ref UsdGeom_LinAlgBasics.  This also dictates order of op creation,
/// since each call to AddXformOp() adds a new op to the end of the
/// \b xformOpOrder array, as a new "most-local" operation.  See 
/// \ref usdGeom_xformableExamples "Example 2 below" for C++ code that could
/// have produced this USD.
/// 
/// If it were important for the prim's rotations to be independently 
/// overridable, we could equivalently (at some performance cost) encode
/// the transformation also like so:
/// \code
/// float xformOp:rotateX = 30
/// float xformOp:rotateY = 60
/// float xformOp:rotateZ = 90
/// float3 xformOp:scale = (2, 2, 2)
/// double3 xformOp:translate = (0, 100, 0)
/// uniform token[] xformOpOrder = [ "xformOp:translate", "xformOp:rotateZ", "xformOp:rotateY", "xformOp:rotateX", "xformOp:scale" ]
/// \endcode
/// 
/// Again, note that although we are encoding an XYZ rotation, the three
/// rotations appear in the **xformOpOrder** in the opposite order, with Z,
/// followed, by Y, followed by X.
/// 
/// Were we to add a Maya-style scalePivot to the above example, it might 
/// look like the following:
/// \code
/// float3 xformOp:rotateXYZ = (30, 60, 90)
/// float3 xformOp:scale = (2, 2, 2)
/// double3 xformOp:translate = (0, 100, 0)
/// double3 xformOp:translate:scalePivot
/// uniform token[] xformOpOrder = [ "xformOp:translate", "xformOp:rotateXYZ", "xformOp:translate:scalePivot", "xformOp:scale" ]
/// \endcode
/// 
/// <b>Paired "Inverted" Ops</b>
/// 
/// We have been claiming that the ordered list of ops serves as a set
/// of instructions to a transform stack, but you may have noticed in the last
/// example that there is a missing operation - the pivot for the scale op
/// needs to be applied in its inverse-form as a final (most local) op!  In the 
/// AbcGeom::Xform schema, we would have encoded an actual "final" translation
/// op whose value was authored by the exporter as the negation of the pivot's
/// value.  However, doing so would be brittle in USD, given that each op can
/// be independently overridden, and the constraint that one attribute must be
/// maintained as the negation of the other in order for successful
/// re-importation of the schema cannot be expressed in USD.
/// 
/// Our solution leverages the **xformOpOrder** member of the schema, which,
/// in addition to ordering the ops, may also contain one of two special
/// tokens that address the paired op and "stack resetting" behavior.
/// 
/// The "paired op" behavior is encoded as an "!invert!" prefix in 
/// \b xformOpOrder, as the result of an AddXformOp(isInverseOp=True) call.  
/// The \b xformOpOrder for the last example would look like:
/// \code
/// uniform token[] xformOpOrder = [ "xformOp:translate", "xformOp:rotateXYZ", "xformOp:translate:scalePivot", "xformOp:scale", "!invert!xformOp:translate:scalePivot" ]
/// \endcode
/// 
/// When asked for its value via UsdGeomXformOp::GetOpTransform(), an
/// "inverted" Op (i.e. the "inverted" half of a set of paired Ops) will fetch 
/// the value of its paired attribute and return its negation.  This works for 
/// all op types - an error will be issued if a "transform" type op is singular 
/// and cannot be inverted. When getting the authored value of an inverted op 
/// via UsdGeomXformOp::Get(), the raw, uninverted value of the associated
/// attribute is returned.
/// 
/// For the sake of robustness, <b>setting a value on an inverted op is disallowed.</b>
/// Attempting to set a value on an inverted op will result in a coding error 
/// and no value being set. 
/// 
/// <b>Resetting the Transform Stack</b>
/// 
/// The other special op/token that can appear in \em xformOpOrder is
/// \em "!resetXformStack!", which, appearing as the first element of 
/// \em xformOpOrder, indicates this prim should not inherit the transformation
/// of its namespace parent.  See SetResetXformStack()
/// 
/// <b>Expected Behavior for "Missing" Ops</b>
/// 
/// If an importer expects Scale-Rotate-Translate operations, but a prim
/// has only translate and rotate ops authored, the importer should assume
/// an identity scale.  This allows us to optimize the data a bit, if only
/// a few components of a very rich schema (like Maya's) are authored in the
/// app.
/// 
/// \anchor usdGeom_xformableExamples
/// <b>Using the C++ API</b>
/// 
/// #1. Creating a simple transform matrix encoding
/// \snippet examples.cpp CreateMatrixWithDefault
/// 
/// #2. Creating the simple SRT from the example above
/// \snippet examples.cpp CreateExampleSRT
/// 
/// #3. Creating a parameterized SRT with pivot using UsdGeomXformCommonAPI
/// \snippet examples.cpp CreateSRTWithDefaults
/// 
/// #4. Creating a rotate-only pivot transform with animated
/// rotation and translation
/// \snippet examples.cpp CreateAnimatedTransform
/// 
/// 
///
class UsdGeomXformable : public UsdGeomImageable
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = true;

    /// Construct a UsdGeomXformable on UsdPrim \p prim .
    /// Equivalent to UsdGeomXformable::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomXformable(const UsdPrim& prim=UsdPrim())
        : UsdGeomImageable(prim)
    {
    }

    /// Construct a UsdGeomXformable on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomXformable(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomXformable(const UsdSchemaBase& schemaObj)
        : UsdGeomImageable(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomXformable();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomXformable holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomXformable(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomXformable
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // XFORMOPORDER 
    // --------------------------------------------------------------------- //
    /// Encodes the sequence of transformation operations in the
    /// order in which they should be pushed onto a transform stack while
    /// visiting a UsdStage's prims in a graph traversal that will effect
    /// the desired positioning for this prim and its descendant prims.
    /// 
    /// You should rarely, if ever, need to manipulate this attribute directly.
    /// It is managed by the AddXformOp(), SetResetXformStack(), and
    /// SetXformOpOrder(), and consulted by GetOrderedXformOps() and
    /// GetLocalTransformation().
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetXformOpOrderAttr() const;

    /// See GetXformOpOrderAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateXformOpOrderAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// \class XformQuery
    /// 
    /// Helper class that caches the ordered vector of UsGeomXformOps that 
    /// contribute to the local transformation of an xformable prim
    /// 
    /// Internally, the class initializes UsdAttributeQuery objects for the 
    /// xformOp attributes in order to facilitate efficient querying of their
    /// values.
    /// 
    /// \note This object does not listen to change notification. If a 
    /// consumer is holding onto a UsdGeomXformable::XformQuery object, it is 
    /// their responsibility to dispose of it, in response to a resync
    /// change to the associated xformOp attributes. The class provides the
    /// convenience method IncludesXformOpAttr for this purpose.
    /// 
    class XformQuery {
        public:
            XformQuery(): 
                _resetsXformStack(false) 
            { }

            /// Constructs an XformQuery object for the given xformable prim.
            /// Caches the ordered xformOps and initializes an UsdAttributeQuery
            /// internally for all the associated attributes.
            USDGEOM_API
            XformQuery(const UsdGeomXformable &xformable);

            /// Utilizes the internally cached UsdAttributeQuery's to efficiently
            /// compute the transform value at the given \p time.
            USDGEOM_API
            bool GetLocalTransformation(GfMatrix4d *transform,
                                        const UsdTimeCode time) const;

            /// Returns whether the xformable resets its parent's transformation.
            bool GetResetXformStack() const { 
                return _resetsXformStack;
            }

            /// Returns whether the xform value might change over time.
            USDGEOM_API
            bool TransformMightBeTimeVarying() const;

            /// Sets the vector of times at which xformOp samples have been 
            /// authored in the cached set of xform ops.
            /// 
            /// \sa UsdXformable::GetTimeSamples
            USDGEOM_API
            bool GetTimeSamples(std::vector<double> *times) const;

            /// Sets the vector of times in the \p interval at which xformOp 
            /// samples have been authored in the cached set of xform ops.
            /// 
            /// \sa UsdXformable::GetTimeSamples
            USDGEOM_API
            bool GetTimeSamplesInInterval(const GfInterval &interval, 
                                          std::vector<double> *times) const;

            /// Returns whether the given attribute affects the local 
            /// transformation computed for this query.
            USDGEOM_API
            bool IsAttributeIncludedInLocalTransform(
                const TfToken &attrName) const;

        private:
            // Cached copy of the vector of ordered xform ops.
            std::vector<UsdGeomXformOp> _xformOps;

            // Cache whether the xformable has !resetsXformStack! in its
            // xformOpOrder.
            bool _resetsXformStack;
    };

    /// Add an affine transformation to the local stack represented by this 
    /// Xformable.  This will fail if there is already a transform operation
    /// of the same name in the ordered ops on this prim (i.e. as returned
    /// by GetOrderedXformOps()), or if an op of the same name exists at all
    /// on the prim with a different precision than that specified.
    ///
    /// The newly created operation will become the most-locally applied
    /// transformation on the prim, and will appear last in the list
    /// returned by GetOrderedXformOps(). It is OK to begin authoring values
    /// to the returned UsdGeomXformOp immediately, interspersed with
    /// subsequent calls to AddXformOp() - just note the order of application,
    /// which \em can be changed at any time (and in stronger layers) via
    /// SetXformOpOrder().
    ///
    /// \param opType is the type of transform operation, one of 
    ///        \ref UsdGeomXformOp::Type.  
    /// \param precision allows you to specify the precision with which you
    ///        desire to encode the data. This should be one of the values in 
    ///        the enum \ref UsdGeomXformOp::Precision .
    /// \param opSuffix allows you to specify the purpose/meaning of the op in 
    ///        the stack. When opSuffix is specified, the associated attribute's 
    ///        name is set to "xformOp:<opType>:<opSuffix>".
    /// \param isInverseOp is used to indicate an inverse transformation 
    ///        operation.
    ///
    /// \return a UsdGeomXformOp that can be used to author to the operation.
    ///         An error is issued and the returned object will be invalid 
    ///         (evaluate to false) if the op being added already exists in 
    ///         \ref GetXformOpOrderAttr() "xformOpOrder" or if the 
    ///         arguments supplied are invalid.
    ///
    /// \note If the attribute associated with the op already exists, but isn't 
    /// of the requested precision, a coding error is issued, but a valid 
    /// xformOp is returned with the existing attribute.
    ///
    USDGEOM_API
    UsdGeomXformOp AddXformOp(UsdGeomXformOp::Type const opType, 
                              UsdGeomXformOp::Precision const
                              precision=UsdGeomXformOp::PrecisionDouble, 
                              TfToken const &opSuffix = TfToken(), 
                              bool isInverseOp=false) const;
    
    /// Add a translate operation to the local stack represented by this 
    /// xformable.
    /// 
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddTranslateOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionDouble,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a scale operation to the local stack represented by this 
    /// xformable.
    /// 
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddScaleOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation about the X-axis to the local stack represented by 
    /// this xformable.
    /// 
    /// Set the angle value of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddRotateXOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation about the YX-axis to the local stack represented by 
    /// this xformable.
    /// 
    /// Set the angle value of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddRotateYOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation about the Z-axis to the local stack represented by 
    /// this xformable.
    /// 
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddRotateZOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with XYZ rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle value of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateXYZOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with XZY rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle values of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateXZYOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with YXZ rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle values of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateYXZOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with YZX rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle values of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateYZXOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with ZXY rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle values of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateZXYOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a rotation op with ZYX rotation order to the local stack 
    /// represented by this xformable.
    /// 
    /// Set the angle values of the resulting UsdGeomXformOp <b>in degrees</b>
    /// \sa AddXformOp(), \ref usdGeom_rotationPackingOrder "note on angle packing order"
    USDGEOM_API
    UsdGeomXformOp AddRotateZYXOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a orient op (arbitrary axis/angle rotation) to the local stack 
    /// represented by this xformable.
    /// 
    /// \sa AddXformOp()
    USDGEOM_API
    UsdGeomXformOp AddOrientOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionFloat,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Add a tranform op (4x4 matrix transformation) to the local stack 
    /// represented by this xformable.
    /// 
    /// \sa AddXformOp()
    /// 
    /// Note: This method takes a precision argument only to be consistent 
    /// with the other types of xformOps. The only valid precision here is 
    /// double since matrix values cannot be encoded in floating-pt precision
    /// in Sdf.
    USDGEOM_API
    UsdGeomXformOp AddTransformOp(
        UsdGeomXformOp::Precision const precision=UsdGeomXformOp::PrecisionDouble,
        TfToken const &opSuffix = TfToken(), bool isInverseOp=false) const;

    /// Specify whether this prim's transform should reset the transformation
    /// stack inherited from its parent prim.  
    /// 
    /// By default, parent transforms are inherited. SetResetXformStack() can be 
    /// called at any time during authoring, but will always add a 
    /// '!resetXformStack!' op as the \em first op in the ordered list, if one 
    /// does not exist already.  If one already exists, and \p resetXform is 
    /// false, it will remove all ops upto and including the last 
    /// "!resetXformStack!" op.
    USDGEOM_API
    bool SetResetXformStack(bool resetXform) const;

    /// Does this prim reset its parent's inherited transformation?
    /// 
    /// Returns true if "!resetXformStack!" appears \em anywhere in xformOpOrder.
    /// When this returns true, all ops upto the last "!resetXformStack!" in
    /// xformOpOrder are ignored when computing the local transformation.
    /// 
    USDGEOM_API
    bool GetResetXformStack() const;

    /// Reorder the already-existing transform ops on this prim.
    ///
    /// All elements in \p orderedXformOps must be valid and represent attributes
    /// on this prim.  Note that it is \em not required that all the existing
    /// operations be present in \p orderedXformOps, so this method can be used to
    /// completely change the transformation structure applied to the prim.
    ///
    /// If \p resetXformStack is set to true, then "!resetXformOp! will be
    /// set as the first op in xformOpOrder, to indicate that the prim does 
    /// not inherit its parent's transformation.
    /// 
    /// \note If you wish to re-specify a prim's transformation completely in
    /// a stronger layer, you should first call this method with an \em empty
    /// \p orderedXformOps vector.  From there you can call AddXformOp() just as if
    /// you were authoring to the prim from scratch.
    ///
    /// \return false if any of the elements of \p orderedXformOps are not extant
    /// on this prim, or if an error occurred while authoring the ordering 
    /// metadata.  Under either condition, no scene description is authored.
    /// 
    /// \sa GetOrderedXformOps()
    USDGEOM_API
    bool SetXformOpOrder(std::vector<UsdGeomXformOp> const &orderedXformOps, 
                         bool resetXformStack = false) const;
    
    /// Return the ordered list of transform operations to be applied to
    /// this prim, in least-to-most-local order.  This is determined by the
    /// intersection of authored op-attributes and the explicit ordering of
    /// those attributes encoded in the \c xformOpOrder attribute on this prim.
    /// Any entries in \c xformOpOrder that do not correspond to valid 
    /// attributes on the xformable prim are skipped and a warning is issued.
    ///
    /// A UsdGeomTransformable that has not had any ops added via AddXformOp()
    /// will return an empty vector.
    /// 
    /// The function also sets \p resetsXformStack to true if "!resetXformStack!"
    /// appears \em anywhere in xformOpOrder (i.e., if the prim resets its 
    /// parent's inherited transformation). 
    /// 
    /// \note A coding error is issued if resetsXformStack is NULL. 
    ///
    /// \sa GetResetXformStack()
    USDGEOM_API
    std::vector<UsdGeomXformOp> GetOrderedXformOps(bool *resetsXformStack) const;

    /// Clears the local transform stack.
    USDGEOM_API
    bool ClearXformOpOrder() const;

    /// Clears the existing local transform stack and creates a new xform op of 
    /// type 'transform'. 
    /// 
    /// This API is provided for convenience since this is the most common 
    /// xform authoring operation.
    /// 
    /// \sa ClearXformOpOrder()
    /// \sa AddTransformOp()
    USDGEOM_API
    UsdGeomXformOp MakeMatrixXform() const;

    /// Determine whether there is any possibility that this prim's \em local
    /// transformation may vary over time.
    ///
    /// The determination is based on a snapshot of the authored state of the
    /// op attributes on the prim, and may become invalid in the face of
    /// further authoring.
    USDGEOM_API
    bool TransformMightBeTimeVarying() const;

    /// \overload
    /// Determine whether there is any possibility that this prim's \em local
    /// transformation may vary over time, using a pre-fetched (cached) list of 
    /// ordered xform ops supplied by the client.
    /// 
    /// The determination is based on a snapshot of the authored state of the
    /// op attributes on the prim, and may become invalid in the face of
    /// further authoring.
    USDGEOM_API
    bool TransformMightBeTimeVarying(
        const std::vector<UsdGeomXformOp> &ops) const;

    /// Sets \p times to the union of all the timesamples at which xformOps that 
    /// are included in the xformOpOrder attribute are authored. 
    /// 
    /// This clears the \p times vector before accumulating sample times 
    /// from all the xformOps.
    /// 
    /// \sa UsdAttribute::GetTimeSamples
    USDGEOM_API
    bool GetTimeSamples(std::vector<double> *times) const;

    /// Sets \p times to the union of all the timesamples in the interval, 
    /// \p interval, at which xformOps that are included in the xformOpOrder
    /// attribute are authored. 
    /// 
    /// This clears the \p times vector before accumulating sample times 
    /// from all the xformOps.
    /// 
    /// \sa UsdAttribute::GetTimeSamples
    USDGEOM_API
    bool GetTimeSamplesInInterval(const GfInterval &interval,
                                  std::vector<double> *times) const;

    /// Returns the union of all the timesamples at which the attributes 
    /// belonging to the given \p orderedXformOps are authored.
    /// 
    /// This clears the \p times vector before accumulating sample times 
    /// from \p orderedXformOps.
    /// 
    /// \sa UsdGeomXformable::GetTimeSamples
    USDGEOM_API
    static bool GetTimeSamples(
        std::vector<UsdGeomXformOp> const &orderedXformOps,
        std::vector<double> *times);

    /// Returns the union of all the timesamples in the \p interval
    /// at which the attributes belonging to the given \p orderedXformOps 
    /// are authored.
    /// 
    /// This clears the \p times vector before accumulating sample times 
    /// from \p orderedXformOps.
    /// 
    /// \sa UsdGeomXformable::GetTimeSamplesInInterval
    USDGEOM_API
    static bool GetTimeSamplesInInterval(
        std::vector<UsdGeomXformOp> const &orderedXformOps,
        const GfInterval &interval,
        std::vector<double> *times);

    /// Computes the fully-combined, local-to-parent transformation for this prim.
    /// 
    /// If a client does not need to manipulate the individual ops themselves, 
    /// and requires only the combined transform on this prim, this method will 
    /// take care of all the data marshalling and linear algebra needed to 
    /// combine the ops into a 4x4 affine transformation matrix, in 
    /// double-precision, regardless of the precision of the op inputs.
    ///
    /// \param transform is the output parameter that will hold the local 
    ///        transform.
    /// \param resetsXformStack is the output parameter that informs client 
    ///        whether they need to reset the transform stack before pushing
    ///        \p transform.
    /// \param time is the UsdTimeCode at which to sample the ops.
    /// 
    /// \return true on success, false if there was an error reading data.
    ///
    /// \note A coding error is issued if \p transform or \p resetsXformStack 
    ///       is NULL. 
    ///
    USDGEOM_API
    bool GetLocalTransformation(
        GfMatrix4d *transform,
        bool *resetsXformStack,
        const UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload 
    /// Computes the fully-combined, local-to-parent transformation for this 
    /// prim as efficiently as possible, using a pre-fetched (cached) list of 
    /// ordered xform ops supplied by the client.
    /// 
    /// \param transform is the output parameter that will hold the local 
    ///        transform.
    /// \param resetsXformStack is the output parameter that informs client 
    ///        whether they need to reset the transform stack before pushing
    ///        \p transform.
    /// \param ops is the ordered set of xform ops for this prim, and will be 
    ///        queried without any validity checking. Passing this in can save
    ///        significant value-resolution costs, if the client is able to 
    ///        retain this data from a call to GetOrderedXformOps().
    /// \param time is the UsdTimeCode at which to sample the ops.
    /// 
    /// \return true on success, false if there was an error reading data.
    /// 
    /// \note A coding error is issued if \p transform or \p resetsXformStack 
    ///       is NULL. 
    ///
    USDGEOM_API
    bool GetLocalTransformation(GfMatrix4d *transform,
                                bool *resetsXformStack,
                                const std::vector<UsdGeomXformOp> &ops,
                                const UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload
    /// This is a static version of the preceding function that takes 
    /// a cached list of ordered xform ops.
    /// 
    /// \param transform is the output parameter that will hold the local 
    ///        transform.
    /// \param ops is the ordered set of xform ops that must be combined 
    ///        together to compute the local transformation.
    /// \param time is the UsdTimeCode at which to sample the ops.
    /// 
    /// \return true on success, false if there was an error reading data.
    ///
    USDGEOM_API
    static bool GetLocalTransformation(GfMatrix4d *transform,
        std::vector<UsdGeomXformOp> const &ops, 
        const UsdTimeCode time);

    /// Returns true if the attribute named \p attrName could affect the local
    /// transformation of an xformable prim.
    USDGEOM_API
    static bool IsTransformationAffectedByAttrNamed(const TfToken &attrName);

private:
    // XXX: Only exists for temporary backwards compatibility.
    UsdAttribute _GetTransformAttr() const;

    // Extracts the value of the xformOpOrder attribute. Returns false if 
    // the xformOpOrder attribute doesn't exist on the prim (eg. when the prim 
    // type is incompatible or if it's a pure over). 
    bool _GetXformOpOrderValue(VtTokenArray *xformOpOrder, 
                               bool *hasAuthoredValue=NULL) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
