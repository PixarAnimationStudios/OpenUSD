//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMedia/authorshipAPI.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"

#include <iomanip>
#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

namespace
{

struct Args {
    std::string inputPath;
    bool debug = false;
    bool unloaded = false;
    bool summary = false;
    bool layer = false;
};

void Configure(CLI::App *app, Args &args) {
    app->add_option(
        "inputPath", args.inputPath, "The input file to process")
        ->required();
    app->add_flag(
        "-d,--debug", args.debug,
        "Also examine every prim spec contributing to each prim, flagging "
        "records shadowed by an explicit apiSchemas list in a stronger layer "
        "and records applied in more than one prim spec. Slower; for "
        "debugging.");
    app->add_flag(
        "--unloaded", args.unloaded, "Do not load payloads.");
    app->add_flag(
        "-s,--summary", args.summary,
        "Prints a summary of found records, including software package and "
        "digital source type tallies.");
    app->add_flag(
        "-l,--layer", args.layer,
        "Inspects a single layer without composition. Reaches specs "
        "unreachable via composed stages, such as those inside unselected "
        "variants. Ignores --debug and --unloaded, which are about "
        "composition.");
}

// Arrays are joined so a field stays on one line.
std::string
GetDisplayValue(const VtValue &value)
{
    if (value.IsHolding<VtArray<std::string>>()) {
        const VtArray<std::string> array =
            value.UncheckedGet<VtArray<std::string>>();
        std::vector<std::string> quoted;
        quoted.reserve(array.size());
        for (const std::string &item : array) {
            quoted.push_back("\"" + item + "\"");
        }
        return "[" + TfStringJoin(quoted, ", ") + "]";
    }

    if (value.IsHolding<std::string>()) {
        return "\"" + value.UncheckedGet<std::string>() + "\"";
    }

    return TfStringify(value);
}

// Empty if \p attr has no authored value.
std::string
GetDisplayValue(const UsdAttribute &attr)
{
    VtValue value;
    if (!attr || !attr.HasAuthoredValue() || !attr.Get(&value)) {
        return std::string();
    }
    return GetDisplayValue(value);
}

VtArray<std::string>
GetStringArray(const UsdAttribute &attr)
{
    VtArray<std::string> array;
    if (attr && attr.HasAuthoredValue()) {
        attr.Get(&array);
    }
    return array;
}

// Pairs the index-matched names and values. A length mismatch means the
// record is malformed, so say so rather than mispairing.
std::string
GetDisplayInputs(const VtArray<std::string> &names,
                 const VtArray<std::string> &values)
{
    if (names.size() != values.size()) {
        return TfStringPrintf(
            "(malformed: %zu name(s) but %zu value(s)) names=%s values=%s",
            names.size(), values.size(),
            GetDisplayValue(VtValue(names)).c_str(),
            GetDisplayValue(VtValue(values)).c_str());
    }

    std::vector<std::string> pairs;
    pairs.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
        pairs.push_back(names[i] + "=\"" + values[i] + "\"");
    }
    return TfStringJoin(pairs, ", ");
}

std::string
GetDisplayInputs(const UsdMediaAuthorshipAPI &record)
{
    return GetDisplayInputs(GetStringArray(record.GetInputNamesAttr()),
                            GetStringArray(record.GetInputValuesAttr()));
}

// The authored fields of one record. Read generically from the property
// namespace so that fields added to the schema later need no code change.
std::vector<std::pair<std::string, std::string>>
GetAuthoredFields(const UsdMediaAuthorshipAPI &record)
{
    std::vector<std::pair<std::string, std::string>> fields;

    const std::string prefix =
        SdfPath::JoinIdentifier(
            UsdMediaTokens->authorship, record.GetName());
    for (const UsdProperty &property :
            record.GetPrim().GetPropertiesInNamespace(prefix)) {
        const UsdAttribute attr = property.As<UsdAttribute>();
        if (!attr) {
            continue;
        }

        const TfToken baseName(SdfPath::StripPrefixNamespace(
            attr.GetName().GetString(), prefix).first);

        // Instance names may be namespaced, so this namespace also contains the
        // properties of nested records. Skip anything that is not our own field.
        if (!UsdMediaAuthorshipAPI::IsSchemaPropertyBaseName(baseName)) {
            continue;
        }

        // The two input arrays are reported together, below.
        if (baseName == "inputNames" ||
                baseName == "inputValues") {
            continue;
        }

        const std::string value = GetDisplayValue(attr);
        if (value.empty()) {
            continue;
        }
        fields.emplace_back(baseName.GetString(), value);
    }

    if (record.GetInputNamesAttr().HasAuthoredValue() ||
            record.GetInputValuesAttr().HasAuthoredValue()) {
        fields.emplace_back("inputs", GetDisplayInputs(record));
    }

    std::sort(fields.begin(), fields.end());

    return fields;
}

using RecordId = std::pair<SdfPath, TfToken>;

