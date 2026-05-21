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
#include "pxr/usd/plugin/usdPmc/usdPmcEncoder.hpp"

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
}

// Encode USD stage using PMC compression
static int UsdCrush(const Args &args) {
    UsdPmcMeshEncoder pmcEncoder;
    int exitCode = 0;

    if (!pmcEncoder.EncodeStage(args.inputFile, args.outputFile)) {
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
