/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef __UTIL_IES_H__
#define __UTIL_IES_H__

#include <string>
#include <vector>

class IESFile {
public:
    IESFile() {}
    ~IESFile();

    int packed_size();
    void pack(float *data);

    bool load(const std::string &ies);
    void clear();

    // returns true if the IES files was successfully loaded and processed and is ready to evaluate
    bool valid() const {
        return !_intensities.empty();
    }

    // evaluate the IES file for the given spherical coordinates
    float eval(float theta, float phi) const;

    float peakIntensity() const {
        return _peakIntensity;
    }

    float forwardIntensity() const {
        return valid() ? _intensities[0][0] : 0.0f;
    }

    float power() const {
        return _power;
    }

protected:
    bool parse(const std::string &ies);
    bool process();
    bool process_type_b();
    bool process_type_c();

    // The brightness distribution is stored in spherical coordinates.
    // The horizontal angles correspond to phi in the PBRT notation
    // and always span the full range from 0° to 360°.
    // The vertical angles correspond to theta and always start at 0°.
    std::vector<float> _v_angles, _h_angles;

    // The actual values are stored here, with every entry storing the values
    // of one horizontal segment.
    std::vector<std::vector<float>> _intensities;

    float _peakIntensity = 0;
    float _power = 0;

    // Types of angle representation in IES files. Currently, only B and C are supported.
    enum IESType { TYPE_A = 3, TYPE_B = 2, TYPE_C = 1 } _type;
};

#endif /* __UTIL_IES_H__ */
