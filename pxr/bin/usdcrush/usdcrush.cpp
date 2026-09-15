//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdPmc/usdPmcEncoder.hpp"

#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

// Command line arguments structure
struct Args {
    std::string inputFile;   // Input USD file path
    std::string outputFile;  // Output compressed USD file path
    std::map<std::string,int> quantizers;  // Number of bits for each attribute
};

// Configure command line interface options
static void Configure(CLI::App *app, Args &args) {
    app->add_option(
        "inputFile", args.inputFile, "The input USD file to process.")
        ->required(true);

    app->add_option(
        "-o,--out", args.outputFile,
        "The output USD file to write to.")
        ->required(true);

    app->add_option(
        "--mesh-qbits", args.quantizers,
        "Number of bits for named mesh attribute quantizer.\n"
        "Format: --mesh-qbits <attribute_name> <bits>\n"
        "Common attribute names: points, normals, uv, st, displayColor, velocities, accelerations\n"
        "Use '*' as attribute name to set default for all attributes.\n"
        "Default: 10 fractional bits, 14 max significant bits\n"
        "Examples:\n"
        "  --mesh-qbits '*' 12                             # 12 bits for all attributes\n"
        "  --mesh-qbits points 14                          # 14 bits for vertex positions\n"
        "  --mesh-qbits points 14 --mesh-qbits normals 10  # Mixed precision");
}

// Encode USD stage using PMC compression
static int UsdCrush(const Args &args) {
    VtDictionary quantizers;
    for (const auto& [attrname, meshQbits] : args.quantizers)
        quantizers[attrname] = VtValue(meshQbits);

    VtDictionary options;
    options["mesh-qbits"] = std::move(quantizers);

    UsdPmcMeshEncoder pmcEncoder;
    int exitCode = 0;

    if (!pmcEncoder.EncodeStage(args.inputFile, args.outputFile, options)) {
        exitCode = 1;
    }
    return exitCode;
}

// Main entry point for usdcrush command line tool
int
main(int argc, char const *argv[]) {
    CLI::App app(
        "Reduce the size of the source USD", "usdcrush");

    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);

    return UsdCrush(args);
}
