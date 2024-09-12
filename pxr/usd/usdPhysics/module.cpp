/*
====
Copyright (c) 2018, NVIDIA CORPORATION
======
*/

//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python.hpp"
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
	TF_WRAP(UsdPhysicsTokens);

	TF_WRAP(UsdPhysicsRigidBodyAPI);
	TF_WRAP(UsdPhysicsCollisionAPI);
	TF_WRAP(UsdPhysicsMeshCollisionAPI);
	TF_WRAP(UsdPhysicsMaterialAPI);
	TF_WRAP(UsdPhysicsScene);	

	TF_WRAP(Metrics);

	// Mass
	TF_WRAP(UsdPhysicsMassAPI);

	// Joints
	TF_WRAP(UsdPhysicsArticulationRootAPI);	
	TF_WRAP(UsdPhysicsJoint);
	TF_WRAP(UsdPhysicsRevoluteJoint);
	TF_WRAP(UsdPhysicsPrismaticJoint);
	TF_WRAP(UsdPhysicsSphericalJoint);
	TF_WRAP(UsdPhysicsDistanceJoint);
	TF_WRAP(UsdPhysicsFixedJoint);
	TF_WRAP(UsdPhysicsLimitAPI);
	TF_WRAP(UsdPhysicsDriveAPI);


	// collision group
	TF_WRAP(UsdPhysicsCollisionGroup);
	TF_WRAP(UsdPhysicsFilteredPairsAPI);	
}
