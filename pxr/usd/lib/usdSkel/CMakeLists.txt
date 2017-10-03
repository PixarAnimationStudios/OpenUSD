set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdSkel)

pxr_library(usdSkel
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}

    PUBLIC_CLASSES
        joint
        skeleton
        packedJointAnimation
        bindingAPI
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp    

    PYMODULE_FILES
        __init__.py

    PYMODULE_CPPFILES
        module.cpp
        wrapJoint.cpp
        wrapSkeleton.cpp
        wrapPackedJointAnimation.cpp
        wrapBindingAPI.cpp
        wrapTokens.cpp        

    RESOURCE_FILES
        generatedSchema.usda
        plugInfo.json
        schema.usda:usdSkel/schema.usda
)
   
