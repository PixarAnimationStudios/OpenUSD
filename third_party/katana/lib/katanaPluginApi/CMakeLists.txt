# This library builds source files from the Katana installation
set(PXR_PACKAGE katanaPluginApi)

add_definitions(
    /DFNASSET_STATIC
    /DFNATTRIBUTE_STATIC
    /DFNATTRIBUTEMODIFIER_STATIC
    /DFNCONFIG_STATIC
    /DFNDEFAULTATTRIBUTEPRODUCER_STATIC
    /DFNLOGGING_STATIC
    /DFNRENDEROUTPUTLOCATION_STATIC
    /DFNSCENEGRAPHGENERATOR_STATIC
    /DFNSCENEGRAPHITERATOR_STATIC
    /DPYSTRING_STATIC
)

# https://msdn.microsoft.com/en-us/library/esew7y1w.aspx
add_definitions("/wd4251")

# https://msdn.microsoft.com/en-us/library/3tdb471s.aspx
add_definitions("/wd4275")

# https://msdn.microsoft.com/en-us/library/b6801kcy.aspx
add_definitions("/wd4800")

pxr_static_library(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${KATANA_API_INCLUDE_DIRS}

    PUBLIC_HEADERS
        FnAsset.h
        FnAttributeModifier.h
        FnScenegraphGenerator.h
        FnViewerModifier.h
        FnViewerModifierInput.h
        ScenegraphLocationDelegate.h

    CPPFILES
        ${KATANA_API_SOURCE_DIR}/pystring/pystring.cpp
        ${KATANA_API_SOURCE_DIR}/FnAsset/client/FnDefaultAssetPlugin.cpp
        ${KATANA_API_SOURCE_DIR}/FnAsset/client/FnDefaultFileSequencePlugin.cpp
        ${KATANA_API_SOURCE_DIR}/FnAsset/plugin/FnAsset.cpp
        ${KATANA_API_SOURCE_DIR}/FnAsset/plugin/FnFileSequence.cpp
        ${KATANA_API_SOURCE_DIR}/FnAttribute/client/FnAttribute.cpp
        ${KATANA_API_SOURCE_DIR}/FnAttribute/client/FnAttributeUtils.cpp
        ${KATANA_API_SOURCE_DIR}/FnAttribute/client/FnDataBuilder.cpp
        ${KATANA_API_SOURCE_DIR}/FnAttributeModifier/plugin/FnAttributeModifier.cpp
        ${KATANA_API_SOURCE_DIR}/FnAttributeModifier/plugin/FnAttributeModifierInput.cpp
        ${KATANA_API_SOURCE_DIR}/FnConfig/FnConfig.cpp
        ${KATANA_API_SOURCE_DIR}/FnDefaultAttributeProducer/plugin/FnDefaultAttributeProducerPlugin.cpp
        ${KATANA_API_SOURCE_DIR}/FnDefaultAttributeProducer/plugin/FnDefaultAttributeProducerUtil.cpp
        ${KATANA_API_SOURCE_DIR}/FnLogging/client/FnLogging.cpp
        ${KATANA_API_SOURCE_DIR}/FnPluginSystem/FnPlugin.cpp
        ${KATANA_API_SOURCE_DIR}/FnRenderOutputLocation/client/FnRenderOutputLocationPluginClient.cpp
        ${KATANA_API_SOURCE_DIR}/FnRenderOutputLocation/plugin/FnRenderOutputLocationPlugin.cpp
        ${KATANA_API_SOURCE_DIR}/FnScenegraphGenerator/plugin/FnScenegraphGenerator.cpp
        ${KATANA_API_SOURCE_DIR}/FnScenegraphIterator/client/FnScenegraphIterator.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewerModifier/plugin/FnViewerModifier.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewerModifier/plugin/FnViewerModifierInput.cpp
)

