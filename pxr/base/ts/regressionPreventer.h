//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_REGRESSION_PREVENTER_H
#define PXR_BASE_TS_REGRESSION_PREVENTER_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/knotData.h"

#include <optional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;


/// An authoring helper class that enforces non-regression in splines.
///
/// See \ref page_ts_regression for a general introduction to regression and
/// anti-regression.
///
/// Construct an instance of this class when a knot is being interactively
/// edited.  Call Set for each change.
///
/// \bug This class does not yet work correctly with inner loops (TsLoopParams).
///
class TsRegressionPreventer
{
public:
    /// Anti-regression modes that are specific to interactive usage.  These are
    /// similar to the modes in \ref TsAntiRegressionMode, except the
    /// interactive modes differentiate between the 'active' and 'opposite'
    /// knots in each segment, favoring one or the other of them.  The 'active'
    /// knot is the one that is being edited in an interactive case.  Batch
    /// cases can't use these modes because we are adjusting an existing spline,
    /// rather than editing a single knot.
    enum TS_API InteractiveMode
    {
        /// Shorten the proposed tangents of the active knot so that there is no
        /// regression, leaving the neighbor tangents alone.
        ModeLimitActive = 100,

        /// When the oppposite tangent is > 1/3 of the interval, shorten it
        /// until non-regression is achieved or the opposite tangent reaches
        /// 1/3; then cap the active tangent at 4/3.
        ///
        /// When the opposite tangent is < 1/3 of the interval, just limit the
        /// active tangent.  This avoids the counter-intuitive result of
        /// lengthening the oposite tangent.
        ModeLimitOpposite
    };

    /// Details of the result of an interactive Set call.
    class SetResult
    {
    public:
        /// Whether any adjustments were made.
        bool adjusted = false;

        /// If there is a pre-segment, what adjustments were made to it.
        bool havePreSegment = false;
        bool preActiveAdjusted = false;
        TsTime preActiveAdjustedWidth = 0;
        bool preOppositeAdjusted = false;
        TsTime preOppositeAdjustedWidth = 0;

        /// If there is a post-segment, what adjustments were made to it.
        bool havePostSegment = false;
        bool postActiveAdjusted = false;
        TsTime postActiveAdjustedWidth = 0;
        bool postOppositeAdjusted = false;
        TsTime postOppositeAdjustedWidth = 0;

    public:
        TS_API
        std::string GetDebugDescription(int precision = 6) const;
    };

public:
    /// Constructor for interactive use (repeated calls to Set).  The mode will
    /// be determined by the value of TsSpline::GetAntiRegressionAuthoringMode()
    /// at the time of construction.  If 'limit' is true, adjustments will be
    /// enforced before knots are written to the spline.  Otherwise, knots will
    /// be written without adjustment, but the SetResult will describe the
    /// adjustments that would be made.  The spline must remain valid for the
    /// lifetime of this object.
    TS_API
    TsRegressionPreventer(
        TsSpline *spline,
        TsTime activeKnotTime,
        bool limit = true);

    /// Same as the above, but with an InteractiveMode.  This form ignores
    /// GetAntiRegressionAuthoringMode(), because interactive modes can't be
    /// specified through that mechanism, since they apply only to
    /// RegressionPreventer.
    TS_API
    TsRegressionPreventer(
        TsSpline *spline,
        TsTime activeKnotTime,
        InteractiveMode mode,
        bool limit = true);

