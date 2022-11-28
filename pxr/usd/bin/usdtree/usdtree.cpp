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

#include <pxr/pxr.h>
#include <pxr/base/tf/pxrCLI11/CLI11.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolverContextBinder.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stagePopulationMask.h>
#include <pxr/usd/usdUtils/flattenLayerStack.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/modelAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

struct Args {
    std::string inputPath;
    bool unloaded = false;
    bool attributes = false;
    bool metadata = false;
    bool simple = false;
    bool flatten = false;
    bool flattenLayerStack = false;
    std::vector<std::string> populationMask = {};
};

void Configure(CLI::App *app, Args &args) {
    app->add_option("inputPath", args.inputPath, "The input file to process");
    app->add_flag("--unloaded", args.unloaded, "Do not load payloads.");
    app->add_flag("-a,--attributes", args.attributes, "Display authored attributes.");
    app->add_flag("-m,--metadata", args.metadata,
                  "Display authored metadata "
                  "(active and kind are part of the label and not shown as individual items");
    app->add_flag("-s,--simple", args.simple,
                  "Only display prim names: no specifier, kind or active state.");
    app->add_flag("-f,--flatten", args.flatten,
                  "Compose the stage with the input file as the root layer"
                  "and write the flattened content.");
    app->add_flag("--flattenLayerStack", args.flattenLayerStack,
                  "Flatten the layer stack with the given root layer. "
                  "Unlike --flatten, this does not flatten composition arcs (such as references");
    app->add_option("--mask", args.populationMask,
                    "Limit stage population to these prims, their descendants and acnestors.\n"\
                    "To specify multiple paths, either use commas with no spaces, or quote the argument and separate "\
                    "paths by commans and/or spaces. Requires --flatten");
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
    for (auto &k: prim->ListInfoKeys()) {
        if (k == TfToken("active")) {
            return true;
        }
    }
    return false;
}

void GetMetadataKeys(const UsdPrim &prim, std::vector<std::string> &buffer) {
    for (auto &pair: prim.GetAllAuthoredMetadata()) {
        buffer.emplace_back(pair.first.GetString());
    }
}

void GetMetadataKeys(const SdfPrimSpecHandle &prim, std::vector<std::string> &buffer) {
    for (auto &key: prim->ListInfoKeys()) {
        buffer.emplace_back(key.GetString());
    }
}

std::vector<std::string> GetPropertyNames(const UsdPrim &prim) {
    std::vector<std::string> buffer;

    for (auto &prop: prim.GetProperties()) {
        buffer.emplace_back(prop.GetName().GetString());
    }

    return buffer;
}

std::vector<std::string> GetPropertyNames(const SdfPrimSpecHandle &prim) {
    std::vector<std::string> buffer;

    for (auto prop: prim->GetProperties()) {
        buffer.emplace_back(prop->GetName());
    }

    return buffer;
}

template<typename PrimType>
void GetPrimLabel(const PrimType &prim, std::string &label) {
    auto spec = GetSpecifier(prim);
    spec = TfStringToLower(spec);

    auto typeName = GetTypeName(prim);
    std::string definition = spec;
    if (!typeName.empty()) {
        definition += " " + typeName;
    }
    label += GetName(prim) + " [" + definition + "]";

    std::vector<std::string> shortMetadata;
    if (!IsActive(prim)) {
        shortMetadata.emplace_back("active = false");
    } else if (HasAuthoredActive(prim)) {
        shortMetadata.emplace_back("active = true");
    }

    auto kind = GetKind(prim);
    if (!kind.empty()) {
        shortMetadata.emplace_back("kind = " + kind);
    }

    if (!shortMetadata.empty()) {
        label += " (" + TfStringJoin(shortMetadata, ", ") + ")";
    }
}

