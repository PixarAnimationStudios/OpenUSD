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

#include <iostream>
#include <regex>
#include <vector>
#include <string>

#include <pxr/pxr.h>
#include <pxr/base/tf/pxrCLI11/CLI11.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdUtils/authoring.h>
#include <pxr/usd/usdUtils/flattenLayerStack.h>
#include <pxr/usd/usd/stagePopulationMask.h>


PXR_NAMESPACE_USING_DIRECTIVE

enum UsdFormats {
    usda,
    usdc,
    none
};

struct Args {
    std::vector<std::string> inputFiles = {};
    std::string output;
    UsdFormats usdFormat;
    std::vector<std::string> mask = {};
    bool loadOnly = false;
    bool flatten = false;
    bool flattenLayerStack = false;
    bool skipSourceFileComment = false;
    bool layerMetadata = false;
};



void Configure(CLI::App *app, Args &args) {
    app->add_option("inputFiles", args.inputFiles, "A list of input files")->required(true);
    app->add_option("-o,--out", args.output, "Write a single input file to this output file instead of stdout.");

    std::map<std::string, UsdFormats> map{{"usda", UsdFormats::usda},
                                          {"usdc", UsdFormats::usdc}};
    app->add_option("--usdFormat", args.usdFormat,
                    "Use this underlying file format for output files with " \
                    "the extension 'usd'. \nFor example, passing '-o output.usd --usdformat usda' " \
                    "will create output.usd as a text file. \nThe USD_DEFAULT_FILE_FORMAT environment "\
                    "variable is another way to achieve this. Valid choices: usda or usdc."
            )
            ->default_val(UsdFormats::none)
            ->envname("USD_DEFAULT_FILE_FORMAT")
            ->transform(CLI::CheckedTransformer(map, CLI::ignore_case));
    app->add_flag("-l,--loadOnly", args.loadOnly,
                  "Attempt to load the specified input files and report 'OK' or 'ERR' for each one.\n" \
                  "After all files are processed, this script will exit with a non-zero exit code if any files failed to load."
    );
    app->add_flag("-f,--flatten", args.flatten,
                  "Compose stages with the input files as root layers "\
                    "and write their flattened content.");
    app->add_flag("--flattenLayerStack", args.flattenLayerStack,
                  "Flatten the layer stack with the given root layer, and "\
                  "write out the results. Unlike --flatten, this does not flatten "\
                  "composition arcs (such as references)."
    );
    app->add_flag("--skipSourceFileComment", args.skipSourceFileComment,
                  "If --flatten is specified, skip adding a comment regarding the source "\
                  "of the flattened layer in the documentation field of the output layer.");
    app->add_option("--mask", args.mask,
                    "Limit stage population to these prims, their descendants and acnestors.\n"\
                    "To specify multiple paths, either use commas with no spaces, or quote the argument and separate "\
                    "paths by commans and/or spaces. Requires --flatten");
    app->add_flag("--layerMetadata", args.layerMetadata,
                  "Load only layer metadata in the USD file. \n"\
                  "This option cannot be combined with either --flatten or --flattenLayerStack.");
}

int Quarantine(std::string &filepath) {
    if (!TfPathExists(filepath)) {
        return -1;
    }

    auto new_name = filepath + ".quarantine";

    try {
        if (std::rename(filepath.c_str(), new_name.c_str()) < 0) {
            std::cerr << "Failed to quarantine file: " << strerror(errno) << std::endl;
            return errno;
        }
        std::cerr << "Possibly corrupt file renamed to " << new_name << std::endl;
    } catch (std::exception &ex) {
        std::cerr << "Error: Failed to rename possibly corrupt file: " << filepath << "  " << ex.what() << std::endl;
        return 1;
    }

    return -1;
}

