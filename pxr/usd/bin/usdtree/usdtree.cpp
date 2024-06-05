//
// Copyright 2022 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/usd/usdUtils/flattenLayerStack.h"

#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

namespace
{

struct Args {
    std::string inputPath;
    bool unloaded = false;
    bool attributes = false;
    bool metadata = false;
    bool simple = false;
    bool flatten = false;
    bool flattenLayerStack = false;
    std::string populationMask;
};

void Configure(CLI::App *app, Args &args) {
    app->add_option(
        "inputPath", args.inputPath, "The input file to process")
        ->required();
    app->add_flag(
        "--unloaded", args.unloaded, "Do not load payloads.");
    app->add_flag(
        "-a,--attributes", args.attributes, "Display authored attributes.");
    app->add_flag(
        "-m,--metadata", args.metadata,
        "Display authored metadata (active and kind are part of the label and "
        "not shown as individual items");
    app->add_flag(
        "-s,--simple", args.simple,
        "Only display prim names: no specifier, kind or active state.");
    app->add_flag(
        "-f,--flatten", args.flatten,
        "Compose the stage with the input file as the root layer and write "
        "the flattened content.");
    app->add_flag(
        "--flattenLayerStack", args.flattenLayerStack,
        "Flatten the layer stack with the given root layer. Unlike --flatten, "
        "this does not flatten composition arcs (such as references).");
    app->add_option(
        "--mask", args.populationMask,
        "Limit stage population to these prims, their descendants and "
        "ancestors. To specify multiple paths, either use commas with no "
        "spaces, or quote the argument and separate paths by commas "
        "and/or spaces. Requires --flatten.")
        ->option_text("PRIMPATH[,PRIMPATH...]");
}

UsdPrimSiblingRange GetChildren(const UsdPrim &prim) {
    return prim.GetAllChildren();
}

SdfPrimSpecView GetChildren(const SdfPrimSpecHandle &prim) {
    return prim->GetNameChildren();
}

std::string GetName(const UsdPrim &prim) {
    return prim.GetName().GetString();
}

std::string GetName(const SdfPrimSpecHandle &prim) {
    return prim->GetName();
}

std::string GetSpecifier(const UsdPrim &prim) {
    return TfEnum::GetDisplayName(prim.GetSpecifier());
}

std::string GetSpecifier(const SdfPrimSpecHandle &prim) {
    return TfEnum::GetDisplayName(prim->GetSpecifier());
}

std::string GetTypeName(const UsdPrim &prim) {
    return prim.GetTypeName().GetString();
}

std::string GetTypeName(const SdfPrimSpecHandle &prim) {
    return prim->GetTypeName().GetString();
}

std::string GetKind(const UsdPrim &prim) {
    TfToken kind;
    UsdModelAPI(prim).GetKind(&kind);
    return kind.GetString();
}

std::string GetKind(const SdfPrimSpecHandle &prim) {
    return prim->GetKind().GetString();
}

bool IsActive(const UsdPrim &prim) {
    return prim.IsActive();
}

bool IsActive(const SdfPrimSpecHandle &prim) {
    return prim->GetActive();
}

bool HasAuthoredActive(const UsdPrim &prim) {
    return prim.HasAuthoredActive();
}

bool HasAuthoredActive(const SdfPrimSpecHandle &prim) {
    return prim->HasInfo(SdfFieldKeys->Active);
}

std::vector<TfToken> GetMetadataKeys(const UsdPrim &prim) {
    std::vector<TfToken> buffer;
    for (auto &pair: prim.GetAllAuthoredMetadata()) {
        buffer.emplace_back(pair.first);
    }
    return buffer;
}

std::vector<TfToken> GetMetadataKeys(const SdfPrimSpecHandle &prim) {
    return prim->ListInfoKeys();
}

std::vector<TfToken> GetPropertyNames(const UsdPrim &prim) {
    std::vector<TfToken> buffer;
    for (const auto& prop: prim.GetAuthoredProperties()) {
        buffer.emplace_back(prop.GetName());
    }
    return buffer;
}

std::vector<TfToken> GetPropertyNames(const SdfPrimSpecHandle &prim) {
    std::vector<TfToken> buffer;
    for (const auto& prop: prim->GetProperties()) {
        buffer.emplace_back(prop->GetName());
    }
    return buffer;
}

template<typename PrimType>
std::string GetPrimLabel(const PrimType &prim) {
    // The display name of specifiers are known to be ASCII only.
    const std::string spec = TfStringToLowerAscii(GetSpecifier(prim));
    const std::string typeName = GetTypeName(prim);
    std::string definition = spec;
    if (!typeName.empty()) {
        definition += " " + typeName;
    }

    std::string label = GetName(prim) + " [" + definition + "]";

    std::vector<std::string> shortMetadata;
    if (!IsActive(prim)) {
        shortMetadata.emplace_back("active = false");
    } else if (HasAuthoredActive(prim)) {
        shortMetadata.emplace_back("active = true");
    }

    const std::string kind = GetKind(prim);
    if (!kind.empty()) {
        shortMetadata.emplace_back("kind = " + kind);
    }

    if (!shortMetadata.empty()) {
        label += " (" + TfStringJoin(shortMetadata, ", ") + ")";
    }

    return label;
}

template<typename PrimType>
void PrintPrim(
    const Args &args, const PrimType &prim, const std::string &prefix, 
    bool isLast) {
    const bool hasChildren = !GetChildren(prim).empty();

    std::string lastStep;
    std::string attrStep;

    if (!isLast) {
        lastStep = " |--";
        if (hasChildren) {
            attrStep = " |   |";
        } else {
            attrStep = " |    ";
        }
    } else {
        lastStep = " `--";
        if (hasChildren) {
            attrStep = "     |";
        } else {
            attrStep = "      ";
        }
    }

    const std::string label = 
        args.simple ? GetName(prim) : GetPrimLabel(prim);

    std::cout << prefix << lastStep << label << "\n";

    std::vector<std::string> attrs;
    if (args.metadata) {
        std::vector<TfToken> metadata = GetMetadataKeys(prim);
        std::sort(metadata.begin(), metadata.end());

        for (const auto &key: metadata) {
            if (key.IsEmpty() ||
                key == SdfFieldKeys->TypeName ||
                key == SdfFieldKeys->Specifier ||
                key == SdfFieldKeys->Kind ||
                key == SdfFieldKeys->Active) {
                continue;
            }

            attrs.emplace_back("(" + key.GetString() + ")");
        }
    }

    if (args.attributes) {
        for (const auto &prop: GetPropertyNames(prim)) {
            attrs.emplace_back("." + prop.GetString());
        }
    }

    for (auto it = attrs.begin(); it != attrs.end(); ++it) {
        std::cout << prefix << attrStep;
        if (std::next(it) != attrs.end()) {
            std::cout << " :--";
        } else {
            std::cout << " `--";
        }

        std::cout << *it << "\n";
    }

}


template<typename PrimType>
void PrintChildren(
    const Args &args, const PrimType &prim, const std::string &prefix) {
    auto children = GetChildren(prim);
    for (auto child = children.begin(); child != children.end(); ++child) {
        if (std::next(child) != children.end()) {
            PrintPrim(args, *child, prefix, false);
            PrintChildren(args, *child, prefix + " |  ");
        } else {
            PrintPrim(args, *child, prefix, true);
            PrintChildren(args, *child, prefix + "    ");
        }
    }
}

template<typename StageOrLayer>
void PrintTree(const Args &args, const StageOrLayer &stageOrLayer) {
    std::cout << "/\n";
    PrintChildren(args, stageOrLayer->GetPseudoRoot(), "");
}

void PrintTree(const Args &args, const ArResolvedPath& resolved) {
    TfErrorMark m;

    if (args.flatten) {
        // Mask can be provided as a comma or space delimited string
        UsdStagePopulationMask mask;
        for (const auto &path : TfStringTokenize(args.populationMask, ", ")) {
            mask.Add(SdfPath(path));
        }

        if (!m.IsClean()) {
            return;
        }

        UsdStageRefPtr stage;
        if (!mask.IsEmpty()) {
            if (args.unloaded) {
                stage = UsdStage::OpenMasked(resolved, mask, UsdStage::LoadNone);
            } else {
                stage = UsdStage::OpenMasked(resolved, mask);
            }
        } else {
            if (args.unloaded) {
                stage = UsdStage::Open(resolved, UsdStage::LoadNone);
            } else {
                stage = UsdStage::Open(resolved);
            }
        }
        
        if (!m.IsClean() || !stage) {
            return;
        }
        
        PrintTree(args, stage);
    } else if (args.flattenLayerStack) {
        UsdStageRefPtr stage = UsdStage::Open(resolved, UsdStage::LoadNone);
        if (!m.IsClean() || !stage) {
            return;
        }

        SdfLayerRefPtr layer = UsdUtilsFlattenLayerStack(stage);
        if (!m.IsClean() || !layer) {
            return;
        }

        PrintTree(args, layer);
    } else {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(resolved);
        if (!m.IsClean() || !layer) {
            return;
        }

        PrintTree(args, layer);
    }
}

int USDTree(const Args &args) {
    if (!args.populationMask.empty() && !args.flatten) {
        // You can only mask a stage, not a layer.
        std::cerr << "error: --mask requires --flatten\n";
        return 1;
    }

    TfErrorMark errMark;

    ArResolver &resolver = ArGetResolver();
    const ArResolverContext context =
        resolver.CreateDefaultContextForAsset(args.inputPath);
    const ArResolverContextBinder binder(context);
    const ArResolvedPath resolved = resolver.Resolve(args.inputPath);

    if (!resolved) {
        TF_RUNTIME_ERROR("Cannot resolve input path");
    }
    else {
        PrintTree(args, resolved);
    }

    if (!errMark.IsClean()) {
        std::cerr << "Failed to process " << std::quoted(args.inputPath) 
                  << " - ";
        for (const auto& err : errMark) {
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
        "usdtree : Writes the tree structure of a USD file. The default is to\n"
        "inspect a single USD file. Use the --flatten argument to see the\n"
        "flattened (or composed) Stage tree. Special metadata 'kind' and\n"
        "'active' are always shown if authored unless --simple is provided.\n",
        "usdtree");

    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);

    return USDTree(args);
}
