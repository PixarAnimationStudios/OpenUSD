//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

//! [ApplyCollections]
bool ApplyCollections(UsdPrim const &prim)
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
    UsdCollectionAPI cars = UsdCollectionAPI::Apply(prim, "cars");
    cars.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers"));
    cars.CreateExcludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckA"));
    cars.CreateExcludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckB"));

    // Create a collection that includes only the bikes by explicitly inluding 
    // just the two bikes in the collection.
    UsdCollectionAPI bikes = UsdCollectionAPI::Apply(prim, "bikes");
    bikes.CreateExpansionRuleAttr(VtValue(UsdTokens->explicitOnly));
    bikes.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BikeA"));
    bikes.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BikeB"));

    // Create an explicit collection of slow-moving vehicles. 
    // An explicit collection implies that descendants (i.e. the front and back 
    // wheels) are not considered to be included in the collection.
    UsdCollectionAPI slowVehicles = UsdCollectionAPI::Apply(prim, "slowVehicles");
    slowVehicles.CreateExpansionRuleAttr(VtValue(UsdTokens->explicitOnly));
    slowVehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/TwoWheelers/BicycleA"));
    slowVehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/Other/TricycleA"));

    UsdCollectionAPI vehicles = UsdCollectionAPI::Apply(prim, "vehicles");
    vehicles.CreateIncludesRel().AddTarget(cars.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(bikes.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(slowVehicles.GetCollectionPath());
    vehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckA"));
    vehicles.CreateIncludesRel().AddTarget(SdfPath("/Vehicles/FourWheelers/TruckB"));


    UsdCollectionAPI::MembershipQuery query = vehicles.ComputeMembershipQuery();

    // CarA is included in the 'vehicles' collection through the 'cars' collection.
    TF_AXIOM(query.IsPathIncluded("/Vehicles/FourWheelers/CarA"))

    // BikeB is included in the 'vehicles' collection through the 'cars' collection.
    TF_AXIOM(query.IsPathIncluded("/Vehicles/TwoWheelers/BikeB"))

    // BikeB is included directly in the 'vehicles' collection 
    TF_AXIOM(query.IsPathIncluded("/Vehicles/FourWheelers/TruckA"))

    // BicycleA is included, but it's descendants are not, since it is part of 
    // an "explicitOnly" collection.
    TF_AXIOM(query.IsPathIncluded("/Vehicles/TwoWheelers/BicycleA"))
    TF_AXIOM(!query.IsPathIncluded("/Vehicles/TwoWheelers/BicycleA/FrontWheel"))

    // TricycleA is included, but it's descendants are not, since it is part of 
    // an "explicitOnly" collection.
    TF_AXIOM(query.IsPathIncluded("/Vehicles/Other/TricycleA"))
    TF_AXIOM(!query.IsPathIncluded("/Vehicles/Other/TricycleA/BackWheels"))

    SdfPathSet includedPaths;
    UsdCollectionAPI::ComputeIncludedPaths(query, prim.GetStage(), 
                                           &includedPaths);
    std::set<UsdObject> includedObjects;
    UsdCollectionAPI::ComputeIncludedObjects(query, prim.GetStage(), 
                                             &includedObjects);
}

//! [ApplyCollections]


PXR_NAMESPACE_CLOSE_SCOPE