RecordId
GetRecordId(const UsdMediaAuthorshipAPI &record)
{
    return RecordId(record.GetPrim().GetPath(), record.GetName());
}

void
PrintRecords(const std::vector<UsdMediaAuthorshipAPI> &records,
             const std::set<RecordId> &duplicates)
{
    SdfPath currentPrimPath;

    for (const UsdMediaAuthorshipAPI &record : records) {
        const SdfPath primPath = record.GetPrim().GetPath();

        // Records arrive sorted by prim path, so group them under one heading.
        if (primPath != currentPrimPath) {
            if (!currentPrimPath.IsEmpty()) {
                std::cout << "\n";
            }
            currentPrimPath = primPath;
            std::cout << primPath.GetString();
            if (record.GetPrim().IsInPrototype()) {
                std::cout << "  (in prototype: shared by every instance)";
            }
            if (!record.GetPrim().IsActive()) {
                std::cout << "  (inactive)";
            }
            std::cout << "\n";
        }

        std::cout << "  " << record.GetName().GetString();
        if (!record.GetPrim().HasAPI<UsdMediaAuthorshipAPI>(record.GetName())) {
            std::cout << "  (shadowed: not applied on the composed prim)";
        }
        if (duplicates.count(GetRecordId(record))) {
            std::cout << "  (applied in more than one prim spec; only the "
                         "strongest opinion for each field survives "
                         "composition)";
        }
        std::cout << "\n";

        const std::vector<std::pair<std::string, std::string>> fields =
            GetAuthoredFields(record);
        if (fields.empty()) {
            std::cout << "      (no authored fields)\n";
            continue;
        }

        size_t width = 0;
        for (const auto &field : fields) {
            width = std::max(width, field.first.size());
        }
        for (const auto &field : fields) {
            std::cout << "      " << std::left << std::setw(int(width))
                      << field.first << " = " << field.second << "\n";
        }
    }
}

void
PrintSummary(const std::vector<UsdMediaAuthorshipAPI> &records)
{
    // Software packages are tallied per version, since two versions of one
    // tool can produce very different results. Keys are lowercased because
    // softwarePackage identifiers compare case-insensitively, but the first
    // spelling seen is what gets displayed.
    std::map<std::string, std::pair<std::string, size_t>> softwarePackages;
    std::map<std::string, size_t> sourceTypes;

    for (const UsdMediaAuthorshipAPI &record : records) {
        std::string softwarePackage;
        if (!record.GetSoftwarePackageAttr().Get(&softwarePackage) ||
                softwarePackage.empty()) {
            softwarePackage = "(unspecified)";
        }
        std::string version;
        if (!record.GetSoftwareVersionAttr().Get(&version) || version.empty()) {
            version = "(unspecified)";
        }

        const std::string display = softwarePackage + " " + version;
        auto &entry = softwarePackages[TfStringToLowerAscii(display)];
        if (entry.second == 0) {
            entry.first = display;
        }
        ++entry.second;

        std::string sourceType;
        if (record.GetDigitalSourceTypeAttr().Get(&sourceType) &&
                !sourceType.empty()) {
            ++sourceTypes[sourceType];
        } else {
            ++sourceTypes["(unspecified)"];
        }
    }

    std::cout << records.size() << " record(s)\n";

    std::cout << "\nSoftware packages:\n";
    for (const auto &entry : softwarePackages) {
        std::cout << "  " << entry.second.second << "  " << entry.second.first
                  << "\n";
    }

    std::cout << "\nDigital source types:\n";
    for (const auto &entry : sourceTypes) {
        std::cout << "  " << entry.second << "  " << entry.first << "\n";
    }
}

// Layer records have no composed prim, so fields are read from the layer's
// attribute specs. Asking the schema for the field names keeps this working as
// fields are added.
std::vector<std::pair<std::string, std::string>>
GetAuthoredFieldsInLayer(const SdfLayerHandle &layer,
                         const SdfPath &primPath,
                         const TfToken &instanceName)
{
    std::vector<std::pair<std::string, std::string>> fields;
    VtArray<std::string> inputNames, inputValues;

    for (const TfToken &propertyName :
            UsdMediaAuthorshipAPI::GetSchemaAttributeNames(false,
                                                           instanceName)) {
        const SdfAttributeSpecHandle attrSpec =
            layer->GetAttributeAtPath(primPath.AppendProperty(propertyName));
        if (!attrSpec || !attrSpec->HasDefaultValue()) {
            continue;
        }

        const VtValue value = attrSpec->GetDefaultValue();
        const std::string baseName =
            SdfPath::StripPrefixNamespace(
                propertyName.GetString(),
                SdfPath::JoinIdentifier(
                    UsdMediaTokens->authorship, instanceName)).first;

        if (baseName == "inputNames") {
            if (value.IsHolding<VtArray<std::string>>()) {
                inputNames = value.UncheckedGet<VtArray<std::string>>();
            }
        } else if (baseName == "inputValues") {
            if (value.IsHolding<VtArray<std::string>>()) {
                inputValues = value.UncheckedGet<VtArray<std::string>>();
            }
        } else {
            fields.emplace_back(baseName, GetDisplayValue(value));
        }
    }

    if (!inputNames.empty() || !inputValues.empty()) {
        fields.emplace_back("inputs",
                            GetDisplayInputs(inputNames, inputValues));
    }

    std::sort(fields.begin(), fields.end());

    return fields;
}

