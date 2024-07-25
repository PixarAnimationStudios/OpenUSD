//
// Copyright 2024 Pixar
//

#include "pxr/base/ts/regressionPreventer.h"

#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/typeHelpers.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/diagnostic.h"

#include <cmath>
#include <sstream>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE


// NOTE TO MAINTAINERS
//
// Be sure to read doxygen/regression.md, which is intended for callers, but
// which summarizes the problem and has some helpful pictures.
//
// In addition to testTsRegressionPreventer, be sure to try
// script/regressDemo.py for an interactive demo of this implementation.

// OVERVIEW
//
// Beziers are parametric: the time function is x(t), where t is the parameter
// value, and x is what we call time to avoid confusion with t.
//
// When the time function has two zero derivatives, there are two vertical
// tangents in the segment, and the curve goes backward between them.  When the
// time function has a single zero derivative, there is one vertical tangent in
// the segment, and the curve never goes backward.  When the time function has
// no zero derivatives, it is monotonically increasing, and the curve never goes
// backward.
//
// We can detect regression by the presence of double verticals.  We can also
// minimally fix regression by shortening knot tangents, collapsing the double
// vertical to a single vertical.

// CONVENTIONS
//
// We work with a normalized time interval [0, 1].  We solve for the endpoints
// of the two knot tangents, which we call startPos and endPos.  We call the
// four Bezier weights [x0, x1, x2, x3].  In our normalized interval, x0 is
// always 0, and x3 is always 1.  x1 and x2 are synonyms for startPos and
// endPos.  We also sometimes represent knot tangents by their lengths instead
// of their endpoints; we say L1 = x1 and L2 = 1 - x2.

// BACKGROUND: BEZIER FORMULAS
//
// The Bezier formula, in power form, with weights [x0 x1 x2 x3], is:
//
//   x(t) = (-x0 + 3x1 - 3x2 + x3) t^3
//          + (3x0 - 6x1 + 3x2) t^2
//          + (-3x0 + 3x1) t
//          + x0
//
// Normalizing the interval to [0, 1], this becomes:
//
//   x(t) = (3x1 - 3x2 + 1) t^3
//          + (-6x1 + 3x2) t^2
//          + (3x1) t
//
// The first derivative, by the power rule, is:
//
//   x'(t) = (9x1 - 9x2 + 3) t^2
//           + (-12x1 + 6x2) t
//           + 3x1

// THE ELLIPSE
//
// We can characterize the conditions under which a (startPos, endPos) pair will
// result in a single vertical as follows.  Start with the formula for x'(t)
// above; find the quadratic discriminant b^2 - 4ac, insisting that it be zero,
// yielding one real root.  After simplification, this yields:
//
//   x1^2 - x2^2 - x1x2 - x1 = 0
//     or
//   L1^2 + L2^2 + L1L2 - 2L1 - 2L2 + 1 = 0
//
// This equation is computed by the function _AreTanWidthsRegressive.
//
// If the discriminant is less than zero, there are no verticals and thus no
// regression.  If the discriminant is greater than zero, there are two
// verticals and thus regression.
//
// If we graph the zero-discriminant equation in L1/L2 space, we get an ellipse
// with:
//
//   - A: L1 minimum at (0, 1)
//   - B: L2 maximum at (1/3, 4/3)
//   - C: L1/L2 balance at (1, 1)
//   - D: L1 maximum at (4/3, 1/3)
//   - E: L2 minimum at (1, 0)
//
// See doxygen/regression.md for an illustration.
//
// The ellipse has these regions:
//
//   - [B, D]: the 'center', between the maxima, inclusive.  To move along the
//     ellipse edge in the center region, we lengthen one knot tangent and
//     shorten the other.  The resulting single verticals cover the time range
//     [1/9, 8/9].
//
//   - (A, B) and (D, E): the 'fringes', between the minima and maxima,
//     exclusive.  To move along the ellipse edge in a fringe region, we
//     lengthen or shorten both knot tangents.  The resulting single verticals
//     cover the time range (0, 1/9) and (8/9, 1).
//
//   - A and E: the 'limits', at the minima.  These result in single verticals
//     at the start and end point of the interval.
//
//   - The rest of the ellipse is unimportant.  This is the part at coordinates
//     where both tangent endpoints are contained within the interval, and there
//     are no verticals.  On the ellipse edge in this region, x'(t) has zeroes,
//     but they are outside the interval.

