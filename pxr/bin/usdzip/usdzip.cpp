//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/zipFile.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usdValidation/usdValidation/context.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validatorTokens.h"
#include "pxr/usdValidation/usdUtilsValidators/validatorTokens.h"


PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

struct Args {
    std::string usdzFile;
    std::vector<std::string> inputFiles = {};
    bool recurse = false;
    std::string asset;
    bool checkCompliance = false;
    std::string listTarget;
    std::string dumpTarget;
    bool verbose = false;
};

void 
Configure(
    CLI::App *app, 
    Args &args) 
{
    app->add_option("usdzFile", args.usdzFile,
                    "Name of the .usdz file to create or to inspect the "
                    "contents of."
    );
    app->add_option("inputFiles", args.inputFiles,
                    "Files to include in the .usdz files"
    );
    app->add_flag("-r,--recurse", args.recurse,
                  "If specified, files in sub-directories are recursively "
                  "added to the package"
    );
    app->add_option("-a,--asset", args.asset,
                    "Resolvable asset path pointing to the root layer"
                    "of the asset to be isolated and copied into the package."
    );
    app->add_flag("-c,--checkCompliance", args.checkCompliance,
                  "Perform compliance checking of the input files."
                  "If the input asset or \"root\" layer fails any of the "
                  "compliance checks, the package is not created and the "
                  "program fails."
    );
    app->add_option("-l,--list", args.listTarget,
                    "List contents of the specified usdz file. If "
                    "a file-path argument is provided, the list is output "
                    "to a file at the given path. If no argument is "
                    "provided or if '-' is specified as the argument, the "
                    "list is output to stdout"
    );
    app->add_option("-d,--dump", args.dumpTarget,
                    "Dump contents of the specified usdz file. If "
                    "a file-path argument is provided, the contents are "
                    "output to a file at the given path. If no argument is "
                    "provided or if '-' is specified as the argument, the "
                    "contents are output to stdout."
    );
    app->add_flag("-v,--verbose", args.verbose,
                  "Enable verbose mode, which causes messages "
                  "regarding files being added to the package to be"
                  " output to stdout");
}

static void
_PrintMessage(
    std::ostream &output, 
    const std::string &msg,
    char const *color = "") 
{
    // use ArchFileIsaTTY to check if the output stream is a terminal
    // and if so, color the message. color for cout and cerr.
    if ((color && color[0] != '\0') &&
        ((&output == &std::cout && ArchFileIsaTTY(fileno(stdout))) ||
         (&output == &std::cerr && ArchFileIsaTTY(fileno(stderr))))) {
        output << color << msg << "\033[0m\n";
    } else {
        output << msg << '\n';
    }
};

// Ported over from _ReportValidationErrors in usdchecker.cpp
bool 
CheckCompliance(
    const std::string &rootLayer,
    std::ostream &output)
{
    const char *_ErrorColor = "\033[91m";
    const char *_WarningColor = "\033[93m";
    const char *_InfoColor = "\033[94m";
    const char *_SuccessColor = "\033[92m";

    std::vector<const UsdValidationValidator *> validators = 
        UsdValidationRegistry::GetInstance().GetOrLoadAllValidators();
    UsdValidationContext ctx(validators);

    auto layer = SdfLayer::FindOrOpen(rootLayer);

    if (!layer) {
        return false;
    }

    UsdValidationErrorVector errors = ctx.Validate(layer);
    if (!errors.size()) {
        return true;
    }

    bool reportFailure = false;
    bool reportWarning = false;

    for (const UsdValidationError &error: errors) {
        switch (error.GetType()) {
            case UsdValidationErrorType::Error:
                reportFailure = true;
                _PrintMessage(output, error.GetErrorAsString(), _ErrorColor);
                break;
            case UsdValidationErrorType::Warn:
                reportWarning = true;
                _PrintMessage(output, error.GetErrorAsString(), _WarningColor);
                break;
            case UsdValidationErrorType::Info:
            case UsdValidationErrorType::None:
                _PrintMessage(output, error.GetErrorAsString(), _InfoColor);
                break;
            default:
                std::cerr << "Error: Unknown error type." << '\n';
                break;
        }
    }

    if (reportFailure) {
        _PrintMessage(output, "Failed!", _ErrorColor);
        return false;
    }

    if (reportWarning) {
        _PrintMessage(output, "Success with warnings...", _WarningColor);
    } else {
        _PrintMessage(output, "Success!", _SuccessColor);
    }

    return false;
}

