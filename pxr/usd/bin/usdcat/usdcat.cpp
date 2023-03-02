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

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usdUtils/authoring.h"
#include "pxr/usd/usdUtils/flattenLayerStack.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

struct Args {
    std::vector<std::string> inputFiles;
    std::string output;
    std::string usdFormat;
    std::string mask;
    bool loadOnly = false;
    bool flatten = false;
    bool flattenLayerStack = false;
    bool skipSourceFileComment = false;
    bool layerMetadata = false;
};

static void Configure(CLI::App *app, Args &args) {
    app->add_option(
        "inputFiles", args.inputFiles, "A list of input files")
        ->required(true)
        ->option_text("...");

    app->add_option(
        "-o,--out", args.output, 
        "Write a single input file to this output file instead of stdout.")
        ->option_text("file");

    app->add_option(
        "--usdFormat", args.usdFormat,
        "Use this underlying file format for output files\n"
        "with the extension 'usd'. For example, passing\n"
        "'-o output.usd --usdformat usda' will create\n"
        "output.usd as a text file. The USD_DEFAULT_FILE_FORMAT\n"
        "environment variable is another way to achieve this.")
        ->check(CLI::IsMember({"usda", "usdc"}, CLI::ignore_case))
        ->option_text("usda|usdc");

    app->add_flag(
        "-l,--loadOnly", args.loadOnly,
        "Attempt to load the specified input files and report 'OK'\n"
        "or 'ERR' for each one. After all files are processed, this\n"
        "script will exit with a non-zero exit code if any files\n"
        "failed to load."
    );

    app->add_flag(
        "-f,--flatten", args.flatten,
        "Compose stages with the input files as root layers\n"
        "and write their flattened content.");

    app->add_flag(
        "--flattenLayerStack", args.flattenLayerStack,
        "Flatten the layer stack with the given root layer, and\n"
        "write out the results. Unlike --flatten, this does not flatten\n"
        "composition arcs (such as references).");

    app->add_flag(
        "--skipSourceFileComment", args.skipSourceFileComment,
        "If --flatten is specified, skip adding a comment regarding the\n"
        "source of the flattened layer in the documentation field of the\n"
        "output layer.");

    app->add_option(
        "--mask", args.mask,
        "Limit stage population to these prims, their descendants and\n"
        "ancestors. To specify multiple paths, either use commas with no\n"
        "spaces, or quote the argument and separate paths by commas\n"
        "and/or spaces. Requires --flatten.")
        ->option_text("PRIMPATH[,PRIMPATH...]");

    app->add_flag(
        "--layerMetadata", args.layerMetadata,
        "Load only layer metadata in the USD file.\n"
        "This option cannot be combined with either --flatten or\n"
        "--flattenLayerStack.");
}

static void Quarantine(const std::string &filepath) {
    if (!TfPathExists(filepath)) {
        return;
    }

    const std::string newName = filepath + ".quarantine";
    if (std::rename(filepath.c_str(), newName.c_str()) != 0) {
        std::cerr << "Failed to rename possibly corrupt output file from "
                  << filepath << " to " << newName << " : "
                  << strerror(errno) << "\n";
        return;
    }

    std::cerr << "Possibly corrupt output file renamed to " << newName << "\n";
}