// FINDING LENGTH PAIRS
//
// Given L1, we can find the corresponding L2 that will create a single
// vertical; we are finding a point on the ellipse edge.  Due to symmetry, it is
// equivalent to solve for L2 given L1, so we talk only of solving for one
// tangent length, given the other.
//
// This being an ellipse, we get two solutions.  To choose between them, we take
// the one that is closer to the prior value of the width we are solving for.
//
// To solve for L2, take the ellipse equation, and put it into power form,
// taking L1 as a constant:
//
//   L2^2 + (L1 - 2) L2 + (L1 - 1)^2 = 0
//
// This equation is computed by the function _ComputeOtherWidthForVert, using
// the quadratic formula.

// FINDING LENGTH PAIR TO PRESERVE RATIO
//
// Here we work in L1/L2 space.  We find the ratio k = L2/L1.  We take the line
// from the initial (L1, L2) through the origin, which is the line of constant
// length ratio k, and find its two intersections with the ellipse.  We always
// take the solution with longer tangents, which produces a single vertical.
// This gives:
//
//   L1 = (sqrt(k) + k + 1) / (k^2 + k + 1)
//   L2 = k * L1
//
// This equation is computed by the method _AdjustWithKeepRatio.


TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(
        TsRegressionPreventer::ModeLimitActive, "Limit Active");
    TF_ADD_ENUM_NAME(
        TsRegressionPreventer::ModeLimitOpposite, "Limit Opposite");
}

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS

// Amount by which we over-fix.  Each tangent will be made shorter than the
// exact solution by this fraction of the unit interval.  This ensures the curve
// is definitely non-regressive, in a way that will stand up to imprecise
// processing.  The fraction is small enough that the evaluation behavior should
// be indistinguishable from an actual vertical.
static constexpr TsTime kWritePadding = 1e-5;

// Amount by which we insist that the curve be over-fixed when deciding whether
// there is regression.  In order to ensure that our own output passes our test
// for non-regression, we use a number smaller than kWritePadding.
static constexpr TsTime kReadPadding = 1e-6;

// Geometric constants.
static constexpr TsTime kContainedMax = 1.0;
static constexpr TsTime kVertMax = 4.0 / 3.0;
static constexpr TsTime kVertMin = 1.0 / 3.0;

////////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

static bool _AreTanWidthsRegressive(TsTime width1, TsTime width2);
static TsTime _ComputeOtherWidthForVert(TsTime width, TsTime hint);

////////////////////////////////////////////////////////////////////////////////
// INITIALIZATION

TsRegressionPreventer::TsRegressionPreventer(
    TsSpline* const spline,
    const TsTime activeKnotTime,
    const bool limit)
    : TsRegressionPreventer(
        spline,
        activeKnotTime,
        _Mode(TsSpline::GetAntiRegressionAuthoringMode()),
        limit)
{
}

TsRegressionPreventer::TsRegressionPreventer(
    TsSpline* const spline,
    const TsTime activeKnotTime,
    const InteractiveMode mode,
    const bool limit)
    : TsRegressionPreventer(spline, activeKnotTime, _Mode(mode), limit)
{
}

