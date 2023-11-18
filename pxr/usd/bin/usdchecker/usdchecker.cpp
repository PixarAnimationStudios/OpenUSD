//
// Copyright 2023 Apple
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
#include "pxr/usd/usdUtils/complianceChecker.h"

#include "pxr/base/tf/pxrCLI11/CLI11.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

struct Args {
    std::string inputFile;
    std::string outFile;
    bool skipVariants = false;
    bool rootPackagesOnly = false;
    bool noAssetChecks = false;
    bool arkit = false;
    bool dumpRules = false;
    bool verbose = false;
    bool strict = false;
};

static void Configure(CLI::App *app, Args &args) {
    app->add_option(
                    "inputFile", args.inputFile, "Name of the input file to inspect.")
            ->option_text("...");
    app->add_option("-o,--out", args.outFile,
                    "The file to which all the failed "
                    "checks are output. If unspecified, the failed checks "
                    "are output to stdout; if \"stderr\", terminal coloring "
                    "will be surpressed.");
    app->add_flag(
            "-s,--skipVariants", args.skipVariants,
            "If specified, only the prims"
            "that are present in the default (i.e. selected) "
            "variants are checked. When this option is not "
            "specified, prims in all possible combinations of "
            "variant selections are checked.");
    app->add_flag("-p,--rootPackageOnly", args.rootPackagesOnly,
                  "Check only the specified "
                  "package. Nested packages, dependencies and their "
                  "contents are not validated.");
    app->add_flag("--noAssetChecks", args.noAssetChecks,
                  "If specified, do NOT perform "
                  "extra checks to help ensure the stage or "
                  "package can be easily and safely referenced into "
                  "aggregate stages.");
    app->add_flag("--arkit", args.arkit,
                  "Check if the given USD stage is compatible with "
                  "ARKit's initial implementation of usdz. These assets "
                  "operate under greater constraints that usdz files for "
                  "more general in-house uses, and this option attempts "
                  "to ensure that these constraints are met.");
    app->add_flag("-d,--dumpRules", args.dumpRules,
                  "Dump the enumerated set of "
                  "rules being checked for the given set of options.");
    app->add_flag("-v,--verbose", args.verbose,
                  "Enable verbose output mode.");
    app->add_flag("-t,--strict", args.strict,
                  "Return failure code even if only warnings are "
                  "issued, for stricter compliance.");
}

int
main(int argc, char const *argv[]) {
    CLI::App app("Utility for checking the "
                 "compliance of a given USD stage or a USDZ package.  Only the first sample"
                 "of any relevant time-sampled attribute is checked, currently.  General"
                 "USD checks are always performed, and more restrictive checks targeted at"
                 "distributable consumer content are also applied when the \"--arkit\" option"
                 "is specified.", "usdchecker");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv)

    if (args.inputFile.empty() && !args.dumpRules) {
        std::cerr << "Either an inputFile or the --dumpRules option must be specified.";
        return -1;
    }

    UsdUtilsComplianceChecker checker(args.arkit, false, args.rootPackagesOnly,
                                      args.skipVariants, args.verbose, !args.noAssetChecks);

    if (args.dumpRules) {
        checker.DumpRules();
        if (args.inputFile.empty()) {
            return 0;
        }
    }

    checker.CheckCompliance(args.inputFile);

    // Setup output streams
    std::streambuf * buf;
    std::ofstream of;
    bool isTerm = args.outFile.empty();

    if(!args.outFile.empty()) {
        of.open(args.outFile);
        buf = of.rdbuf();
    } else {
        buf = std::cout.rdbuf();
    }

    std::ostream out(buf);

    bool fail = false;
    bool failStrict = false;
    bool hasWarnings = false;

    const std::string TermWarn = "\033[93m";
    const std::string TermFail = "\033[91m";
    const std::string TermEnd = "\033[0m";

    for (const auto& warning: checker.GetWarnings()) {
        failStrict = true;
        hasWarnings = true;
        if (isTerm) out << TermWarn;
        out << warning;
        if (isTerm) out << TermEnd;
        out << std::endl;
    }

    for (const auto& error: checker.GetErrors()) {
        failStrict = true;
        fail = true;
        if (isTerm) out << TermFail;
        out << error;
        if (isTerm) out << TermEnd;
        out << std::endl;
    }

    for (const auto& failedChecks: checker.GetFailedChecks()) {
        failStrict = true;
        fail = true;
        if (isTerm) out << TermFail;
        out << failedChecks;
        if (isTerm) out << TermEnd;
        out << std::endl;
    }

    if (fail || (args.strict && failStrict)) {
        std::cout << "Failed!" << std::endl;
        return 1;
    }

    if (hasWarnings) {
        std::cout << "Success with warnings..." << std::endl;
    } else {
        std::cout << "Success!" << std::endl;
    }

    return 0;
}