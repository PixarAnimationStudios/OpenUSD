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
#ifndef PXRUSDMAYA_REFERENCEASSEMBLY_H
#define PXRUSDMAYA_REFERENCEASSEMBLY_H

#include "usdMaya/api.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/usdPrimProvider.h"

#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MPxAssembly.h>
#include <maya/MPxRepresentation.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

extern TfEnvSetting<bool> PIXMAYA_USE_USD_ASSEM_NAMESPACE;


class UsdMayaReferenceAssembly : public MPxAssembly, 
    public PxrUsdMayaUsdPrimProvider
{
public:
    USDMAYA_API
    static const MString _classification;

    /// \brief Helper struct to hold MObjects for this class.
    ///
    /// These would normally be static members but since we have this class
    /// registered in multiple plugins, we have the actual data stored
    /// statically in the plugin.cpp.  
    ///
    /// A reference to this is setup by creator().
    /// 
    /// \sa PxrUsdMayaPluginStaticData
    struct PluginStaticData {

        // these get set in initialize()
        MObject filePath;
        MObject primPath;
        MObject excludePrimPaths;
        MObject time;
        MObject complexity;
        MObject tint;
        MObject tintColor;
        MObject kind;
        MObject initialRep;
        MObject inStageData;
        MObject inStageDataCached;
        MObject outStageData;
        std::vector<MObject> attrsAffectingRepresentation;

        // this will not change once constructed.
        const MTypeId typeId;
        const MString typeName;
        const MTypeId stageDataTypeId;

        // this node uses data from proxyShape to make connections between the
        // two.
        const UsdMayaProxyShape::PluginStaticData& proxyShape;

        PluginStaticData(
                const MTypeId& typeId,
                const MString& typeName,
                const MTypeId& stageDataTypeId,
                const UsdMayaProxyShape::PluginStaticData& proxyShape) :
            typeId(typeId),
            typeName(typeName),
            stageDataTypeId(stageDataTypeId),
            proxyShape(proxyShape)
        { }
    };

    // Static Member Functions ==
    USDMAYA_API
    static void*   creator(
            const PluginStaticData& psData);
    USDMAYA_API
    static MStatus initialize(
            PluginStaticData* psData);

    // == Base Class Virtuals ==
    USDMAYA_API
    virtual MStatus    compute( const MPlug& plug,
                                MDataBlock& dataBlock );

    USDMAYA_API
    virtual bool       setInternalValueInContext( const MPlug& plug,
                                         const MDataHandle& dataHandle,
                                         MDGContext& ctx);

    // Required overrides
    USDMAYA_API
    virtual MString createRepresentation(
            const MString& input,
            const MString& type,
            const MString& representation,
            MDagModifier*  undoRedo = NULL,
            MStatus*       ReturnStatus = NULL);
    
    USDMAYA_API
    virtual MString      getActive() const;
    USDMAYA_API
    virtual MStringArray getRepresentations(MStatus* ReturnStatus = NULL) const;
    USDMAYA_API
    virtual MString      getRepType(const MString& representation) const;
    USDMAYA_API
    virtual MString      getRepLabel(const MString& representation) const;
    USDMAYA_API
    virtual MStringArray repTypes() const;
    USDMAYA_API
    virtual MStatus      deleteRepresentation(const MString& representation);
    USDMAYA_API
    virtual MStatus      deleteAllRepresentations();
    USDMAYA_API
    virtual MString      setRepName(const MString& representation,
                                    const MString& newName,
                                    MStatus*       ReturnStatus = NULL);
    USDMAYA_API
    virtual MStatus      setRepLabel(const MString& representation,
                                     const MString& label);
    USDMAYA_API
    virtual bool         activateRep(const MString& representation);

    // Optional overrides
    virtual bool         supportsEdits() const { return true;};
    virtual bool         supportsMemberChanges() const { return false;};
    virtual bool         canRepApplyEdits(const MString& rep) const {return (rep.length() > 0);};

    USDMAYA_API
    virtual void         postLoad();
    USDMAYA_API
    virtual bool         inactivateRep();
    USDMAYA_API
    virtual MString      getRepNamespace() const;
    //virtual bool         activate(const MString& representation);
    //virtual bool         isActive(const MString& representation) const;
    //virtual void         updateRepNamespace(const MString& repNamespace);
    //virtual bool         handlesApplyEdits() const;
    //virtual MStatus      applyEdits(MObject& target, MItEdits& edits);
    //virtual bool         handlesAddEdits() const;
    //virtual MStatus      addEdits();
    //virtual void         beforeSave();
    //virtual void         memberAdded(MObject& member);
    //virtual void         memberRemoved(MObject& member);

    //virtual void         preApplyEdits();
    //virtual void         preUnapplyEdits();
    //virtual MString      getInitialRep(const MObject &assembly,
    //                                  bool& hasInitialRep,
    //                                  MStatus* ReturnStatus=NULL) const;

    USDMAYA_API
    virtual MStatus      setDependentsDirty( const MPlug& plug, MPlugArray& plugArray);

    // PxrUsdMayaUsdPrimProvider overrides:
    USDMAYA_API
    UsdPrim usdPrim() const override;

    // Additional public functions
    bool HasEdits() const { return _hasEdits; }
    void SetHasEdits(bool val) { _hasEdits = val; }

    // This method returns a map of variantSet names to variant selections based
    // on the variant selections specified on the Maya assembly node. The list
    // of valid variantSets is retrieved from the referenced prim, so only
    // Maya attributes with a selection that correspond to a valid variantSet
    // are included in the returned map.
    USDMAYA_API
    std::map<std::string, std::string> GetVariantSetSelections() const;


  private:

    friend class UsdMayaRepresentationBase;
    const PluginStaticData& _psData;

    UsdMayaReferenceAssembly(const PluginStaticData& psData);
    virtual ~UsdMayaReferenceAssembly();

    // Private Class functions
    MStatus computeInStageDataCached(MDataBlock& dataBlock);
    MStatus computeOutStageData(MDataBlock& dataBlock);

    // Class member variables
    std::map<std::string, boost::shared_ptr<MPxRepresentation> > _representations;
    bool _activateRepOnFileLoad; 
    boost::shared_ptr<MPxRepresentation> _activeRep;
    bool _inSetInternalValue;
    bool _hasEdits;

};



