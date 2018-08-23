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
#include "usdMaya/referenceAssembly.h"

#include "usdMaya/editUtil.h"
#include "usdMaya/jobArgs.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/query.h"
#include "usdMaya/readJob.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/stageData.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdUtils/stageCache.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>
#include <maya/MEdit.h>
#include <maya/MFileIO.h>
#include <maya/MFnAssembly.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItEdits.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <map>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaReferenceAssemblyTokens,
                        PXRUSDMAYA_REFERENCE_ASSEMBLY_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(UsdMayaVariantSetTokens,
                        PXRUSDMAYA_VARIANT_SET_TOKENS);

TF_DEFINE_ENV_SETTING(PIXMAYA_USE_USD_ASSEM_NAMESPACE,
                      true,
                      "Prefixes unrolled USD assemblies with namespaces");


bool
UsdMayaUseUsdAssemblyNamespace()
{
    return TfGetEnvSetting(PIXMAYA_USE_USD_ASSEM_NAMESPACE);
}

// == Statics ==
const MTypeId UsdMayaReferenceAssembly::typeId(0x0010A251);
const MString UsdMayaReferenceAssembly::typeName(
    UsdMayaReferenceAssemblyTokens->MayaTypeName.GetText());
const MString UsdMayaReferenceAssembly::_classification("drawdb/geometry/transform");

// Attributes
MObject UsdMayaReferenceAssembly::filePathAttr;
MObject UsdMayaReferenceAssembly::primPathAttr;
MObject UsdMayaReferenceAssembly::excludePrimPathsAttr;
MObject UsdMayaReferenceAssembly::timeAttr;
MObject UsdMayaReferenceAssembly::complexityAttr;
MObject UsdMayaReferenceAssembly::kindAttr;
MObject UsdMayaReferenceAssembly::initialRepAttr;
MObject UsdMayaReferenceAssembly::repNamespaceAttr;
MObject UsdMayaReferenceAssembly::drawModeAttr;
MObject UsdMayaReferenceAssembly::inStageDataAttr;
MObject UsdMayaReferenceAssembly::inStageDataCachedAttr;
MObject UsdMayaReferenceAssembly::outStageDataAttr;
std::vector<MObject> UsdMayaReferenceAssembly::attrsAffectingRepresentation;

/* static */
void*
UsdMayaReferenceAssembly::creator()
{
    return new UsdMayaReferenceAssembly();
}

