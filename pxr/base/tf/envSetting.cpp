//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

#include <mutex>
#include <variant>

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
            auto emitError = [&fileName, &lineNo](char const *fmt, ...)
                /*ARCH_PRINTF_FUNCTION(1, 2)*/ {
                va_list ap;
                va_start(ap, fmt);
                fprintf(stderr, "File '%s' (From PIXAR_TF_ENV_SETTING_FILE) "
                        "line %d: %s.\n", fileName.c_str(), lineNo,
                        TfVStringPrintf(fmt, ap).c_str());
                va_end(ap);
            };

            while (fgets(buffer, sizeof(buffer), fp)) {
                ++lineNo;
                string line = string(buffer);
                if (line.back() != '\n') {
                    emitError("line too long; ignored");
                    continue;
                }

                string trimmed = TfStringTrim(line);
                if (trimmed.empty() || trimmed.front() == '#') {
                    continue;
                }

                size_t eqPos = trimmed.find('=');
                if (eqPos == std::string::npos) {
                    emitError("no '=' found");
                    continue;
                }
                    
                string key = TfStringTrim(trimmed.substr(0, eqPos));
                string value = TfStringTrim(trimmed.substr(eqPos+1));
                if (key.empty()) {
                    emitError("empty key");
                    continue;
                }
                    
                ArchSetEnv(key, value, /*overwrite=*/false);
#ifdef PXR_PYTHON_SUPPORT_ENABLED
                if (syncPython && ArchGetEnv(key) == value) {
                    TfPySetenv(key, value);
                }
#endif // PXR_PYTHON_SUPPORT_ENABLED
            }

            fclose(fp);
        }
                    
        _printAlerts = TfGetenvBool("TF_ENV_SETTING_ALERTS_ENABLED", true);
        TfSingleton<Tf_EnvSettingRegistry>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<Tf_EnvSettingRegistry>();
    }

    using VariantType = std::variant<int, bool, std::string>;

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
                // Only the caller that successfully set the value
                // should print the alert.
                return false;
            }

            TfHashMap<string, VariantType, TfHash>::iterator it;
            std::tie(it, inserted) = _valuesByName.insert({varName, value});

            U* entryPointer = std::get_if<U>(std::addressof(it->second));
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

    VariantType const *LookupByName(string const& name) const {
        std::lock_guard<std::mutex> lock(_lock);
        return TfMapLookupPtr(_valuesByName, name);
    }

private:
    mutable std::mutex _lock;
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
std::variant<int, bool, std::string> const *
Tf_GetEnvSettingByName(std::string const& name)
{
    return Tf_EnvSettingRegistry::GetInstance().LookupByName(name);
}

PXR_NAMESPACE_CLOSE_SCOPE
