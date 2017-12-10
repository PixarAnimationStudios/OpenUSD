//
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
#ifndef __GUSD_PRIMWRAPPER_H__
#define __GUSD_PRIMWRAPPER_H__

#include <GT/GT_Primitive.h>
#include <UT/UT_ConcurrentHashMap.h>

#include "gusd/api.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/imageable.h"

#include "GT_Utils.h"
#include "purpose.h"

class GU_PackedImpl;

PXR_NAMESPACE_OPEN_SCOPE

class GusdGT_AttrFilter;
class UsdGeomXformCache;
class GusdContext;

/// \class GusdPrimWrapper
/// \brief A GT_Primitive that wraps a USD Prim. 
///
/// A GusdPrimWrapper is responsible for copying attribute data between 
/// USD and GT.
///
/// To write USD geometry, the following steps are taken:
///
/// The ROP uses GusdRefiner to refine the cooked geometry to GT primitive types
/// that have a matching USD type. 
///
/// For each GT primitive we create a primWrapper by calling the defineForWrite
/// method. This will create a usd prim on the current stage. 
///
/// On each frame updateFromGTPrim is called to copy attribtutes from the 
/// GT prim to the USD prim.
///
/// We support:
///      Writing a sequence of frames from one process. 
///      Writing each frame of a sequence to a seperate file from a seperate process. 
///      Writing each frame of a sequence to a seperate file from one process.
///
/// When writing all frames to a single file, we try and compress attribtute values. 
/// The data we need to do this compression is kept in the prim wrapper.
///
/// In the rare case where we want to sequentially write a sequence to per frame 
/// files, we need the primWrapper to persist across the sequence so we can do 
/// the attribute compression. However, we need to create the USD prim on each
/// per frame file. The "redefine" method is used for this.
///
/// To read USD geometry we start with a GusdGU_PackedUSD prim. A GusdGT_PrimCollect
/// object has been registered to convert these prims to GT_Primitives for drawing
/// in the view port. This object will call the "fullGT" method of the GU prim
/// which in turn calls the "defineForRead" to create a GusdPrimWrapper. These 
/// prims can be refined into native GT_Primitives that the viewport can draw.


typedef std::map<SdfPath,UT_Matrix4D> GusdSimpleXformCache;

class GusdPrimWrapper : public GT_Primitive
{
public:

    typedef boost::function<GT_PrimitiveHandle
            (const GT_PrimitiveHandle&, /* sourcePrim */
             const UsdStagePtr&,
             const SdfPath&        /* path */,
             const GusdContext&)>
        DefinitionForWriteFunction;

    typedef boost::function<GT_PrimitiveHandle
             (const UsdGeomImageable&,
              UsdTimeCode,
              GusdPurposeSet)>
        DefinitionForReadFunction;

    typedef boost::function<bool
            (const GT_PrimitiveHandle&,
             std::string &primName)>
        GetPrimNameFunction;

    typedef boost::function<GT_DataArrayHandle
            ( const GT_DataArrayHandle & )>
        ResampleArrayFunction;

    /// \brief Given a GT_Primitive, create a USD prim of the proper type.
    /// 
    /// When writing a USD file, we refine the geometry to a set of prims that we
    /// can deal with then we call this method on each of those prims.
    GUSD_API
    static GT_PrimitiveHandle
    defineForWrite( const GT_PrimitiveHandle& sourcePrim,
                    const UsdStagePtr& stage,
                    const SdfPath& path,
                    const GusdContext& ctxt);

    /// If prim type can generate a useful name for a prim, sets primName 
    /// and returns true.
    /// So far only F3D volumes do this. They can derive a name from meta
    /// data stored in the f3d file.
    GUSD_API
    static bool 
    getPrimName( const GT_PrimitiveHandle &sourcePrim,
                 std::string &primName );

    // When we write USD for the given type, we will use a name like $USDNAME_0.
    // where USDNAME is the name registered for this type
    GUSD_API
    static const char*
    getUsdName( int gtPrimId );

    // When we USD for an object that is marked as a group type, we write 
    // the object and then all its children.
    GUSD_API
    static bool
    isGroupType( int gtPrimId );

    /// \brief Given a USD prim, create a GusdPrimWrapper of the proper type.
    ///
    /// When reading a USD file, we call this function to create a Gusd_GTPrimitive
    /// for each USD prim, we then refine that to something that can be 
    /// used in a detail.
    GUSD_API
    static GT_PrimitiveHandle
    defineForRead( const UsdGeomImageable&  sourcePrim, 
                   UsdTimeCode              time,
                   GusdPurposeSet           purposes );

