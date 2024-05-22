/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_IES_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_IES_H


#include <string>
#include <vector>

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace pxr_ccl {

using std::string;
using std::vector;

class IESFile {
 public:
  IESFile() {}
  ~IESFile();

  int packed_size();
  void pack(float *data);

  bool load(const string &ies);
  void clear();

 protected:
  bool parse(const string &ies);
  bool process();

  void process_type_a();
  void process_type_b();
  void process_type_c();

  /* The brightness distribution is stored in spherical coordinates.
   * The horizontal angles correspond to theta in the regular notation
   * and always span the full range from 0° to 360°.
   * The vertical angles correspond to phi and always start at 0°. */
  vector<float> v_angles, h_angles;
  /* The actual values are stored here, with every entry storing the values
   * of one horizontal segment. */
  vector<vector<float>> intensity;

  /* Types of angle representation in IES files. */
  enum IESType { TYPE_A = 3, TYPE_B = 2, TYPE_C = 1 } type;
};

} /* namespace pxr_ccl */

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_IMAGING_PLUGIN_HD_EMBREE_PXRIES_IES_H */
