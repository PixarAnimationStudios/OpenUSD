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
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/fileSystem.h"

#include <boost/python.hpp>
#include <boost/noncopyable.hpp>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

class Tf_EnvSettingRegistry : boost::noncopyable {
public:
    static Tf_EnvSettingRegistry& GetInstance() {
        return TfSingleton<Tf_EnvSettingRegistry>::GetInstance();
    }

    Tf_EnvSettingRegistry() {
        string fileName = TfGetenv("PIXAR_TF_ENV_SETTING_FILE", "");
        if (FILE* fp = ArchOpenFile(fileName.c_str(), "r")) {
            char buffer[1024];
            bool syncPython = TfPyIsInitialized();
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
                        if (syncPython) {
                            if (ArchGetEnv(key) == value) {
                                TfPySetenv(key, value);
                            }
                        }
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

    // Function to convert a cached C++ object to a Python object on demand.
    typedef boost::function<boost::python::object()> MakePyObject;

    struct _Record {
        MakePyObject defValue, value;
        string description;
    };

    bool _Define(void* ptrKey, string const& varName, MakePyObject defValue,
                 string const& description, MakePyObject value,
                 std::atomic<void*>* cachedValue, void** potentialCachedValue) {
        _Record r;
        r.defValue = defValue;
        r.value = value;
        r.description = description;

        bool inserted = false;
        {
            std::lock_guard<std::mutex> lock(_lock);
            inserted = _recordsByName.insert(std::make_pair(varName, r)).second;
            if (inserted) {
                _recordsByPtrKey[ptrKey] = r;

                // Install the cached value into the setting if one
                // doesn't already exist, which it should not or we
                // wouldn't be defining the setting.  If it didn't
                // already exist then ensure the called doesn't delete
                // it later.
                void* expectedNullPtr = nullptr;
                if (cachedValue->compare_exchange_strong(
                        expectedNullPtr, *potentialCachedValue)) {
                    *potentialCachedValue = nullptr;
                }
            }
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
            if (*potentialCachedValue) {
                TF_CODING_ERROR("TfEnvSetting value for %s was already "
                                "initialized.", varName.c_str());
            }
            return _printAlerts;
        }
    }

    template <typename T, typename U>
    bool Define(void* ptrKey, string const& varName, T defValue,
                string const& description, U value,
                std::atomic<void*>* cachedValue, void** potentialCachedValue) {
        constexpr bool reportError = true;
        return _Define(ptrKey, varName,
                       boost::bind(&TfPyObject<U>, defValue, !reportError),
                       description,
                       boost::bind(&TfPyObject<U>, value, !reportError),
                       cachedValue, potentialCachedValue);
    }

    boost::python::object LookupByName(string const& name) {
        std::lock_guard<std::mutex> lock(_lock);
        _Record* r = TfMapLookupPtr(_recordsByName, name);
        return r ? r->value() : boost::python::object();
    }

    boost::python::object GetAllEntries() {
        std::lock_guard<std::mutex> lock(_lock);
        boost::python::dict result;
        TF_FOR_ALL(it, _recordsByName) {
            result[it->first] = boost::python::make_tuple(it->second.defValue(),
                                                          it->second.description);
        }

        return result;
    }

    std::mutex _lock;
    TfHashMap<void*, _Record, TfHash> _recordsByPtrKey;
    TfHashMap<string, _Record, TfHash> _recordsByName;
    bool _printAlerts;
};

TF_INSTANTIATE_SINGLETON(Tf_EnvSettingRegistry);

static bool _Getenv(char const *name, bool def) {
    return TfGetenvBool(name, def);
}
static int _Getenv(char const *name, int def) {
    return TfGetenvInt(name, def);
}
static string _Getenv(char const *name, const char *def) {
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
    // Create an object to install as the cached value.
    const T value = _Getenv(setting->_name, setting->_default);
    T *cachedValue = new T(value);

    // Define the setting in the registry and install the cached setting
    // value.
    Tf_EnvSettingRegistry &reg = Tf_EnvSettingRegistry::GetInstance();
    if (reg.Define(static_cast<void *>(setting),
                   setting->_name, setting->_default,
                   setting->_description, *cachedValue,
                   reinterpret_cast< std::atomic<void*>* >(setting->_value),
                   reinterpret_cast<void**>(&cachedValue))) {
        // Setting was defined successfully and we should print alerts.
        if (setting->_default != value) {
            string text =
                TfStringPrintf("#  %s is overridden to '%s'.  "
                               "Default is '%s'.  #",
                               setting->_name,
                               _Str(value).c_str(),
                               _Str(setting->_default).c_str());
            string line(text.length(), '#');
            fprintf(stderr, "%s\n%s\n%s\n",
                    line.c_str(), text.c_str(), line.c_str());
        }
    }

    // If the setting was already defined or the cached setting already
    // existed then cachedValue will not have been set to NULL and we
    // discard it here.
    delete cachedValue;
}

// Explicitly instantiate for the supported types: bool, int, and string.
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<bool> *);
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<int> *);
template void TF_API Tf_InitializeEnvSetting(TfEnvSetting<string> *);

TF_API
boost::python::object
Tf_GetEnvSettingByName(string const& name)
{
    return Tf_EnvSettingRegistry::GetInstance().LookupByName(name);
}

TF_API
boost::python::object
Tf_GetEnvSettingDictionary()
{
    return Tf_EnvSettingRegistry::GetInstance().GetAllEntries();
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
