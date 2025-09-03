//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SUBRANGE_VIEW_H
#define PXR_EXEC_VDF_SUBRANGE_VIEW_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/exec/vdf/vectorSubrangeAccessor.h"

#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class VdfContext;
template<typename> class VdfSubrangeView;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSubrangeView
/// 
/// This class enables iteration over subranges of input values, where each
/// subrange contains values originating from one unique topological source.
/// These data sources may be from unique outputs in the network, or from
/// unique sources that have been combined into a single boxed value.
/// 
/// For example, when iterating over values on an input that is connected to
/// multiple outputs, VdfReadIterator visits these values in sequential order.
/// The VdfReadIterator does not differentiate between the multiple data
/// sources, so long as there is a dependency on the input values that these
/// data sources provide. Usually, this is the desired behavior. Sometimes,
/// however, the client code may want to differentiate between the values based
/// on the data source. This is important, for example, when the client code
/// wants to associate input values of variable length, provided on two or
/// more inputs that source from the same number of unique data sources.
/// 
/// The VdfSubrangeView provides an iterator range for each unique data source.
/// It may be used like this:
/// 
/// ```{.cpp}
/// 
/// VdfSubrangeView<VdfReadIteratorRange<double>> subranges(ctx, _tokens->in);
/// for (const VdfReadIteratorRange<double> &subrange : subranges) {
///     DoSomethingWithData(subrange.begin(), subrange.end());
/// }
/// 
/// ```
///
template<typename T>
class VdfSubrangeView<VdfReadIteratorRange<T>> : public VdfIterator
{
public:

    /// The type of the subrange to iterator over.
    ///
    using Subrange = VdfReadIteratorRange<T>;

    /// Constructs a subrange view of the input values on the specified
    /// \p inputName.
    ///
    VdfSubrangeView(const VdfContext &context, const TfToken &inputName) :
        _context(&context),
        _inputName(inputName)
    {}

    /// The iterator representing an individual subrange of input values.
    ///
    class const_iterator
    {
    public:

        /// Type of the elements this iterator gives access to.
        ///
        using value_type = const Subrange;

        /// Type of a reference to a value of this iterator.
        ///
        using reference = value_type &;

        /// Type of a pointer to a value of this iterator.
        ///
        using pointer = value_type *;

        /// The STL category of this iterator type.
        ///
        using iterator_category = std::forward_iterator_tag;

        /// Returns \c true if this iterator and \p rhs compare equal.
        ///
        bool operator==(const const_iterator &rhs) const;

        /// Returns \c true if this iterator and \p rhs do not compare equal.
        ///
        bool operator!=(const const_iterator &rhs) const {
            return !operator==(rhs);
        }

        /// Increment the iterator to make it point at the next subrange of
        /// input values.
        ///
        const_iterator &operator++();

        /// Returns a reference to the current subrange of input values.
        ///
        reference operator*() const {
            return _subrange;
        }

        /// Returns a pointer to the current subrange of input values.
        ///
        pointer operator->() const {
            return &_subrange;
        }
        
    private:

        // Only VdfSubrangeView is allowed to construct instances of this class.
        friend class VdfSubrangeView;

        // Construct an iterator owned by the specified view at the given
        // connection index. Constructs an iterator at-end if connectionIndex is
        // less than 0.
        const_iterator(const VdfSubrangeView &view, int connectionIndex);

        // Sets the subrange from the currently set connection- and range index.
        void _AdvanceSubrange(unsigned int rangeIndex);

        // Set the subrange from the currently set connection.
        bool _SubrangeFromConnectionIndex(
            const VdfContext &context,
            const TfToken &inputName);

        // Sets the subrange from the currently set range index.
        bool _SubrangeFromRangeIndex(
            const VdfContext &context,
            const TfToken &inputName,
            const Vdf_VectorSubrangeAccessor<T> &accessor);

        // Returns a subrange that is at-end, i.e. an empty range with both
        // begin and end iterators at-end.
        Subrange _SubrangeAtEnd();

        // Advances this iterator to the end.
        void _AdvanceToEnd();

        // The view owning this iterator.
        const VdfSubrangeView *_view;

        // The current connection index.
        int _connectionIndex;

        // The current range index within the connection.
        unsigned int _rangeIndex;

        // The current iterator subrange.
        Subrange _subrange;

    };

    /// Returns a subrange iterator at the beginning of the subrange, i.e. the
    /// first range of input values.
    ///
    const_iterator begin() const {
        return const_iterator(*this, 0);
    }

    /// Returns a subrange iterator at the end of the subrange, i.e. the element
    /// after the last range of input values.
    ///
    const_iterator end() const {
        return const_iterator(*this, -1);
    }

private:

    // A pointer to the context instance.
    const VdfContext *_context;

    // The name token of the input to build subranges for. Must extend the
    // lifetime of the token.
    const TfToken _inputName;

};

////////////////////////////////////////////////////////////////////////////////

template<typename T>
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::const_iterator(
    const VdfSubrangeView &view,
    int connectionIndex) :
    _view(&view),
    _connectionIndex(connectionIndex),
    _rangeIndex(0),
    _subrange(_SubrangeAtEnd())
{
    // If we have a valid connection index, advance to the first valid subrange.
    if (connectionIndex >= 0) {
        _AdvanceSubrange(0);
    }
}

