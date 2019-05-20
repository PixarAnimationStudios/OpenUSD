//
// Copyright 2016 Pixar
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
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/fileSystem.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyUtils.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <mutex>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

class Tf_EnvSettingRegistry {
public:
    Tf_EnvSettingRegistry(const Tf_EnvSettingRegistry&) = delete;
    Tf_EnvSettingRegistry& operator=(const Tf_EnvSettingRegistry&) = delete;

    static Tf_EnvSettingRegistry& GetInstance() {
        return TfSingleton<Tf_EnvSettingRegistry>::GetInstance();
    }

    Tf_EnvSettingRegistry() {
        string fileName = TfGetenv("PIXAR_TF_ENV_SETTING_FILE", "");
        if (FILE* fp = ArchOpenFile(fileName.c_str(), "r")) {
            char buffer[1024];

#ifdef PXR_PYTHON_SUPPORT_ENABLED
            bool syncPython = TfPyIsInitialized();
#endif // PXR_PYTHON_SUPPORT_ENABLED

            int lineNo = 0;
            while (fgets(buffer, 1024, fp)) {
                if (buffer[strlen(buffer)-1] != '\n') {
                    fprintf(stderr, "File '%s' "
                            "(from PIXAR_TF_ENV_SETTING_FILE): "
                            "line %d is too long: ignored\n",
                            fileName.c_str(), lineNo+1);
                    continue;
                }
                ++lineNo;

                if (TfStringTrim(buffer).empty() || buffer[0] == '#')
                    continue;

                if (char* eqPtr = strchr(buffer, '=')) {
                    string key = TfStringTrim(string(buffer, eqPtr));
                    string value = TfStringTrim(string(eqPtr+1));
                    if (!key.empty()) {
                        ArchSetEnv(key, value, false /* overwrite */);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
                        if (syncPython) {
                            if (ArchGetEnv(key) == value) {
                                TfPySetenv(key, value);
                            }
                        }
#endif // PXR_PYTHON_SUPPORT_ENABLED

                    }
                    else {
                        fprintf(stderr, "File '%s' "
                                "(from PIXAR_TF_ENV_SETTING_FILE): "
                                "empty key on line %d\n",
                                fileName.c_str(), lineNo);
                    }
                }
                else {
                    fprintf(stderr, "File '%s' "
                            "(from PIXAR_TF_ENV_SETTING_FILE): "
                            "no '=' found on line %d\n",
                            fileName.c_str(), lineNo);
                }
            }

            fclose(fp);
        }
                    
        _printAlerts = TfGetenvBool("TF_ENV_SETTING_ALERTS_ENABLED", true);
        TfSingleton<Tf_EnvSettingRegistry>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<Tf_EnvSettingRegistry>();
    }

    using VariantType = boost::variant<int, bool, std::string>;

    template <typename U>
    bool Define(string const& varName,
                U const& value,
                std::atomic<U*>* cachedValue) {

        bool inserted = false;
        {
            std::lock_guard<std::mutex> lock(_lock);
            // Double check cachedValue now that we've acquired the registry
            // lock.  It's entirely possible that another thread may have
            // initialized our TfEnvSetting while we were waiting.
            if (cachedValue->load()) {
                return _printAlerts;
            }

            TfHashMap<string, VariantType, TfHash>::iterator it;
            std::tie(it, inserted) = _valuesByName.insert({varName, value});

            U* entryPointer = boost::get<U>(&(it->second));
            cachedValue->store(entryPointer);
        }

        if (!inserted) {
            TF_CODING_ERROR("Multiple definitions of TfEnvSetting variable "
                            "detected.  This is usually due to software "
                            "misconfiguration.  Contact the build team for "
                            "assistance.  (duplicate '%s')",
                            varName.c_str());
            return false;
        }
        else {
            return _printAlerts;
        }
    }

    VariantType LookupByName(string const& name) {
        std::lock_guard<std::mutex> lock(_lock);
        VariantType* v = TfMapLookupPtr(_valuesByName, name);
        return v ? *v : VariantType(); 
    }

private:
    std::mutex _lock;
    TfHashMap<string, VariantType, TfHash> _valuesByName;
    bool _printAlerts;
};

TF_INSTANTIATE_SINGLETON(Tf_EnvSettingRegistry);

static bool _Getenv(std::string const& name, bool def) {
    return TfGetenvBool(name, def);
}
static int _Getenv(std::string const& name, int def) {
    return TfGetenvInt(name, def);
}
static string _Getenv(std::string const& name, const char *def) {
    return TfGetenv(name, def);
}

static string _Str(bool value) {
    return value ? "true" : "false";
}
static string _Str(int value) {
    return TfStringPrintf("%d", value);
}
static string _Str(const char *value) {
    return string(value);
}
static string _Str(const std::string &value) {
    return value;
}

template <class T>
void Tf_InitializeEnvSetting(TfEnvSetting<T> *setting)
{
    const std::string settingName = setting->_name;

    // Create an object to install as the cached value.
    const T value = _Getenv(settingName, setting->_default);

    // Define the setting in the registry and install the cached setting
    // value.
    Tf_EnvSettingRegistry &reg = Tf_EnvSettingRegistry::GetInstance();
    if (reg.Define(settingName,
                   value,
                   setting->_value)) {
        // Setting was defined successfully and we should print alerts.
        if (setting->_default != value) {
            string text = TfStringPrintf("#  %s is overridden to '%s'.  "
                                         "Default is '%s'.  #",
                                         setting->_name,
                                         _Str(value).c_str(),
                                         _Str(setting->_default).c_str());
            string line(text.length(), '#');
            fprintf(stderr, "%s\n%s\n%s\n",
                    line.c_str(), text.c_str(), line.c_str());
        }
    }
}

// Explicitly instantiate for the supported types: bool, int, and string.
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<bool> *);
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<int> *);
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<string> *);

TF_API
boost::variant<int, bool, std::string>
Tf_GetEnvSettingByName(std::string const& name)
{
    return Tf_EnvSettingRegistry::GetInstance().LookupByName(name);
}

void TF_API Tf_InitEnvSettings()
{
    // Cause the registry to be created.  Crucially, this subscribes to
    // Tf_EnvSettingRegistry, ensuring that all env settings are defined
    // before we return.  If we don't do this TfGetEnvSetting() will call
    // Tf_InitializeEnvSetting() which will do the subscribe which will
    // call TfGetEnvSetting() again which will do Tf_InitializeEnvSetting()
    // and both Tf_InitializeEnvSetting() will try to define the setting.
    Tf_EnvSettingRegistry::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
