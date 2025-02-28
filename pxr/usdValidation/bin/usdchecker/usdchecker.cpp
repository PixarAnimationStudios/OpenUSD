//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usdValidation/usdValidation/context.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validatorTokens.h"
#include "pxr/usdValidation/usdUtilsValidators/validatorTokens.h"

#include "pxr/base/arch/fileSystem.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyInvoke.h"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
using namespace pxr_boost::python;
#endif

static const char *_ErrorColor = "\033[91m";
static const char *_WarningColor = "\033[93m";
static const char *_InfoColor = "\033[94m";
static const char *_SuccessColor = "\033[92m";
// Default limit set to restrict the number of variants validation calls.
static const unsigned int _DefaultVariantsValidationLimit = 1000;

using StringVector = std::vector<std::string>;

struct Args {
    std::string inputFile;
    std::string outFile = "stdout";
    StringVector variants;
    StringVector variantSets;
    bool skipVariants = false;
    bool disableVariantValidationLimit = false;
    bool rootPackageOnly = false;
    bool noAssetChecks = false;
    bool arkit = false;
    bool dumpRules = false;
    bool verbose = false;
    bool strict = false;
    bool useNewValidationFramework = false;
};

static
void 
_Configure(CLI::App* app, Args& args) {
    app->add_option(
        "inputFile", args.inputFile, 
        "Name of the input file to inspect.")
        ->type_name("FILE");
    app->add_flag(
        "-s, --skipVariants", args.skipVariants, 
        "If specified, only the prims that are present in the default (i.e. "
        "selected) variants are checked. When this option is not specified, "
        "prims in all possible combinations of variant selections are "
        "checked.");
    app->add_flag(
        "-p, --rootPackageOnly", args.rootPackageOnly, 
        "Check only the specified package. Nested packages, dependencies and "
        "their contents are not validated.");
    app->add_option(
        "-o, --out", args.outFile, 
        "The file to which all the failed checks are output. If unspecified, "
        "the failed checks are output to stdout; if \"stderr\", terminal "
        "coloring will be suppressed.")
        ->type_name("FILE");
    app->add_flag(
        "--noAssetChecks", args.noAssetChecks, 
        "If specified, do NOT perform extra checks to help ensure the stage or "
        "package can be easily and safely referenced into aggregate stages.");
    app->add_flag(
        "--arkit", args.arkit, 
        "Check if the given USD stage is compatible with RealityKit's "
        "implementation of USDZ as of 2023. These assets operate under greater "
        "constraints that usdz files for more general in-house uses, and this "
        "option attempts to ensure that these constraints are met.");
    app->add_flag(
        "-d, --dumpRules", args.dumpRules, "Dump the enumerated set of rules "
        "being checked for the given set of options.");
    app->add_flag(
        "-v, --verbose", args.verbose, 
        "Enable verbose output mode.");
    app->add_flag(
        "-t, --strict", args.strict, 
        "Return failure code even if only warnings are issued, for stricter "
        "compliance.");
    app->add_flag(
        "--useNewValidationFramework", args.useNewValidationFramework, 
        "Enable the new validation framework.");
    app->add_option(
        "--variantSets", args.variantSets,
        "List of variantSets to validate. All variants for the given "
        "variantSets are validated. This can also be used with --variants to "
        "validate the given variants in combination with variants from the "
        "explicitly specified variantSets. This option is only valid when "
        "using the new validation framework.");
    app->add_option(
        "--variants", args.variants, 
        "List of ',' separated variantSet:variant pairs to validate. Each set "
        "of variants in the list are validated separately. Example: "
        "--variants foo:bar,baz:qux will validate foo:bar and baz:qux together "
        "but --variants foo:bar --variants baz:qux will validate foo:bar and "
        "baz:qux separately. This can also be used with --variantSets to "
        "validate the given variants in combination with variants from the "
        "explicitly specified variantSets. This option is only valid when "
        "using the new validation framework.");
    app->add_flag(
        "--disableVariantValidationLimit", args.disableVariantValidationLimit,
        "Disable the limit set to restrict the number of variants validation "
        "calls. This is useful when the number of variants is large and we "
        "want to validate all possible combinations of variants. Default is to "
        "limit the number of validation calls to 1000. This option is only "
        "valid when using the new validation framework.");
}

