//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//


#ifndef PXR_IMAGING_PX_OSD_MESH_TOPOLOGY_VALIDATION_H
#define PXR_IMAGING_PX_OSD_MESH_TOPOLOGY_VALIDATION_H

#include "pxr/imaging/pxOsd/tokens.h"

#include <array>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class PxOsdMeshTopology;

/// Utility to help validate an OpenSubdiv Mesh topology.
///
/// This class is created by PxOsdMeshTopology's Validate method.
///
/// Internally, this will avoid dynamic allocations as long as
/// the topology is valid (currently using std::unique_ptr but
/// targeting std::optional for C++17).
///
/// This class does a set of basic validation tests on
/// the topology of a mesh.  This set of tests isn't necessarily
/// complete. There are other cases like invalid primvar size
/// that this will not check for.
///
/// Topology is considered valid if it passes a series of checks
/// enumerated by the Code class enum.
///
/// \warn This doesn't currently validate that the topology of crease
/// indices match valid edges.
///
/// \note This class is convertable to bool and converts to true if the
/// the topology is valid and false if any invalidations were found.
/// That is to say, a conversion to true implies an empty invalidation
/// vector and false implies a non-empty invalidation vector.
class PxOsdMeshTopologyValidation {
public:
    friend class PxOsdMeshTopology;
    /// Codes for various invalid states for PxOsdMeshTopology
    enum class Code {
        /// Encodes invalid scheme token value
        InvalidScheme,
        /// Encodes invalid orientation token value
        InvalidOrientation,
        /// Encodes invalid triangle subdivision token value
        InvalidTriangleSubdivision,
        /// Encodes invalid vertex interpolation rule token value
        InvalidVertexInterpolationRule,
        /// Encodes invalid face varying interpolation rule token value
        InvalidFaceVaryingInterpolationRule,
        /// Encodes invalid crease method token value
        InvalidCreaseMethod,
        /// Encodes crease lengths element less than 2
        InvalidCreaseLengthElement,
        /// Encodes crease indices size not matching the sum of the lengths
        /// array
        InvalidCreaseIndicesSize,
        /// Encodes crease indices element is not in the face vertex indices
        /// vector
        InvalidCreaseIndicesElement,
        /// Encodes if crease weights is the size of the number of creases or
        /// the number of crease edges
        InvalidCreaseWeightsSize,
        /// Encodes if crease weights are negative
        NegativeCreaseWeights,
        /// Encodes corner indices element is not in the face vertex indices
        /// vector
        InvalidCornerIndicesElement,
        /// Encodes if corner weights are negative
        NegativeCornerWeights,
        /// Encodes if corner weights is not the size of the number of corner
        /// indices
        InvalidCornerWeightsSize,
        /// Encodes if the hole indices are negative or greater than the maximum
        /// face index (face count - 1)
        InvalidHoleIndicesElement,
        /// Encodes if a vertex count is less than 3
        InvalidFaceVertexCountsElement,
        /// Encodes if the element is negative
        InvalidFaceVertexIndicesElement,
        /// Encodes if the indices size does not match the sum of the face
        /// vertex counts array
        InvalidFaceVertexIndicesSize,
    };
    /// A tuple containing a code describing an invalidation and a descriptive
    /// message
    struct Invalidation {
        Code code;
        std::string message;
    };
private:
    // TODO: In C++17, this class is uncessary and should be replaced with
    // std::optional<std::vector<Invalidation>>
    class _OptionalInvalidationVector {
        std::unique_ptr<std::vector<Invalidation>> _value;

    public:
        _OptionalInvalidationVector() = default;
        _OptionalInvalidationVector(_OptionalInvalidationVector&&) = default;
        _OptionalInvalidationVector& operator=(_OptionalInvalidationVector&&) =
            default;
        _OptionalInvalidationVector(_OptionalInvalidationVector const& other)
            : _value(nullptr) {
            if (other._value) {
                _value.reset(new std::vector<Invalidation>(*other._value));
            }
        }
        _OptionalInvalidationVector& operator=(
            _OptionalInvalidationVector const& other) {
            _value = nullptr;
            if (other._value) {
                _value.reset(new std::vector<Invalidation>(*other._value));
            }
            return *this;
        }
        void emplace() { _value.reset(new std::vector<Invalidation>); }
        explicit operator bool() const { return _value != nullptr; }
        std::vector<Invalidation>& value() {
            TF_DEV_AXIOM(*this);
            return *_value;
        }
        std::vector<Invalidation> const& value() const {
            TF_DEV_AXIOM(*this);
            return *_value;
        }
    };

    _OptionalInvalidationVector _invalidations;
    template <size_t S>
    void _ValidateToken(PxOsdMeshTopologyValidation::Code code,
                        const char* name, const TfToken& token,
                        const std::array<TfToken, S>& validTokens);
    /// initializes the vector if necessary
    void _AppendInvalidation(const Invalidation& invalidation) {
        if (!_invalidations) {
            _invalidations.emplace();
        }
        _invalidations.value().push_back(invalidation);
    }
    PxOsdMeshTopologyValidation(PxOsdMeshTopology const&);

public:
    PxOsdMeshTopologyValidation() = default;
    PxOsdMeshTopologyValidation(PxOsdMeshTopologyValidation&&) = default;
    PxOsdMeshTopologyValidation& operator=(PxOsdMeshTopologyValidation&&) =
        default;
    PxOsdMeshTopologyValidation(PxOsdMeshTopologyValidation const& other) =
        default;
    PxOsdMeshTopologyValidation& operator=(
        PxOsdMeshTopologyValidation const& other) = default;

    /// Return true if the topology is valid
    explicit operator bool() const {
        return !_invalidations || _invalidations.value().empty();
    }

    using iterator = std::vector<Invalidation>::const_iterator;
    using const_iterator = std::vector<Invalidation>::const_iterator;

    /// Returns an iterator for the beginning of the invalidation vector
    /// if it has been initialized. Otherwise, returns an empty iterator.
    const_iterator begin() const {
        return _invalidations ? _invalidations.value().cbegin()
                              : const_iterator();
    }
    /// Returns an iterator for the end of the invalidation vector
    /// if it has been initialized. Otherwise, returns an empty iterator.
    const_iterator end() const {
        return _invalidations ? _invalidations.value().cend()
                              : const_iterator();
    }

    /// Returns an iterator for the beginning of the invalidation vector
    /// if it has been initialized. Otherwise, returns an empty iterator.
    const_iterator cbegin() const {
        return _invalidations ? _invalidations.value().cbegin()
                              : const_iterator();
    }
    /// Returns an iterator for the end of the invalidation vector
    /// if it has been initialized. Otherwise, returns an empty iterator.
    const_iterator cend() const {
        return _invalidations ? _invalidations.value().cend()
                              : const_iterator();
    }
private:
    void _ValidateScheme(PxOsdMeshTopology const&);
    void _ValidateOrientation(PxOsdMeshTopology const&);
    void _ValidateTriangleSubdivision(PxOsdMeshTopology const&);
    void _ValidateVertexInterpolation(PxOsdMeshTopology const&);
    void _ValidateFaceVaryingInterpolation(PxOsdMeshTopology const&);
    void _ValidateCreaseMethod(PxOsdMeshTopology const&);
    void _ValidateCreasesAndCorners(PxOsdMeshTopology const&);
    void _ValidateHoles(PxOsdMeshTopology const&);
    void _ValidateFaceVertexCounts(PxOsdMeshTopology const&);
    void _ValidateFaceVertexIndices(PxOsdMeshTopology const&);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
