//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_WEIGHTED_ITERATOR_H
#define PXR_EXEC_VDF_WEIGHTED_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/indexedWeights.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/diagnostic.h"

#include <cstddef>
#include <initializer_list>

PXR_NAMESPACE_OPEN_SCOPE

/// The information held per weight slot in a weighted iterator.
///
struct Vdf_WeightSlot
{
    /// The vector of weights we are iterating over.
    const VdfIndexedWeights *weights;

    /// The current iterator index into the VdfIndexedWeights above.
    size_t currentIndex;
};

/// Array of weight slots for weighted iterators.
///
/// Inline storage is provided for one weight slot to avoid heap allocation in
/// the common case.  However, the local storage and remote storage pointer are
/// not overlapped as this results in undesirable overhead when accessing
/// elements.  Iterators are almost always stack allocated and
/// VdfWeightedIterator is non-copyable making compactness less valuable.
///
class Vdf_WeightSlotArray
{
    Vdf_WeightSlotArray(const Vdf_WeightSlotArray &) = delete;
    Vdf_WeightSlotArray& operator=(const Vdf_WeightSlotArray &) = delete;

public:
    using value_type = Vdf_WeightSlot;
    using reference = Vdf_WeightSlot &;
    using const_reference = const Vdf_WeightSlot &;
    using iterator = Vdf_WeightSlot *;
    using const_iterator = const Vdf_WeightSlot *;

    /// Construct an empty array.
    ///
    Vdf_WeightSlotArray()
        : _begin(nullptr)
        , _end(nullptr)
    {}

    /// Destructor.
    ///
    ~Vdf_WeightSlotArray() {
        if (_begin != _local) {
            delete[] _begin;
        }
    }

    /// Allocate storage for \p numInputs elements.
    ///
    /// This function may only be called once during the lifetime of the
    /// array.  The size of the array is fixed the first time it is allocated.
    /// Elements in the array are uninitialized.
    ///
    void Allocate(uint32_t numInputs) {
        if (!TF_VERIFY(!_begin && !_end)) {
            return;
        }

        _begin = numInputs > NUM_LOCAL_STORAGE
            ? new value_type[numInputs]
            : _local;
        _end = _begin + numInputs;
    }

    /// Return the number of slots in the array.
    ///
    size_t size() const {
        return _end - _begin;
    }

    /// Return an iterator to the first slot in the array.
    ///
    iterator begin() {
        return _begin;
    }

    /// Return an iterator to the end of the array.
    ///
    iterator end() {
        return _end;
    }

    /// Return an iterator to the first slot in the array.
    ///
    const_iterator begin() const {
        return _begin;
    }

    /// Return an iterator to the end of the array.
    ///
    const_iterator end() const {
        return _end;
    }

    /// Access the slot at index \p slot.
    //
    reference operator[](size_t slot) {
        return _begin[slot];
    }

    /// Access the slot at index \p slot.
    //
    const_reference operator[](size_t slot) const {
        return _begin[slot];
    }

    /// Returns the next index that has a weight at or after \p index.
    /// Updates the currentIndex.
    ///
    int AdvanceToNextExplicitIndex(int index);

private:
    value_type *_begin;
    value_type *_end;

    // The amount of local storage reserved for slots.
    // A single slot is stored locally, multiple slots are stored remotely.
    static const size_t NUM_LOCAL_STORAGE = 1;

    value_type _local[NUM_LOCAL_STORAGE];
};


////////////////////////////////////////////////////////////////////////////////
///
/// This iterator can be used to iterate through an input that is weighted by
/// one or more weight vectors.
///
template<class IteratorType>
class VdfWeightedIterator final : public VdfIterator 
{
public:

    /// Type of the elements this iterator gives access to.
    ///
    typedef typename IteratorType::value_type value_type;

    /// Type of a reference to a value of this iterator.
    ///
    typedef typename IteratorType::reference reference;

    /// Constructs a weighted iterator using a single weight name.
    ///
    template<typename... Args>
    VdfWeightedIterator(
        const VdfContext &context,
        const TfToken &weightName,
        Args&&... args) :
        _iterator(context, std::forward<Args>(args)...) {
        const TfToken *weightNamePtr = &weightName;
        _Init(context, weightNamePtr, weightNamePtr + 1);
    }

    /// Constructs a weighted iterator using an initializer list for weight 
    /// names. 
    ///
    template<typename... Args>
    VdfWeightedIterator(
        const VdfContext &context,
        std::initializer_list<TfToken> weightNames,
        Args&&... args) :
        _iterator(context, std::forward<Args>(args)...) {
        _Init(context, weightNames.begin(), weightNames.end());    
    }

    /// Constructs a weighted iterator using a vector for weight names. 
    ///
    template<typename... Args>
    VdfWeightedIterator(
        const VdfContext &context,
        const std::vector<TfToken> &weightNames,
        Args&&... args) :
        _iterator(context, std::forward<Args>(args)...) {
        const TfToken *weightNamesBegin = weightNames.data();
        const TfToken *weightNamesEnd = weightNamesBegin + weightNames.size();
        _Init(context, weightNamesBegin, weightNamesEnd);
    }