static
bool 
_ValidateArgs(const Args& args) {
    // Check conflicting options
    if (args.rootPackageOnly && args.skipVariants) {
        std::cerr<<"Error: The options --rootPackageOnly and --skipVariants "
            "are mutually exclusive.\n";
        return false;
    }

    if (!args.dumpRules && args.inputFile.empty()) {
        std::cerr<<"Error: Either an inputFile or the --dumpRules option "
            "must be specified.\n";
        return false;
    }

    // variants option is only valid when using new validation framework.
    if (!args.variants.empty() && !args.useNewValidationFramework) {
        std::cerr<<"Error: The --variants option is only valid when using the "
            "--useNewValidationFramework option."<<"\n";
        return false;
    }

    if (!args.variantSets.empty() && !args.useNewValidationFramework) {
        std::cerr<<"Error: The --variantSets option is only valid when using "
            "the --useNewValidationFramework option."<<"\n";
        return false;
    }

    return true;
}

// Used for reporting validation errors for each combination of variant
struct _VariantInfo {
    SdfPath primPath;
    StringVector variants;
};

static
void
_PrintMessage(std::ostream& output, const std::string& msg, 
              char const* color = "") {
    // use ArchFileIsaTTY to check if the output stream is a terminal
    // and if so, color the message. color for cout and cerr.
    if ( (color && color[0] != '\0') && 
         ((&output == &std::cout && ArchFileIsaTTY(fileno(stdout))) ||
         (&output == &std::cerr && ArchFileIsaTTY(fileno(stderr)))) ){
        output<<color<<msg<<"\033[0m\n";
    } else {
        output<<msg<<'\n';
    }
};


