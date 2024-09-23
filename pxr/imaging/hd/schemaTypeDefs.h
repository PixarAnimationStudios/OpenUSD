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
    HdTypedVectorSchema<HdIntArrayDataSource>;

// Vectors of Schemas
using HdRenderProductVectorSchema =
    HdSchemaBasedVectorSchema<class HdRenderProductSchema>;
using HdRenderVarVectorSchema =
    HdSchemaBasedVectorSchema<class HdRenderVarSchema>;
using HdInstanceIndicesVectorSchema =
    HdSchemaBasedVectorSchema<class HdInstanceIndicesSchema>;
using HdMaterialInterfaceMappingVectorSchema =
    HdSchemaBasedVectorSchema<class HdMaterialInterfaceMappingSchema>;
using HdMaterialConnectionVectorSchema =
    HdSchemaBasedVectorSchema<HdMaterialConnectionSchema>;

// Containers of sampled data sources
using HdSampledDataSourceContainerSchema =
    HdTypedContainerSchema<HdSampledDataSource>;

// Containers of schemas
using HdMaterialNodeContainerSchema =
    HdSchemaBasedContainerSchema<class HdMaterialNodeSchema>;
using HdMaterialNodeParameterContainerSchema =
    HdSchemaBasedContainerSchema<class HdMaterialNodeParameterSchema>;
using HdMaterialNetworkContainerSchema =
    HdSchemaBasedContainerSchema<class HdMaterialNetworkSchema>;
using HdMaterialConnectionContainerSchema =
    HdSchemaBasedContainerSchema<HdMaterialConnectionSchema>;
using HdExtComputationInputComputationContainerSchema =
    HdSchemaBasedContainerSchema<class HdExtComputationInputComputationSchema>;
using HdExtComputationOutputContainerSchema =
    HdSchemaBasedContainerSchema<class HdExtComputationOutputSchema>;
using HdSampledDataSourceContainerContainerSchema =
    HdSchemaBasedContainerSchema<HdSampledDataSourceContainerSchema>;

// Containers of vectors of schemas
using HdMaterialConnectionVectorContainerSchema =
    HdSchemaBasedContainerSchema<HdMaterialConnectionVectorSchema>;
using HdMaterialInterfaceMappingsContainerSchema =
    HdSchemaBasedContainerSchema<HdMaterialInterfaceMappingVectorSchema>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