TsRegressionPreventer::TsRegressionPreventer(
    TsSpline* const spline,
    const TsTime activeKnotTime,
    const _Mode mode,
    const bool limit)
    : _spline(spline),
      _mode(_Mode(mode)),
      _limit(limit),
      _valid(true),
      _initialAdjustmentDone(false)
{
    if (!_spline)
    {
        TF_CODING_ERROR("Null spline");
        _valid = false;
        return;
    }

    if (_spline->GetCurveType() != TsCurveTypeBezier)
    {
        TF_CODING_ERROR(
            "Cannot use TsRegressionPreventer on non-Bezier spline");
        _valid = false;
        return;
    }

    // Find the active knot.
    const TsKnotMap &map = _spline->GetKnots();
    const TsKnotMap::const_iterator activeIt = map.find(activeKnotTime);
    if (activeIt == map.end())
    {
        TF_CODING_ERROR("No knot at time %g", activeKnotTime);
        _valid = false;
        return;
    }

    // Make sure the active knot isn't an echoed knot.
    if (_spline->HasInnerLoops())
    {
        const TsLoopParams lp = _spline->GetInnerLoopParams();
        if (lp.GetLoopedInterval().Contains(activeKnotTime)
            && !lp.GetPrototypeInterval().Contains(activeKnotTime))
        {
            TF_CODING_ERROR(
                "Cannot edit echoed knot at time %g", activeKnotTime);
            _valid = false;
            return;
        }
    }

    // Set up state for the active and neighbor knots.
    const TsKnot &activeKnot = *activeIt;
    _activeKnotState.emplace(_spline, activeKnot);
    if (activeIt != map.begin())
    {
        const TsKnot &preOppositeKnot = *(activeIt - 1);
        if (preOppositeKnot.GetNextInterpolation() == TsInterpCurve)
        {
            _preKnotState.emplace(_spline, preOppositeKnot);
        }
    }
    if (activeIt + 1 != map.end())
    {
        const TsKnot &postOppositeKnot = *(activeIt + 1);
        if (activeKnot.GetNextInterpolation() == TsInterpCurve)
        {
            _postKnotState.emplace(_spline, postOppositeKnot);
        }
    }
}

// Init a SetResult to indicate unadjusted tangents.
void TsRegressionPreventer::_InitSetResult(
    const TsKnot &proposedActiveKnot,
    SetResult* const resultOut) const
{
    if (!resultOut)
    {
        return;
    }

    resultOut->havePreSegment = bool(_preKnotState);
    resultOut->havePostSegment = bool(_postKnotState);
    
    resultOut->preActiveAdjustedWidth =
        proposedActiveKnot.GetPreTanWidth();
    resultOut->postActiveAdjustedWidth =
        proposedActiveKnot.GetPostTanWidth();

    if (_preKnotState)
    {
        resultOut->preOppositeAdjustedWidth =
            _preKnotState->originalKnot.GetPostTanWidth();
    }

    if (_postKnotState)
    {
        resultOut->postOppositeAdjustedWidth =
            _postKnotState->originalKnot.GetPreTanWidth();
    }
}

////////////////////////////////////////////////////////////////////////////////
// INTERACTIVE PROCESSING

bool TsRegressionPreventer::Set(
    const TsKnot &proposedActiveKnot,
    SetResult* const resultOut)
{
    // Init the result to indicate unadjusted tangents.
    _InitSetResult(proposedActiveKnot, resultOut);

    if (!_valid)
    {
        return false;
    }

    // If anti-regression is disabled, just write the knot as proposed.
    if (_mode == _ModeNone)
    {
        _activeKnotState->Write(proposedActiveKnot);
        return true;
    }

    // Perform initial anti-regression if needed.
    _HandleInitialAdjustment(proposedActiveKnot, resultOut);

    // If the active knot's time has changed, update state.
    _HandleTimeChange(proposedActiveKnot.GetTime());

    // Solve the segments.
    _DoSet(proposedActiveKnot, _mode, resultOut);
    return true;
}

void TsRegressionPreventer::_HandleInitialAdjustment(
    const TsKnot &proposedActiveKnot,
    SetResult* const resultOut)
{
    // Have we already run?
    if (_initialAdjustmentDone)
    {
        return;
    }

    _initialAdjustmentDone = true;

    // Perform a no-op change to the active knot, using Contain or Limit
    // Opposite.  If there is initial regression, this will fix it.  If there is
    // no initial regression, this will do nothing.
    const _Mode initialMode =
        (_mode == _ModeContain ? _ModeContain : _ModeLimitOpposite);
    _DoSet(_activeKnotState->originalKnot, initialMode, resultOut);

    // Latch any edits we made so that they are tracked as original.  This
    // ensures that, in restoring to prior values, we never restore to a
    // regressive state.
    if (_preKnotState)
    {
        TsKnot knot = _preKnotState->originalKnot;
        knot._GetData()->postTanWidth =
            _preKnotState->currentParams.postTanWidth;
        _preKnotState.emplace(_spline, knot);
    }
    if (_postKnotState)
    {
        TsKnot knot = _postKnotState->originalKnot;
        knot._GetData()->preTanWidth =
            _postKnotState->currentParams.preTanWidth;
        _postKnotState.emplace(_spline, knot);
    }
}

