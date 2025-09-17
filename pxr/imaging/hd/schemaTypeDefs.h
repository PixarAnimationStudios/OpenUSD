//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCHEMA_TYPE_DEFS_H
#define PXR_IMAGING_HD_SCHEMA_TYPE_DEFS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/containerSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/vectorSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMaterialConnectionSchema;

// Vectors of numeric types
using HdIntArrayVectorSchema = 
    HdVectorOfTypedSampledDataSourcesSchema<HdIntArrayDataSource>;

// Vectors of Schemas
using HdRenderProductVectorSchema =
    HdVectorOfSchemasSchema<class HdRenderProductSchema>;
using HdRenderVarVectorSchema =
    HdVectorOfSchemasSchema<class HdRenderVarSchema>;
using HdInstanceIndicesVectorSchema =
    HdVectorOfSchemasSchema<class HdInstanceIndicesSchema>;
using HdMaterialInterfaceMappingVectorSchema =
    HdVectorOfSchemasSchema<class HdMaterialInterfaceMappingSchema>;
using HdMaterialConnectionVectorSchema =
    HdVectorOfSchemasSchema<HdMaterialConnectionSchema>;

// Containers of sampled data sources
using HdSampledDataSourceContainerSchema =
    HdContainerOfTypedSampledDataSourcesSchema<HdSampledDataSource>;

// Containers of schemas
using HdMaterialNodeContainerSchema =
    HdContainerOfSchemasSchema<class HdMaterialNodeSchema>;
using HdMaterialNodeParameterContainerSchema =
    HdContainerOfSchemasSchema<class HdMaterialNodeParameterSchema>;
using HdMaterialNetworkContainerSchema =
    HdContainerOfSchemasSchema<class HdMaterialNetworkSchema>;
using HdMaterialConnectionContainerSchema =
    HdSchemaBasedContainerSchema<HdMaterialConnectionSchema>;
using HdMaterialInterfaceParameterContainerSchema =
    HdContainerOfSchemasSchema<class HdMaterialInterfaceParameterSchema>;
using HdExtComputationInputComputationContainerSchema =
    HdContainerOfSchemasSchema<class HdExtComputationInputComputationSchema>;
using HdExtComputationOutputContainerSchema =
    HdContainerOfSchemasSchema<class HdExtComputationOutputSchema>;
using HdSampledDataSourceContainerContainerSchema =
    HdContainerOfSchemasSchema<HdSampledDataSourceContainerSchema>;

// Containers of vectors of schemas
using HdMaterialConnectionVectorContainerSchema =
    HdContainerOfSchemasSchema<HdMaterialConnectionVectorSchema>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
