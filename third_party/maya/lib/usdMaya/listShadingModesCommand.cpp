#include "usdMaya/listShadingModesCommand.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/registryHelper.h"

#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MString.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaListShadingModesCommand::UsdMayaListShadingModesCommand() {

}

UsdMayaListShadingModesCommand::~UsdMayaListShadingModesCommand() {

}

MStatus
UsdMayaListShadingModesCommand::doIt(const MArgList& args) {    
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    TfTokenVector v;
    if (argData.isFlagSet("export")) {
        v = PxrUsdMayaShadingModeRegistry::ListExporters();
    } else if (argData.isFlagSet("import")) {
        v = PxrUsdMayaShadingModeRegistry::ListImporters();
    }

    // Always include the "none" shading mode.
    appendToResult(PxrUsdMayaShadingModeTokens->none.GetText());

    for (const auto& e : v) {
        appendToResult(e.GetText());
    }

    return MS::kSuccess;
}

MSyntax
UsdMayaListShadingModesCommand::createSyntax() {
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* UsdMayaListShadingModesCommand::creator() {
    return new UsdMayaListShadingModesCommand();
}

PXR_NAMESPACE_CLOSE_SCOPE