void TsRegressionPreventer::_HandleTimeChange(
    const TsTime proposedActiveTime)
{
    // Do nothing if active knot time hasn't changed.
    if (proposedActiveTime == _activeKnotState->currentParams.time)
    {
        return;
    }

    // Remove current active knot.  There is no primitive to move a knot in
    // time; we remove the old and add the new.
    _activeKnotState->RemoveCurrent();

    // Do nothing further if we haven't crossed either neighbor.
    if (!_overwrittenKnotState
        && (!_preKnotState
            || proposedActiveTime > _preKnotState->originalKnot.GetTime())
        && (!_postKnotState
            || proposedActiveTime < _postKnotState->originalKnot.GetTime()))
    {
        return;
    }

    // Restore tentatively overwritten knot, if any.
    if (_overwrittenKnotState)
    {
        _overwrittenKnotState->RestoreOriginal();
        _overwrittenKnotState.reset();
    }

    // Restore original neighbors, if any, since we may have modified one of
    // them.
    if (_preKnotState)
    {
        _preKnotState->RestoreOriginal();
        _preKnotState.reset();
    }
    if (_postKnotState)
    {
        _postKnotState->RestoreOriginal();
        _postKnotState.reset();
    }

    // Find the insert position.
    const TsKnotMap &map = _spline->GetKnots();
    const TsKnotMap::const_iterator lbIt = map.lower_bound(proposedActiveTime);

    // If we're tentatively overwriting a knot at this time, store its
    // original state for possible restoration.
    if (lbIt != map.end() && lbIt->GetTime() == proposedActiveTime)
    {
        _overwrittenKnotState.emplace(_spline, *lbIt);
    }

    // If there's a knot before this time, store its original state for
    // comparison and possible restoration.
    if (lbIt != map.begin())
    {
        _preKnotState.emplace(_spline, *(lbIt - 1));
    }

    // If there's a knot after this time, store its original state for
    // comparison and possible restoration.
    const size_t postOffset = (_overwrittenKnotState ? 1 : 0);
    if (lbIt + postOffset != map.end())
    {
        _postKnotState.emplace(_spline, *(lbIt + postOffset));
    }
}

void TsRegressionPreventer::_DoSet(
    const TsKnot &proposedActiveKnot,
    const _Mode mode,
    SetResult* const resultOut)
{
    _WorkingKnotState activeWorking(
        &*_activeKnotState, proposedActiveKnot);
    std::optional<_WorkingKnotState> preWorking;
    std::optional<_WorkingKnotState> postWorking;

    // Adjust pre-segment, if it exists.
    if (_preKnotState)
    {
        preWorking.emplace(&*_preKnotState);

        _SegmentSolver preSolver(
            _SegmentSolver::PreSegment,
            mode,
            &activeWorking,
            &*preWorking,
            resultOut);

        preSolver.Adjust();
    }

    // Adjust post-segment, if it exists.
    if (_postKnotState)
    {
        postWorking.emplace(&*_postKnotState);

        _SegmentSolver postSolver(
            _SegmentSolver::PostSegment,
            mode,
            &activeWorking,
            &*postWorking,
            resultOut);

        postSolver.Adjust();
    }

    if (_limit)
    {
        // Write possibly adjusted knots to spline.
        activeWorking.WriteWorking();
        if (preWorking)
        {
            preWorking->WriteWorking();
        }
        if (postWorking)
        {
            postWorking->WriteWorking();
        }
    }
    else
    {
        // Just write the active knot as proposed.  This doesn't mean the
        // adjustments above were pointless; their results are given in
        // *resultOut.
        activeWorking.WriteProposed();
    }
}

////////////////////////////////////////////////////////////////////////////////
// BATCH PROCESSING

