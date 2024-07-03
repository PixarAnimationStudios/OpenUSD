//
// Copyright 2022 Apple
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

#include "pxr/pxr.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/exception.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/usdRender/pass.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/usd/usdUtils/timeCodeRange.h"
#include "pxr/usdImaging/usdAppUtils/camera.h"
#include "pxr/usdImaging/usdAppUtils/frameRecorder.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <iostream>
#include <vector>
#include <string>
#include <regex>

#if PXR_USE_GLFW
#include <GLFW/glfw3.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

#if PXR_USE_GLFW
class GLFWOpenGLContext;
using GLFWOpenGLContextPtr = std::unique_ptr<GLFWOpenGLContext>;

// RAII wrapper around the GLFW resources. Not currently safe for multiple uses in other tools
// would need to be a singleton wrapper around the init/terminate, with a call to serve up
// managed windows/contexts. 
class GLFWOpenGLContext
{
    GLFWOpenGLContext() {};

    GLFWOpenGLContext(const GLFWOpenGLContext&) = delete;
    GLFWOpenGLContext &operator=(const GLFWOpenGLContext&) = delete;

public:
    ~GLFWOpenGLContext() {
        glfwTerminate();
    };

    static GLFWOpenGLContextPtr create(unsigned int imageWidth, unsigned int imageHeight) {
        if (!glfwInit())
            return nullptr;

        GLFWOpenGLContextPtr contextPtr = std::unique_ptr<GLFWOpenGLContext>(new GLFWOpenGLContext());

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        // Create a windowed mode window and its OpenGL context
        GLFWwindow* window = glfwCreateWindow(imageWidth, imageWidth, "no title", NULL, NULL);
        if (!window) {
            return nullptr;
        }

        // Make the window's context current
        glfwMakeContextCurrent(window);

        return contextPtr;
    }
};
#endif


using namespace pxr_CLI;

struct Args {
    std::string usdFilePath;
    std::string outputImagePath;
    std::string populationMask;
    std::string purposes;
    std::string sessionLayerPath;
    bool disableGpu = false;
    bool cameraLightEnabled = true;
    std::string camera;
    bool defaultTime = false;
    std::string framesStr;
    std::vector<UsdTimeCode> frames;
    std::string rendererPlugin;
    std::string colorCorrectionMode;
    std::string complexity;
    int32_t imageWidth;
    std::string aovName;
    bool domeLightVisibility = true;
    std::string rsPrimPath;
    std::string rpPrimPath;
};

// cameraArgs.py module
SdfPath cameraArgs_GetCameraSdfPath(const std::string &cameraPath) {
    // This avoids an Sdf warning if an empty string is given, which someone
    // might do for example with usdview to open the app using the 'Free' camera
    // instead of the primary camera.
    if (cameraPath.empty())
        return SdfPath::EmptyPath();
    return SdfPath(cameraPath);
}

void cameraArgs_AddCmdlineArgs(CLI::App *app, Args &args, std::string defaultValue="", std::string altHelpText="") {
    // Adds camera-related command line arguments to argsParser.
    //
    // The resulting 'camera' argument will be an Sdf.Path. If no value is given
    // and defaultValue is not overridden, 'camera' will be a single-element path
    // containing the primary camera name.

    if (defaultValue.empty()) {
        defaultValue = UsdUtilsGetPrimaryCameraName(true);
    }

    std::string helpText = altHelpText;
    if (helpText.empty()) {
        helpText = "Which camera to use - may be given as either just the "
                   "camera\'s prim name (i.e. just the last element in the prim "
                   "path), or as a full prim path. Note that if only the prim name "
                   "is used and more than one camera exists with that name, which "
                   "one is used will effectively be random";
    }

    app->add_option("--cam, --camera", args.camera, helpText)
            ->default_val(defaultValue)
            ->option_text("Camera Prim Path");
}

// frameArgs.py module
struct FrameNumberFormatter
{
    FrameNumberFormatter( uint32_t w, uint32_t p ) : width(w), precision(p) {}

    uint32_t width;
    uint32_t precision;
};

typedef std::unique_ptr<FrameNumberFormatter> FrameNumberFormatterPtr;

