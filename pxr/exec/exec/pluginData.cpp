//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/pluginData.h"

#include "pxr/base/js/types.h"
#include "pxr/base/js/utils.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticLite.h"

PXR_NAMESPACE_OPEN_SCOPE

// This function isn't static because it is friended by Exec_PluginData to
// get access to the private constructor.
Exec_PluginData &
Exec_GetPluginData() {
    static Exec_PluginData execPluginData;
    return execPluginData;
}

Exec_PluginData::Exec_PluginData()
{
    // For each plugin found by plugin discovery, look for the metadata that
    // tells us which schemas that plugin defines computations for.
    for (const PlugPluginPtr &plugin :
             PlugRegistry::GetInstance().GetAllPlugins()) {
        _ReadPluginMetadata(plugin);
    }
}

bool
Exec_PluginData::HasExecMetadata(const PlugPluginPtr &plugin)
{
    return static_cast<bool>(JsFindValue(plugin->GetMetadata(), "Exec"));
}

bool
Exec_PluginData::LoadPluginComputationsForSchema(const TfType schemaType)
{
    const auto it = Exec_GetPluginData()._execSchemaPlugins.find(schemaType);
    if (it != Exec_GetPluginData()._execSchemaPlugins.end()) {
        if (const PlugPluginPtr &plugin = it->second.plugin; TF_VERIFY(plugin)) {
            plugin->Load();
            return true;
        }
    }
    return false;
}

bool
Exec_PluginData::SchemaAllowsPluginComputations(
    const TfType schemaType,
    std::string *const pluginName)
{
    const auto it = Exec_GetPluginData()._execSchemaPlugins.find(schemaType);
    if (it == Exec_GetPluginData()._execSchemaPlugins.end()) {
        return true;
    }
        
    if (TF_VERIFY(pluginName)) {
        *pluginName = it->second.plugin->GetName();
    }
    return it->second.allowsPluginComputations;
}

static bool
_AllowsPluginComputations(const JsValue &schemaValue)
{
    const JsOptionalValue allowsPluginComputationsValue =
        JsFindValue(schemaValue.GetJsObject(), "allowsPluginComputations");
    if (!allowsPluginComputationsValue) {
        // In the absense of 'allowsPluginComputations' metadata, the schema is
        // allowsPluginComputations by default.
        return true;
    }

    if (!allowsPluginComputationsValue->IsBool()) {
        TF_CODING_ERROR(
            "Exec 'allowsPluginComputations' metadatum holding type %s; "
            "expected type bool.",
            allowsPluginComputationsValue->GetTypeName().c_str());
        // On error, we consider the schema to *not* allow plugin computations.
        return false;
    }

    return allowsPluginComputationsValue->GetBool();
}

void
Exec_PluginData::_ReadPluginMetadata(const PlugPluginPtr &plugin)
{
    const JsOptionalValue metadataValue =
        JsFindValue(plugin->GetMetadata(), "Exec");
    if (!metadataValue) {
        return;
    }

    const JsOptionalValue schemasValue =
        JsFindValue(metadataValue->GetJsObject(), "Schemas");
    if (!schemasValue) {
        return;
    }

    for (const auto& [schemaName, schemaValue] : schemasValue->GetJsObject()) {
        const TfType schemaType = TfType::FindByName(schemaName);
        if (schemaType.IsUnknown()) {
            TF_CODING_ERROR(
                "Unknown schema type name '%s' encountered when reading Exec "
                "plugInfo.",
                schemaName.c_str());
            continue;
        }

        // Attempt to emplace an entry mapping the schema type to the plugin,
        // noting whether or not the schema allows computations to be registered
        // for it.
        const bool allowsPluginComputations =
            _AllowsPluginComputations(schemaValue);
        const auto [it, emplaced] =
            _execSchemaPlugins.emplace(
                schemaType,
                Exec_PluginData::SchemaData{plugin, allowsPluginComputations});
        if (emplaced) {
            continue;
        }

        // Emit a suitable error, since we already had an entry for this schema.
        const PlugPluginPtr &oldPlugin = it->second.plugin;
        const bool oldAllowsPluginComputations =
            it->second.allowsPluginComputations;
        if (allowsPluginComputations == oldAllowsPluginComputations) {
            TF_CODING_ERROR(
                "Plugin '%s' declares schema %s as %sallowing plugin "
                "computations, but plugin '%s' already declared this schema.",
                (allowsPluginComputations ? " " : "not "),
                plugin->GetName().c_str(),
                schemaType.GetTypeName().c_str(),
                oldPlugin->GetName().c_str());
        } else {
            // In the case of disagreement, ensure the schema is marked as
            // not allowing plugin computations.
            it->second.allowsPluginComputations = false;

            TF_CODING_ERROR(
                "Plugin '%s' declares schema %s as %sallowing plugin "
                "computations, but plugin '%s' already declared it as "
                "%sallowing plugin computations.",
                (allowsPluginComputations ? " " : "not "),
                plugin->GetName().c_str(),
                schemaType.GetTypeName().c_str(),
                oldPlugin->GetName().c_str(),
                (oldAllowsPluginComputations ? " " : "not "));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
 