int UsdCat(Args &args) {
    auto mask = UsdStagePopulationMask();
    // Mask can be provided as a comma or space delimited string
    for (auto &m: args.mask) {

        std::string replaced = std::regex_replace(m, std::regex(","), " ");
        auto results = TfStringSplit(replaced, " ");


        for (auto &token: results) {
            mask.Add(SdfPath(token));
        }
    }


    // Validate the arguments
    if (!args.output.empty()) {
        if (args.inputFiles.size() != 1) {
            std::cerr << "udscat error: Must supply exactly one input file when writing to an output file." << std::endl;
            return 1;
        }

        auto ext = TfGetExtension(args.output);
        if (args.usdFormat != UsdFormats::none && ext != "usd") {
            std::cerr << "usdcat error: Specifying usdFormat requires a `.usd` extension." << std::endl;
            return 1;
        }

        if (!SdfFileFormat::FindByExtension(ext)) {
            std::cerr << "usdcat error: Unknown output file extension." << std::endl;
            return 1;
        }
    } else if (args.usdFormat == UsdFormats::usdc) {
        std::cerr << "usdcat error: Can only write 'usda' format to stdout." << std::endl;
        return 1;
    }

    if (!mask.IsEmpty() && !args.flatten) {
        std::cerr << "usdcat error: masks requires --flatten" << std::endl;
        return 1;
    }

    if (args.layerMetadata && (args.flatten || args.flattenLayerStack)) {
        std::cerr << "usdcat error: layerMetadata can not be used when flattening" << std::endl;
        return 1;
    }

    int exit_code = 0;

    std::map<std::string, std::string> formatArgs;
    if (args.usdFormat == UsdFormats::usdc) {
        formatArgs["format"] = "usdc";
    } else if (args.usdFormat == UsdFormats::usda) {
        formatArgs["format"] = "usda";
    }

    for (auto &input: args.inputFiles) {
        SdfLayerRefPtr layer = nullptr;
        UsdStageRefPtr stage = nullptr;
        try {
            if (args.flatten) {
                if (mask.IsEmpty()) {
                    stage = UsdStage::Open(input);
                } else {
                    stage = UsdStage::OpenMasked(input, mask);
                }
            } else if (args.flattenLayerStack) {
                stage = UsdStage::Open(input, UsdStage::LoadNone);
                layer = UsdUtilsFlattenLayerStack(stage);
            } else if (args.layerMetadata) {
                auto src_layer = SdfLayer::OpenAsAnonymous(input, true);
                layer = SdfLayer::CreateAnonymous(input, SdfFileFormat::FindById(TfToken("usda")));
                UsdUtilsCopyLayerMetadata(src_layer, layer);
            } else {
                layer = SdfLayer::FindOrOpen(input);
            }

            if (args.loadOnly) {
                std::cout << " OK " << input << std::endl;
                continue;
            }
        }
        catch (std::exception &ex) {
            exit_code = 1;

            if (args.loadOnly) {
                std::cout << "ERR " << input << std::endl;
                continue;
            }
            std::cerr << "usdcat error: Failed to open : " << input << " - " << ex.what() << std::endl;
        }

        if (exit_code != 0) {
            return exit_code;
        }

        if (layer == nullptr && stage == nullptr) {
            std::cerr << "usdcat error: Could not open layer: " << input << std::endl;
            return 1;
        }

        if (args.output.empty()) {
            std::string usd_string;
            if (layer == nullptr) {
                stage->ExportToString(&usd_string);
            } else {
                layer->ExportToString(&usd_string);
            }
            std::cout << usd_string << std::endl;
            continue;
        }

        try {
            if (layer == nullptr) {
                stage->Export(args.output, !args.skipSourceFileComment, formatArgs);
            } else {
                layer->Export(args.output, std::string(), formatArgs);
            }

            return 0;
        }
        catch (std::exception &ex) {
            std::cerr << "usdcat error: Failed to write file: " << args.output << "  " << ex.what() << std::endl;
            return Quarantine(input);
        }
    }

    return exit_code;
}

std::string GetVersionString() {
    return std::to_string(PXR_MAJOR_VERSION) + "." + std::to_string(PXR_MINOR_VERSION) + "." + std::to_string(PXR_PATCH_VERSION);
}

int
main(int argc, char const *argv[]) {
    auto description = "usdcat " + GetVersionString()
                       + " : Write usd file(s) either as text to stdout or to a specified output file.";
    CLI::App app(description, "usdcat");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);
    return UsdCat(args);
}