void
PrintLayerRecords(const SdfLayerHandle &layer, const SdfPathVector &paths)
{
    SdfPath currentPrimPath;

    for (const SdfPath &path : paths) {
        // Records inside a variant sit at a prim variant selection path, whose
        // properties GetPrimPath() would not find.
        const SdfPath primPath = path.GetPrimOrPrimVariantSelectionPath();
        TfToken instanceName;
        if (!UsdMediaAuthorshipAPI::IsAuthorshipAPIPath(path, &instanceName)) {
            continue;
        }

        if (primPath != currentPrimPath) {
            if (!currentPrimPath.IsEmpty()) {
                std::cout << "\n";
            }
            currentPrimPath = primPath;
            std::cout << primPath.GetString() << "\n";
        }

        std::cout << "  " << instanceName.GetString() << "\n";

        const std::vector<std::pair<std::string, std::string>> fields =
            GetAuthoredFieldsInLayer(layer, primPath, instanceName);
        if (fields.empty()) {
            std::cout << "      (no authored fields)\n";
            continue;
        }

        size_t width = 0;
        for (const auto &field : fields) {
            width = std::max(width, field.first.size());
        }
        for (const auto &field : fields) {
            std::cout << "      " << std::left << std::setw(int(width))
                      << field.first << " = " << field.second << "\n";
        }
    }
}

int USDAuthors(const Args &args) {
    TfErrorMark errMark;

    ArResolver &resolver = ArGetResolver();
    const ArResolverContext context =
        resolver.CreateDefaultContextForAsset(args.inputPath);
    const ArResolverContextBinder binder(context);
    const ArResolvedPath resolved = resolver.Resolve(args.inputPath);

    if (!resolved) {
        TF_RUNTIME_ERROR("Cannot resolve input path");
    } else if (args.layer) {
        const SdfLayerRefPtr layer = SdfLayer::FindOrOpen(resolved);
        if (errMark.IsClean() && layer) {
            const SdfPathVector paths =
                UsdMediaAuthorshipAPI::GetAllInLayer(layer);
            if (paths.empty()) {
                std::cout << "No authorship records found.\n";
            } else {
                PrintLayerRecords(layer, paths);
            }
        }
    } else {
        const UsdStageRefPtr stage = args.unloaded
            ? UsdStage::Open(resolved, UsdStage::LoadNone)
            : UsdStage::Open(resolved);

        if (errMark.IsClean() && stage) {
            std::vector<UsdMediaAuthorshipAPI> records =
                UsdMediaAuthorshipAPI::GetAllOnStage(stage);
            std::set<RecordId> duplicates;

            if (args.debug) {
                const std::vector<UsdMediaAuthorshipAPI> shadowed =
                    UsdMediaAuthorshipAPI::GetAllShadowed(stage);
                records.insert(records.end(), shadowed.begin(), shadowed.end());
                std::sort(records.begin(), records.end(),
                          [](const UsdMediaAuthorshipAPI &lhs,
                             const UsdMediaAuthorshipAPI &rhs) {
                              return GetRecordId(lhs) < GetRecordId(rhs);
                          });
                for (const UsdMediaAuthorshipAPI &record :
                        UsdMediaAuthorshipAPI::GetAllDuplicates(stage)) {
                    duplicates.insert(GetRecordId(record));
                }
            }

            if (args.summary) {
                PrintSummary(records);
            } else if (records.empty()) {
                std::cout << "No authorship records found.\n";
            } else {
                PrintRecords(records, duplicates);
            }
        }
    }

    if (!errMark.IsClean()) {
        std::cerr << "Failed to process " << std::quoted(args.inputPath)
                  << " - ";
        for (const auto &err : errMark) {
            std::cerr << err.GetCommentary() << "\n";
        }
        errMark.Clear();
        return 1;
    }

    return 0;
}

} // end anonymous namespace

int main(int argc, char const *argv[]) {
    CLI::App app(
        "usdauthors : Lists authorship records in a USD file, grouped by\n"
        "prim. Each record represents a single authoring step (e.g.,\n"
        "generation, export, or manual cleanup).\n"
        "\n"
        "Records are read from the composed stage. Use --debug to also flag\n"
        "records hidden or collapsed by composition, or --layer to report\n"
        "only what a single layer authors.\n",
        "usdauthors");

    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);

    return USDAuthors(args);
}