/* static */
MStatus
UsdMayaReferenceAssembly::initialize()
{
    MStatus status;

    MFnCompoundAttribute compoundAttrFn;
    MFnNumericAttribute numericAttrFn;
    MFnTypedAttribute typedAttrFn;
    MFnUnitAttribute unitAttrFn;

    filePathAttr = typedAttrFn.create("filePath", "fp",MFnData::kString,MObject::kNullObj,&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setInternal(true); // trigger getInternalValueInContext() on change
    status = addAttribute(filePathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    primPathAttr = typedAttrFn.create("primPath", "pp",MFnData::kString,MObject::kNullObj,&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setInternal(true); // trigger getInternalValueInContext() on change
    status = addAttribute(primPathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    excludePrimPathsAttr =
        typedAttrFn.create("excludePrimPaths","epp",MFnData::kString,MObject::kNullObj,&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(excludePrimPathsAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    timeAttr = unitAttrFn.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(timeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    complexityAttr = numericAttrFn.create("complexity", "cplx", MFnNumericData::kInt,0,&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numericAttrFn.setMin(0);
    numericAttrFn.setSoftMax(4);
    numericAttrFn.setMax(8);
    numericAttrFn.setStorable(false); // not written to the file
    status = addAttribute(complexityAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnStringData defaultKind;
    kindAttr =
        typedAttrFn.create("kind","knd",MFnData::kString,MObject::kNullObj,&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setConnectable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    status = addAttribute(kindAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);


    initialRepAttr = typedAttrFn.create("initialRep", "irp", MFnData::kString,MObject::kNullObj,&status);
    addAttribute(initialRepAttr);

    inStageDataAttr = typedAttrFn.create(
        "inStageData",
        "id",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setDisconnectBehavior(MFnNumericAttribute::kReset); // on disconnect, reset to Null
    status = addAttribute(inStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Having to store the representation namespace in an attribute on the
    // assembly is not ideal, but it is necessary to ensure that namespace
    // changes are handled correctly and that assembly edits do not fall off
    // because of renaming/duplicating/etc. MPxAssembly does not do this for us.
    // This pattern is adapted from Autodesk's sample assembly reference node:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8cpp_example_html
    repNamespaceAttr = typedAttrFn.create(
        "repNamespace",
        "rns",
        MFnData::kString,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setInternal(true);
    status = addAttribute(repNamespaceAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    drawModeAttr = typedAttrFn.create(
        "drawMode",
        "dm",
        MFnData::kString,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    status = addAttribute(drawModeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // inStageData or filepath-> inStageDataCached -> outStageData
    inStageDataCachedAttr = typedAttrFn.create(
        "inStageDataCached",
        "idc",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    status = addAttribute(inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outStageDataAttr = typedAttrFn.create(
        "outStageData",
        "od",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    status = addAttribute(outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    //
    // add attribute dependencies
    //
    status = attributeAffects(inStageDataAttr, inStageDataCachedAttr);
    status = attributeAffects(inStageDataAttr, outStageDataAttr);

    status = attributeAffects(filePathAttr, inStageDataCachedAttr);
    status = attributeAffects(filePathAttr, outStageDataAttr);

    status = attributeAffects(inStageDataCachedAttr, outStageDataAttr);

    status = attributeAffects(primPathAttr, outStageDataAttr);

    status = attributeAffects(drawModeAttr, inStageDataCachedAttr);
    status = attributeAffects(drawModeAttr, outStageDataAttr);


    // Also see setDependentsDirty() for variantSets dynamically added

    attrsAffectingRepresentation.push_back(filePathAttr);
    attrsAffectingRepresentation.push_back(primPathAttr);

    return status;
}

// == Private Member Functions ==

// CONSTRUCTORS/DESTRUCTORS
UsdMayaReferenceAssembly::UsdMayaReferenceAssembly() :
    _updatingRepNamespace(false),
    _activateRepOnFileLoad(false),
    _inSetInternalValue(false),
    _hasEdits(false)
{
    //
    // REMINDER: Also update usdMaya.mel:usdMaya_UsdMayaReferenceAssembly_listRepTypes()
    //           if adding a new Representation
    //
    _representations[std::string(UsdMayaRepresentationCollapsed::_assemblyType.asChar())] =
        std::make_shared<UsdMayaRepresentationCollapsed>(this, UsdMayaRepresentationCollapsed::_assemblyType);

    _representations[std::string(UsdMayaRepresentationCards::_assemblyType.asChar())] =
        std::make_shared<UsdMayaRepresentationCards>(this, UsdMayaRepresentationCards::_assemblyType);

    _representations[std::string(UsdMayaRepresentationPlayback::_assemblyType.asChar())] =
        std::make_shared<UsdMayaRepresentationPlayback>(this, UsdMayaRepresentationPlayback::_assemblyType);

    _representations[std::string(UsdMayaRepresentationExpanded::_assemblyType.asChar())] =
        std::make_shared<UsdMayaRepresentationExpanded>(this, UsdMayaRepresentationExpanded::_assemblyType);

    _representations[std::string(UsdMayaRepresentationFull::_assemblyType.asChar())] =
        std::make_shared<UsdMayaRepresentationFull>(this, UsdMayaRepresentationFull::_assemblyType);
}


UsdMayaReferenceAssembly::~UsdMayaReferenceAssembly()
{
}


// ==========================
// Virtual Function Overrides
// ==========================

//
// == Required Function Overrides
//

MString UsdMayaReferenceAssembly::createRepresentation(
    const MString&  /*input*/,
    const MString&  /*type*/,
    const MString&  /*representation*/,
    MDagModifier*   /*undoRedo*/,
    MStatus*        /*status*/)
{
    // New representations cannot be created.
    return MString("");
}

MString UsdMayaReferenceAssembly::getActive() const
{
    return (_activeRep) ? _activeRep->getName() : MString("");
}

MStringArray UsdMayaReferenceAssembly::getRepresentations(MStatus*  /*status*/) const
{
    MStringArray repTypes;
    std::map<std::string, std::shared_ptr<MPxRepresentation> >::const_iterator it;
    for (it=_representations.begin(); it != _representations.end(); ++it) {
        repTypes.append(MString(it->first.c_str()));
    }
    return repTypes;
}

MString UsdMayaReferenceAssembly::getRepType(const MString& rep) const
{
    std::string tmpRep(rep.asChar());
    std::map<std::string, std::shared_ptr<MPxRepresentation> >::const_iterator repIt;
    repIt = _representations.find(tmpRep);
    if (repIt != _representations.end()) {
        return repIt->second->getType();
    }
    else {
        return MString();
    }
}

MString UsdMayaReferenceAssembly::getRepLabel(const MString& rep) const
{
    return getRepType(rep);
}

MStringArray UsdMayaReferenceAssembly::repTypes() const
{
    MStringArray repTypes;
    std::map<std::string, std::shared_ptr<MPxRepresentation> >::const_iterator it;
    for (it=_representations.begin(); it != _representations.end(); ++it) {
        repTypes.append(MString(it->first.c_str()));
    }
    return repTypes;
}

MStatus UsdMayaReferenceAssembly::deleteRepresentation(const MString&  /*representation*/)
{
    return MS::kFailure;
}

MStatus UsdMayaReferenceAssembly::deleteAllRepresentations()
{
    return MS::kFailure;
}


MString UsdMayaReferenceAssembly::setRepName(const MString&  /*representation*/,
                                const MString&  /*newName*/,
                                MStatus*        /*ReturnStatus*/)
{
    return MString("");
}

MStatus UsdMayaReferenceAssembly::setRepLabel(const MString&  /*representation*/,
                                 const MString&  /*label*/)
{
    return MS::kFailure;
}

bool UsdMayaReferenceAssembly::activateRep(const MString& repMStr)
{
    if (repMStr.length() == 0) {
        return true;
    }
    std::string rep(std::string(repMStr.asChar()));

    std::map<std::string, std::shared_ptr<MPxRepresentation> >::const_iterator repIt;
    repIt = _representations.find(rep);
    if (repIt == _representations.end()) {
        return false;
    }

    _activeRep = repIt->second;
    _activeRep->activate();
    // Set the initialRep plug from the active rep
    MPlug initialRepPlg(thisMObject(), initialRepAttr);
    if (!initialRepPlg.isNull()) {
        initialRepPlg.setString(repMStr);
    }
    return true;
}

//
// == Optional Function Overrides
//

void UsdMayaReferenceAssembly::postLoad()
{
    MFnAssembly assemblyFn(thisMObject());

    // If this is not a top-level assembly, lock the repNamespace attribute.
    // Users should not be able to change this attribute on nested assemblies.
    // This was adapted from Autodesk's sample assembly reference node:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8cpp_example_html
    if (!assemblyFn.isTopLevel()) {
        MPlug repNamespacePlug(thisMObject(), repNamespaceAttr);
        repNamespacePlug.setLocked(true);
    }

    // Activate Representation
    if (_activateRepOnFileLoad) {
        //logging.debug("In postLoad activate: isTopLevel=%r canActivate=%r"%(assemblyFn.isTopLevel(), assemblyFn.canActivate()))
        if (assemblyFn.canActivate()) {  // Consider adding assemblyFn.isTopLevel() to the conditional
            MPlug initialRepPlg(thisMObject(), initialRepAttr);
            MString initialRep = initialRepPlg.asString();
            //logging.debug('Activating %s initialRep %r'%(assemblyFn.name(), initialRep))
            if (initialRep.length() > 0) {
                activate(initialRep);
            }
        }
    }
}

bool UsdMayaReferenceAssembly::inactivateRep()
{
    if (_activeRep) {
        _activeRep->inactivate();
        _activeRep.reset();
        // Clear the initialRep plug value
        MPlug initialRepPlg(thisMObject(), initialRepAttr);
        if ( !initialRepPlg.isNull() ) {
            initialRepPlg.setString("");
        }
    }

    return true;
}

MString
UsdMayaReferenceAssembly::getDefaultRepNamespace() const
{
    const std::string defaultNs(MPxAssembly::getRepNamespace().asChar());
    const std::string ns(TfStringPrintf("NS_%s", TfStringGetBeforeSuffix(defaultNs, '_').c_str()));

    return MString(ns.c_str());
}

/* virtual */
MString
UsdMayaReferenceAssembly::getRepNamespace() const
{
    MString repNamespaceStr;
    if (!TfGetEnvSetting(PIXMAYA_USE_USD_ASSEM_NAMESPACE)) {
        return repNamespaceStr;
    }

    // This was adapted from Autodesk's sample assembly reference node:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8cpp_example_html
    MPlug repNamespacePlug(thisMObject(), repNamespaceAttr);
    repNamespacePlug.getValue(repNamespaceStr);

    if (repNamespaceStr.numChars() == 0) {
        repNamespaceStr = getDefaultRepNamespace();

        // Update the attribute with the default representation namespace since
        // the attribute was previously empty.
        repNamespacePlug.setValue(repNamespaceStr);
    }

    return repNamespaceStr;
}

/* virtual */
void
UsdMayaReferenceAssembly::updateRepNamespace(const MString& repNamespace)
{
    // This was adapted from Autodesk's sample assembly reference node:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8cpp_example_html
    MPlug repNamespacePlug(thisMObject(), repNamespaceAttr);
    MString repCurrentNamespaceStr;
    repNamespacePlug.getValue(repCurrentNamespaceStr);

    bool prevVal = _updatingRepNamespace;
    _updatingRepNamespace = true;

    // Update the assembly attribute.
    repNamespacePlug.setValue(repNamespace);

    _updatingRepNamespace = prevVal;
}

// virtual
MStatus UsdMayaReferenceAssembly::setDependentsDirty( const MPlug& dirtiedPlug, MPlugArray& affectedPlugs)
{
    // Hardcoded dynamic attr naming: usdVariantSet_*
    // If an attr starts with "usdVariantSet_", then dirty the stage
    MString dirtiedPlugName = dirtiedPlug.partialName();
    const MString variantSetPrefix(UsdMayaVariantSetTokens->PlugNamePrefix.GetText());
    if ((dirtiedPlugName.length() > variantSetPrefix.length()) &&
        (dirtiedPlugName.substring(0, variantSetPrefix.length()-1) == variantSetPrefix))
    {
        TF_STATUS(
                "Dirtying stage due to variant selection change: %s",
                dirtiedPlugName.asChar());
        MObject thisNode = thisMObject();
        affectedPlugs.append( MPlug(thisNode, inStageDataCachedAttr) );
        affectedPlugs.append( MPlug(thisNode, outStageDataAttr) );
    }

    return MS::kSuccess;
}

MStatus UsdMayaReferenceAssembly::compute(const MPlug& aPlug,
                             MDataBlock& dataBlock)
{
    MStatus retValue = MS::kUnknownParameter;

    //
    // make sure the state of the model is normal
    //

    if (aPlug == inStageDataCachedAttr)
    {
        retValue = computeInStageDataCached(dataBlock);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
    }
    else if (aPlug == outStageDataAttr)
    {
        retValue = computeOutStageData(dataBlock);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
    }
    else {
        return MS::kUnknownParameter;
    }

    return MS::kSuccess;
}

static
MItEdits
_GetEdits(
        const MObject& assemObj)
{
    MObject editsOwner(assemObj);
    MObject targetNode(assemObj);
    return MItEdits(editsOwner, targetNode);
}

static
std::set<std::string> _GetVariantSetNamesForStageCache(
        const MFnDependencyNode& depNodeFn)
{
    const auto& regVarSets = UsdUtilsGetRegisteredVariantSets();
    if (!regVarSets.empty()) {
        std::set<std::string> ret;
        for (const auto& regVarSet: regVarSets) {
            ret.insert(regVarSet.name);
        }
        return ret;
    }

    std::set<std::string> varSetNames;
    for (unsigned int i = 0; i < depNodeFn.attributeCount(); i++) {
        MObject attrObj = depNodeFn.attribute(i);
        if (attrObj.isNull()) {
            continue;
        }

        MPlug attrPlug = depNodeFn.findPlug(attrObj);
        if (attrPlug.isNull()) {
            continue;
        }

        std::string attrName(attrPlug.partialName().asChar());
        if (!TfStringStartsWith(attrName, UsdMayaVariantSetTokens->PlugNamePrefix)) {
            continue;
        }

        std::string variantSet = attrName.substr(
            UsdMayaVariantSetTokens->PlugNamePrefix.GetString().size());
        varSetNames.insert(variantSet);
    }
    return varSetNames;
}

MStatus UsdMayaReferenceAssembly::computeInStageDataCached(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataHandle = dataBlock.inputValue(inStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // If inData has an incoming connection, then use it. Otherwise generate stage from the filepath
    if (!inDataHandle.data().isNull() ) {
        //
        // Propagate inData -> inDataCached
        //
        MDataHandle inDataCachedHandle =
            dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.copy(inDataHandle);

        inDataCachedHandle.setClean();
    }
    else {
        //
        // Calculate from USD filepath and primPath
        //

        // Get input attr values
        const MString aFile = dataBlock.inputValue(filePathAttr, &retValue).asString();
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        //
        // let the usd stage cache deal with caching the usd stage data
        //
        std::string fileString = TfStringTrimRight(aFile.asChar());

        // == Load the Stage
        UsdStageRefPtr usdStage;
        SdfPath        primPath;

        if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(fileString)) {
            MFnDependencyNode depNodeFn(thisMObject());

            std::map<std::string, std::string> varSels;
            TfToken modelName = UsdUtilsGetModelNameFromRootLayer(rootLayer);
            const std::set<std::string> varSetNamesForCache = _GetVariantSetNamesForStageCache(depNodeFn);
            TF_FOR_ALL(variantSet, varSetNamesForCache) {
                MString variantSetPlugName(UsdMayaVariantSetTokens->PlugNamePrefix.GetText());
                variantSetPlugName += variantSet->c_str();
                MPlug varSetPlg = depNodeFn.findPlug(variantSetPlugName, true);
                if (!varSetPlg.isNull()) {
                    MString varSetVal = varSetPlg.asString();
                    if (varSetVal.length() > 0) {
                        varSels[*variantSet] = varSetVal.asChar();
                    }
                }
            }

            TfToken drawMode;
            MPlug drawModePlug = depNodeFn.findPlug(drawModeAttr, true);
            if (!drawModePlug.isNull()) {
                drawMode = TfToken(drawModePlug.asString().asChar());
            }

            SdfLayerRefPtr sessionLayer =
                    UsdMayaStageCache::GetSharedSessionLayer(
                        SdfPath::AbsoluteRootPath().AppendChild(modelName),
                        varSels,
                        drawMode);

            // If we have assembly edits, do not share session layers with
            // other models that have our same set of variant selections,
            // since our edits may differ from theirs. Theoretically we
            // could hash all of our edit strings and share the same usd
            // stage as other models with the same hash, but it's not
            // typical to have enough models in a scene that share the same
            // set of edits in order to make that worthwhile.
            MObject assemObj = thisMObject();
            MItEdits assemEdits(_GetEdits(assemObj));
            if (!assemEdits.isDone()) {
                _hasEdits = true;
                SdfLayerRefPtr unsharedSessionLayer = SdfLayer::CreateAnonymous();
                unsharedSessionLayer->TransferContent(sessionLayer);
                sessionLayer = unsharedSessionLayer;
            }

            UsdStageCacheContext ctx(UsdMayaStageCache::Get());
            usdStage = UsdStage::Open(rootLayer,
                                      sessionLayer,
                                      ArGetResolver().GetCurrentContext());
            usdStage->SetEditTarget(usdStage->GetSessionLayer());

            primPath = usdStage->GetDefaultPrim() ?
                usdStage->GetDefaultPrim().GetPath() :

                // XXX:
                // Preserving prior behavior for now-- eventually might make
                // more sense to bail in this case.
                SdfPath::AbsoluteRootPath();
        }

        // If fileString is non-empty but we couldn't create a stage from there,
        // issue an error. (If fileString is empty, it just means that the
        // reference assembly hasn't been set up yet.)
        // We'll still return a success code from this function because we can
        // provide Maya with a sane result (an empty UsdMayaStageData).
        if (!fileString.empty() && !usdStage) {
            TF_RUNTIME_ERROR(
                    "Could not open stage with root layer '%s'",
                    fileString.c_str());
        }

        // Create the output outData ========
        MFnPluginData pluginDataFn;
        MObject stageDataObj =
            pluginDataFn.create(UsdMayaStageData::mayaTypeId, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        UsdMayaStageData* stageData =
            reinterpret_cast<UsdMayaStageData*>(pluginDataFn.data(&retValue));
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        // Set the outUsdStageData
        stageData->stage = usdStage;
        stageData->primPath = primPath;

        //
        // set the data on the output plug
        //
        MDataHandle inDataCachedHandle =
            dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.set(stageData);
        inDataCachedHandle.setClean();
    }
    return MS::kSuccess;
}



MStatus UsdMayaReferenceAssembly::computeOutStageData(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataCachedHandle =
        dataBlock.inputValue(inStageDataCachedAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdStageRefPtr usdStage;

    UsdMayaStageData* inData =
        dynamic_cast<UsdMayaStageData*>(inDataCachedHandle.asPluginData());
    if(inData)
    {
        usdStage = inData->stage;
    }

    // If failed to get a valid stage, then
    // Propagate inDataCached -> outData
    // and return
    if (!usdStage) {
        MDataHandle outDataHandle =
            dataBlock.outputValue(outStageDataAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        outDataHandle.copy(inDataCachedHandle);
        return MS::kSuccess;
    }

    // Get the primPath
    const MString aPrimPath =
        dataBlock.inputValue(primPathAttr, &retValue).asString();
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Get the prim
    // If no primPath string specified, then use the pseudo-root.
    UsdPrim usdPrim;
    std::string primPathStr = aPrimPath.asChar();
    if (primPathStr.empty() && usdStage->GetDefaultPrim()) {
        usdPrim = usdStage->GetDefaultPrim();
    }
    if (!usdPrim && !primPathStr.empty()) {
        SdfPath primPath(primPathStr);

        // Validate assumption: primPath is descendent of passed-in stage primPath
        //   Make sure that the primPath is a child of the passed in stage's primpath
        //   This allows data for variants to flow down the hierarchy as expected
        if ( primPath.HasPrefix(inData->primPath) ) {
            usdPrim = usdStage->GetPrimAtPath( primPath );
        }
        else {
            TF_WARN("%s: Assembly primPath <%s> is not a descendant of input "
                    "stage primPath <%s>. Skipping variant assignment.",
                    MPxNode::name().asChar(),
                    primPathStr.c_str(),
                    inData->primPath.GetText());
        }
    } else {
        TF_WARN("%s: Stage primPath MISSING", MPxNode::name().asChar());
    }

    // Handle UsdPrim variant overrides and draw modes for subassemblies (i.e.,
    // assemblies brought in by aggregate models).
    // Note that if we need to make any changes to the session layer here, we
    // must create a new UsdStage because the input stage might be shared
    // between multiple assemblies.
    MFnAssembly assemblyFn(thisMObject());
    if (usdPrim && !assemblyFn.isTopLevel()) {
        MFnDependencyNode depNodeFn(thisMObject());

        std::vector<std::string> variantSetNames = usdPrim.GetVariantSets().GetNames();
        std::map<std::string, std::string> varSets;
        TF_FOR_ALL(variantSet, variantSetNames) {
            MString variantSetPlugName(UsdMayaVariantSetTokens->PlugNamePrefix.GetText());
            variantSetPlugName += variantSet->c_str();
            MPlug varSetPlg = depNodeFn.findPlug(variantSetPlugName, true);
            if (!varSetPlg.isNull()) {
                MString varSetVal = varSetPlg.asString();
                std::string newVarSelect = varSetVal.asChar();
                std::string existingVarSelect = usdPrim
                        .GetVariantSet(*variantSet)
                        .GetVariantSelection();
                if (newVarSelect != existingVarSelect) {
                    varSets[*variantSet] = newVarSelect;
                }
            }
        }

        TfToken drawMode;
        MPlug drawModePlug = depNodeFn.findPlug(drawModeAttr, true);
        if (!drawModePlug.isNull()) {
            TfToken newDrawMode(drawModePlug.asString().asChar());
            TfToken existingDrawMode =
                    UsdGeomModelAPI(usdPrim).ComputeModelDrawMode();
            if (newDrawMode != existingDrawMode) {
                drawMode = newDrawMode;
            }
        }

        // There's something that we need to modify on the session layer.
        // Replace usdStage with a new stage where we can just insert our new
        // session layer.
        if (!varSets.empty() || !drawMode.IsEmpty()) {
            SdfLayerRefPtr newLayer = UsdMayaStageCache::GetSharedSessionLayer(
                    usdPrim.GetPath(),
                    varSets,
                    drawMode);
            SdfLayerRefPtr oldLayer = usdPrim.GetStage()->GetSessionLayer();

            SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous();
            sessionLayer->TransferContent(oldLayer);
            sessionLayer->TransferContent(newLayer);

            UsdStageCacheContext ctx(UsdMayaStageCache::Get());
            usdStage = UsdStage::Open(usdPrim.GetStage()->GetRootLayer(),
                                      sessionLayer,
                                      ArGetResolver().GetCurrentContext());
            usdStage->SetEditTarget(usdStage->GetSessionLayer());
        }
    }

    // Create the output outData
    MFnPluginData pluginDataFn;
    MObject stageDataObj =
        pluginDataFn.create(UsdMayaStageData::mayaTypeId, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdMayaStageData* stageData =
        reinterpret_cast<UsdMayaStageData*>(pluginDataFn.data(&retValue));
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Set the outUsdStageData
    stageData->stage = usdStage;
    // If usdPrim is still invalid, then the stage has no default prim.
    stageData->primPath = usdPrim ? usdPrim.GetPath() :
                                    SdfPath::AbsoluteRootPath();

    //
    // set the data on the output plug
    //
    MDataHandle outDataHandle =
        dataBlock.outputValue(outStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outDataHandle.set(stageData);
    outDataHandle.setClean();

    return MS::kSuccess;
}


bool UsdMayaReferenceAssembly::setInternalValueInContext( const MPlug& plug,
                                             const MDataHandle& dataHandle,
                                             MDGContext& ctx)
{
    MStatus status;
    if (_inSetInternalValue) {
        return false;
    }

    // This was adapted from Autodesk's sample assembly reference node:
    //
    // http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__cpp_ref_scene_assembly_2assembly_reference_8cpp_example_html
    if (plug == repNamespaceAttr && !_updatingRepNamespace) {
        // Rename the namespace associated with the assembly with the new
        // repNamespace. Correct the repNamespace if needed.
        // To rename the namespace, there are 2 cases to get the oldNS to
        // rename:
        //     1 - If the assembly namespace attribute is changed directly
        //         (i.e. someone did a setAttr directly, or modified it via the
        //         attribute editor), we get the oldNS (the namespace to be
        //         renamed) using the plug value, which has not been set yet.
        //         So query the oldNS name from current state of the datablock,
        //         and the new one from the the data handle that is passed into
        //         this method.
        //
        //     2 - If we are in IO, the plug value has already been set, but
        //         the namespace still has the default value given by
        //         getDefaultRepNamespace().
        MString oldNS;
        plug.getValue(oldNS);

        // Early-out if the plug value is empty: the namespace has not been created yet.
        if (oldNS.numChars() == 0) {
            return false;
        }

        // Get the default namespace to rename.
        if (MFileIO::isOpeningFile() || MFileIO::isReadingFile()) {
            oldNS = getDefaultRepNamespace();
        }

        MString newNS = dataHandle.asString();

        // Validate the name and only use it if it is valid (not empty).
        // If the name is not valid, or if the user entered "" as repNamespace,
        // use the default namespace.
        MString validNewNS = MNamespace::validateName(newNS, &status);
        if (status != MStatus::kSuccess) {
            return false;
        }

        if (validNewNS.numChars() == 0) {
            validNewNS = getDefaultRepNamespace();
        }

        if (validNewNS != newNS) {
            // Update the value of newNS and of the data-handle.
            newNS = validNewNS;
            MDataHandle* nonConstHandle = (MDataHandle*) &dataHandle;
            nonConstHandle->set(newNS);
        }

        // Finally, tell Maya to rename namespaces.
        if (oldNS.numChars() > 0 && newNS.numChars() > 0 && oldNS != newNS) {
            status = MNamespace::renameNamespace(oldNS, newNS);
            if (status != MStatus::kSuccess) {
                // The rename failed. Set back the old value.
                // Note: if the rename failed, it is probably because the
                // namespace newNS already existed, but it is the
                // responsibility of the user to provide a name that does not
                // exist.
                MDataHandle* nonConstHandle = (MDataHandle*) &dataHandle;
                nonConstHandle->set(oldNS);
            }
        }

        return true;
    }

    bool setAttrSuccess = MPxAssembly::setInternalValueInContext(plug, dataHandle, ctx);

    bool varSelChanged = TfStringStartsWith(plug.partialName().asChar(), UsdMayaVariantSetTokens->PlugNamePrefix);

    if (varSelChanged ||
            (std::find(attrsAffectingRepresentation.begin(),
                       attrsAffectingRepresentation.end(),
                       plug.attribute())
             != attrsAffectingRepresentation.end())) {
        // == Block off this code from being called recursively
        _inSetInternalValue = true;

        MString activeRep( getActive() );
        // NOTE:
        //     Cannot activate and cause dg modifications while in this evaluation of
        //     setInternalValueInContext().
        //     Using executeCommandOnIdle() to get around this limitation
        //
        //     XXX: This is somewhat of an abuse of the internal attribute
        //     system and should be refactored. It is also brittle in that the
        //     name of the node could be changed in between the deferred command
        //     being built and when it is actually executed.
        if (activeRep.length() > 0) {
            MString cmd;
            cmd += "{ ";
            cmd += "$undoState = `undoInfo -q -state`; ";
            cmd += "undoInfo -stateWithoutFlush 0; ";
            cmd += "catch(`assembly -e -active \"" + activeRep +"\" \"" + name() + "\"`); ";
            cmd += "undoInfo -stateWithoutFlush $undoState; ";
            cmd += "}";
            MGlobal::executeCommandOnIdle(cmd, false);
        }

        // == Unblock off this code from being executed
        _inSetInternalValue = false;
    }

    return setAttrSuccess;
}


UsdPrim UsdMayaReferenceAssembly::usdPrim() const
{
    MStatus status;
    UsdPrim usdPrim;

    UsdMayaReferenceAssembly* nonConstThis = const_cast<UsdMayaReferenceAssembly*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();

    MDataHandle outDataHandle = dataBlock.inputValue(outStageDataAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, usdPrim);

    UsdMayaStageData* outData = dynamic_cast<UsdMayaStageData*>(outDataHandle.asPluginData());
    if(!outData) {
        return usdPrim; // empty UsdPrim
    }

    if(!outData->stage) {
        return usdPrim; // empty UsdPrim
    }

    usdPrim = outData->primPath.IsEmpty() ?
        (outData->stage->GetDefaultPrim() ?
            outData->stage->GetDefaultPrim() :
            outData->stage->GetPseudoRoot()) :
        outData->stage->GetPrimAtPath(outData->primPath);

    return usdPrim;
}

std::map<std::string, std::string> UsdMayaReferenceAssembly::GetVariantSetSelections() const
{
    std::map<std::string, std::string> result;

    UsdPrim usdPrim = this->usdPrim();
    if (!usdPrim) {
        return result;
    }

    MFnDependencyNode depNodeFn(thisMObject());

    std::vector<std::string> variantSetNames = usdPrim.GetVariantSets().GetNames();
    for (std::string variantSetName : variantSetNames) {
        MString variantSetPlugName(UsdMayaVariantSetTokens->PlugNamePrefix.GetText());
        variantSetPlugName += variantSetName.c_str();
        MPlug variantSetPlg = depNodeFn.findPlug(variantSetPlugName, true);
        if (!variantSetPlg.isNull()) {
            MString variantSelection = variantSetPlg.asString();
            if (variantSelection.length() > 0) {
                result[variantSetName] = variantSelection.asChar();
            }
        }
    }

    return result;
}

void
UsdMayaReferenceAssembly::ConnectMayaTimeToAssemblyTime()
{
    MFnAssembly assemblyFn(thisMObject());
    MPlug assemblyTimePlug = assemblyFn.findPlug(timeAttr, true);
    if (!assemblyTimePlug || assemblyTimePlug.isConnected()) {
        // Bail out if we couldn't find the plug, or if it is already connected.
        return;
    }

    const MPlug mayaTimePlug = UsdMayaUtil::GetMayaTimePlug();
    if (mayaTimePlug.isNull()) {
        return;
    }

    MDGModifier dgMod;
    dgMod.connect(mayaTimePlug, assemblyTimePlug);
    dgMod.doIt();
}

void
UsdMayaReferenceAssembly::DisconnectAssemblyTimeFromMayaTime()
{
    MFnAssembly assemblyFn(thisMObject());
    MPlug assemblyTimePlug = assemblyFn.findPlug(timeAttr, true);
    if (!assemblyTimePlug || !assemblyTimePlug.isConnected()) {
        // Bail out if we couldn't find the plug, or if it is NOT already
        // connected.
        return;
    }

    const MPlug mayaTimePlug = UsdMayaUtil::GetMayaTimePlug();
    if (mayaTimePlug.isNull()) {
        return;
    }

    MDGModifier dgMod;
    dgMod.disconnect(mayaTimePlug, assemblyTimePlug);
    dgMod.doIt();
}

// =========================================================

UsdMayaRepresentationBase::UsdMayaRepresentationBase(
        MPxAssembly* assembly,
        const MString& name)
    : MPxRepresentation(assembly, name)
{
}

bool UsdMayaRepresentationBase::inactivate()
{
    // XXX: We would rather not override inactivate() and leave the work to
    // Maya's default implementation which removes everything under the
    // assembly:
    //
    // http://help.autodesk.com/cloudhelp/2015/ENU/Maya-SDK/cpp_ref/class_m_px_representation.html#a8e33c21ff5c58325959c7a187a6ee401
    //
    // Unfortunately, it also flushes the undo queue, which we don't want.
    // Because of this, we manually remove everything under the assembly node
    // and do NOT call up to the parent class' inactivate().
    //
    MStatus status;
    MFnDagNode dagFn(getAssembly()->thisMObject());
    unsigned int numChildNodes = dagFn.childCount(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDagModifier dagMod;

    while (numChildNodes > 0) {
        MObject childNode = dagFn.child(0, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        MStatus status = dagFn.removeChild(childNode);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dagMod.deleteNode(childNode);
        CHECK_MSTATUS_AND_RETURN(status, false);

        numChildNodes = dagFn.childCount(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}


// =========================================================

const MString UsdMayaRepresentationCollapsed::_assemblyType("Collapsed");
const MString UsdMayaRepresentationCards::_assemblyType("Cards");
const MString UsdMayaRepresentationPlayback::_assemblyType("Playback");

bool UsdMayaRepresentationProxyBase::activate()
{
    // Get original selection list
    MSelectionList origSelList;
    MGlobal::getActiveSelectionList(origSelList, true);

    MDagModifier dagMod;
    MObject shapeObj = dagMod.createNode(UsdMayaProxyShape::typeName, getAssembly()->thisMObject());
    dagMod.renameNode(shapeObj, "CollapsedProxy");
    dagMod.doIt();

    MDGModifier dgMod;
    MFnDependencyNode shapeFn(shapeObj);
    MFnAssembly assemblyFn(getAssembly()->thisMObject());
    dgMod.connect(
        assemblyFn.findPlug(UsdMayaReferenceAssembly::filePathAttr, true),
        shapeFn.findPlug(UsdMayaProxyShape::filePathAttr, true));
    dgMod.connect(
        assemblyFn.findPlug(UsdMayaReferenceAssembly::primPathAttr, true),
        shapeFn.findPlug(UsdMayaProxyShape::primPathAttr, true) );
    dgMod.connect(
        assemblyFn.findPlug(UsdMayaReferenceAssembly::complexityAttr, true),
        shapeFn.findPlug(UsdMayaProxyShape::complexityAttr, true));
    dgMod.connect(
        assemblyFn.findPlug(UsdMayaReferenceAssembly::outStageDataAttr, true),
        shapeFn.findPlug(UsdMayaProxyShape::inStageDataAttr, true));

    _OverrideProxyPlugs(shapeFn, dgMod);

    dgMod.newPlugValueBool(
        shapeFn.findPlug(UsdMayaProxyShape::softSelectableAttr, true),
        _proxyIsSoftSelectable);

    dgMod.doIt();

    _PushEditsToProxy();

    // Restore original selection
    MStatus status;
    status = MGlobal::setActiveSelectionList(origSelList);
    CHECK_MSTATUS(status);

    return true;

}

void
UsdMayaRepresentationProxyBase::_PushEditsToProxy()
{
    // The following logic is all currently only supported for the "default"
    // unvarying time.

    MObject assemObj = getAssembly()->thisMObject();
    UsdMayaReferenceAssembly* usdAssem = dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly());
    MFnAssembly assemblyFn(assemObj);
    MString assemblyPathStr = assemblyFn.partialPathName();
    MItEdits assemEdits(_GetEdits(assemObj));
    bool hasEdits = !assemEdits.isDone();
    if (usdAssem->HasEdits() != hasEdits) {
        usdAssem->SetHasEdits(hasEdits);

        // If we now have edits but previous did not, or vice versa, make sure
        // we invalidate our UsdStage so that we are not sharing with other
        // model instances that do not have edits.
        MGlobal::executeCommand("dgdirty " + assemblyPathStr);
    }

    UsdPrim proxyRootPrim = dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly())->usdPrim();
    if (!proxyRootPrim) {
        return;
    }
    UsdStagePtr stage = proxyRootPrim.GetStage();

    UsdMayaEditUtil::PathEditMap refEdits;
    std::vector< std::string > invalidEdits, failedEdits;

    UsdMayaEditUtil::GetEditsForAssembly( assemObj, &refEdits, &invalidEdits );

    if( !refEdits.empty() )
    {
        // Create an anonymous layer to hold the assembly edit opinions, and
        // sublayer it into the stage's session layer.
        _sessionSublayer = SdfLayer::CreateAnonymous();
        stage->GetSessionLayer()->GetSubLayerPaths().clear();
        stage->GetSessionLayer()->GetSubLayerPaths().push_back(
            _sessionSublayer->GetIdentifier());

        // Make the session sublayer the edit target before applying the Maya
        // edits to ensure that we don't pollute other assemblies using the
        // same layer(s).
        UsdEditContext editContext(stage, _sessionSublayer);

        UsdMayaEditUtil::ApplyEditsToProxy( refEdits, stage, proxyRootPrim, &failedEdits );
    }

    if( !invalidEdits.empty() )
    {
        TF_WARN("The following edits could not be read from the proxy for '%s':"
                "\n\t%s",
                assemblyPathStr.asChar(),
                TfStringJoin(invalidEdits, "\n\t").c_str());
    }

    if( !failedEdits.empty() )
    {
        TF_WARN("The following edits could not be pushed to the proxy for '%s':"
                "\n\t%s",
                assemblyPathStr.asChar(),
                TfStringJoin(failedEdits, "\n\t").c_str());
    }
}

bool UsdMayaRepresentationProxyBase::inactivate()
{
    // Clear out session sublayer and remove it from the layer stack, to avoid
    // polluting other representations of the same stage.
    UsdPrim proxyRootPrim = dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly())->usdPrim();
    if (proxyRootPrim) {
        proxyRootPrim.GetStage()->GetSessionLayer()->GetSubLayerPaths().clear();
        _sessionSublayer = SdfLayerRefPtr();
    }

    return UsdMayaRepresentationBase::inactivate();
}

void
UsdMayaRepresentationProxyBase::_OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                                    MDGModifier &dgMod)
{
    MFnAssembly assemblyFn(getAssembly()->thisMObject());

    dgMod.connect(assemblyFn.findPlug(UsdMayaReferenceAssembly::timeAttr, true),
                  shapeFn.findPlug(UsdMayaProxyShape::timeAttr, true));

    dgMod.connect(assemblyFn.findPlug(UsdMayaReferenceAssembly::excludePrimPathsAttr, true),
                  shapeFn.findPlug(UsdMayaProxyShape::excludePrimPathsAttr, true));
}

void
UsdMayaRepresentationCollapsed::_OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                                    MDGModifier &dgMod)
{
    dgMod.newPlugValueBool(
        shapeFn.findPlug(UsdMayaProxyShape::fastPlaybackAttr, true),
        false);

    // Call parent for common proxy overrides
    UsdMayaRepresentationProxyBase::_OverrideProxyPlugs(shapeFn, dgMod);
}

bool
UsdMayaRepresentationCards::activate()
{
    MFnDagNode dagFn(getAssembly()->thisMObject());
    MPlug drawMode = dagFn.findPlug(UsdMayaReferenceAssembly::drawModeAttr, true);
    drawMode.setString("cards");

    return UsdMayaRepresentationProxyBase::activate();
}

bool
UsdMayaRepresentationCards::inactivate()
{
    MFnDagNode dagFn(getAssembly()->thisMObject());
    MPlug drawMode = dagFn.findPlug(UsdMayaReferenceAssembly::drawModeAttr, true);
    drawMode.setString("");

    return UsdMayaRepresentationProxyBase::inactivate();
}

void
UsdMayaRepresentationCards::_OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                                    MDGModifier &dgMod)
{
    dgMod.newPlugValueBool(
        shapeFn.findPlug(UsdMayaProxyShape::fastPlaybackAttr, true),
        false);

    // Call parent for common proxy overrides
    UsdMayaRepresentationProxyBase::_OverrideProxyPlugs(shapeFn, dgMod);
}

/* virtual */
bool
UsdMayaRepresentationPlayback::activate()
{
    UsdMayaReferenceAssembly* usdAssembly =
        dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly());
    usdAssembly->ConnectMayaTimeToAssemblyTime();

    return UsdMayaRepresentationProxyBase::activate();
}

/* virtual */
bool
UsdMayaRepresentationPlayback::inactivate()
{
    UsdMayaReferenceAssembly* usdAssembly =
        dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly());
    usdAssembly->DisconnectAssemblyTimeFromMayaTime();

    return UsdMayaRepresentationProxyBase::inactivate();
}

void
UsdMayaRepresentationPlayback::_OverrideProxyPlugs(MFnDependencyNode &shapeFn,
                                                   MDGModifier &dgMod)
{
    dgMod.newPlugValueBool(
        shapeFn.findPlug(UsdMayaProxyShape::fastPlaybackAttr, true),
        true);

    // Call parent for common proxy overrides
    UsdMayaRepresentationProxyBase::_OverrideProxyPlugs(shapeFn, dgMod);
}



// =========================================================

void
UsdMayaRepresentationHierBase::_ConnectSubAssemblyPlugs()
{
    MStatus status;

    MFnDagNode dagFn(getAssembly()->thisMObject());
    MDagPath assemblyPath;
    dagFn.getPath(assemblyPath);
    MSelectionList childUsdAssemblyNodes;

    std::string cmdStr = TfStringPrintf(
            "select `listRelatives -allDescendents -type \"%s\" \"%s\"`",
            UsdMayaReferenceAssembly::typeName.asChar(),
            assemblyPath.partialPathName().asChar());

    MGlobal::executeCommand(MString(cmdStr.c_str()));
    MGlobal::getActiveSelectionList(childUsdAssemblyNodes);

    MDGModifier dgMod;
    MObject childAssemblyNodeObj;
    for ( MItSelectionList it(childUsdAssemblyNodes); !it.isDone(); it.next() ) {
        status = it.getDependNode(childAssemblyNodeObj);
        CHECK_MSTATUS(status);
        MFnAssembly childAssembly(childAssemblyNodeObj, &status);
        CHECK_MSTATUS(status);
        dgMod.connect(dagFn.findPlug(UsdMayaReferenceAssembly::complexityAttr, true),
                      childAssembly.findPlug(UsdMayaReferenceAssembly::complexityAttr, true));
        dgMod.connect(dagFn.findPlug(UsdMayaReferenceAssembly::outStageDataAttr, true),
                      childAssembly.findPlug(UsdMayaReferenceAssembly::inStageDataAttr, true));
    }
    dgMod.doIt();
}

void
UsdMayaRepresentationHierBase::_ConnectProxyPlugs()
{
    MStatus status;

    MFnDagNode dagFn(getAssembly()->thisMObject());
    MDagPath assemblyPath;
    dagFn.getPath(assemblyPath);
    MSelectionList childUsdProxyNodes;

    std::string cmdStr = TfStringPrintf(
            "select `listRelatives -allDescendents -type \"%s\" \"%s\"`",
            UsdMayaProxyShape::typeName.asChar(),
            assemblyPath.partialPathName().asChar());

    MGlobal::executeCommand(MString(cmdStr.c_str()));
    MGlobal::getActiveSelectionList(childUsdProxyNodes);

    MDGModifier dgMod;
    MObject childUsdProxyNodeObj;
    for ( MItSelectionList it(childUsdProxyNodes); !it.isDone(); it.next() ) {
        status = it.getDependNode(childUsdProxyNodeObj);
        CHECK_MSTATUS(status);
        MFnDependencyNode proxyDepNodeFn(childUsdProxyNodeObj, &status);
        CHECK_MSTATUS(status);
        dgMod.connect(dagFn.findPlug(UsdMayaReferenceAssembly::timeAttr, true),
                      proxyDepNodeFn.findPlug(UsdMayaProxyShape::timeAttr, true));
    }
    dgMod.doIt();
}

bool UsdMayaRepresentationHierBase::activate()
{
    MStatus status;

    // Get original selection list
    MSelectionList origSelList;
    MGlobal::getActiveSelectionList(origSelList, true);

    // Get attr values
    MFnAssembly assemblyFn(getAssembly()->thisMObject());
    MString usdFilePath(assemblyFn.findPlug(UsdMayaReferenceAssembly::filePathAttr, true).asString());
    MString usdPrimPath(assemblyFn.findPlug(UsdMayaReferenceAssembly::primPathAttr, true).asString());

    // Get the variant set selections from the Maya assembly node.
    UsdMayaReferenceAssembly* usdAssembly =
        dynamic_cast<UsdMayaReferenceAssembly*>(getAssembly());
    std::map<std::string, std::string> variantSetSelections =
        usdAssembly->GetVariantSetSelections();

    VtDictionary userArgs;
    bool shouldImportWithProxies = _ShouldImportWithProxies();
    if (shouldImportWithProxies) {
        // In this mode, sub-assembly nodes we create should come in unloaded.
        userArgs[UsdMayaJobImportArgsTokens->assemblyRep] =
                UsdMayaJobImportArgsTokens->Unloaded.GetString();
    }

    UsdMayaJobImportArgs importArgs =
            UsdMayaJobImportArgs::CreateFromDictionary(
                userArgs, shouldImportWithProxies,
                GfInterval::GetFullInterval());
    UsdMaya_ReadJob readJob(usdFilePath.asChar(),
                       usdPrimPath.asChar(),
                       variantSetSelections,
                       importArgs);

    // Set the assembly node as the root node of the read job.
    MDagPath assemblyDagPath;
    status = assemblyFn.getPath(assemblyDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);
    readJob.SetMayaRootDagPath(assemblyDagPath);

    std::vector<MDagPath> addedDagPaths;
    if (!readJob.Read(&addedDagPaths)) {
        return false;
    }

    _ConnectSubAssemblyPlugs();
    _ConnectProxyPlugs();

    // Restore original selection
    status = MGlobal::setActiveSelectionList(origSelList);
    CHECK_MSTATUS(status);

    return true;
}


// ==========================================================

const MString UsdMayaRepresentationExpanded::_assemblyType("Expanded");

const MString UsdMayaRepresentationFull::_assemblyType("Full");

PXR_NAMESPACE_CLOSE_SCOPE