bool 
CreateUsdzPackage(
    const std::string &usdzFile, 
    const std::vector<std::string> &filesToAdd, 
    bool recurse,
    bool checkCompliance, 
    bool verbose) 
{
    std::vector<std::string> fileList;
    for (auto &path: filesToAdd) {
        if (TfIsDir(path)) {
            auto paths = TfListDir(path, recurse);
            std::sort(paths.begin(), paths.end());
            for (auto &p: paths) {
                if (!TfIsDir(p) && ArchGetFileLength(p.c_str()) > 0) {
                    if (verbose) {
                        std::cout << ".. adding: " << p << std::endl;
                    }
                    fileList.emplace_back(p);
                }
            }
        } else {
            if (ArchGetFileLength(path.c_str()) > 0) {
                if (verbose) {
                    std::cout << ".. adding: " << path << std::endl;
                }
                fileList.emplace_back(path);
            }
        }
    }

    if (fileList.empty()) {
        std::cerr << "No files to package" << std::endl;
        return false;
    }

    if (checkCompliance) {
        if (!CheckCompliance(fileList[0], std::cout)) {
            return false;
        }
    }

    auto writer = SdfZipFileWriter::CreateNew(usdzFile);
    for (auto &f: fileList) {
        if (writer.AddFile(f).empty()) {
            std::cerr << "Failed to add file " << f << 
                          " to package. Discarding package" << std::endl;
            writer.Discard();
            return false;
        }
    }

    return true;
}

void 
ListContents(
    const std::string &path, 
    SdfZipFile &zipfile) 
{
    const auto writeFileNames = [&zipfile](std::ostream &out) {
        std::vector<std::string> filenames(zipfile.begin(), zipfile.end());
        for (auto &fn: filenames) {
            out << fn << std::endl;
        }
    };

    if (path == "-") {
        writeFileNames(std::cout);
    } else {
        std::ofstream file(path);
        writeFileNames(file);
    }
}

void 
DumpContents(
    const std::string &path, 
    SdfZipFile &zipfile) 
{
    const auto writeContents = [&zipfile](std::ostream &out) {
        std::vector<std::string> filenames(zipfile.begin(), zipfile.end());

        out << "    Offset\t      Comp\t    Uncomp\tName" << std::endl;
        out << "    ------\t      ----\t    ------\t----" << std::endl;

        for (auto &fn: filenames) {
            // Copying the logic from Python, instead of trying to be fast
            auto info = zipfile.Find(fn).GetFileInfo();
            out << std::setw(10) << info.dataOffset << "\t"
                << std::setw(10) << info.size << "\t"
                << std::setw(10) << info.uncompressedSize << "\t"
                << fn << std::endl;
        }

        out << "----------\n" << std::to_string(filenames.size())
            << " files total" << std::endl;
    };

    if (path == "-") {
        writeContents(std::cout);
    } else {
        std::ofstream file(path);
        writeContents(file);
    }
}