// static
bool Ts_RegressionPreventerBatchAccess::IsSegmentRegressive(
    const Ts_KnotData* const startKnot,
    const Ts_KnotData* const endKnot,
    const TsAntiRegressionMode modeIn)
{
    using RP = TsRegressionPreventer;

    // Determine whether this is a Bezier segment.
    if (startKnot->nextInterp != TsInterpCurve)
    {
        return false;
    }

    // Find normalized tangent widths.
    const TsTime interval = endKnot->time - startKnot->time;
    const TsTime startWidth = startKnot->GetPostTanWidth() / interval;
    const TsTime endWidth = endKnot->GetPreTanWidth() / interval;

    // In Contain mode, check simple max.
    const RP::_Mode mode = RP::_Mode(modeIn);
    if (mode == RP::_ModeContain)
    {
        return startWidth > kContainedMax || endWidth > kContainedMax;
    }

    // Call math helper.
    return _AreTanWidthsRegressive(startWidth, endWidth);
}

// static
bool Ts_RegressionPreventerBatchAccess::ProcessSegment(
    Ts_KnotData* const startKnot,
    Ts_KnotData* const endKnot,
    const TsAntiRegressionMode modeIn)
{
    using RP = TsRegressionPreventer;

    // If anti-regression is disabled, nothing to do.
    const RP::_Mode mode = RP::_Mode(modeIn);
    if (mode == RP::_ModeNone)
    {
        return false;
    }

    // Determine whether this is a Bezier segment.
    if (startKnot->nextInterp != TsInterpCurve)
    {
        return false;
    }

    // Use the start knot as active, and the end knot as opposite.
    RP::_WorkingKnotState startWorking(*startKnot);
    RP::_WorkingKnotState endWorking(*endKnot);

    // Create a solver for the segment.
    RP::SetResult setResult;
    RP::_SegmentSolver solver(
        RP::_SegmentSolver::PostSegment,
        mode, &startWorking, &endWorking, &setResult);

    // Find adjustments.
    solver.Adjust();

    // Write any adjusted tangent widths back to the knots.
    if (setResult.postActiveAdjusted)
    {
        startKnot->postTanWidth = startWorking.workingParams.postTanWidth;
    }
    if (setResult.postOppositeAdjusted)
    {
        endKnot->preTanWidth = endWorking.workingParams.preTanWidth;
    }

    // Return whether anything was changed.
    return setResult.adjusted;
}

////////////////////////////////////////////////////////////////////////////////
// SEGMENT SOLVER MAIN LOGIC

bool TsRegressionPreventer::_SegmentSolver::Adjust()
{
    // Contain mode.  This adjusts tangents even when non-regressive.
    if (_mode == _ModeContain)
    {
        return _AdjustWithContain();
    }

    // If no regression, nothing to do.
    if (!_AreTanWidthsRegressive(
            _GetProposedActiveWidth(), _GetProposedOppositeWidth()))
    {
        return true;
    }

    // Other modes.
    switch (_mode)
    {
        case _ModeKeepRatio: return _AdjustWithKeepRatio();
        case _ModeKeepStart: return _AdjustWithKeepStart();
        case _ModeLimitActive: return _AdjustWithLimitActive();
        case _ModeLimitOpposite: return _AdjustWithLimitOpposite();

        default:
            TF_CODING_ERROR("Unexpected mode");
    }

    return false;
}

bool TsRegressionPreventer::_SegmentSolver::_AdjustWithContain()
{
    // Don't use write padding for Contain.  We want the maximum to exactly
    // equal the interval.  We rely on our math not losing precision in the
    // writing and reading of this condition; we are doing things like
    // multiplying and dividing by 1, or dividing a number by itself to yield 1.

    // Limit active tangent.
    if (_GetProposedActiveWidth() > kContainedMax)
    {
        _SetActiveWidth(kContainedMax);
    }

    // Limit opposite tangent.
    if (_GetProposedOppositeWidth() > kContainedMax)
    {
        _SetOppositeWidth(kContainedMax);
    }

    return true;
}