void framesArgs_AddCmdlineArgs(CLI::App *app, Args &args, std::string altDefaultTimeHelpText="",
                               std::string altFramesHelpText="") {
    // Adds frame-related command line arguments to argsParser.
    //
    // The resulting 'frames' argument will be an iterable of UsdTimeCodes.
    //
    // If no command-line arguments are given, 'frames' will be a list containing
    // only Usd.TimeCode.EarliestTime(). If '--defaultTime' is given, 'frames'
    // will be a list containing only Usd.TimeCode.Default(). Otherwise,
    // '--frames' must be given a FrameSpec (or a comma-separated list of
    // multiple FrameSpecs), and 'frames' will be a FrameSpecIterator which when
    // iterated will yield the time codes specified by the FrameSpec(s).

    std::string helpText = altDefaultTimeHelpText;
    if (helpText.empty()) {
        helpText = "explicitly operate at the Default time code (the default "
                   "behavior is to operate at the startTimeCode authored on the "
                   "'UsdStage which defaults to 0.0))";
    }

    auto defaultTimeOpt = app->add_option("-d,--defaultTime", args.defaultTime, helpText);

    std::string helpText2 = altFramesHelpText;
    if (helpText2.empty()) {
        helpText2 = "specify FrameSpec(s) of the time codes to operate on - A "
                    "FrameSpec consists of up to three floating point values for the "
                    "start time code, end time code, and stride of a time code range. "
                    "A single time code can be specified, or a start and end time "
                    "code can be specified separated by a colon (:). When a start "
                    "and end time code are specified, the stride may optionally be "
                    "specified as well, separating it from the start and end time "
                    "codes with (x). Multiple FrameSpecs can be combined as a "
                    "comma-separated list. The following are examples of valid "
                    "FrameSpecs: 123 - 101:105 - 105:101 - 101:109x2 - 101:110x2 - "
                    "101:104x0.5";
    }
    auto framesOpt = app->add_option("-f,--frames", args.framesStr, helpText2)
            ->option_text("FRAMESPEC[,FRAMESPEC...]");

    // make --defaultTime and --frames options mutually exclusive.
    defaultTimeOpt->excludes(framesOpt);
    framesOpt->excludes(defaultTimeOpt);
}

uint32_t framesArgs_getFloatStringPrecision(const std::string &floatString) {
    //Gets the floating point precision specified by floatString.
    //
    //floatString can either contain an actual float in string form, or it can be
    //        a frame placeholder. We simply split the string on the dot (.) and return
    //the length of the part after the dot, if any.
    //
    //If there is no dot in the string, a precision of zero is assumed.

    uint32_t floatPrecision = 0;

    if (floatString.empty()) {
        return floatPrecision;
    }

    auto floatStringParts = TfStringSplit(floatString, ".");
    if (floatStringParts.size() > 1) {
        floatPrecision = floatStringParts[1].size();
    }
    return floatPrecision;
}

class FrameSpecIterator
{
    const std::string FRAMESPEC_SEPARATOR = ",";

public:
    FrameSpecIterator(const std::string &frameSpec) {
        auto subFrameSpecs = TfStringSplit(frameSpec, FRAMESPEC_SEPARATOR);
        for (auto const &subFrameSpec: subFrameSpecs) {
            auto timeCodeRange = UsdUtilsTimeCodeRange::CreateFromFrameSpec(subFrameSpec);
            timeCodeRanges.emplace_back(timeCodeRange);

            auto specParts = TfStringSplit(subFrameSpec, UsdUtilsTimeCodeRangeTokens->StrideSeparator.GetString());

            if (specParts.size() == 2) {
                auto stride = specParts[1];
                auto stridePrecision = framesArgs_getFloatStringPrecision(stride);
                minFloatPrecision = std::max(minFloatPrecision, stridePrecision);
            }
        }
    }

    uint32_t getMinFloatPrecision() const { return minFloatPrecision; }
    std::vector<UsdTimeCode> getTimeCodes() const {
        std::vector<UsdTimeCode> result;
        for (const auto &timeCodeRange: timeCodeRanges) {
            for (const auto &timeCode: timeCodeRange) {
                result.emplace_back(timeCode);
            }
        }
        return result;
    }

private:
    uint32_t minFloatPrecision{0};
    std::vector<UsdUtilsTimeCodeRange> timeCodeRanges;
};

std::ostream &operator<<(std::ostream &o, const FrameNumberFormatterPtr &a) {
    // Frame numbers are zero-padded up to the field width.
    o.fill('0');
    o.width(a->width);
    o.precision(a->precision);
    return o;
}