    /// Set an edited version of the active knot into the spline, adjusting
    /// tangent widths if needed, based on the mode.  Any aspect of the active
    /// knot may be changed; the aspects that affect regression are knot time
    /// and tangent widths.  Returns true on success, false on failure.
    ///
    /// If this is the first call to Set, and the spline was initially
    /// regressive, the opposite tangent may be shortened, in a way that isn't
    /// required when the spline starts out non-regressive.  In Contain mode,
    /// this initial anti-regression will limit the opposite tangent following
    /// the usual Contain rules.  In any other mode, initial anti-regression
    /// will behave as though Limit Opposite were in effect: the opposite
    /// tangent will be shortened so that the spline is not regressive given the
    /// initial active knot, or to 1/3 of the interval if the active tangent is
    /// longer than 4/3 of the interval.
    ///
    /// When knot time is changed, the tangent widths in the altered segments on
    /// either side are adjusted to prevent regression.  If knot time is changed
    /// to match another existing knot, the prior knot is removed, and the
    /// active knot substituted for it; this is undone if the time is again
    /// changed.  If knot time changes enough to alter the sort order of knots
    /// in the spline, the active knot's neighbor knots will be recomputed for
    /// the new insert point, and the resulting new segments will be adjusted to
    /// prevent regression as needed.
    ///
    /// When a loop-prototype knot is being edited, the spline's loop parameters
    /// may fall out of sync if the knot time is changed.  This can include the
    /// knot drifting out of the prototype interval and becoming hidden; it can
    /// also include the prototype interval bounds failing to track the first or
    /// last prototype knot as it moves.  Clients should make policy as to how
    /// this situation should be handled.  If loop parameters are going to be
    /// updated to match moved knots, that edit should be done before calling
    /// Set.
    ///
    TS_API
    bool Set(
        const TsKnot &proposedActiveKnot,
        SetResult *resultOut = nullptr);

private:
    friend struct Ts_RegressionPreventerBatchAccess;

    // Unified enum for both interactive and batch use.
    enum _Mode
    {
        _ModeNone = TsAntiRegressionNone,
        _ModeContain = TsAntiRegressionContain,
        _ModeKeepRatio = TsAntiRegressionKeepRatio,
        _ModeKeepStart = TsAntiRegressionKeepStart,
        _ModeLimitActive = ModeLimitActive,
        _ModeLimitOpposite = ModeLimitOpposite
    };

    // Private constructor to which the public constructors delegate.
    TsRegressionPreventer(
        TsSpline *spline,
        TsTime activeKnotTime,
        _Mode mode,
        bool limit);

    // NOTE: we store knot data in two different ways.  When we need access to
    // all the knot data, suitable for setting into the spline, we store a
    // TsKnot, which has a copy of all the data, including typed data and custom
    // data.  When we only need access to a copy of the time parameters, we
    // store an un-subclassed Ts_KnotData.

    // PERFORMANCE NOTE: this class would probably be faster if it dealt
    // directly with Ts_SplineData and Ts_KnotData, rather than going through
    // TsSpline and TsKnot.

    // Knot state stored for the lifetime of an interactive Preventer.  Tracks
    // the original knot from construction time, and the current time parameters
    // in the spline.
    struct _KnotState
    {
    public:
        // Uses the original value for both 'original' and 'current'.
        _KnotState(
            TsSpline *spline,
            const TsKnot &originalKnot);

        // Write the original back to the spline, undoing any prior writes.
        void RestoreOriginal();

        // Remove the knot from the spline.  This is needed when knot time is
        // changing.
        void RemoveCurrent();

        // Write a new version of the knot, and record it as 'current'.
        void Write(
            const TsKnot &newKnot);

    public:
        // The spline, so we can write into it.
        TsSpline* const spline;

        // Original knot.
        const TsKnot originalKnot;

        // Current time parameters, possibly modified from original.
        Ts_KnotData currentParams;
    };

    // Knot state used for the duration of a single Preventer iteration (Set or
    // _ProcessSegment).  Tracks the proposed new knot, and a potentially
    // adjusted working version of the time parameters.
    struct _WorkingKnotState
    {
    public:
        // Uses the proposed value for 'proposed' and 'working'.  This is for
        // interactive use with active knots, for which a proposed new value is
        // given as input.
        _WorkingKnotState(
            _KnotState *parentState,
            const TsKnot &proposedKnot);

        // Uses the parent's original for 'proposed' and 'working'.  This is for
        // interactive use with opposite knots, which always start out proposed
        // as the original knots.
        _WorkingKnotState(
            _KnotState *parentState);