// Report validation errors for the given errors vector.
// Used for reporting default stage as well as combination of variants.
static
bool
_ReportValidationErrors(
    const UsdValidationErrorVector& errors, std::ostream& output, bool strict, 
    const _VariantInfo& variantInfo = _VariantInfo())
{
    bool reportFailure = false;
    bool reportWarning = false;

    if (!variantInfo.primPath.IsEmpty()) {
        _PrintMessage(output, 
            TfStringPrintf(
                "\nValidation Result for Variants: %s on Prim: %s", 
                TfStringJoin(variantInfo.variants,",").c_str(), 
                variantInfo.primPath.GetText()), _InfoColor);
    } else {
        _PrintMessage(output, 
            "\nValidation Result with no explicit variants set", _InfoColor);
    }

    for (const UsdValidationError& error : errors) {
        switch(error.GetType()) {
            case UsdValidationErrorType::Error:
                reportFailure = true;
                _PrintMessage(output, error.GetErrorAsString(), _ErrorColor);
                break;
            case UsdValidationErrorType::Warn:
                if (strict) {
                    reportFailure = true;
                }
                reportWarning = true;
                _PrintMessage(output, error.GetErrorAsString(), _WarningColor);
                break;
            case UsdValidationErrorType::Info:
            case UsdValidationErrorType::None:
                _PrintMessage(output, error.GetErrorAsString(), _InfoColor);
                break;
            default:
                std::cerr<<"Error: Unknown error type."<<'\n';
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
    return true;
}

// If the validation limit is reached, skip the remaining variants. This is to 
// avoid running validation on all possible combinations of variants, which can 
// be expensive. But don't check this if variant exploration limit is explicitly
// disabled.
struct _VariantsLimitChecker
{
    _VariantsLimitChecker(
        std::ostream& output,
        const bool disableVariantValidationLimit)
        : _output(output),
          _disableVariantValidationLimit(disableVariantValidationLimit)
    {
    }

    bool CheckLimit()
    {
        bool limitReached = !_disableVariantValidationLimit && 
            _variantValidationCounter >= _DefaultVariantsValidationLimit;
        if (limitReached) {
            _PrintMessage(
                _output, 
                TfStringPrintf("Validation limit of %u variants validated "
                               "reached. Skipping remaining variants.", 
                               _DefaultVariantsValidationLimit), 
                _WarningColor);
        }
        return limitReached;
    }

    bool IncrementAndCheckLimit()
    {
        ++_variantValidationCounter;
        return CheckLimit();
    }

    void PrintCounter()
    {
        _PrintMessage(
            _output, 
            TfStringPrintf("Number of variants validated so far: %u", 
                           _variantValidationCounter), 
            _InfoColor);
    }

private:
    std::ostream &_output;
    unsigned int _variantValidationCounter = 0;
    const bool _disableVariantValidationLimit;
};

// Recursively validate all the possible combinations of variants for the given
// prim. This is done by setting the variant selection for each variantSet and
// validating the stage.
// If explicitVariantSets is provided, validate the variants in the given
// variantSets only and not all the variantSets in the prim.
// If disableVariantValidationLimit is true, validate all the possible
// combinations of variants without any limit, else limit the number of
// validation calls to _DefaultVariantsValidationLimit.
// variantValidationCounter keeps track of the number of validation calls made, 
// so far.
static
bool
_ValidatePrimVariants(
    const UsdStageRefPtr& stage, const UsdPrim& prim,
    const UsdValidationContext& ctx, std::ostream& output, bool strict,
    StringVector& variantToValidate, 
    std::unordered_set<std::string>& visitedSets,
    const StringVector& explicitVariantSets,
    _VariantsLimitChecker &variantsLimitChecker)
{
    UsdVariantSets variantSets = prim.GetVariantSets();
    const StringVector variantSetNames = (explicitVariantSets.empty()) ?
        variantSets.GetNames() : explicitVariantSets;
    bool success = true;

    // Iterate through the variant sets
    for (const auto & setName : variantSetNames) {

        if (variantsLimitChecker.CheckLimit()) {
            // reached limit, nothing to clean and stop iterating further
            break;
        }

        // Possible in case of explicit variantSets, that the variantSet is not
        // present in the prim.
        if (!variantSets.HasVariantSet(setName)) {
            continue;
        }

        // Skip if this variantSet has already been visited
        bool inserted = visitedSets.insert(setName).second;
        if (!inserted) {
            continue;
        }

        UsdVariantSet currentSet = variantSets.GetVariantSet(setName);
        // Get all variants, including the empty variant (no selection)
        const StringVector variants = [&currentSet]() {
            StringVector v = currentSet.GetVariantNames();
            v.emplace_back("");
            return v;
        }();

        for (const std::string& variant : variants) {
            if (variantsLimitChecker.CheckLimit()) {
                // reached limit, stop iterating further, nothing to clean.
                break;
            }
            // Set the current variant selection
            variantSets.SetSelection(setName, variant);
            // Helps in reporting the variant selection for the prim, in case of
            // validation errors.
            variantToValidate.push_back(setName + ":" + variant);

            // Validate this combination of variants for the prim
            UsdValidationErrorVector errors = ctx.Validate(stage);
            success &= _ReportValidationErrors(errors, output, strict, 
                                               {prim.GetPath(), 
                                               variantToValidate});

            if (variantsLimitChecker.IncrementAndCheckLimit()) {
                // reached limit, stop recursing further
                variantToValidate.pop_back();
                break;
            }

            // Recursively validate the next variantSets
            success &= _ValidatePrimVariants(stage, prim, ctx, output, strict, 
                                             variantToValidate, visitedSets,
                                             explicitVariantSets,
                                             variantsLimitChecker);

            variantToValidate.pop_back();
        }

        // Done with this currentSet, clear the variant selection on it.
        currentSet.ClearVariantSelection();

        // Remove the variantSet from the visitedSets after processing all the
        // variants in the variantSet. This is to ensure that the variantSet is
        // visited again when it is encountered in a different path.
        visitedSets.erase(setName);
    }
    return success;
}

static
bool
_ValidateVariants(const UsdStageRefPtr& stage, const UsdValidationContext& ctx,
                  std::ostream& output, const StringVector& variantPairs, 
                  const StringVector& variantSets, bool strict, 
                  bool disableVariantValidationLimit)
{
    bool success = true;
    UsdPrimRange range = stage->Traverse();
    
    _VariantsLimitChecker variantsLimitChecker(output, 
                                               disableVariantValidationLimit);
    // Use UsdEditContext to temporarily set the edit target to the session 
    // layer
    UsdEditContext editContext(stage, stage->GetSessionLayer());

    // For all prims in the stage, check if the prim has variants and if so, 
    // set each combination of variant selection on the stage's session layer.
    for (const UsdPrim& prim : range) {
        if (variantsLimitChecker.CheckLimit()) {
            // reached limit, stop iterating further
            break;
        }

        if (!prim.HasVariantSets()) {
            continue;
        }

        // Lambda to validate combination of variants in the given variantSets.
        // This is used a few times below:
        // - When no explicit variants are specified, validate all the possible
        //  combinations of variants from all variantSets on the prim.
        // - When explicit variantSets are specified, validate these explicit
        //  variantSets.
        // - When explicit variants are specified, validate the given variants
        //  in combination with variants from the explicit specified 
        //  variantSets.
        auto _ValidateVariantSets = 
            [&stage, &prim, &ctx, &output, strict, &variantsLimitChecker](
                StringVector &variantToValidate,
                const StringVector &variantSets) {
            bool success = true;
            std::unordered_set<std::string> visitedSets;
            success &= _ValidatePrimVariants(
                stage, prim, ctx, output, strict, variantToValidate, 
                visitedSets, variantSets, variantsLimitChecker);
            return success;
        };

        if (variantPairs.empty()) {
            // If no explicit variants are specified, validate all the 
            // possible combinations of variants from all variantSets on the
            // prim or from the explicitly specified variantSets.

            // variantToValidate keeps track of the variant selections, for
            // reporting.
            StringVector variantToValidate;
            variantToValidate.reserve(prim.GetVariantSets().GetNames().size());
            
            success &= _ValidateVariantSets(variantToValidate, variantSets);
        } else {
            for (const std::string& variantPairCombination : variantPairs) {
                if (variantsLimitChecker.CheckLimit()) {
                    // reached limit, stop iterating further, nothing to clean,
                    // and return the current success state.
                    break;
                }
                // Helps in determining the variantSets that are specified in 
                // the variantPairs, to ignore the same variantSet in the
                // --variantSets option, since a specific variant of this
                // variantSet is already asked to be validated.
                std::unordered_set<std::string> setsInVariantPairs;
                StringVector variantSetVariants = 
                    TfStringTokenize(variantPairCombination, ",");
                for (const std::string& variantSetVariant : variantSetVariants) {
                    const StringVector variantSetAndVariant = 
                        TfStringTokenize(variantSetVariant, ":");
                    if (variantSetAndVariant.size() == 2) {
                        // Both variantSet and variantName are provided, 
                        // set the specified variant to be validated.
                        const std::string& variantSet = variantSetAndVariant[0];
                        const std::string& variant = variantSetAndVariant[1];
                        setsInVariantPairs.insert(variantSet);
                        prim.GetVariantSets().SetSelection(variantSet, variant);
                    } else {
                        TF_WARN("Invalid variant pair: %s. Skipping", 
                                variantSetVariant.c_str());
                    }
                }
                if (variantSets.empty()) {
                    // No variantSets are specified, validate the variants
                    // explicitly Set above.
                    UsdValidationErrorVector errors = ctx.Validate(stage);
                    success &= _ReportValidationErrors(
                        errors, output, strict, {prim.GetPath(), 
                                                 variantSetVariants});
                    if (variantsLimitChecker.IncrementAndCheckLimit()) {
                        // reached limit, stop iterating further
                        break;
                    }
                } else {
                    // Validate the above variants in combination with variants 
                    // from the explicit specified variantSets.
                    
                    // We do not need to variants from variantSets for which we
                    // have explicitly set variantSet:variant pair above, if
                    // any such variantSet is found in the variantSets, skip it.
                    StringVector prunedVariantSets;
                    prunedVariantSets.reserve(variantSets.size());
                    for (const std::string& variantSet : variantSets) {
                        if (setsInVariantPairs.find(variantSet) == 
                            setsInVariantPairs.end()) {
                            prunedVariantSets.push_back(variantSet);
                        } else {
                            TF_WARN("VariantSet %s is specified in both "
                                    "--variants and --variantSets. Skipping the "
                                    "variantSet.", variantSet.c_str());
                        }
                    }
                    
                    // variantSetVariants keep tracks of variants for reporting 
                    // in combination with the variants from the explicit 
                    // variantSets.
                    success &= _ValidateVariantSets(
                        variantSetVariants, prunedVariantSets);
                }
                // Clear the variant selection after validation runs of this
                // variantPairCombination.
                for (const std::string& variantSet : setsInVariantPairs) {
                    prim.GetVariantSets().GetVariantSet(variantSet)
                        .ClearVariantSelection();
                }
            }
        }
    }
    return success;
}

static 
int 
_UsdChecker(const Args& args) 
{
    if (!_ValidateArgs(args)) {
        return 1;
    }

    if (args.useNewValidationFramework) {
        UsdValidationRegistry &validationReg = 
            UsdValidationRegistry::GetInstance();
        UsdValidationValidatorMetadataVector metadata = 
            validationReg.GetAllValidatorMetadata();
        if (!args.arkit) {
            // Remove metadata which have the UsdzValidators keyword, in its
            // keyword vector.
            metadata.erase(
                std::remove_if(
                    metadata.begin(), metadata.end(), 
                    [](const UsdValidationValidatorMetadata &meta) {
                        return std::find(
                            meta.keywords.begin(), meta.keywords.end(), 
                            UsdUtilsValidatorKeywordTokens->UsdzValidators) != 
                            meta.keywords.end();
                    }), metadata.end());
        }
        if (args.noAssetChecks) {
            // Remove metadata which have the stageMetadataChecker validator
            // name.
            metadata.erase(
                std::remove_if(
                    metadata.begin(), metadata.end(), 
                    [](const UsdValidationValidatorMetadata &meta) {
                        return meta.name == 
                            UsdValidatorNameTokens->stageMetadataChecker;
                    }), metadata.end());
        }
        if (args.rootPackageOnly) {
            // Remove UsdUtilsValidators:UsdzPackageValidator
            metadata.erase(
                std::remove_if(
                    metadata.begin(), metadata.end(), 
                    [](const UsdValidationValidatorMetadata &meta) {
                        return meta.name == 
                            UsdUtilsValidatorNameTokens->usdzPackageValidator;
                    }), metadata.end());
        } else {
            // Remove UsdUtilsValidators:RootPackageValidator
            metadata.erase(
                std::remove_if(
                    metadata.begin(), metadata.end(), 
                    [](const UsdValidationValidatorMetadata &meta) {
                        return meta.name == 
                            UsdUtilsValidatorNameTokens->rootPackageValidator;
                    }), metadata.end());
        }
        // TODO rootPackageOnly
        //   - This can be handled via a keyword. Have a UsdzPackageValidator
        //   and a UsdzRootPackageValidator, and appropriately select the
        //   validator metadata given the --rootPackageOnly flag.
        
        // Sort metadata based on the name of the validator. Helps with
        // deterministic dumping of validation rules.
        std::sort(
            metadata.begin(), metadata.end(), 
            [](const UsdValidationValidatorMetadata &a, 
               const UsdValidationValidatorMetadata &b) {
                return a.name < b.name;
            });
        
        if (args.dumpRules) {
            for (const UsdValidationValidatorMetadata &meta : metadata) {
                std::cout<<"["<<meta.name<<"]:\n";
                std::cout<<"\t"<<"Doc: "<<meta.doc<<'\n';
                if (!meta.keywords.empty()) {
                    std::cout<<"\t"<<"Keywords: "<<
                        TfStringJoin(meta.keywords.begin(),
                                     meta.keywords.end(), ", ")<<'\n';
                }
                if (!meta.schemaTypes.empty()) {
                    std::cout<<"\t"<<"SchemaTypes: "<<
                        TfStringJoin(meta.schemaTypes.begin(),
                                     meta.schemaTypes.end(), ", ")<<'\n';
                }
                std::string suiteDoc = meta.isSuite ? "True" : "False";
                std::cout<<"\t"<<"isSuite: "<<suiteDoc<<'\n';
            }
        }

        if (args.inputFile.empty()) {
            return 0;
        }
        
        UsdStageRefPtr stage = UsdStage::Open(args.inputFile);
        if (!stage) {
            std::cerr<<"Error: Failed to open stage.\n";
            return 1;
        }
        UsdValidationContext ctx(metadata);

        // Do a validation run without any variants set, to get default stage
        // validation errors.
        UsdValidationErrorVector errors = ctx.Validate(stage);

        std::ofstream outFileStream;
        std::ostream &output = [&]() -> std::ostream & {
            if (args.outFile == "stderr") {
                return std::cerr;
            }
            if (args.outFile == "stdout") {
                return std::cout;
            }
            outFileStream.open(args.outFile);
            if (!outFileStream) {
                std::cerr<<"Error: Failed to open output file "<<args.outFile
                    <<" for writing.\n";
            }
            return outFileStream;
        }();

        if (!output) {
            return 1;
        }

        bool success = _ReportValidationErrors(errors, output, args.strict);

        // If skipVariants is not set, validate the given variants.
        if (!args.skipVariants) {
            success &= _ValidateVariants(
                stage, ctx, output, args.variants, args.variantSets, 
                args.strict, args.disableVariantValidationLimit);
        }
        return success ? 0 : 1;
    }

    #ifndef PXR_PYTHON_SUPPORT_ENABLED

    std::cerr<<"usdchecker using UsdUtilsComplianceChecker requires Python "
        "support to be enabled in the build of USD. Its recommended to use "
        "--useNewValidationFramework which doesn't require any python "
        "support.\n";
    return 1;

    #else

    try {
        TfPyInitialize();
        TfPyLock gil;
        TfPyKwArg arkitArg("arkit", args.arkit);
        TfPyKwArg skipARKitRootLayerCheck("skipARKitRootLayerCheck", false);
        TfPyKwArg rootPackageOnlyArg("rootPackageOnly", args.rootPackageOnly);
        TfPyKwArg skipVariantsArg("skipVariants", args.skipVariants);
        TfPyKwArg verboseArg("verbose", args.verbose);
        TfPyKwArg assetLevelChecks("assetLevelChecks", !args.noAssetChecks);

        object checker;
        if (!TfPyInvokeAndReturn(
            "pxr.UsdUtils", "ComplianceChecker", &checker, arkitArg, 
            skipARKitRootLayerCheck, rootPackageOnlyArg, skipVariantsArg, 
            verboseArg, assetLevelChecks)) {
            std::cerr<<"Error: Failed to initialize ComplianceChecker.\n"; 
            return 1;
        }

        if (!checker) {
            std::cerr<<"Error: Failed to initialize ComplianceChecker.\n"; 
            return 1;
        }

        if (args.dumpRules) {
            checker.attr("DumpRules")();
            // If there's no input file to check, exit after dumping the rules.
            if (args.inputFile.empty()) {
                return 0;
            }
        }

        // We must have an input file to check, based on the validation above
        checker.attr("CheckCompliance")(args.inputFile);

        // Extract warnings, errors, and failed checks from 
        // ComplianceChecker.
        StringVector warnings = extract<StringVector>(
            checker.attr("GetWarnings")());
        StringVector errors = extract<StringVector>(
            checker.attr("GetErrors")());
        StringVector failedChecks = extract<StringVector>(
            checker.attr("GetFailedChecks")());

        std::ofstream outFileStream;
        std::ostream &output = [&]() -> std::ostream & {
            if (args.outFile == "stderr") {
                return std::cerr;
            }
            if (args.outFile == "stdout") {
                return std::cout;
            }
            outFileStream.open(args.outFile);
            if (!outFileStream) {
                std::cerr<<"Error: Failed to open output file "<<args.outFile
                    <<" for writing.\n";
            }
            return outFileStream;
        }();

        if (!output) {
            return 1;
        }

        for (const auto& warning : warnings) {
            _PrintMessage(output, warning, _WarningColor);
        }
        for (const auto& error : errors) {
            _PrintMessage(output, error, _ErrorColor);
        }
        for (const auto& failedCheck : failedChecks) {
            _PrintMessage(output, failedCheck, _ErrorColor);
        }

        if (args.strict && (!warnings.empty() || !errors.empty() || 
                !failedChecks.empty())) {
            _PrintMessage(output, "Failed!\n", _ErrorColor);
            return 1;
        }

        if (!errors.empty() || !failedChecks.empty()) {
            _PrintMessage(output, "Failed!\n", _ErrorColor);
            return 1;
        }

        if (!warnings.empty()) {
            _PrintMessage(output, "Success with warnings...", "\033[93m");
        } else {
            _PrintMessage(output, "Success!", _SuccessColor);
        }
    } catch (error_already_set const&) {
        PyErr_Print();
        std::cerr<<"Error: An exception occurred while running the compliance "
            "checker.\n";
        return 1;
    }
    return 0;

    #endif
}

int 
main(int argc, char const *argv[]) {
    CLI::App app(
        "Utility for checking the compliance of a given USD stage or a USDZ "
        "package.  Only the first sample of any relevant time-sampled "
        "attribute is checked, currently.  General USD checks are always "
        "performed, and more restrictive checks targeted at distributable "
        "consumer content are also applied when the \"--arkit\" option is "
        "specified. In order to use the new validation framework provide the "
        "'--useNewValidationFramework' option.");

    Args args;
    _Configure(&app, args);
    CLI11_PARSE(app, argc, argv);
    return _UsdChecker(args);
}