bool framesArgs_SplitAroundFramePlaceholder(const std::string &frameFormat, std::string &prefix,
                                            std::string &placeholder, std::string &suffix) {
    //Gets the frame placeholder in a frame format string.
    //
    //This function expects the input frameFormat string to contain exactly one
    //frame placeholder. The placeholder must be composed of exactly one or two
    //groups of one or more hashes ('#'), and if there are two, they must be
    //separated by a dot ('.').
    //
    //If no such placeholder exists in the frame format string, None is returned.

    if (frameFormat.empty()) {
        return false;
    }

    auto const PLACEHOLDER_PATTERN = std::regex("([^#]*)(#+\\.?#*)([^#\n]*)");

    std::smatch m;
    if (!std::regex_search(frameFormat, m, PLACEHOLDER_PATTERN)) {
        return false;
    }

    // we're slicing in 3 parts, but match always adds the full element as the first capture.
    if (m.size() != 4) {
        return false;
    }
    prefix      = m[1];
    placeholder = m[2];
    suffix      = m[3];
    return true;
}

FrameNumberFormatterPtr framesArgs_ConvertFramePlaceholderToFloatSpec(const std::string &framePlaceholder) {
    //Converts the frame placeholder in a frame format string to a Python
    //{}-style float specifier for use with string.format().
    //
    //This function expects the input frameFormat string to contain exactly one
    //frame placeholder. The placeholder must be composed of exactly one or two
    //groups of one or more hashes ('#'), and if there are two, they must be
    //separated by a dot ('.').
    //
    //The hashes after the dot indicate the floating point precision to use in
    //the frame numbers inserted into the frame format string. If there is only
    //a single group of hashes, the precision is zero and the inserted frame
    //numbers will be integer values.
    //
    //The overall width of the frame placeholder specifies the minimum width to
    //use when inserting frame numbers into the frame format string. Formatted
    //frame numbers smaller than the minimum width will be zero-padded on the
    //left until they reach the minimum width.
    //
    //If the input frame format string does not contain exactly one frame
    //placeholder, this function will return None, indicating that this frame
    //format string cannot be used when operating with a frame range.

    // The full width of the placeholder determines the minimum field width.
    uint32_t specWidth = framePlaceholder.size();

    // The hashes after the dot, if any, determine the precision. If there are
    // none, integer frame numbers are used.
    uint32_t specPrecision = 0;
    auto parts = TfStringSplit(framePlaceholder, ".");
    if (parts.size() > 1) {
        specPrecision = parts[1].size();
    }

    return std::make_unique<FrameNumberFormatter>(specWidth, specPrecision);
}

// complexityArgs.py module
void complexityArgs_AddCmdlineArgs(CLI::App *app, Args &args, const std::string& defaultValue="low", const std::string& altHelpText="") {
    // Adds complexity-related command line arguments to argsParser.
    //
    // The resulting 'complexity' argument will be one of the standard RefinementComplexities.

    std::string helpText = altHelpText;
    if (helpText.empty()) {
        helpText = "level of refinement to use";
    }

    app->add_option("-c,--complexity", args.complexity, helpText)
        ->default_val(defaultValue)
        ->check(CLI::IsMember({"low", "medium", "high", "veryhigh"}));
}

// colorArgs.py module
void colorArgs_AddCmdlineArgs(CLI::App *app, Args &args, const std::string &defaultValue="sRGB",
                              const std::string &altHelpText="") {
    // Adds color-related command line arguments to argsParser.
    //
    // The resulting 'colorCorrectionMode' argument will be a string.

    std::string helpText = altHelpText;
    if (helpText.empty()) {
        helpText = "the color correction mode to use";
    }
    app->add_option("--color,--colorCorrectionMode", args.colorCorrectionMode, helpText)
            ->default_val(defaultValue)
            ->check(CLI::IsMember({"disabled", "sRGB", "openColorIO"}));
}

// rendererArgs.py module
std::vector<std::string> rendererArgs_GetAllPluginArguments() {
    std::vector<std::string> result;
    for (const auto& pluginId : UsdImagingGLEngine::GetRendererPlugins()) {
        result.emplace_back( UsdImagingGLEngine::GetRendererDisplayName(pluginId));
    }
    return result;
}