    /// Destructor.
    ///
    ~VdfWeightedIterator() = default;

    /// Increment operator to point to the next element.
    ///
    VdfWeightedIterator &operator++();

    /// Returns reference to current element.
    ///
    reference operator*() const {
        return _iterator.operator*();
    }

    /// Returns true if the iterator is done iterating and false otherwise.
    ///
    bool IsAtEnd() const {
        return _iterator.IsAtEnd();
    }

    /// Returns the current index for the current connection.
    /// 
    /// This method should not generally be used.
    ///
    int GetCurrentIndex() const { 
        return Vdf_GetIteratorIndex(_iterator);
    }

    /// Advance the iterator to the end.
    ///
    void AdvanceToEnd() {
        _iterator.AdvanceToEnd();
    }

    /// Returns the weight at the current element.  If no weight is 
    /// explicitly present at the given \p slot, \p defWeight (default is 0.0)
    /// is returned.
    ///
    double GetWeight(size_t slot = 0, double defWeight = 0.0) const {
        _GetExplicitWeight(slot, &defWeight);
        return defWeight;
    }

    /// Returns true if the weight at the current element is explicitly set
    /// at \p slot.
    ///
    bool HasExplicitWeight(size_t slot) const {
        double ret;
        return _GetExplicitWeight(slot, &ret);
    }

    /// Returns a pair (bool, double) indicating whether there is a weight
    /// explicitly present at the given \p slot and giving the weight or the
    /// given default weight as fallback.
    ///
    std::pair<bool, double> GetExplicitWeight(
        size_t slot = 0, double defWeight = 0.0) const;

    /// Get the number of weight slots used.
    ///
    size_t GetNumSlots() const {
        return _slots.size();
    }

    /// Returns the # of explicit weights for \p slot.
    ///
    size_t GetNumExplicitWeights(size_t slot = 0) const {
        return (slot < _slots.size()) ?
            _slots[slot].weights->GetSize() : 0;
    }

private:

    // Noncopyable
    VdfWeightedIterator(const VdfWeightedIterator &) = delete;
    VdfWeightedIterator &operator=(const VdfWeightedIterator &) = delete;

    // Returns the current index into the data source.
    friend int Vdf_GetIteratorIndex(const VdfWeightedIterator &it) {
        return Vdf_GetIteratorIndex(it._iterator);
    }

    // Two constructors above call this shared code upon construction
    void _Init(
        const VdfContext &context,
        const TfToken *begin,
        const TfToken *end);

    // Extracts weighted data from weightInput and adds it to our vector
    // of weight vectors.
    void _AddWeightsInput(
        Vdf_WeightSlot *slot,
        const VdfInput *weightInput,
        const VdfContext &context);

    // Returns true and the weigth in *ret if the weight is explicity set for
    // this slot, otherwise, it returns false.
    bool _GetExplicitWeight(size_t slot, double *ret) const;

    // Implements the template parameter specified behavior
    // 
    // The affects mask iterator is advanced to the next explicit weight
    // only if the template parameter is SkipElementsWithNoWeights.
    // The currentIndex is always updated.
    // 
    // Advances the current _iterator to the first index where we have
    // both a weight explicity set and an element set in the mask.
    void _AdvanceIterator();

private:

    // The underlying iterator.
    IteratorType _iterator;
    Vdf_WeightSlotArray _slots;
};

////////////////////////////////////////////////////////////////////////////////

template<class IteratorType>
void
VdfWeightedIterator<IteratorType>::_Init(
    const VdfContext &context,
    const TfToken *begin,
    const TfToken *end)
{
    // If there's nothing set in the mask, there's no need to go on.
    if (_iterator.IsAtEnd())  {
        return;
    }

    // Get the number of slots.
    const std::ptrdiff_t numInputs = std::distance(begin, end);

    // Reserve storage for the slots, if required.
    if (numInputs > 0) {
        _slots.Allocate(numInputs);
    }

    // Add all the weights inputs.
    Vdf_WeightSlot *slot = _slots.begin();
    const VdfNode &node = _GetNode(context);
    for (const TfToken *it = begin; it != end; ++it, ++slot) {
        const VdfInput *input = node.GetInput(*it);
        if (!input) {
            TF_CODING_ERROR(
                "Can't find input '%s' on node %s",
                it->GetText(),
                node.GetDebugName().c_str());
        }
        _AddWeightsInput(slot, input, context);
    }

    // No inputs.
    if (numInputs == 0) {
        TF_CODING_ERROR("Weighted Iterator instantiated with no weights.");
        return;
    }   
    
    // By here, *_iterator has been initialized with the first set mask.
    // What we'd like to do is advance it to the next index such that
    // we have both an explicit weight and a set element in the mask.
    _AdvanceIterator();
}