    /// \brief Is this gt prim a point instancer?
    ///
    /// This is used to know if we need to write the instance prototypes.
    GUSD_API
    static bool 
    isPointInstancerPrim(const GT_PrimitiveHandle& prim,
                         const GusdContext& ctxt);

    /// Register function for creating new USD prims from GT_Primitives and, optionally,
    /// a function for giving these prims a name.
    GUSD_API
    static bool registerPrimDefinitionFuncForWrite(int gtPrimId,
                                                   DefinitionForWriteFunction function,
                                                   GetPrimNameFunction getNameFunction = NULL,
                                                   bool isGroupType = false,
                                                   const char* usdName = NULL );

    /// Register function for creating new GusdPrimWrappers from USD prim.
    GUSD_API
    static bool registerPrimDefinitionFuncForRead(const TfToken& usdTypeName,
                                                  DefinitionForReadFunction function);

    /// Return true is the give prim can be supported directly in USD. This
    /// is used by the refiner to know when to stop refining.
    GUSD_API
    static bool isGTPrimSupported(const GT_PrimitiveHandle& prim);

    GUSD_API
    GusdPrimWrapper();
    GUSD_API
    GusdPrimWrapper( const UsdTimeCode &time, const GusdPurposeSet &purposes );
    GUSD_API
    GusdPrimWrapper( const GusdPrimWrapper &in );
    GUSD_API
    virtual ~GusdPrimWrapper();

    /// Return true if the underlying USD prim is valid
    GUSD_API
    virtual bool isValid() const;

    virtual const UsdGeomImageable getUsdPrim() const = 0;
    GUSD_API
    virtual bool unpack(
        GU_Detail&          gdr,
        const UT_StringRef& fileName,
        const SdfPath&      primPath,
        const UT_Matrix4D&  xform,
        fpreal              frame,
        const char *        viewportLod,
        GusdPurposeSet      purposes );

    /// \brief Create a new USD prim to match GT primitive.
    ///
    /// When writing per frame USD files, we need to recreate the stage
    /// and all the primitives on it each frame. However, there is some
    /// data we want to persist across frames. So we keep the GusdPrimWrappers
    /// and ask them to redefine their USD prims on each frame.
    GUSD_API
    virtual bool redefine( 
           const UsdStagePtr& stage,
           const SdfPath& path, 
           const GusdContext& ctxt,
           const GT_PrimitiveHandle& sourcePrim );
    
    /// Fill a USD prim's attribute samples for a frame from the
    /// attributes in a GT primitive.
    ///
    /// If \p sourcePrim is an instance, \p localXform is the instance transform
    /// otherwise it is the primitive transform from the prim.
    GUSD_API
    virtual bool updateFromGTPrim(
        const GT_PrimitiveHandle& sourcePrim,
        const UT_Matrix4D&        houXform,
        const GusdContext&        ctxt,
        GusdSimpleXformCache&     xformCache );

    /// Add a sample just before the current time that invisies this prim.
    // For points and instances this means writing a empty point attribute.
    // Other prims set their visibility flag.
    // It might be possible to avoid this if we are on the first frame.
    virtual void addLeadingBookend( double curFrame, double startFrame );

    /// Add a sample at the current frame, invising this from.
    virtual void addTrailingBookend( double curFrame );

    /// Keep track of the visibility state of the prim for book marks.
    void markVisible( bool in ) { m_visible = in; }
    bool isVisible() const { return  m_visible; }

    GUSD_API
    virtual void setVisibility(const TfToken& visibility, UsdTimeCode time);
    
    GUSD_API
    static GT_DataArrayHandle convertPrimvarData( 
                    const UsdGeomPrimvar& primvar, 
                    UsdTimeCode time );

    // Load primvars for prim from USD.
    // remapIndicies is used to expand curve primvars into point attributes if
    // needed.
    GUSD_API
    void loadPrimvars( 
                    UsdTimeCode               time,
                    const GT_RefineParms*     rparms,
                    int                       minUniform,
                    int                       minPoint,
                    int                       minVertex,
                    const std::string&        primPath,
                    GT_AttributeListHandle*   vertex,
                    GT_AttributeListHandle*   point,
                    GT_AttributeListHandle*   primitive,
                    GT_AttributeListHandle*   constant,
                    const GT_DataArrayHandle& remapIndicies = GT_DataArrayHandle() ) const;

    // Map to translate from GT_Owner enums to USD interpolation type tokens
    static std::map<GT_Owner, TfToken> s_ownerToUsdInterp;
    static std::map<GT_Owner, TfToken> s_ownerToUsdInterpCurve;

protected:

