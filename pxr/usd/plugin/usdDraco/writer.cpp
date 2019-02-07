//
// Copyright 2019 Google LLC
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

#include "exportTranslator.h"
#include "flag.h"
#include "writer.h"

#include "pxr/pxr.h"

#include <draco/compression/encode.h>
#include <draco/mesh/mesh.h>

#include <algorithm>
#include <fstream>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


bool UsdDraco_WriteDraco(const UsdGeomMesh &usdMesh,
                         const std::string &fileName,
                         int qp,
                         int qt,
                         int qn,
                         int cl,
                         int preservePolygons,
                         int preservePositionOrder,
                         int preserveHoles)
{
  // Translate USD mesh to Draco mesh.
  draco::Mesh dracoMesh;
  bool success = UsdDracoExportTranslator::Translate(
      usdMesh, &dracoMesh,
      UsdDracoFlag<bool>::MakeBooleanFlag(preservePolygons),
      UsdDracoFlag<bool>::MakeBooleanFlag(preservePositionOrder),
      UsdDracoFlag<bool>::MakeBooleanFlag(preserveHoles));
  if (!success) {
      std::cout << "Could not translate USD mesh to Draco mesh." << std::endl;
      return false;
  }

  // Set Draco options and encode mesh.
  draco::EncoderBuffer buffer;
  draco::Encoder encoder;
  encoder.SetEncodingMethod(draco::MESH_EDGEBREAKER_ENCODING);
  if (qp != 0)
      encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, qp);
  if (qt != 0)
      encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, qt);
  if (qn != 0)
      encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, qn);

  const int speed = 10 - cl;
  encoder.SetSpeedOptions(speed, speed);
  if (!encoder.EncodeMeshToBuffer(dracoMesh, &buffer).ok()) {
      std::cout << "Could not encode mesh." << std::endl;
      return false;
  }

  // Write encoded Draco mesh to file.
  std::ofstream fout(fileName.c_str(), std::ios::binary);
  if (!fout.is_open()) {
      std::cout << "Failed to open file " << fileName << std::endl;
      return false;
  }
  fout.write(buffer.data(), buffer.size());
  fout.close();
  return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

