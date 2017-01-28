#include <FnAttributeFunction/plugin/FnAttributeFunctionPlugin.h>
#include <FnGeolib/util/AttributeKeyedCache.h>


// Cache for the UsdMaterialReference AttributeFunction
class MaterialReferenceAttrFncCache : public FnGeolibUtil::AttributeKeyedCache<
FnAttribute::GroupAttribute >
{
public:
    MaterialReferenceAttrFncCache() : FnGeolibUtil::AttributeKeyedCache<
        FnAttribute::GroupAttribute>() {}
protected:
    IMPLPtr createValue(const FnAttribute::Attribute & attr);
};


class MaterialReferenceAttrFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args);
    static void flush();
};



// Cache for the LibraryMaterialNames AttributeFunction
class LibraryMaterialNamesAttrFncCache : public FnGeolibUtil::AttributeKeyedCache<
FnAttribute::StringAttribute >
{
public:
    LibraryMaterialNamesAttrFncCache() : FnGeolibUtil::AttributeKeyedCache<
        FnAttribute::StringAttribute>() {}
protected:
    IMPLPtr createValue(const FnAttribute::Attribute & attr);
};


class LibraryMaterialNamesAttrFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args);
    static void flush();
};
