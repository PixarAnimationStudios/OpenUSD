/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "ies.h"

#include <algorithm>
#include <limits>
#include <math.h>
#include <stdio.h>

bool IESFile::load(const std::string &ies) {
    clear();

    if (!parse(ies) || !process()) {
        clear();
        return false;
    }

    return true;
}

void IESFile::clear() {
    _intensities.clear();
    _v_angles.clear();
    _h_angles.clear();
}

int IESFile::packed_size() {
    if (_v_angles.size() && _h_angles.size() > 0) {
        return 2 + _h_angles.size() + _v_angles.size() +
                _h_angles.size() * _v_angles.size();
    }
    return 0;
}

template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From) &&
                    std::is_trivially_copyable_v<From> &&
                    std::is_trivially_copyable_v<To>,
                    To>
    // constexpr support needs compiler magic
    bit_cast(const From &src) noexcept {
    static_assert(std::is_trivially_constructible_v<To>,
                "This implementation additionally requires "
                "destination type to be trivially constructible");

    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

void IESFile::pack(float *data) {
    if (_v_angles.size() && _h_angles.size()) {
        *(data++) = bit_cast<int, float>(_h_angles.size());
        *(data++) = bit_cast<int, float>(_v_angles.size());

        memcpy(data, &_h_angles[0], _h_angles.size() * sizeof(float));
        data += _h_angles.size();
        memcpy(data, &_v_angles[0], _v_angles.size() * sizeof(float));
        data += _v_angles.size();

        for (int h = 0; h < _intensities.size(); h++) {
            memcpy(data, &_intensities[h][0], _v_angles.size() * sizeof(float));
            data += _v_angles.size();
        }
    }
}

class IESTextParser {
public:
    std::vector<char> text;
    char *data;
    bool error;

    IESTextParser(const std::string &str)
        : text(str.begin(), str.end()), error(false) {
        std::replace(text.begin(), text.end(), ',', ' ');
        data = strstr(&text[0], "\nTILT=");
    }

    bool eof() { return (data == NULL) || (data[0] == '\0'); }

    bool has_error() { return error; }

    double get_double() {
        if (eof()) {
            error = true;
            return 0.0;
        }
        char *old_data = data;
        double val = strtod(data, &data);
        if (data == old_data) {
            data = NULL;
            error = true;
            return 0.0;
        }
        return val;
    }

    long get_long() {
        if (eof()) {
            error = true;
            return 0;
        }
        char *old_data = data;
        long val = strtol(data, &data, 10);
        if (data == old_data) {
            data = NULL;
            error = true;
            return 0;
        }
        return val;
    }
};

bool IESFile::parse(const std::string &ies) {
    if (ies.empty()) {
        return false;
    }

    IESTextParser parser(ies);
    if (parser.eof()) {
        return false;
    }

    // Handle the tilt data block
    if (strncmp(parser.data, "\nTILT=INCLUDE", 13) == 0) {
        parser.data += 13;
        parser.get_double();              //< Lamp to Luminaire geometry
        int num_tilt = parser.get_long(); //< Amount of tilt angles and factors

        // Skip over angles and factors
        for (int i = 0; i < 2 * num_tilt; i++) {
            parser.get_double();
        }
    } else {
        // Skip to next line
        parser.data = strstr(parser.data + 1, "\n");
    }

    if (parser.eof()) {
        return false;
    }
    parser.data++;

    parser.get_long();                    //< Number of lamps
    parser.get_double();                  //< Lumens per lamp
    double factor = parser.get_double();  //< Candela multiplier
    int v_angles_num = parser.get_long(); //< Number of vertical angles
    int h_angles_num = parser.get_long(); //< Number of horizontal angles 
    _type = (IESType)parser.get_long();   //< Photometric type

    // TODO(lukas): Test whether the current type B processing can also deal with
    // type A files. In theory the only difference should be orientation which we
    // ignore anyways, but with IES you never know...
    //
    if (_type != TYPE_B && _type != TYPE_C) {
        return false;
    }

    parser.get_long();             //< Unit of the geometry data
    parser.get_double();           //< Width
    parser.get_double();           //< Length
    parser.get_double();           //< Height
    factor *= parser.get_double(); //< Ballast factor
    factor *= parser.get_double(); //< Ballast-Lamp Photometric factor
    parser.get_double();           //< Input Watts

    _v_angles.reserve(v_angles_num);
    for (int i = 0; i < v_angles_num; i++) {
        _v_angles.push_back((float)parser.get_double());
    }

    _h_angles.reserve(h_angles_num);
    for (int i = 0; i < h_angles_num; i++) {
        _h_angles.push_back((float)parser.get_double());
    }

    _intensities.resize(h_angles_num);
    for (int i = 0; i < h_angles_num; i++) {
        _intensities[i].reserve(v_angles_num);
        for (int j = 0; j < v_angles_num; j++) {
            _intensities[i].push_back((float)(factor * parser.get_double()));
        }
    }

    return !parser.has_error();
}

bool IESFile::process_type_b() {
    std::vector<std::vector<float>> newintensity;
    newintensity.resize(_v_angles.size());

    for (int i = 0; i < _v_angles.size(); i++) {
        newintensity[i].reserve(_h_angles.size());
        for (int j = 0; j < _h_angles.size(); j++) {
            newintensity[i].push_back(_intensities[j][i]);
        }
    }
    _intensities.swap(newintensity);
    _h_angles.swap(_v_angles);

    float h_first = _h_angles[0], h_last = _h_angles[_h_angles.size() - 1];
    if (h_last != 90.0f) {
        return false;
    }

    if (h_first == 0.0f) {
        // The range in the file corresponds to 90°-180°, we need to mirror that to
        // get the full 180° range
        std::vector<float> new_h_angles;
        std::vector<std::vector<float>> new_intensity;
        int hnum = _h_angles.size();
        new_h_angles.reserve(2 * hnum - 1);
        new_intensity.reserve(2 * hnum - 1);
        for (int i = hnum - 1; i > 0; i--) {
            new_h_angles.push_back(90.0f - _h_angles[i]);
            new_intensity.push_back(_intensities[i]);
        }
        for (int i = 0; i < hnum; i++) {
            new_h_angles.push_back(90.0f + _h_angles[i]);
            new_intensity.push_back(_intensities[i]);
        }
        _h_angles.swap(new_h_angles);
        _intensities.swap(new_intensity);
    } else if (h_first == -90.0f) {
        // We have full 180° coverage, so just shift to match the angle range
        // convention.
        for (int i = 0; i < _h_angles.size(); i++) {
            _h_angles[i] += 90.0f;
        }
    }

    // To get correct results with the cubic interpolation in the kernel, the
    // horizontal range has to cover all 360°. Therefore, we copy the 0° entry to
    // 360° to ensure full coverage and seamless interpolation
    _h_angles.push_back(360.0f);
    _intensities.push_back(_intensities[0]);

    float v_first = _v_angles[0], v_last = _v_angles[_v_angles.size() - 1];
    if (v_last != 90.0f) {
        return false;
    }

    if (v_first == 0.0f) {
        // The range in the file corresponds to 90°-180°, we need to mirror that to
        // get the full 180° range
        std::vector<float> new_v_angles;
        int hnum = _h_angles.size();
        int vnum = _v_angles.size();
        new_v_angles.reserve(2 * vnum - 1);

        for (int i = vnum - 1; i > 0; i--) {
            new_v_angles.push_back(90.0f - _v_angles[i]);
        }

        for (int i = 0; i < vnum; i++) {
            new_v_angles.push_back(90.0f + _v_angles[i]);
        }

        for (int i = 0; i < hnum; i++) {
            std::vector<float> new_intensity;
            new_intensity.reserve(2 * vnum - 1);

            for (int j = vnum - 2; j >= 0; j--) {
                new_intensity.push_back(_intensities[i][j]);
            }

            new_intensity.insert(new_intensity.end(), _intensities[i].begin(),
                                _intensities[i].end());
            _intensities[i].swap(new_intensity);
        }

        _v_angles.swap(new_v_angles);

    } else if (v_first == -90.0f) {
        // We have full 180° coverage, so just shift to match the angle range
        // convention
        for (int i = 0; i < _v_angles.size(); i++) {
            _v_angles[i] += 90.0f;
        }
    }

    return true;
}

bool IESFile::process_type_c() {
    if (_h_angles[0] == 90.0f) {
        // Some files are stored from 90° to 270°, so we just rotate them to the
        // regular 0°-180° range here
        for (int i = 0; i < _h_angles.size(); i++) {
            _h_angles[i] -= 90.0f;
        }
    }

    if (_h_angles[0] != 0.0f) {
        return false;
    }

    if (_h_angles.size() == 1) {
        _h_angles.push_back(360.0f);
        _intensities.push_back(_intensities[0]);
    }

    if (_h_angles[_h_angles.size() - 1] == 90.0f) {
        // Only one quadrant is defined, so we need to mirror twice (from one to
        // two, then to four). Since the two->four mirroring step might also be
        // required if we get an input of two quadrants, we only do the first mirror
        // here and later do the second mirror in either case.
        int hnum = _h_angles.size();
        for (int i = hnum - 2; i >= 0; i--) {
            _h_angles.push_back(180.0f - _h_angles[i]);
            _intensities.push_back(_intensities[i]);
        }
    }

    if (_h_angles[_h_angles.size() - 1] == 180.0f) {
        // Mirror half to the full range
        int hnum = _h_angles.size();
        for (int i = hnum - 2; i >= 0; i--) {
            _h_angles.push_back(360.0f - _h_angles[i]);
            _intensities.push_back(_intensities[i]);
        }
    }

    // Some files skip the 360° entry (contrary to standard) because it's supposed
    // to be identical to the 0° entry. If the file has a discernible order in its
    // spacing, just fix this.
    if (_h_angles[_h_angles.size() - 1] != 360.0f) {
        int hnum = _h_angles.size();
        float last_step = _h_angles[hnum - 1] - _h_angles[hnum - 2];
        float first_step = _h_angles[1] - _h_angles[0];
        float difference = 360.0f - _h_angles[hnum - 1];
        if (last_step == difference || first_step == difference) {
            _h_angles.push_back(360.0f);
            _intensities.push_back(_intensities[0]);
        } else {
            return false;
        }
    }

    float v_first = _v_angles[0], v_last = _v_angles[_v_angles.size() - 1];
    if (v_first == 90.0f) {
        if (v_last != 180.0f) {
            return false;
        }
    } else if (v_first != 0.0f) {
        return false;
    }

    return true;
}

static float linearstep(float x, float a, float b) {
    if (x <= a) {
        return 0.0f;
    }

    if (x >= b) {
        return 1.0f;
    }

    return (x - a) / (b - a);
}

static float lerp(float a, float b, float t) {
    return (1.0f - t) * a + t * b;
}

bool IESFile::process() {
    if (_h_angles.size() == 0 || _v_angles.size() == 0) {
        return false;
    }

    if (_type == TYPE_B) {
        if (!process_type_b()) {
            return false;
        }
    } else {
        assert(type == TYPE_C);
        if (!process_type_c()) {
            return false;
        }
    }

    assert(_v_angles[0] == 0.0f || _v_angles[0] == 90.0f);
    assert(_h_angles[0] == 0.0f);
    assert(_h_angles[_h_angles.size() - 1] == 360.0f);

    #if !defined(M_PI)
    #define M_PI 3.14159265358979323846
    #endif

    // Convert from deg to rad
    float v_angleMin = std::numeric_limits<float>::max();
    float v_angleMax = std::numeric_limits<float>::min();
    for (int i = 0; i < _v_angles.size(); i++) {
        _v_angles[i] *= M_PI / 180.f;
        v_angleMin = std::min(v_angleMin, _v_angles[i]);
        v_angleMax = std::max(v_angleMax, _v_angles[i]);
    }

    // // does the distribution cover the whole sphere?
    bool is_sphere = false;
    if ((v_angleMax - v_angleMin) > (M_PI/2 + 0.1 /* fudge factor*/)) {
        is_sphere = true;
    }

    for (int i = 0; i < _h_angles.size(); i++) {
        _h_angles[i] *= M_PI / 180.f;
    }

    for (auto const& v_slice: _intensities) {
        for (auto const& i: v_slice) {
            _peakIntensity = std::max(_peakIntensity, i);
        }
    }

    // integrate the intensity over solid angle to get power
    for (int h = 0; h < _h_angles.size() - 1; ++h) {
        for (int v = 0; v < _v_angles.size() - 1; ++v) {
            // approximate dimensions of the patch
            float dh = _h_angles[h+1] - _h_angles[h];
            float dv = _v_angles[v+1] - _v_angles[v];
            // bilinearly interpolate intensity at the patch center
            float i0 = (_intensities[h][v] + _intensities[h][v+1]) / 2;
            float i1 = (_intensities[h+1][v] + _intensities[h+1][v+1]) / 2;
            float intensity = (i0 + i1) / 2;
            // solid angle of the patch
            float dS = dh * dv * sinf(_v_angles[v] + dv/2);
            _power += dS * intensity;
        }
    }
    
    // XXX: and this factor matches Karma & RIS
    _power /= M_PI * (is_sphere ? 4 : 2);

    return true;
}

float IESFile::eval(float theta, float phi) const {
    int hi = -1;
    int vi = -1;
    float dh = 0;
    float dv = 0;

    if (phi < 0) {
        phi += 2 * M_PI;
    } else if (phi > 2* M_PI) {
        phi -= 2 * M_PI;
    }

    for (int i = 0; i < _h_angles.size() - 1; ++i) {
        if (phi >= _h_angles[i] && phi < _h_angles[i+1]) {
            hi = i;
            dh = linearstep(phi, _h_angles[i], _h_angles[i+1]);
            break;
        }
    }

    if (theta < 0) {
        vi = 0;
        dv = 0;
    } else if (theta >= M_PI) {
        vi = _v_angles.size()-2;
        dv = 1;
    } else {
        for (int i = 0; i < _v_angles.size() - 1; ++i) {
            if (theta >= _v_angles[i] && theta < _v_angles[i+1]) {
                vi = i;
                dv = linearstep(theta, _v_angles[i], _v_angles[i+1]);
                break;
            }
        }
    }

    if (hi == -1 || vi == -1) {
        // XXX: need to indicate error somehow here
        return 0.0f;
    }

    // XXX: This should be a cubic interpolation
    float i0 = lerp(_intensities[hi][vi], _intensities[hi][vi+1], dv);
    float i1 = lerp(_intensities[hi+1][vi], _intensities[hi+1][vi+1], dv);

    return lerp(i0, i1, dh);
}

IESFile::~IESFile() { clear(); }