bool TsRegressionPreventer::_SegmentSolver::_AdjustWithKeepRatio()
{
    if (_GetProposedActiveWidth() < kReadPadding)
    {
        // Zero active width.  Clamp opposite to 1.
        _SetOppositeWidth(kContainedMax - kWritePadding);
    }
    else if (_GetProposedOppositeWidth() < kReadPadding)
    {
        // Zero opposite width.  Clamp active to 1.
        _SetActiveWidth(kContainedMax - kWritePadding);
    }
    else
    {
        // Find ratio of proposed active to opposite width.
        const double ratio =
            _GetProposedActiveWidth() / _GetProposedOppositeWidth();

        // Solve for line / ellipse intersection.
        const TsTime adjustedOpposite =
            (std::sqrt(ratio) + ratio + 1)
            / (ratio * ratio + ratio + 1);
        _SetActiveWidth(ratio * adjustedOpposite - kWritePadding);
        _SetOppositeWidth(adjustedOpposite - kWritePadding);
    }

    return true;
}

bool TsRegressionPreventer::_SegmentSolver::_AdjustWithKeepStart()
{
    if (_GetProposedStartWidth() >= kVertMax)
    {
        // Clamp to longest start width.
        _SetStartWidth(kVertMax - kWritePadding);
        _SetEndWidth(kVertMin - kWritePadding);
    }
    else
    {
        // Keep start width; solve for end width.
        const TsTime adjustedWidth =
            _ComputeOtherWidthForVert(
                _GetProposedStartWidth(), _GetProposedEndWidth());
        _SetEndWidth(adjustedWidth - kWritePadding);
    }

    return true;
}

bool TsRegressionPreventer::_SegmentSolver::_AdjustWithLimitActive()
{
    if (_GetProposedOppositeWidth() >= kVertMax)
    {
        // Clamp to longest opposite width.
        _SetOppositeWidth(kVertMax - kWritePadding);
        _SetActiveWidth(
            GfMin(kVertMin - kWritePadding, _GetProposedActiveWidth()));
    }
    else
    {
        // Keep opposite width; solve for active width.
        const TsTime adjustedWidth =
            _ComputeOtherWidthForVert(
                _GetProposedOppositeWidth(), _GetProposedActiveWidth());
        _SetActiveWidth(adjustedWidth - kWritePadding);
    }

    return true;
}

