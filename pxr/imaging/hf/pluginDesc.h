//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_PLUGIN_DESC_H
#define PXR_IMAGING_HF_PLUGIN_DESC_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


///
/// Common structure used to report registered plugins in one of the plugin 
/// registries.  The id token is used for internal api communication
/// about the name of the plugin.
/// displayName is a human readable name given to the plugin intended
/// to be used in menus.
/// priority is used to provide an ordering of plugins.  The plugin
/// with the highest priority is determined to be the default (unless
/// overridden by the application).  In the event of a tie
/// the string version of id is used to sort alphabetically ('a' has priority
/// over 'b').
///
struct HfPluginDesc {
    TfToken     id;
    std::string displayName;
    int         priority;
};

typedef std::vector<HfPluginDesc> HfPluginDescVector;


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HF_PLUGIN_DESC_H