TfToken rendererArgs_GetPluginIdFromArgument(const std::string& argumentString) {
    // Returns plugin id, if found, for the passed in argument string.
    //
    // Valid argument strings are returned by GetAllPluginArguments().

    for (const auto& p : UsdImagingGLEngine::GetRendererPlugins())
    {
        if (argumentString == UsdImagingGLEngine::GetRendererDisplayName(p)) {
            return p;
        }
    }

    return TfToken("");
}

void rendererArgs_AddCmdlineArgs(CLI::App *app, Args &args, const std::string& altHelpText="") {
    //    Adds Hydra renderer-related command line arguments to argsParser.
    //
    //    The resulting 'rendererPlugin' argument will be a _RendererPlugin instance
    //    representing one of the available Hydra renderer plugins.

    std::string helpText = altHelpText;
    if (helpText.empty()) {
        helpText = "Hydra renderer plugin to use when generating images";
    }

    auto renderers = rendererArgs_GetAllPluginArguments();

    app->add_option("-r,--renderer", args.rendererPlugin, helpText)
            ->check(CLI::IsMember(renderers));

}

static void Configure(CLI::App *app, Args &args) {
    app->add_option(
                    "usdFilePath", args.usdFilePath, "USD file to record")
            ->required(true)
            ->option_text("...");

    app->add_option(
                    "outputImagePath", args.outputImagePath,
                    "Output image path. For frame ranges, the path must contain "
                    "exactly one frame number placeholder of the form \"###\" or "
                    "\"###.###\". Note that the number of hash marks is variable in "
                    "each group.")
            ->required(true)
            ->option_text("...");

    app->add_option(
                    "--mask", args.populationMask,
                    "Limit stage population to these prims, their descendants and "
                    "ancestors. To specify multiple paths, either use commas with no "
                    "spaces or quote the argument and separate paths by commas and/or "
                    "spaces.")
            ->option_text("'PRIMPATH[,PRIMPATH...]");

    app->add_option(
                    "--purposes", args.purposes,
                    "Specify which UsdGeomImageable purposes should be included "
                    "in the renders.  The \"default\" purpose is automatically included, "
                    "so you need specify only the *additional* purposes.  If you want "
                    "more than one extra purpose, either use commas with no spaces or "
                    "quote the argument and separate purposes by commas and/or spaces.")
            ->default_val("proxy")
            ->option_text("PURPOSE[,PURPOSE...]");

    app->add_option(
                    "--sessionLayer", args.sessionLayerPath,
                    "If specified, the stage will be opened with the "
                    "'sessionLayer' in place of the default anonymous layer.")
        ->option_text("SESSION_LAYER");

    app->add_flag(
                    "--disableGpu", args.disableGpu,
                    "Indicates if the GPU should not be used for rendering. If set "
                    "this not only restricts renderers to those which only run on "
                    "the CPU, but additionally it will prevent any tasks that require "
                    "the GPU from being invoked.");

    app->add_flag(
        "--disableCameraLight", args.cameraLightEnabled,
        "Indicates if the default camera lights should not be used for rendering."
        );

    cameraArgs_AddCmdlineArgs(app, args);
    framesArgs_AddCmdlineArgs(app, args);
    complexityArgs_AddCmdlineArgs(app, args);
    colorArgs_AddCmdlineArgs(app, args);
    rendererArgs_AddCmdlineArgs(app, args);

    app->add_option< int, int>(
                    "-w,--imageWidth", args.imageWidth,
                    "Width of the output image. The height will be computed from this "
                    "value and the camera\'s aspect ratio")
            ->default_val(960);

    app->add_option(
                    "-a,--aov", args.aovName,
                    "Specify the aov to output")
            ->default_val("color")
            ->check(CLI::IsMember({"color", "depth", "primId"}));

    app->add_flag(
                    "--enableDomeLightVisibility", args.domeLightVisibility,
                    "Show the dome light background in the rendered output.  "
                    "If this option is not included and there is a dome light in "
                    "the stage, the IBL from it will be used for lighting but not "
                    "drawn into the background.");

    app->add_option("--rp,--renderPassPrimPath", args.rpPrimPath,
                    "Specify the Render Pass Prim to use to render the given "
                    "usdFile. "
                    "Note that if a renderSettingsPrimPath has been specified in the "
                    "stage metadata, using this argument will override that opinion. "
                    "Furthermore any properties authored on the RenderSettings will "
                    "override other arguments (imageWidth, camera, outputImagePath)");

    app->add_option(
                    "--rs,--renderSettingsPrimPath", args.rsPrimPath,
                    "Specify the Render Settings Prim to use to render the given usdFile. "
                    "Note that if a renderSettingsPrimPath has been specified in the "
                    "stage metadata, using this argument will override that opinion. "
                    "Furthermore any properties authored on the RenderSettings will "
                    "override other arguments (imageWidth, camera, outputImagePath)");
}

