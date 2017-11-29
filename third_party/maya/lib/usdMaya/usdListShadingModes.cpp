#include "usdMaya/usdListShadingModes.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/registryHelper.h"

#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

usdListShadingModes::usdListShadingModes() {

}

usdListShadingModes::~usdListShadingModes() {

}

MStatus
usdListShadingModes::doIt(const MArgList& args) {    
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        MGlobal::displayError("Invalid parameters detected. Exiting.");
        return status;
    }

    TfTokenVector v;
    if (argData.isFlagSet("export")) {
        v = PxrUsdMayaShadingModeRegistry::ListExporters();
    } else if (argData.isFlagSet("import")) {
        v = PxrUsdMayaShadingModeRegistry::ListImporters();
    }

    // This is remapped in JobArgs.cpp manually
    appendToResult("None");

    for (const auto& e : v) {
        // Manual remappings
        if (e == PxrUsdMayaShadingModeTokens->displayColor) {
            appendToResult("GPrim Colors");
            appendToResult("Material Colors");
        } else if (e == "pxrRis") {
            appendToResult("RfM Shaders");
        } else {
            appendToResult(e.GetString().c_str());
        }
    }

    return MS::kSuccess;
}

MSyntax
usdListShadingModes::createSyntax() {
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* usdListShadingModes::creator() {
    return new usdListShadingModes();
}

PXR_NAMESPACE_CLOSE_SCOPE