static int UsdCat(const Args &args) {
    // If --out was specified, it must either not exist or must be writable, the
    // extension must correspond to a known Sdf file format, and we must have
    // exactly one input file.
    if (!args.output.empty()) {
        if (TfPathExists(args.output) && !TfIsWritable(args.output)) {
            std::cerr << "error: no write permission for existing output file "
                      << std::quoted(args.output, '\'') << "\n";
            return 1;
        }

        if (args.inputFiles.size() != 1) {
            std::cerr << "error: must supply exactly one input file "
                      << "when writing to an output file.\n";
            return 1;
        }

        const std::string ext = TfGetExtension(args.output);
        if (!args.usdFormat.empty() && ext != "usd") {
            std::cerr << "error: use of --usdFormat requires output "
                      << "file end with '.usd' extension.\n";
            return 1;
        }

        if (!SdfFileFormat::FindByExtension(ext)) {
            std::cerr << "error: unknown output file extension.\n";
            return 1;
        }
    } 
    // If --out was not specified, then --usdFormat must be unspecified or must
    // be 'usda'.
    else if (!args.usdFormat.empty() && args.usdFormat != "usda") {
        std::cerr << "error: --usdFormat must be unspecified or 'usda' if "
                  << "writing to stdout; specify an output file with -o/--out "
                  << "to write other formats.\n";
        return 1;
    }

    if (!args.mask.empty() && !args.flatten) {
        // You can only mask a stage, not a layer.
        std::cerr << "error: --mask requires --flatten\n";
        return 1;
    }

    if (args.layerMetadata && (args.flatten || args.flattenLayerStack)) {
        // Cannot parse only metadata when flattening.
        std::cerr << "error: --layerMetadata cannot be used together "
                  << "with " 
                  << (args.flatten ? "--flatten" : "--flattenLayerStack")
                  << "\n";
        return 1;
    }

    int exitCode = 0;

    std::map<std::string, std::string> formatArgs;
    if (!args.usdFormat.empty()) {
        formatArgs[UsdUsdFileFormatTokens->FormatArg] = args.usdFormat;
    }

    for (auto &input: args.inputFiles) {
        SdfLayerRefPtr layer;
        UsdStageRefPtr stage;

        // Capture errors that are emitted so we can do error handling below.
        TfErrorMark errMark;

        // Either open a layer or compose a stage, depending on whether or not
        // --flatten was specified.
        if (args.flatten) {
            if (args.mask.empty()) {
                stage = UsdStage::Open(input);
            } else {
                // Mask can be provided as a comma or space delimited string
                auto mask = UsdStagePopulationMask();
                for (const auto &path : TfStringTokenize(args.mask, ", ")) {
                    mask.Add(SdfPath(path));
                }
                stage = UsdStage::OpenMasked(input, mask);
            }
        } else if (args.flattenLayerStack) {
            stage = UsdStage::Open(input, UsdStage::LoadNone);
            layer = UsdUtilsFlattenLayerStack(stage);
        } else if (args.layerMetadata) {
            auto srcLayer = SdfLayer::OpenAsAnonymous(
                input, /* metadataOnly = */ true);
            // Not all file format plugins support metadata-only parsing.
            // Create a new anonymous layer and copy just the layer metadata.
            layer = SdfLayer::CreateAnonymous(".usda");
            UsdUtilsCopyLayerMetadata(srcLayer, layer);
        } else {
            layer = SdfLayer::FindOrOpen(input);
        }

        if (!layer && !stage) {
            // If we failed to open a layer or stage, generate a generic
            // error message if one hasn't already been emitted above.
            if (errMark.IsClean()) {
                TF_RUNTIME_ERROR("Could not open layer");
            }
        }

        if (errMark.IsClean()) {
            if (args.loadOnly) {
                std::cout << "OK  " << input << "\n";
                continue;
            }
        }
        else {
            if (args.loadOnly) {
                std::cout << "ERR " << input << "\n";
                for (const auto& err : errMark) {
                    std::cout << '\t' << err.GetCommentary() << "\n";
                }
            }
            else {
                std::cerr << "Failed to open " << std::quoted(input) << " - ";
                for (const auto& err : errMark) {
                    std::cerr << err.GetCommentary() << "\n";
                }
            }

            errMark.Clear();
            exitCode = 1;
            continue;
        }

        // Write to either stdout or the specified output file
        if (!args.output.empty()) {
            if (stage) {
                stage->Export(
                    args.output, !args.skipSourceFileComment, formatArgs);
            }
            else if (layer) {
                layer->Export(args.output, std::string(), formatArgs);
            }

            if (!errMark.IsClean()) {
                exitCode = 1;

                // Let the user know an error occurred.
                std::cerr << "Error exporting " << std::quoted(input) << " to "
                          << std::quoted(args.output) << " - ";
                for (const auto& err : errMark) {
                    std::cerr << err.GetCommentary() << "\n";
                }

                // If the output file exists, let's try to rename it with
                // '.quarantine' appended and let the user know.
                Quarantine(args.output);
            }
        }
        else {
            std::string usdString;
            if (stage) {
                stage->ExportToString(&usdString);
            } else {
                layer->ExportToString(&usdString);
            }

            if (errMark.IsClean()) {
                std::cout << usdString;
            }
            else {
                exitCode = 1;

                std::cerr << "Error writing " << std::quoted(input) 
                          << " to stdout - ";
                for (const auto& err : errMark) {
                    std::cerr << err.GetCommentary() << "\n";
                }
            }
        }
    }

    return exitCode;
}

int
main(int argc, char const *argv[]) {
    CLI::App app(
        "Write usd file(s) either as text to stdout or to a specified "
        "output file.", "usdcat");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);
    return UsdCat(args);
}
