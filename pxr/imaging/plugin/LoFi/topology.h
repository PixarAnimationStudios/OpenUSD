//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H
#define PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct LoFiTopology
///
struct LoFiTopology
{
  enum Type {
      POINTS,
      LINES,
      TRIANGLES    
  };
  Type                type;
  const int*          samples;
  size_t              numElements;
};

enum LoFiCurveType {
  LOFI_CURVE_LINEAR,
  LOFI_CURVE_CUBIC
};

enum LoFiCurveBasis {
  LOFI_CURVE_BASIS_NONE,
  LOFI_CURVE_BASIS_BEZIER,
  LOFI_CURVE_BASIS_BSPLINE,
  LOFI_CURVE_BASIS_CATMULLROM
};

struct LoFiCurvesTopology : public LoFiTopology
{
  const int*          bases;
  size_t              numBases;
  LoFiCurveType       curveType;
  LoFiCurveBasis      curveBasis;
  bool                wrap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H