        // For batch use.  Stores only the proposed time parameters.  Has no
        // parent state, and cannot be used to write to the spline.  The only
        // output is 'working'.
        _WorkingKnotState(
            const Ts_KnotData &original);

        // Write the proposed value to the spline, without adjustment.  Update
        // the parent state's 'current'.
        void WriteProposed();

        // Write the possibly adjusted value to the spline.  Update the parent
        // state's 'current'.
        void WriteWorking();

    public:
        // Link to whole-operation state.
        _KnotState* const parentState;

        // Proposed knot.
        const TsKnot proposedKnot;

        // Copy of time parameters that we are modifying.
        Ts_KnotData workingParams;
    };

    // Encapsulates the core math, and the details specific to whether we're
    // operating on a pre-segment (the one before the active knot) or a
    // post-segment (the one after).
    class _SegmentSolver
    {
    public:
        enum WhichSegment
        {
            PreSegment,
            PostSegment
        };

        _SegmentSolver(
            WhichSegment whichSegment,
            _Mode mode,
            _WorkingKnotState *activeKnotState,
            _WorkingKnotState *oppositeKnotState,
            SetResult *result);

        // If adjustments are needed, update activeKnotState->working,
        // oppositeKnotState->working, and *result.  Does not immediately write
        // to the spline.  Returns true on success, false on failure.
        bool Adjust();

    private:
        // Mode kernels.
        bool _AdjustWithContain();
        bool _AdjustWithKeepRatio();
        bool _AdjustWithKeepStart();
        bool _AdjustWithLimitActive();
        bool _AdjustWithLimitOpposite();

        // Accessors and mutators for the active and opposite tangent widths.
        // The widths passed and returned here are always normalized to the
        // [0, 1] segment time interval.
        TsTime _GetProposedActiveWidth() const;
        TsTime _GetProposedOppositeWidth() const;
        void _SetActiveWidth(TsTime width);
        void _SetOppositeWidth(TsTime width);

        // Like the above, but for asymmetrical algorithms that differentiate
        // between start and end knots rather than active and opposite.
        TsTime _GetProposedStartWidth() const;
        TsTime _GetProposedEndWidth() const;
        void _SetStartWidth(TsTime width);
        void _SetEndWidth(TsTime width);

        // Plumbing helpers.
        TsTime _GetSegmentWidth() const;

    private:
        const WhichSegment _whichSegment;
        const _Mode _mode;
        _WorkingKnotState* const _activeKnotState;
        _WorkingKnotState* const _oppositeKnotState;
        SetResult* const _result;
    };

private:
    // Set() helpers.
    void _InitSetResult(
        const TsKnot &proposedActiveKnot,
        SetResult *resultOut) const;
    void _HandleInitialAdjustment(
        const TsKnot &proposedActiveKnot,
        SetResult* resultOut);
    void _HandleTimeChange(TsTime proposedActiveTime);
    void _DoSet(
        const TsKnot &proposedActiveKnot,
        _Mode mode,
        SetResult* resultOut);

private:
    TsSpline* const _spline;
    const _Mode _mode;
    const bool _limit;

    bool _valid;
    bool _initialAdjustmentDone;

    std::optional<_KnotState> _activeKnotState;
    std::optional<_KnotState> _preKnotState;
    std::optional<_KnotState> _postKnotState;
    std::optional<_KnotState> _overwrittenKnotState;
};


struct Ts_RegressionPreventerBatchAccess
{
    // Batch operation for one segment of a spline.  In Contain mode, this
    // method returns true for "bold" tangents that are non-regressive but
    // exceed the segment interval.
    static bool IsSegmentRegressive(
        const Ts_KnotData *startKnot,
        const Ts_KnotData *endKnot,
        TsAntiRegressionMode mode);

    // Batch operation for one segment of a spline.  Returns whether anything
    // was changed.
    static bool ProcessSegment(
        Ts_KnotData *startKnot,
        Ts_KnotData *endKnot,
        TsAntiRegressionMode mode);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
