#include "pxr/pxr.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdr/registry.h"
#include "amber/pr/tokens.h"
#include "amber/pr/pathResolverUtils.h"
#include <pixver.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct HdPrman_Context;

TF_REGISTRY_FUNCTION(HdPrman_Context)
{
    const std::string unitName = TfGetenv("UNIT");
    const std::string rmantree = TfGetenv("RMANTREE");
    const std::string rmanpkgpath = PixverGetPackageLocation("rmanpkg");

    std::vector<std::string> shaderpaths =
        SdrRegistry::GetInstance().GetSearchURIs();
    shaderpaths.push_back(rmantree + "/lib/shaders");
    std::string shaderpath = TfStringJoin(shaderpaths, ":");

    std::vector<std::string> rixpluginpaths =
        PrComputeResourcePath(unitName, PrResourceTokens->RIXPLUGINPATH);
    rixpluginpaths.push_back(rmanpkgpath + "/plugin");
    rixpluginpaths.push_back(rmantree + "/lib/plugins");
    std::string rixpluginpath = TfStringJoin(rixpluginpaths, ":");

    std::vector<std::string> texturepaths = PrComputeResolveSearchPath();
    // Add rmanpkg/plugin so we can find rtx_glfImage.
    texturepaths.push_back(rmanpkgpath + "/plugin");
    std::string texturepath = TfStringJoin(texturepaths, ":");

    ArchSetEnv("RMAN_SHADERPATH", shaderpath, /* overwrite = */ true);
    ArchSetEnv("RMAN_RIXPLUGINPATH", rixpluginpath, /* overwrite = */ true);
    ArchSetEnv("RMAN_TEXTUREPATH", texturepath, /* overwrite = */ true);

    ArchSetEnv("HDX_PRMAN_INTEGRATOR", "PbsPathTracer", /* overwrite = */ true);
}

} // namespace

PXR_NAMESPACE_CLOSE_SCOPE