    /// Look for "visible" attribute on sourcePrim. If it doesn't exist
    /// set a visibility sample based on isVisible()
    void updateVisibilityFromGTPrim( const GT_PrimitiveHandle& sourcePrim,
                                     UsdTimeCode time,
                                     bool forceWrite = true );

    /// Look for a "usdactive" attribute on sourcePrim. UsdPrim::SetActive
    /// based on this value. If attribute doesn't exist, do nothing.
    void updateActiveFromGTPrim( const GT_PrimitiveHandle& sourcePrim,
                                 UsdTimeCode time );

    void updateTransformFromGTPrim( const GfMatrix4d &xform, UsdTimeCode time, bool force );

    bool updateAttributeFromGTPrim( GT_Owner owner, 
                                    const std::string& name,
                                    const GT_DataArrayHandle& houAttr, 
                                    UsdAttribute& usdAttr, 
                                    UsdTimeCode time );

    bool updatePrimvarFromGTPrim( 
                const TfToken&              name,
                const GT_Owner&             owner,
                const TfToken&              interpolation,
                UsdTimeCode                 time,
                const GT_DataArrayHandle&   data );

    // Write primvar values from a GT attribute list to USD.
    bool updatePrimvarFromGTPrim( const GT_AttributeListHandle& gtAttrs,
                                  const GusdGT_AttrFilter&      primvarFilter,
                                  const TfToken&                interpolation,
                                  UsdTimeCode                   time );

    void clearCaches();

    // Compute a USD transform from a Houdini transform.
    //
    // \p houXform is the transform from world to the prim's space in Houdini.
    // This includes the object node transformation and the transform of any
    // containing packed prim.
    //
    // \p xformCache is a map of the transforms of any groups that have been 
    // written on the current frame.
    
    static GfMatrix4d computeTransform( 
                const UsdPrim&              prim,
                UsdTimeCode                 time,
                const UT_Matrix4D&          houXform,
                const GusdSimpleXformCache& xformCache );
    
protected:

    UsdTimeCode     m_time;
    GusdPurposeSet  m_purposes;

    bool            m_visible;

    //////////////
    // Support for collapsing transform values across frames

    GfMatrix4d m_xformCache;
    UsdTimeCode m_lastXformSet;
    UsdTimeCode m_lastXformCompared;

    //////////////
    // Support from collapsing attribute values across frames

    struct AttrLastValueEntry {

        AttrLastValueEntry( const UsdTimeCode &time, GT_DataArrayHandle data_ ) {
            data = data_;
            lastSet = time;
            lastCompared = time;
        }

        GT_DataArrayHandle  data;
        UsdTimeCode         lastSet;
        UsdTimeCode         lastCompared;
    };

    typedef std::pair<GT_Owner, std::string> AttrLastValueKeyType; 
    typedef UT_Map<AttrLastValueKeyType, AttrLastValueEntry> AttrLastValueDict;

    mutable AttrLastValueDict m_lastAttrValueDict;

    //////////////

private:

    struct GTTypeInfo {
        DefinitionForWriteFunction  writeFunc;
        GetPrimNameFunction         primNameFunc;
        bool                        isGroupType;
        const char *                templateName;

        GTTypeInfo() : 
                writeFunc( NULL ), 
                primNameFunc( NULL ), 
                isGroupType( false ), 
                templateName(NULL) {}
        GTTypeInfo( 
            DefinitionForWriteFunction writeFunc_, 
            GetPrimNameFunction        primNameFunc_,
            bool                       isGroupType_,
            const char *               templateName_  ) : 
                writeFunc( writeFunc_ ), 
                primNameFunc( primNameFunc_ ), 
                isGroupType( isGroupType_ ),
                templateName( templateName_ ) {}
    };

    struct TfTokenHashCmp {
        static bool equal(const TfToken& a, const TfToken& b)
        { return a == b; }

        static size_t hash(const TfToken& key)
        { return key.Hash(); }
    };

    typedef UT_Map<int64,   GTTypeInfo>                 GTTypeInfoMap;
    typedef UT_Set<int64>                               GTTypeSet;
    typedef UT_ConcurrentHashMap<
        TfToken, DefinitionForReadFunction, TfTokenHashCmp>
        USDTypeToDefineFuncMap;

    static GTTypeInfoMap                s_gtTypeInfoMap;
    static USDTypeToDefineFuncMap       s_usdTypeToFuncMap; 
    static GTTypeSet                    s_supportedNativeGTTypes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_PRIMWRAPPER_H__