bool TsRegressionPreventer::_SegmentSolver::_AdjustWithLimitOpposite()
{
    if (_GetProposedOppositeWidth() <= kVertMin)
    {
        // Non-regressive limit will be in fringe.
        // Don't adjust opposite; just clamp active.
        // This avoids counter-intuitively forcing opposite to be longer.
        const TsTime adjustedWidth =
            _ComputeOtherWidthForVert(
                _GetProposedOppositeWidth(), _GetProposedActiveWidth());
        _SetActiveWidth(adjustedWidth - kWritePadding);
    }
    else if (_GetProposedActiveWidth() >= kVertMax)
    {
        // Clamp to longest active width.
        _SetActiveWidth(kVertMax - kWritePadding);
        _SetOppositeWidth(kVertMin - kWritePadding);
    }
    else
    {
        // Keep active width; solve for opposite width.
        const TsTime adjustedWidth =
            _ComputeOtherWidthForVert(
                _GetProposedActiveWidth(), _GetProposedOppositeWidth());
        _SetOppositeWidth(adjustedWidth - kWritePadding);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// MATH HELPERS

// static
bool _AreTanWidthsRegressive(
    const TsTime width1,
    const TsTime width2)
{
    // If contained, then not regressive.  This helps performance, but it is
    // also for correctness.  There are non-regressive (w1, w2) points outside
    // the ellipse but inside the contained square.  See the note in
    // _AdjustWithContain regarding why we don't use padding in this check.
    if (width1 <= kContainedMax && width2 <= kContainedMax)
    {
        return false;
    }

    // Consider both widths with padding.
    const TsTime w1 = width1 + kReadPadding;
    const TsTime w2 = width2 + kReadPadding;

    // Determine whether (w1, w2) lies outside the ellipse.
    return (w1 * w1) + (w2 * w2) - 2 * (w1 + w2) + (w1 * w2) + 1 > 0.0;
}

// static
TsTime _ComputeOtherWidthForVert(
    const TsTime width,
    const TsTime hint)
{
    // Clamp to longest given width / shortest other width.
    if (width > kVertMax)
    {
        TF_WARN("Unexpectedly long tangent");
        return kVertMin;
    }

    // Solve for the two ellipse points that have the given width.
    const TsTime b = width - 2.0;
    const TsTime c = std::pow(width - 1.0, 2);
    const TsTime rootBase = -b / 2.0;
    const TsTime rootOffset = std::sqrt(b * b - 4 * c) / 2.0;

    // Choose the solution closer to the hint.
    return (hint > rootBase ? rootBase + rootOffset : rootBase - rootOffset);
}

////////////////////////////////////////////////////////////////////////////////
// SEGMENT SOLVER PLUMBING

TsRegressionPreventer::_SegmentSolver::_SegmentSolver(
    const WhichSegment whichSegment,
    const _Mode mode,
    _WorkingKnotState* const activeKnotState,
    _WorkingKnotState* const oppositeKnotState,
    SetResult* const result)
    : _whichSegment(whichSegment),
      _mode(mode),
      _activeKnotState(activeKnotState),
      _oppositeKnotState(oppositeKnotState),
      _result(result)
{
}

TsTime TsRegressionPreventer::_SegmentSolver::_GetProposedActiveWidth() const
{
    const TsTime width = (
        _whichSegment == PreSegment ?
        _activeKnotState->proposedKnot.GetPreTanWidth() :
        _activeKnotState->proposedKnot.GetPostTanWidth());
    return width / _GetSegmentWidth();
}

TsTime TsRegressionPreventer::_SegmentSolver::_GetProposedOppositeWidth() const
{
    const TsTime width = (
        _whichSegment == PreSegment ?
        _oppositeKnotState->proposedKnot.GetPostTanWidth() :
        _oppositeKnotState->proposedKnot.GetPreTanWidth());
    return width / _GetSegmentWidth();
}

void TsRegressionPreventer::_SegmentSolver::_SetActiveWidth(
    const TsTime width)
{
    const bool adjusted = (width != _GetProposedActiveWidth());
    const TsTime rawWidth = width * _GetSegmentWidth();

    if (_whichSegment == PreSegment)
    {
        _activeKnotState->workingParams.SetPreTanWidth(rawWidth);

        if (_result)
        {
            _result->adjusted |= adjusted;
            _result->preActiveAdjusted |= adjusted;
            _result->preActiveAdjustedWidth = rawWidth;
        }
    }
    else
    {
        _activeKnotState->workingParams.SetPostTanWidth(rawWidth);

        if (_result)
        {
            _result->adjusted |= adjusted;
            _result->postActiveAdjusted |= adjusted;
            _result->postActiveAdjustedWidth = rawWidth;
        }
    }
}

void TsRegressionPreventer::_SegmentSolver::_SetOppositeWidth(
    const TsTime width)
{
    const bool adjusted = (width != _GetProposedOppositeWidth());
    const TsTime rawWidth = width * _GetSegmentWidth();

    if (_whichSegment == PreSegment)
    {
        _oppositeKnotState->workingParams.SetPostTanWidth(rawWidth);

        if (_result)
        {
            _result->adjusted |= adjusted;
            _result->preOppositeAdjusted |= adjusted;
            _result->preOppositeAdjustedWidth = rawWidth;
        }
    }
    else
    {
        _oppositeKnotState->workingParams.SetPreTanWidth(rawWidth);

        if (_result)
        {
            _result->adjusted |= adjusted;
            _result->postOppositeAdjusted |= adjusted;
            _result->postOppositeAdjustedWidth = rawWidth;
        }
    }
}

TsTime TsRegressionPreventer::_SegmentSolver::_GetProposedStartWidth() const
{
    return (
        _whichSegment == PreSegment ?
        _GetProposedOppositeWidth() :
        _GetProposedActiveWidth());
}

TsTime TsRegressionPreventer::_SegmentSolver::_GetProposedEndWidth() const
{
    return (
        _whichSegment == PreSegment ?
        _GetProposedActiveWidth() :
        _GetProposedOppositeWidth());
}

void TsRegressionPreventer::_SegmentSolver::_SetStartWidth(
    const TsTime width)
{
    if (_whichSegment == PreSegment)
    {
        _SetOppositeWidth(width);
    }
    else
    {
        _SetActiveWidth(width);
    }
}

void TsRegressionPreventer::_SegmentSolver::_SetEndWidth(
    const TsTime width)
{
    if (_whichSegment == PreSegment)
    {
        _SetActiveWidth(width);
    }
    else
    {
        _SetOppositeWidth(width);
    }
}

TsTime TsRegressionPreventer::_SegmentSolver::_GetSegmentWidth() const
{
    TsTime width =
        _activeKnotState->proposedKnot.GetTime() -
        _oppositeKnotState->proposedKnot.GetTime();

    if (_whichSegment == PostSegment)
    {
        width *= -1;
    }

    if (!TF_VERIFY(width > 0))
    {
        return 1.0;
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////
// KNOT STATE PLUMBING

TsRegressionPreventer::_KnotState::_KnotState(
    TsSpline *spline,
    const TsKnot &originalKnotIn)
    : spline(spline),
      originalKnot(originalKnotIn),
      currentParams(*(originalKnotIn._GetData()))
{
}

void TsRegressionPreventer::_KnotState::RestoreOriginal()
{
    spline->_SetKnotUnchecked(originalKnot);
}

void TsRegressionPreventer::_KnotState::RemoveCurrent()
{
    spline->RemoveKnot(currentParams.time);
}

void TsRegressionPreventer::_KnotState::Write(
    const TsKnot &newKnot)
{
    RemoveCurrent();
    spline->_SetKnotUnchecked(newKnot);
    currentParams = *(newKnot._GetData());
}

TsRegressionPreventer::_WorkingKnotState::_WorkingKnotState(
    _KnotState* const parentState,
    const TsKnot &proposedKnotIn)
    : parentState(parentState),
      proposedKnot(proposedKnotIn),
      workingParams(*(proposedKnotIn._GetData()))
{
}

TsRegressionPreventer::_WorkingKnotState::_WorkingKnotState(
    _KnotState* const parentState)
    : parentState(parentState),
      proposedKnot(parentState->originalKnot),
      workingParams(*(parentState->originalKnot._GetData()))
{
}

TsRegressionPreventer::_WorkingKnotState::_WorkingKnotState(
    const Ts_KnotData &originalParamsIn)
    : parentState(nullptr),
      // Not a real knot; just conforming to the storage.
      proposedKnot(
          new Ts_KnotData(originalParamsIn),
          Ts_GetType<double>(),
          VtDictionary()),
      workingParams(originalParamsIn)
{
}

void TsRegressionPreventer::_WorkingKnotState::WriteProposed()
{
    parentState->spline->_SetKnotUnchecked(proposedKnot);

    parentState->currentParams = *(proposedKnot._GetData());
}

void TsRegressionPreventer::_WorkingKnotState::WriteWorking()
{
    TsKnot knot = proposedKnot;
    Ts_KnotData* const knotData = knot._GetData();
    knotData->preTanWidth = workingParams.preTanWidth;
    knotData->postTanWidth = workingParams.postTanWidth;
    parentState->spline->_SetKnotUnchecked(knot);

    parentState->currentParams = workingParams;
}

////////////////////////////////////////////////////////////////////////////////
// STRUCTS

#define PRINT_MEMBER(m) ss << "  " #m ": " << m << std::endl

std::string
TsRegressionPreventer::SetResult::GetDebugDescription(
    int precision) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision);
    ss << std::boolalpha;

    ss << "TsRegressionPreventer::SetResult:" << std::endl;

    PRINT_MEMBER(adjusted);
    PRINT_MEMBER(havePreSegment);
    PRINT_MEMBER(preActiveAdjusted);
    PRINT_MEMBER(preActiveAdjustedWidth);
    PRINT_MEMBER(preOppositeAdjusted);
    PRINT_MEMBER(preOppositeAdjustedWidth);
    PRINT_MEMBER(havePostSegment);
    PRINT_MEMBER(postActiveAdjusted);
    PRINT_MEMBER(postActiveAdjustedWidth);
    PRINT_MEMBER(postOppositeAdjusted);
    PRINT_MEMBER(postOppositeAdjustedWidth);

    return ss.str();
}


PXR_NAMESPACE_CLOSE_SCOPE
