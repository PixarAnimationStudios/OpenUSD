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
#include "pxrUsdInShipped/declareCoreOps.h"

#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readMaterial.h"
#include "usdKatana/blindDataObject.h"
#include "usdKatana/readBlindData.h"

#include "pxr/usd/usdShade/material.h"


#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <list>
#include <map>

#include <FnAPI/FnAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE



namespace
{

// bounded LRU cache
class ConvertedMaterialCache
{
public:
    
    typedef boost::shared_ptr<PxrUsdKatanaAttrMap> PxrUsdKatanaAttrMapRefPtr;
    
    
    ConvertedMaterialCache(size_t maxEntries)
    : m_maxEntries(maxEntries)
    {
        
    }
    
    PxrUsdKatanaAttrMapRefPtr get(const std::string & key)
    {
        boost::upgrade_lock<boost::upgrade_mutex> readerLock(m_mutex);
        
        auto mapI = m_entryIteratorMap.find(key);
        
        if (mapI == m_entryIteratorMap.end())
        {
            //std::cerr << "cache miss: " << key << std::endl;
        
            return PxrUsdKatanaAttrMapRefPtr();
        }
        
        //std::cerr << "cache hit: " << key << std::endl;
        
        boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                writerLock(readerLock);
        
        m_entries.splice(m_entries.end(), m_entries, (*mapI).second);
        
        return (*((*mapI).second)).value;
        
    }
    
    void insert(const std::string & key, PxrUsdKatanaAttrMapRefPtr value)
    {
        //std::cerr << "inserting: " << key << std::endl;
        
        boost::upgrade_lock<boost::upgrade_mutex> readerLock(m_mutex);
        
        auto mapI = m_entryIteratorMap.find(key);
        if (mapI != m_entryIteratorMap.end())
        {
            //replace in-place if it's already there
            boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                    writerLock(readerLock);
            (*(*mapI).second).value = value;
            return;
        }
        
        
        boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                    writerLock(readerLock);
        
        // evict from front
        while (m_entries.size() > m_maxEntries)
        {
            auto entryI = m_entries.begin();
            //std::cerr << "evicting: " << (*entryI).key << std::endl;
            m_entryIteratorMap.erase((*entryI).key);
            m_entries.erase(entryI);
        }
        
        m_entryIteratorMap[key] = m_entries.insert(m_entries.end(),
                Entry(key, value));
        
    }
    
    void clear()
    {
        boost::upgrade_lock<boost::upgrade_mutex> readerLock(m_mutex);
        boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                    writerLock(readerLock);
        
        m_entryIteratorMap.clear();
        m_entries.clear();
    }
    
private:
    
    struct Entry
    {
        Entry(const std::string & _key, PxrUsdKatanaAttrMapRefPtr _value)
        : key(_key)
        , value(_value)
        {
        }
        
        std::string key;
        PxrUsdKatanaAttrMapRefPtr value;
    };
    
    typedef std::list<Entry> EntryList;
    typedef std::map<std::string, EntryList::iterator> EntryListIteratorMap;
    
    EntryList m_entries;
    EntryListIteratorMap m_entryIteratorMap;
    
    size_t m_maxEntries;
    
    boost::upgrade_mutex m_mutex;
    
};

ConvertedMaterialCache g_materialCache(250);

void FlushMaterialCache()
{
    g_materialCache.clear();
}


} // anonymous namespace




PXRUSDKATANA_USDIN_PLUGIN_DEFINE_WITH_FLUSH(
        PxrUsdInCore_LookOp, privateData, opArgs, interface, FlushMaterialCache)
{
    // always flatten individual materials
    bool flatten = true;
    
    // unless someone tells us not to force it
    flatten = FnAttribute::IntAttribute(
            opArgs.getChildByName("forceFlattenLooks")
                    ).getValue(flatten, false);
    
    
    UsdShadeMaterial materialSchema(privateData.GetUsdPrim());
    if (!flatten)
    {
        flatten = !materialSchema.HasBaseMaterial();
    }
    
    
    std::string looksGroupLocation = FnAttribute::StringAttribute(
            opArgs.getChildByName("looksGroupLocation")).getValue("", false);
    
    
    
    ConvertedMaterialCache::PxrUsdKatanaAttrMapRefPtr attrs;
    
    
    FnAttribute::Attribute looksCacheKeyPrefixAttr =
            opArgs.getChildByName("looksCacheKeyPrefixAttr");
    
    
    std::string key;
    
    // We currently only cache if a key prefix is provided by a parent Looks
    // scope. Free-floating materials are not cached.
    if (looksCacheKeyPrefixAttr.isValid())
    {
        key = FnAttribute::GroupAttribute(
                "a", looksCacheKeyPrefixAttr,
                "b", FnAttribute::StringAttribute(
                            privateData.GetUsdPrim().GetName()),
                true).getHash().str();
        
        
        attrs = g_materialCache.get(key);
    }
    
    
    if (!attrs)
    {
        attrs = ConvertedMaterialCache::PxrUsdKatanaAttrMapRefPtr(
                new PxrUsdKatanaAttrMap);
        
        
        PxrUsdKatanaReadMaterial(
            materialSchema,
            flatten,
            privateData,
            *attrs,
            looksGroupLocation,
            interface.getOutputLocationPath());

        
        // Read blind data.
        PxrUsdKatanaReadBlindData(
            UsdKatanaBlindDataObject(materialSchema), *attrs);
        
        
        if (!key.empty())
        {
            g_materialCache.insert(key, attrs);
        }
        
    }
    
    
    
    attrs->toInterface(interface);

    
    // tell the op handling the traversal to skip all children
    interface.setAttr("__UsdIn.skipAllChildren", FnAttribute::IntAttribute(1));
    
    // If we ourselves are built via an intermediate and intermediate
    // children are present, re-run the intermediate from here.
    FnAttribute::GroupAttribute staticScene =
            opArgs.getChildByName("staticScene");
    if (staticScene.isValid())
    {
        
        interface.execOp("PxrUsdIn.BuildIntermediate",
                    opArgs
// katana 2.x doesn't allow private data to be overridden in execOp as it'll
// automatically use that of the calling op while katana 3.x does allow it
// to be overridden but requires it to be specified even if unchanged (as here).
#if KATANA_VERSION_MAJOR >= 3
                    ,
                    new PxrUsdKatanaUsdInPrivateData(
                                privateData.GetUsdInArgs()->GetRootPrim(),
                                privateData.GetUsdInArgs(), &privateData),
                    PxrUsdKatanaUsdInPrivateData::Delete
#endif
                    );
    }
    
}