template<class IteratorType>
void
VdfWeightedIterator<IteratorType>::_AddWeightsInput(
    Vdf_WeightSlot *slot,
    const VdfInput *weightInput,
    const VdfContext &context)
{
    // We always expect exactly one input connection.
    if (weightInput && weightInput->GetNumConnections() == 1) {
        const VdfConnection &connection  = (*weightInput)[0];
        const VdfVector &out = _GetRequiredInputValue(
            context, connection, connection.GetMask());

        // We always expect exactly one element.
        if (ARCH_LIKELY(out.GetSize() == 1)) {
            // Note that here we hold on to a pointer from the executor because
            // we don't want to copy the vector of weights.  It is okay to hold
            // on to the pointer only because the lifetime of this iterator is
            // limited.
            slot->weights = &out.GetReadAccessor<VdfIndexedWeights>()[0];
            slot->currentIndex = 0;
            return;
        }
    
        TF_CODING_ERROR("Weight input must have exactly one element (got %zu)",
                        out.GetSize());
    }

    else if (weightInput && weightInput->GetNumConnections() > 1) {
        // This is an error, all weight connectors must have exactly one input.
        TF_CODING_ERROR("Weight connector must have at most one input (got %zu)",
                        weightInput->GetNumConnections());
    }

    // Not exactly one input connection and data element.
    slot->weights = nullptr;
    slot->currentIndex = 0;
}

template<class IteratorType>
VdfWeightedIterator<IteratorType> &
VdfWeightedIterator<IteratorType>::operator++()
{
    // We need to differentiate between two cases here: 
    //
    // a) There can be holes in the mask _iterator works on, we want to skip
    //    those fast.
    // b) There can be holes in the explicit weights, which we also want to 
    //    skip fast.

    // Advance iterator to the next element as indicated by mask, this may
    // or may not skip holes in the mask.

    _iterator.operator++();
    _AdvanceIterator();

    return *this;
}

template<class IteratorType>
void
VdfWeightedIterator<IteratorType>::_AdvanceIterator()
{
    while (!_iterator.IsAtEnd()) {

        // Find the next index that has an explicit weight after currentIndex
        const int currentIndex = Vdf_GetIteratorIndex(_iterator);
        const int nextExplicitIndex =
            _slots.AdvanceToNextExplicitIndex(currentIndex);

        // If there are no more explicit weights, we're done.
        if (nextExplicitIndex == std::numeric_limits<int>::max()) {
            // We're done iterating, set _iterator to end().
            _iterator.AdvanceToEnd();
            break;

        } else if (nextExplicitIndex != currentIndex) {

            // In this case the next explicit weight was further along than
            // our iterator.  So let's try to advance the iterator.
            while (!_iterator.IsAtEnd() &&
                Vdf_GetIteratorIndex(_iterator) < nextExplicitIndex) {
                ++_iterator;
            }

            if (_iterator.IsAtEnd() ||
                Vdf_GetIteratorIndex(_iterator) == nextExplicitIndex) {
                // Great!  Both the iterator and the next explicit weights
                // have a value, or we have reached the end of iterator.  We're
                // done.
                break;
            }

            // If we get here, the iterator did not visit an element at the
            // explicit index, and we have now advanced it beyond the explicit
            // index. Let's retry at the current index.

        } else {
            // There is a next explicit weight at our current iterator index. 
            // We're done. 
            break;
        }
    }
}

inline int 
Vdf_WeightSlotArray::AdvanceToNextExplicitIndex(
    int index)
{
    int nextExplicitIndex = std::numeric_limits<int>::max();

    // Iterate over all the weight slots.
    for (Vdf_WeightSlot &p : *this) {
        const VdfIndexedWeights *w = p.weights;

        // If we're done with that slot, don't bother trying to find one.
        if (ARCH_UNLIKELY(!w || p.currentIndex >= w->GetSize())) {
            continue;
        }

        TF_DEV_AXIOM(w);

        // We'll try to do a quick local search from our last known
        // position, for speed.
        p.currentIndex = w->GetFirstDataIndex(index, p.currentIndex);
        if (p.currentIndex < w->GetSize()) {
            // We already found the next index that we care about, we
            // won't do any more work than is necessary.
            int wIndexVal = w->GetIndex(p.currentIndex);
            if (wIndexVal < nextExplicitIndex) {
                nextExplicitIndex = wIndexVal;
            }
        }
    }

    return nextExplicitIndex;
}

template<class IteratorType>
std::pair<bool, double>
VdfWeightedIterator<IteratorType>::GetExplicitWeight(
    size_t slot, double defWeight) const
{
    bool hasExplicitWeight = _GetExplicitWeight(slot, &defWeight);
    return std::pair<bool, double>(hasExplicitWeight, defWeight);
}

template<class IteratorType>
inline bool
VdfWeightedIterator<IteratorType>::_GetExplicitWeight(
    size_t slot, double *ret) const
{
    TF_DEV_AXIOM(ret);

    if (slot < _slots.size()) {
        const Vdf_WeightSlot &p = _slots[slot];
        if (p.weights &&
            p.currentIndex < p.weights->GetSize() &&
            p.weights->GetIndex(p.currentIndex) ==
                Vdf_GetIteratorIndex(_iterator)) {
            *ret = p.weights->GetData(p.currentIndex);
            return true;
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