static int32_t UsdRecord(const Args &args) {

    TfToken colorCorrectionMode(args.colorCorrectionMode);

    auto gpuEnabled = !args.disableGpu;

    // clamp the image width to a positive value
    int32_t imageWidth = std::max(args.imageWidth, 1);

    // tokenize the purposes input in to a vector of TfToken
    auto purposesStrs = TfStringTokenize(args.purposes, " ,");
    std::vector<TfToken> purposes;
    for (const auto& purposeStr : purposesStrs) {
        purposes.emplace_back(TfToken(purposeStr));
    }

    // Load the root layer.
    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(args.usdFilePath);
    if (!rootLayer) {
        std::cerr << "Could not open layer: " << args.usdFilePath << std::endl;
        return 1;
    }

    SdfLayerRefPtr sessionLayer = nullptr;
    if (!args.sessionLayerPath.empty()) {
        sessionLayer = SdfLayer::FindOrOpen(args.sessionLayerPath);
        if (!sessionLayer) {
          std::cerr << "Could not open layer: " << args.sessionLayerPath << std::endl;
          return 1;
        }
    } else {
        sessionLayer = SdfLayer::CreateAnonymous();
    }

    UsdStageRefPtr usdStage = nullptr;

    // Open the USD stage, using a population mask if paths were given.
    if (!args.populationMask.empty()) {
        auto populationMaskPaths = TfStringTokenize(args.populationMask, " ,");

        auto populationMask = UsdStagePopulationMask();
        for (const auto& maskPath : populationMaskPaths) {
            populationMask.Add(SdfPath(maskPath));
        }

        usdStage = UsdStage::OpenMasked(rootLayer, sessionLayer, populationMask);
    } else {
        usdStage = UsdStage::Open(rootLayer, sessionLayer);
    }

    if (!usdStage) {
        std::cerr << "Could not open USD stage: " << args.usdFilePath << std::endl;
        return 1;
    }

    std::vector<UsdTimeCode> frames;

    std::string outputImagePathPrefix = args.outputImagePath;
    std::string framePlaceholder = "";
    std::string outputImagePathSuffix = "";
    framesArgs_SplitAroundFramePlaceholder(args.outputImagePath, outputImagePathPrefix, framePlaceholder, outputImagePathSuffix);

    FrameNumberFormatterPtr frameNumberFormater = nullptr;
    if (!args.framesStr.empty()) {

      if (framePlaceholder.empty()) {
        std::cerr << "'outputImagePath' must contain exactly one frame number "
                     "placeholder of the form '###' or '###.###'. Note that "
                     "the number of hash marks is variable in each group." << std::endl;
        return 1;
      }

      frameNumberFormater = framesArgs_ConvertFramePlaceholderToFloatSpec(framePlaceholder);

      auto frameSpec = FrameSpecIterator(args.framesStr);
      frames = frameSpec.getTimeCodes();

      auto placeholderPrecision = framesArgs_getFloatStringPrecision(framePlaceholder);
      auto minFloatPrecision = frameSpec.getMinFloatPrecision();

      if (placeholderPrecision < minFloatPrecision) {
        std::cerr << "The given FrameSpecs require a minimum floating point precision of " << minFloatPrecision
                  << ", but the frame placeholder in 'outputImagePath' only specified a precision of "
                  << placeholderPrecision << " (" << framePlaceholder << "). The precision of the frame "
                                                                         "placeholder must be equal to or greater than " << minFloatPrecision << "." << std::endl;
        return 1;
      }

    } else {
      if (!framePlaceholder.empty()) {
        std::cerr << "'outputImagePath' cannot contain a frame number placeholder "
                     "when not operating on a frame range." << std::endl;
        return 1;
      }

      if (args.defaultTime) {
        frames = {UsdTimeCode::Default()};
      } else {
        frames = {usdStage->GetStartTimeCode()};
      }
    }

    // Get the RenderSettings Prim Path from the stage metadata if not specified.
    std::string rsPrimPath = args.rsPrimPath;
    // Get the RenderSettings Prim Path.
    // It may be specified directly (--renderSettingsPrimPath),
    // via a render pass (--renderPassPrimPath),
    // or by stage metadata (renderSettingsPrimPath).
    if (!rsPrimPath.empty() && !args.rpPrimPath.empty()) {
        std::cerr << "Cannot specify both --renderSettingsPrimPath and --renderPassPrimPath" << std::endl;
        return 1;
    }

    if (!args.rpPrimPath.empty()) {
        // A pass was specified, so next we get the associated settings prim.
        auto renderPass = UsdRenderPass(usdStage->GetPrimAtPath(SdfPath(args.rpPrimPath)));
        if (!renderPass) {
            std::cerr << "Unknown render pass <"<< args.rpPrimPath << ">" << std::endl;
            return 1;
        }

        SdfPathVector sourceRelTargets;
        if (!renderPass.GetRenderSourceRel().GetTargets(&sourceRelTargets)) {
            std::cerr << "Render source not authored on " << args.rpPrimPath << std::endl;
            return 1;
        }

        rsPrimPath = sourceRelTargets[0].GetAsString();

        if (sourceRelTargets.size() > 1) {
          TF_WARN("Render pass <"+args.rpPrimPath+"> has multiple targets; using <"+args.rsPrimPath+">");
        }
    }

    if (rsPrimPath.empty()) {
        usdStage->GetMetadata<std::string>(TfToken("renderSettingsPrimPath"), &rsPrimPath);
    }

    // Get the camera at the given path (or with the given name).
    auto usdCamera = UsdAppUtilsGetCameraAtPath(usdStage, cameraArgs_GetCameraSdfPath(args.camera));

    // NOTE - this isn't required when we use Metal, but if we want to pass this code back to pixar we may need to reintroduce this
    // and test on other platforms.
#if PXR_USE_GLFW
    GLFWOpenGLContextPtr glContext = nullptr;
    if (gpuEnabled) {
        glContext = GLFWOpenGLContext::create(args.imageWidth, args.imageWidth);
        if (!glContext)
            return -1;
    }
#endif

    TfToken rendererPluginId = rendererArgs_GetPluginIdFromArgument(args.rendererPlugin);

    // This is the only the necessary part of RefinementComplexities python class needed for usdrecord.
    static const std::unordered_map<std::string, float> complexities {
            {"low", 1.0},
            {"medium", 1.1},
            {"high", 1.2},
            {"veryhigh", 1.3}};

    float complexity = complexities.at(args.complexity);

    std::unique_ptr<UsdAppUtilsFrameRecorder> frameRecorder = std::make_unique<UsdAppUtilsFrameRecorder>(rendererPluginId, gpuEnabled);

    if (!rsPrimPath.empty()) {
        frameRecorder->SetActiveRenderSettingsPrimPath(SdfPath(rsPrimPath));
    }

    if (!args.rpPrimPath.empty()) {
        frameRecorder->SetActiveRenderPassPrimPath(SdfPath(args.rpPrimPath));
    }

    frameRecorder->SetImageWidth(imageWidth);
    frameRecorder->SetComplexity(complexity);
    frameRecorder->SetCameraLightEnabled(args.cameraLightEnabled);
    frameRecorder->SetColorCorrectionMode(colorCorrectionMode);
    frameRecorder->SetIncludedPurposes(purposes);
    frameRecorder->SetDomeLightVisibility(args.domeLightVisibility);

    for (const auto &timeCode: frames) {
        std::cout << "Recording time code: " << timeCode << std::endl;

        std::string outputImagePath = outputImagePathPrefix;
        if (frameNumberFormater != nullptr) {
            // if we have a frame number formatter - then that means we successfully parsed a frame number place holder
            // otherwise the entire original filename is in the prefix string.
            std::stringstream ss;
            ss << frameNumberFormater;
            ss << timeCode << "." << outputImagePathSuffix;
            outputImagePath += ss.str();
        }

        try {
            frameRecorder->Record(usdStage, usdCamera, timeCode, outputImagePath);
        } catch (const TfBaseException& e) {
            std::cerr << "Recording aborted due to the following failure at time code " << timeCode << ": "
                      << e.what() << std::endl;
            return 1;
        }
    }

    // Release our reference to the frame recorder so it can be deleted before other resources are freed
    frameRecorder = nullptr;

    return 0;
}

int
main(int argc, char const *argv[]) {
    CLI::App app(
            "Generates images from a USD file.", "usdrecord");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);
    return UsdRecord(args);
}
