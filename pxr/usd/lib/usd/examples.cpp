//
// Copyright 2017 Pixar
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
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

//! [AddCollections]
bool AddCollections(UsdPrim const &prim)
{       
    /* Assuming the folling prim hierarchy:
    |- Vehicles 
    |    |- FourWheelers
    |    |    |- CarA
    |    |    |- CarB
    |    |    |- CarC
    |    |    |- CarD
    |    |    |- TruckA
    |    |    |- TruckB
    |    |- TwoWheelers
    |    |    |- BikeA
    |    |    |- BikeB
    |    |    |- BicycleA
    |    |        |- FrontWheel
    |    |        |- BackWheel
    |    |- Other
    |    |    |- TricycleA
    |    |        |- FrontWheel
    |    |        |- BackWheels
    */

    // Create a collection that includes only the cars, by adding all 
    // of "FourWheelers" and excluding the trucks.
    UsdCollectionAPI cars = UsdCollectionAPI::AddCollection(
        prim, "cars", /* expansionRule */ UsdTokens->expandPrims);
    cars.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers"));
    cars.CreateExcludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckA"));
    cars.CreateExcludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckB"));

    // Create a collection that includes only the bikes by explicitly inluding 
    // just the two bikes in the collection.
    UsdCollectionAPI bikes = UsdCollectionAPI::AddCollection(
        prim, "bikes", /* expansionRule */ UsdTokens->explicitOnly);
    bikes.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BikeA"));
    bikes.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BikeB"));

    // Create an explicit collection of slow-moving vehicles. 
    // An explicit collection implies that descendants (i.e. the front and back 
    // wheels) are not considered to be included in the collection.
    UsdCollectionAPI slowVehicles = UsdCollectionAPI::AddCollection(prim, 
        "slowVehicles", /* expansionRule */ UsdTokens->explicitOnly);
    slowVehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BicycleA"));
    slowVehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/Other/TricycleA"));

    UsdCollectionAPI vehicles = UsdCollectionAPI::AddCollection(prim, 
        "vehicles", /* expansionRule */ UsdTokens->expandPrims);
    vehicles.CreateIncludesRel().AddTarget(cars.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(bikes.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(slowVehicles.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckA"));
    vehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckB"));


    UsdCollectionAPI::MembershipQuery query = vehicles.ComputeMembershipQuery();

    // CarA is included in the 'vehicles' collection through the 'cars' collection.
    assert(query.IsPathIncluded("/Vehicles/FourWheelers/CarA"))

    // BikeB is included in the 'vehicles' collection through the 'cars' collection.
    assert(query.IsPathIncluded("/Vehicles/TwoWheelers/BikeB"))

    // BikeB is included directly in the 'vehicles' collection 
    assert(query.IsPathIncluded("/Vehicles/FourWheelers/TruckA"))

    // BicycleA is included, but it's descendants are not, since it is part of 
    // an "explicitOnly" collection.
    assert(query.IsPathIncluded("/Vehicles/TwoWheelers/BicycleA"))
    assert(!query.IsPathIncluded("/Vehicles/TwoWheelers/BicycleA/FrontWheel"))

    // TricycleA is included, but it's descendants are not, since it is part of 
    // an "explicitOnly" collection.
    assert(query.IsPathIncluded("/Vehicles/Other/TricycleA"))
    assert(!query.IsPathIncluded("/Vehicles/Other/TricycleA/BackWheels"))

    SdfPathSet includedPaths;
    UsdCollectionAPI::ComputeIncludedPaths(query, prim.GetStage(), 
                                           &includedPaths);
    std::set<UsdObject> includedObjects;
    UsdCollectionAPI::ComputeIncludedObjects(query, prim.GetStage(), 
                                             &includedObjects);
}

//! [AddCollections]


PXR_NAMESPACE_CLOSE_SCOPE
