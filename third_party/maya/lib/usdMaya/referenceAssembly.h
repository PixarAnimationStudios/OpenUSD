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
#ifndef PXRUSDMAYA_REFERENCE_ASSEMBLY_H
#define PXRUSDMAYA_REFERENCE_ASSEMBLY_H

/// \file usdMaya/referenceAssembly.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/usdPrimProvider.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MPxAssembly.h>
#include <maya/MPxRepresentation.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_REFERENCE_ASSEMBLY_TOKENS \
    ((MayaTypeName, "pxrUsdReferenceAssembly"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaReferenceAssemblyTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_REFERENCE_ASSEMBLY_TOKENS);


#define PXRUSDMAYA_VARIANT_SET_TOKENS \
    ((PlugNamePrefix, "usdVariantSet_"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaVariantSetTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_VARIANT_SET_TOKENS);


/// Returns the PIXMAYA_USE_USD_ASSEM_NAMESPACE env setting.
PXRUSDMAYA_API
bool UsdMayaUseUsdAssemblyNamespace();


class UsdMayaReferenceAssembly : public MPxAssembly,
                                 public PxrUsdMayaUsdPrimProvider
{
public:
    PXRUSDMAYA_API
    static const MTypeId typeId;
    PXRUSDMAYA_API
    static const MString typeName;
    PXRUSDMAYA_API
    static const MString _classification;

    // Attributes
    PXRUSDMAYA_API
    static MObject filePathAttr;
    PXRUSDMAYA_API
    static MObject primPathAttr;
    PXRUSDMAYA_API
    static MObject excludePrimPathsAttr;
    PXRUSDMAYA_API
    static MObject timeAttr;
    PXRUSDMAYA_API
    static MObject complexityAttr;
    PXRUSDMAYA_API
    static MObject tintAttr;
    PXRUSDMAYA_API
    static MObject tintColorAttr;
    PXRUSDMAYA_API
    static MObject kindAttr;
    PXRUSDMAYA_API
    static MObject initialRepAttr;
    PXRUSDMAYA_API
    static MObject repNamespaceAttr;
    PXRUSDMAYA_API
    static MObject drawModeAttr;
    PXRUSDMAYA_API
    static MObject inStageDataAttr;
    PXRUSDMAYA_API
    static MObject inStageDataCachedAttr;
    PXRUSDMAYA_API
    static MObject outStageDataAttr;
    PXRUSDMAYA_API
    static std::vector<MObject> attrsAffectingRepresentation;

    // Static Member Functions ==
    PXRUSDMAYA_API
    static void* creator();
    PXRUSDMAYA_API
    static MStatus initialize();

    // == Base Class Virtuals ==
    PXRUSDMAYA_API
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

    PXRUSDMAYA_API
    bool setInternalValueInContext(
            const MPlug& plug,
            const MDataHandle& dataHandle,
            MDGContext& ctx) override;

    // Required overrides
    PXRUSDMAYA_API
    MString createRepresentation(
            const MString& input,
            const MString& type,
            const MString& representation,
            MDagModifier* undoRedo = nullptr,
            MStatus* ReturnStatus = nullptr) override;

    PXRUSDMAYA_API
    MString getActive() const override;
    PXRUSDMAYA_API
    MStringArray getRepresentations(
            MStatus* ReturnStatus = nullptr) const override;
    PXRUSDMAYA_API
    MString getRepType(const MString& representation) const override;
    PXRUSDMAYA_API
    MString getRepLabel(const MString& representation) const override;
    PXRUSDMAYA_API
    MStringArray repTypes() const override;
    PXRUSDMAYA_API
    MStatus deleteRepresentation(
            const MString& representation) override;
    PXRUSDMAYA_API
    MStatus deleteAllRepresentations() override;
    PXRUSDMAYA_API
    MString setRepName(
            const MString& representation,
            const MString& newName,
            MStatus* ReturnStatus = nullptr) override;
    PXRUSDMAYA_API
    MStatus setRepLabel(
            const MString& representation,
            const MString& label) override;
    PXRUSDMAYA_API
    bool activateRep(const MString& representation) override;

    // Optional overrides
    PXRUSDMAYA_API
    bool supportsEdits() const override { return true; };
    PXRUSDMAYA_API
    bool supportsMemberChanges() const override { return false; };
    PXRUSDMAYA_API
    bool canRepApplyEdits(const MString& rep) const override {
        return (rep.length() > 0);
    };

    PXRUSDMAYA_API
    void postLoad() override;
    PXRUSDMAYA_API
    bool inactivateRep() override;
    PXRUSDMAYA_API
    MString getRepNamespace() const override;
    PXRUSDMAYA_API
    void updateRepNamespace(const MString& repNamespace) override;

    PXRUSDMAYA_API
    MStatus setDependentsDirty(
            const MPlug& plug,
            MPlugArray& plugArray) override;

    // PxrUsdMayaUsdPrimProvider overrides:
    PXRUSDMAYA_API
    UsdPrim usdPrim() const override;

    // Additional public functions
    bool HasEdits() const { return _hasEdits; }
    void SetHasEdits(bool val) { _hasEdits = val; }

    /// This method returns a map of variantSet names to variant selections based
    /// on the variant selections specified on the Maya assembly node. The list
    /// of valid variantSets is retrieved from the referenced prim, so only
    /// Maya attributes with a selection that correspond to a valid variantSet
    /// are included in the returned map.
    PXRUSDMAYA_API
    std::map<std::string, std::string> GetVariantSetSelections() const;

    /// Connect Maya's global time to the assembly's time attribute
    ///
    /// This function is called when the assembly's Playback representation is
    /// activated to enable scrubbing through animation using the timeline,
    /// since we also create a connection from the assembly to its proxies.
    PXRUSDMAYA_API
    void ConnectMayaTimeToAssemblyTime();

    /// Disconnect the assembly's time attribute from Maya's global time
    ///
    /// This function is called when the assembly's Playback representation is
    /// deactivated so that we do not incur the performance overhead of
    /// propagating Maya's global time to the assembly and its proxies.
    /// This also disables scrubbing through animation.
    PXRUSDMAYA_API
    void DisconnectAssemblyTimeFromMayaTime();

  private:

    friend class UsdMayaRepresentationBase;

    UsdMayaReferenceAssembly();
    ~UsdMayaReferenceAssembly() override;

    // Private Class functions
    MStatus computeInStageDataCached(MDataBlock& dataBlock);
    MStatus computeOutStageData(MDataBlock& dataBlock);

    // After discussion with Autodesk, we've decided to adopt the namespace
    // handling functionality from their sample assembly reference
    // implementation here:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8h_example_html
    //
    // This should really be implemented internally as built-in functionality
    // of Maya assemblies rather than having to deal with it in plugin code,
    // but there are currently no plans to make that happen, so we're forced to
    // do it ourselves. This helps ensure that assembly edits do not fall off
    // when assembly nodes are renamed/duplicated/etc.

    /// UsdMayaReferenceAssembly objects use a slightly different scheme for
    /// the representation namespace than the default behavior of
    /// MPxAssembly::getRepNamespace(), but they use that as a starting point.
    /// This function returns the "default" namespace for this assembly. This
    /// may be different from the assembly's actual namespace if the
    /// repNamespace attribute has been set to a different value.
    MString getDefaultRepNamespace() const;

    // This variable is used to tell if we're in the process of updating the
    // repNamespace. It helps distinguish between cases when the namespace
    // change was initiated by Maya or via the namespace editor (in which case
    // _updatingRepNamespace == true) versus when the repNamespace attribute
    // was edited directly (in which case _updatingRepNamespace == false).
    bool _updatingRepNamespace;

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
    PXRUSDMAYA_API
    UsdMayaRepresentationBase(MPxAssembly *assembly, const MString &name);
    PXRUSDMAYA_API
    ~UsdMayaRepresentationBase() override {};

    bool activate() override = 0;
    PXRUSDMAYA_API
    bool inactivate() override;

    // == Required Virtual Overrides
    MString getType() const override = 0;

    // == Optional Virtual Overrides
    //virtual bool    canApplyEdits() const;

    // == New functions for UsdMayaRepresentationBase ==
    // Expose protected function getAssembly() as public
    MPxAssembly* GetAssembly()  { return getAssembly(); };
};

class UsdMayaRepresentationProxyBase : public UsdMayaRepresentationBase
{
  public:
    UsdMayaRepresentationProxyBase(
            MPxAssembly *assembly,
            const MString &name,
            bool proxyIsSoftSelectable) :
        UsdMayaRepresentationBase(assembly, name),
        _proxyIsSoftSelectable(proxyIsSoftSelectable) {};

    PXRUSDMAYA_API
    bool activate() override;
    PXRUSDMAYA_API
    bool inactivate() override;

  protected:
    PXRUSDMAYA_API
    virtual void _OverrideProxyPlugs(
            MFnDependencyNode &shapeFn,
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
    PXRUSDMAYA_API
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    PXRUSDMAYA_API
    UsdMayaRepresentationCollapsed(MPxAssembly *assembly, const MString &name) :

        // We only support soft selection on "collapsed" proxies.  While we may
        // want to move proxies that are not root of the model, we suspect this
        // is more likely to lead to undesired behavior.
        UsdMayaRepresentationProxyBase(assembly, name, true) {};

    MString getType() const override {
        return UsdMayaRepresentationCollapsed::_assemblyType;
    };

  protected:
    PXRUSDMAYA_API
    void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                             MDGModifier &dgMod) override;
};

// ===========================================================
//
// Render a USD model as a single set of collapsed cards.
//   Draw the subgraph using a single UsdMayaProxyShape.
//
class UsdMayaRepresentationCards : public UsdMayaRepresentationProxyBase
{
  public:
    // == Statics
    PXRUSDMAYA_API
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    PXRUSDMAYA_API
    UsdMayaRepresentationCards(MPxAssembly *assembly, const MString &name) :

      // We only support soft selection on "collapsed" proxies.  While we may
      // want to move proxies that are not root of the model, we suspect this
      // is more likely to lead to undesired behavior.
      UsdMayaRepresentationProxyBase(assembly, name, true) {};

    MString getType() const override {
        return UsdMayaRepresentationCards::_assemblyType;
    };

    PXRUSDMAYA_API
    bool activate() override;
    PXRUSDMAYA_API
    bool inactivate() override;

  protected:
    PXRUSDMAYA_API
    void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                             MDGModifier &dgMod) override;
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
    PXRUSDMAYA_API
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationPlayback(MPxAssembly *assembly, const MString &name) :
        UsdMayaRepresentationProxyBase(assembly, name, false) {};

    MString getType() const override {
        return UsdMayaRepresentationPlayback::_assemblyType;
    };

    PXRUSDMAYA_API
    bool activate() override;
    PXRUSDMAYA_API
    bool inactivate() override;

  protected:
    PXRUSDMAYA_API
    void _OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                             MDGModifier &dgMod) override;
};

// Base class for representations that unroll a hierarchy.

class UsdMayaRepresentationHierBase : public UsdMayaRepresentationBase
{
  public:
    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationHierBase(MPxAssembly *assembly, const MString &name) :
        UsdMayaRepresentationBase(assembly, name) {};

    PXRUSDMAYA_API
    bool activate() override;

  protected:
    PXRUSDMAYA_API
    void _ConnectSubAssemblyPlugs();
    PXRUSDMAYA_API
    void _ConnectProxyPlugs();

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
    PXRUSDMAYA_API
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationExpanded(MPxAssembly *assembly, const MString &name) :
        UsdMayaRepresentationHierBase(assembly, name) {};

    MString getType() const override {
        return UsdMayaRepresentationExpanded::_assemblyType;
    };

  protected:
    bool _ShouldImportWithProxies() const override { return true; };
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
    PXRUSDMAYA_API
    static const MString _assemblyType;

    // == Overrides for MPxRepresentation ==
    UsdMayaRepresentationFull(MPxAssembly *assembly, const MString &name) :
        UsdMayaRepresentationHierBase(assembly, name) {};

    MString getType() const override {
        return UsdMayaRepresentationFull::_assemblyType;
    };
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