int 
USDZip(
    Args &args)
{
    if (args.inputFiles.size() > 0 &&  !args.asset.empty()) {
        std::cerr << "Specify either inputFiles or an asset (via --asset)" 
                  << std::endl;
        return 1;
    }

    // If usdzFile is not specified directly as an argument, check if it has
    // been specified as an argument to the --list or --dump options. In these
    // cases, output the list or the contents to stdout.
    std::string listTarget = args.listTarget;
    std::string dumpTarget = args.dumpTarget;
    std::string usdzFile = args.usdzFile;
    if (usdzFile.empty()) {
        if (!listTarget.empty() && listTarget != "-"
            && TfGetExtension(TfStringToLower(listTarget)) == "usdz"
            && TfPathExists(listTarget)) {
            usdzFile = listTarget;
            listTarget = "-";
        } else if (!dumpTarget.empty() && dumpTarget != "-"
                   && TfGetExtension(TfStringToLower(dumpTarget)) == "usdz"
                   && TfPathExists(dumpTarget)) {
            usdzFile = dumpTarget;
            dumpTarget = "-";
        } else {
            std::cerr << "No usdz file specified." << std::endl;
            return 1;
        }
    }

    // Check if we're in package creation mode and verbose mode is enabled
    // print some useful information
    if (!args.asset.empty() || args.inputFiles.size() > 0) {
        if (TfGetExtension(TfStringToLower(usdzFile)) != "usdz") {
            usdzFile += ".usdz";
        }

        if (args.verbose) {
            if (TfPathExists(usdzFile)) {
                std::cout << "File at path " << usdzFile << " already exists. "
                        << "Overwriting file." << std::endl;
            }

            if (args.inputFiles.size() > 0) {
                std::cout << "Creating package " << usdzFile << " with files ";
                for (auto &i: args.inputFiles) {
                    std::cout << i << ", ";
                }
                std::cout << std::endl;
            }

            if (!args.asset.empty()) {
                TfDebug::SetDebugSymbolsByName(
                    "USDUTILS_CREATE_USDZ_PACKAGE", 1);
            }

            if (!args.recurse) {
                std::cout << "Not recursing into sub-directories." << std::endl;
            }
        }
    } else {
        if (args.checkCompliance) {
            std::cerr << "--checkCompliance should only be specified when "
                    << "creating a usdz package. Please use 'usdchecker' to"
                    << "check compliance of an existing .usdz file."
                    << std::endl;
            return 1;
        }
    }

    bool success = true;
    if (args.inputFiles.size() > 0) {
        success = CreateUsdzPackage(usdzFile, args.inputFiles, args.recurse,
                                    args.checkCompliance, args.verbose);
    } else if (!args.asset.empty()) {
        ArResolver &resolver = ArGetResolver();
        auto resolvedAsset = resolver.Resolve(args.asset);
        if (args.checkCompliance) {
            success = CheckCompliance(resolvedAsset, std::cout);
        }

        auto context = resolver.CreateDefaultContextForAsset(resolvedAsset);
        auto binder = ArResolverContextBinder(context);
        // Create the package only if the compliance check was passed.
        success = success && UsdUtilsCreateNewUsdzPackage(
            SdfAssetPath(args.asset), args.usdzFile);
    }

    if (!success) {
        std::cerr << "Failed to author USDZ file" << std::endl;
        return 1;
    }

    if (!listTarget.empty() || !dumpTarget.empty()) {
        if (TfPathExists(usdzFile)) {
            auto zipfile = SdfZipFile::Open(usdzFile);
            if (zipfile) {
                if (!dumpTarget.empty()) {
                    if (dumpTarget == usdzFile) {
                        std::cerr << "The file into which the contents will be "
                                     "dumped " << usdzFile << " must be "
                                     "different from the file itself." 
                                  << std::endl;
                        return 1;
                    }
                    DumpContents(dumpTarget, zipfile);
                }
                if (!listTarget.empty()) {
                    if (listTarget == usdzFile) {
                        std::cerr << "The file into which the contents will be "
                                     "listed " << usdzFile << " must be "
                                     "different from the file itself." 
                                  << std::endl;
                        return 1;
                    }
                    ListContents(listTarget, zipfile);
                }
            } else {
                std::cerr << "Failed to open usdz file at path " << usdzFile 
                          << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Can't find usdz file at path " << usdzFile 
                      << std::endl;
            return 1;
        }
    }

    return 0;
}

int
main(
    int argc, 
    char const *argv[])
{
    CLI::App app("Utility for creating a .usdz file containing USD assets and "
                 "for inspecting existing .usdz files.", "usdzip");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);
    return USDZip(args);
}