template<typename T>
bool
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::operator==(
    const const_iterator &rhs) const
{
    return
        _view->_inputName == rhs._view->_inputName &&
        _connectionIndex == rhs._connectionIndex &&
        _rangeIndex == rhs._rangeIndex;
}

template<typename T>
typename VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator &
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::operator++()
{
    // Advance to the next subrange.
    _AdvanceSubrange(_rangeIndex + 1);
    return *this;
}

template<typename T>
void
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::_AdvanceSubrange(
    unsigned int rangeIndex)
{
    const VdfContext &context = *_view->_context;
    const TfToken &inputName = _view->_inputName;
    const VdfInput *input = _view->_GetNode(context).GetInput(inputName);

    // Set the next range index.
    _rangeIndex = rangeIndex;

    // If we have a valid input and connection index, set the current subrange.
    if (input && _connectionIndex >= 0) {

        // Start with the current connection, and keep moving on to the next
        // connection until we have found the current subrange, or have reached
        // the last connection.
        const int numConnections = input->GetNumConnections();
        for (; _connectionIndex < numConnections; ++_connectionIndex) {

            // Get the connection and mask.
            const VdfConnection &c = (*input)[_connectionIndex];
            const VdfMask &mask = c.GetMask();

            // If the mask is all zeros or if the connected output is not
            // required, move on to the next connection.
            if (mask.IsAllZeros() || !_view->_IsRequiredInput(context, c)) {
                continue;
            }

            // If the connected output does not provide a value, try to set
            // an empty subrange from the current connection.
            const VdfVector *v = _view->_GetInputValue(context, c, mask);
            if (!v) {
                if (_SubrangeFromConnectionIndex(context, inputName)) {
                    return;
                }
            } else {
                const Vdf_VectorSubrangeAccessor<T> accessor =
                    v->GetSubrangeAccessor<T>();
    
                // If the current connection does not provide a boxed value, try
                // to set the subrange from the current connection. 
                if (!accessor.IsBoxed()) {
                    if (_SubrangeFromConnectionIndex(context, inputName)) {
                        return;
                    }
                }
    
                // If the current connection provides a boxed value, try to set
                // the subrange from the boxed container provided on the current
                // connection.
                else {
                    if (_SubrangeFromRangeIndex(context, inputName, accessor)) {
                        return;
                    }
                }
            }

            // The subrange is not on the current connection. Reset the range
            // index and move on to the next connection.
            _rangeIndex = 0;
        }
    }

    // If we have not found a single valid connection, there are no more
    // subranges. We have reached the end.
    _AdvanceToEnd();
}

template<typename T>
bool
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::
    _SubrangeFromConnectionIndex(
        const VdfContext &context,
        const TfToken &inputName)
{
    using Iterator = typename Subrange::iterator;

    // If the current range index exceeds the number of ranges provided on this
    // connection (non-boxed values only provide a single range), we need to
    // move on to the next connection.
    if (_rangeIndex > 0) {
        return false;
    }

    // Build an iterator range beginning at the current connection, and ending
    // at the next connection.
    _subrange = Subrange(
        Iterator(context, inputName, _connectionIndex, 0),
        Iterator(context, inputName, _connectionIndex + 1, 0));

    return true;
}

template<typename T>
bool
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::
    _SubrangeFromRangeIndex(
        const VdfContext &context,
        const TfToken &inputName,
        const Vdf_VectorSubrangeAccessor<T> &accessor)
{
    using Iterator = typename Subrange::iterator;

    // Get the boxed container provided by the current connection value.
    const Vdf_BoxedRanges &boxedRanges = accessor.GetBoxedRanges();

    // If the current range index exceeds the number of ranges provided on this
    // connection (boxed values can provide multiple ranges), we need to move
    // on to the next connection.
    if (_rangeIndex >= boxedRanges.GetNumRanges()) {
        return false;
    }

    // Get the boxed container range that corresponds to the current
    // range index.
    const Vdf_BoxedRanges::Range boxedRange = boxedRanges.GetRange(_rangeIndex);

    // Build an iterator range beginning at the current connection (offset by
    // the beginning of the boxed range), and ending at the current connection
    // (offset by the end of the boxed range.)
    _subrange = Subrange(
        Iterator(context, inputName, _connectionIndex, boxedRange.begin),
        Iterator(context, inputName, _connectionIndex, boxedRange.end));

    return true;
}

template<typename T>
typename VdfSubrangeView<VdfReadIteratorRange<T>>::Subrange
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::_SubrangeAtEnd()
{
    using Iterator = typename Subrange::iterator;

    return Subrange(
        Iterator(*_view->_context, _view->_inputName, -1, 0),
        Iterator(*_view->_context, _view->_inputName, -1, 0));
}

template<typename T>
void
VdfSubrangeView<VdfReadIteratorRange<T>>::const_iterator::_AdvanceToEnd()
{
    _connectionIndex = -1;
    _rangeIndex = 0;
    _subrange = _SubrangeAtEnd();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
