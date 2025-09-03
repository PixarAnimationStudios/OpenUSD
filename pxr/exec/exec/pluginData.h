//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PLUGIN_DATA_H
#define PXR_EXEC_EXEC_PLUGIN_DATA_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"

#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(PlugPlugin);

/// A structure used to initialize a map from schema types to plugins that
/// define computations for the schema.
///
/// The plugInfo that we expect here is of the form:
///
///     "Info": {
///         "Exec" : {
///             "Schemas": {
///                 "MySchemaType1": {
///                 },
///                 "MySchemaType2": {
///                 }
///             }
///         }
///     }
///
class Exec_PluginData {
public:
    /// Returns `true` if the plugInfo for the given \p plugin contains exec
    /// metadata.
    ///
    static bool HasExecMetadata(const PlugPluginPtr &plugin);

    /// If a plugin registers computations for \p schemaType, load it and return
    /// `true`.
    ///
    static bool
    LoadPluginComputationsForSchema(const TfType schemaType);

    /// Returns `true` if \p schemaType allows the registration of plugin
    /// computations.
    ///
    /// If exec plugin metadata is found for \p schemaType, \p pluginName is set
    /// to the name of the plugin that contains the plugin metadata.
    ///
    static bool
    SchemaAllowsPluginComputations(
        const TfType schemaType,
        std::string *pluginName);

private:
    // This class isn't publicly constructable; this function gices the
    // implementation access to a static instance.
    friend Exec_PluginData &Exec_GetPluginData();

    Exec_PluginData();

    void _ReadPluginMetadata(const PlugPluginPtr &plugin);

private:
    // For each schema, we record the plugin that (may) define computations for
    // that schema and a bool that indicates whether or not the schema is
    // allowed to have plugin computations.
    //
    struct SchemaData {
        PlugPluginPtr plugin;
        bool allowsPluginComputations;
    };

    std::unordered_map<TfType, SchemaData, TfHash> _execSchemaPlugins;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