template<typename PrimType>
void PrintPrim(const Args &args, const PrimType &prim, const std::string &prefix, bool isLast) {
    bool hasChildren = !GetChildren(prim).empty();

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

    std::string label;
    if (args.simple) {
        label += GetName(prim);
    } else {
        GetPrimLabel(prim, label);
    }

    std::cout << prefix << lastStep << label << std::endl;


    std::vector<std::string> attrs;
    if (args.metadata) {
        GetMetadataKeys(prim, attrs);
        for (auto &key: attrs) {
            if (key.empty()) continue;
            if (key == "typeName" || key == "specifier"
                || key == "kind" || key == "active") {
                continue;
            }

            attrs.emplace_back("(" + key + ")");
        }
    }

    if (args.attributes) {
        for (auto prop: GetPropertyNames(prim)) {
            attrs.emplace_back("." + prop);
        }
    }

    for (auto it = attrs.begin(); it != attrs.end(); ++it) {
        std::cout << prefix << attrStep;
        if (std::next(it) != attrs.end()) {
            std::cout << " :--";
        } else {
            std::cout << " `--";
        }

        std::cout << *it << std::endl;
    }

}


template<typename PrimType>
void PrintChildren(const Args &args, const PrimType &prim, const std::string &prefix) {
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

int PrintStage(Args &args, UsdStageRefPtr stage) {
    std::cout << "/" << std::endl;
    auto root = stage->GetPseudoRoot();
    PrintChildren(args, root, "");
    return 0;
}

int PrintLayer(Args &args, SdfLayerRefPtr layer) {
    std::cout << "/" << std::endl;
    auto root = layer->GetPseudoRoot();
    PrintChildren(args, root, "");
    return 0;
}

int USDTree(Args &args) {
    if (!args.populationMask.empty()) {
        if (!args.flatten) {
            std::cerr << "usd tree error: --mask requires --flatten";
            return 1;
        }
    }

    ArResolver &resolver = ArGetResolver();
    auto context = resolver.CreateDefaultContextForAsset(args.inputPath);
    auto binder = ArResolverContextBinder(context);

    auto resolved = resolver.Resolve(args.inputPath);
    if (resolved.empty()) {
        std::cerr << "Cannot resolve inputPath" << args.inputPath << std::endl;
        return 1;
    }


    UsdStageRefPtr stage;

    if (args.flatten) {
        auto mask = UsdStagePopulationMask();

        // Mask can be provided as a comma or space delimited string
        for (auto &m: args.populationMask) {
            std::string replaced = std::regex_replace(m, std::regex(","), " ");
            auto results = TfStringSplit(replaced, " ");

            for (auto &token: results) {
                mask.Add(SdfPath(token));
            }
        }

        if (!mask.IsEmpty()) {
            if (args.unloaded) {
                stage = UsdStage::OpenMasked(args.inputPath, mask, UsdStage::LoadNone);
            } else {
                stage = UsdStage::OpenMasked(args.inputPath, mask);
            }
        } else {
            if (args.unloaded) {
                stage = UsdStage::Open(args.inputPath, UsdStage::LoadNone);
            } else {
                stage = UsdStage::Open(args.inputPath);
            }
        }

        return PrintStage(args, stage);
    }

    SdfLayerRefPtr layer;
    if (args.flattenLayerStack) {
        stage = UsdStage::Open(args.inputPath, UsdStage::LoadNone);
        layer = UsdUtilsFlattenLayerStack(stage);
    } else {
        layer = SdfLayer::FindOrOpen(args.inputPath);
    }
    return PrintLayer(args, layer);

}


std::string GetVersionString() {
    return std::to_string(PXR_MAJOR_VERSION) + "." + std::to_string(PXR_MINOR_VERSION) + "." +
           std::to_string(PXR_PATCH_VERSION);
}

int main(int argc, char const *argv[]) {
    auto description = "usdtree " + GetVersionString()
                       + " : Writes the tree structure of a USD file. The default is to inspect a single USD file";
    CLI::App app(description, "usdtree");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);

    return USDTree(args);
}