// ===========================================================
//
// Base Class for UsdMayaRepresentations
//
class UsdMayaRepresentationBase : public MPxRepresentation
{
  public:
    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationBase(MPxAssembly *assembly, const MString &name);
    virtual ~UsdMayaRepresentationBase() {};

    virtual bool activate() = 0;
    virtual bool inactivate();

    // == Required Virtual Overrides
    virtual MString getType() const = 0;

    // == Optional Virtual Overrides
    //virtual bool    canApplyEdits() const;

    // == New functions for UsdMayaRepresentationBase ==
    // Expose protected function getAssembly() as public
    MPxAssembly* GetAssembly()  { return getAssembly(); };

  protected:
    // == Protected Data
    const UsdMayaReferenceAssembly::PluginStaticData& _psData;
};

class UsdMayaRepresentationProxyBase : public UsdMayaRepresentationBase 
{
  public:
    UsdMayaRepresentationProxyBase(MPxAssembly *assembly, const MString &name, 
            bool proxyIsSoftSelectable) : 
        UsdMayaRepresentationBase(assembly, name),
        _proxyIsSoftSelectable(proxyIsSoftSelectable) {};

    virtual bool activate();
    virtual bool inactivate();

  protected:
    virtual void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                     MDGModifier &dgMod);

  private:
    void _PushEditsToProxy();

  private:
    SdfLayerRefPtr _sessionSublayer;
    bool _proxyIsSoftSelectable;
};

// ===========================================================
//
// Render a collapsed USD File
//   Draw the subgraph using a single UsdMayaProxyShape.  
//
class UsdMayaRepresentationCollapsed : public UsdMayaRepresentationProxyBase 
{
  public:
    // == Statics
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationCollapsed(MPxAssembly *assembly, const MString &name) : 

        // We only support soft selection on "collapsed" proxies.  While we may
        // want to move proxies that are not root of the model, we suspect this
        // is more likely to lead to undesired behavior.
        UsdMayaRepresentationProxyBase(assembly, name, true) {};

    virtual MString getType () const { return UsdMayaRepresentationCollapsed::_assemblyType; };

  protected:
    virtual void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                     MDGModifier &dgMod);
};

// ===========================================================
//
// Render a collapsed USD File
//   Draw the subgraph using a single UsdMayaProxyShape.  
//
class UsdMayaRepresentationPlayback : public UsdMayaRepresentationProxyBase
{
  public:
    // == Statics
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationPlayback(MPxAssembly *assembly, const MString &name) : 
        UsdMayaRepresentationProxyBase(assembly, name, false) {};

    virtual MString getType () const { return UsdMayaRepresentationPlayback::_assemblyType; };

  protected:
    virtual void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                     MDGModifier &dgMod);
};

// Base class for representations that unroll a hierarchy.

class UsdMayaRepresentationHierBase : public UsdMayaRepresentationBase 
{
  public:
    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationHierBase(MPxAssembly *assembly, const MString &name) : 
        UsdMayaRepresentationBase(assembly, name) {};

    virtual bool activate();

  protected:
    void _ConnectSubAssemblyPlugs();

    virtual bool _ShouldImportWithProxies() const { return false; };
};

// ===========================================================
//
// Expand a USD hierarchy into sub-assemblies
//   Imports xforms as maya groups and other prims as usdPrimShapes.
//   Children that are models, model groups, and sets will be imported as UsdAssemblies
//
class UsdMayaRepresentationExpanded : public UsdMayaRepresentationHierBase 
{
  public:
    // == Statics
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationExpanded(MPxAssembly *assembly, const MString &name) : 
        UsdMayaRepresentationHierBase(assembly, name) {};

    virtual MString getType () const { return UsdMayaRepresentationExpanded::_assemblyType; };

  protected:
    virtual bool _ShouldImportWithProxies() const { return true; };
};

// ===========================================================
//
// 
// Imports the USD subgraph (via usdImport command) as full maya geometry.
//
class UsdMayaRepresentationFull : public UsdMayaRepresentationHierBase 
{
  public:
    // == Statics
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationFull(MPxAssembly *assembly, const MString &name) : 
        UsdMayaRepresentationHierBase(assembly, name) {};

    virtual MString getType () const { return UsdMayaRepresentationFull::_assemblyType; };
};

#endif // PXRUSDMAYA_REFERENCEASSEMBLY_H
