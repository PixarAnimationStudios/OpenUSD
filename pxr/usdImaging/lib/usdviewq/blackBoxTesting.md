# Usdview Black Box Testing

The testusdview automated testing harness allows us to test many of
usdview's features, but not all.  As we develop, when we fix or deploy
a feature for which it does not seem reasonable to add a testusdview
test-case, we should instead add a test for it here.

## Viewport prim Vising and Invising

### Goal

Ensure that when we select (including multi-select) prims (or models, when
"Pick Mode" is "Models") in the viewport, and **then** use the hotkeys for 
_Make Invisible_, _Make Visible_, _Vis Only_, _Remove Session Visibility_, and 
_Remove All Session Visibility_, that:
- The prims actually appear/disappear as expected in the viewport
- When the selected items are viewed in the Prim View browser subsequently, 
  the **Vis** column shows "I" appropriately for invisible prims and their 
  descendants, and "V" elsewhere (except for non-Imageable prims such as
  Materials and Shaders, which should show nothing).

### Method

- Start usdview with a sufficiently complex asset such that not all prims are 
  open in the Prim View browser.  The Kitchen_set.usd asset works well.
- In the viewport, Shift-select several pieces of geometry, from different 
  parts of the scene.
- Run through the visibility operations via hotkey (or RMB context menu), 
  ensuring that geometry disappears and reappears.  E.g. Ctrl-h to invis/hide,
  followed by Shift-h to vis/unhide.
- After invising some geometry, hover mouse over Prim View, and hit 'f' to
  frame the selection(s), which should cause rows to be expanded, showing the
  selected prims.  You should see that the newly exposed prims have 'I' in the
  _Vis_ column (as well as all of their descendants, if you are in "Pick Models"
  mode, and open the model in the browser).


## Vis and Draw Mode Columns Do Not Affect Selection

### Goal

The Prim View Browser should select prims when the user clicks within the 
__Prim Name__ or __Type__ columns, but when clicking in the __Vis__ or
__Draw Mode__ columns, the current selection should not be affected, and instead
the user should just be modifying the prim's visibility or Draw Mode.

After initial deployment of this feature, there was an uncaught regression in 
which interacting with a prim/row when the selected prim/row was not visible
in the browser would cause the browser to scroll to the selected prim and cancel
the interaction. 


## Prim View Framing

### Goal

When making selections in the viewport, we do not expect the "expansion state"
of the Prim View to change.  Instead, if selected prim(s) is already visible in
the browser, then it should highlight (become selected); but if it is not 
visible/exposed, then only its visible ancestors should be highlighted in a
secondary, muted selection color.

### Method

- Start usdview with a sufficiently complex asset such that not all prims are 
  open in the Prim View browser.  The Kitchen_set.usd asset works well.
- Pick some geometry in the viewer, and ensure we see desired behavior in 
  browser, as described in Goal above. (The top of the kitchen table is a 
  good one.)
- Now hover over the browser, and hit 'f' to frame the selection (or first 
  selected prim if a multi-selection).
- Reset Prim View with Alt-3 (Show > Prim View Depth > Level 3)
- Go back to the viewport, and select something else (e.g refridgerator door).
  Ensure browser updates only in that a different ancestor selection is made
  (since the refridgerator is in the _North_Group_ as opposed to the 
  _DiningTable_group_ ).
- Hover over the Prim View, and use the arrow/cursor keys to change selection.
  Upon first keypress, the selected item should become visible/expanded, and
  uniquely selected (if you had a multi-selection), and navigation should
  then proceed as expected using up/down, left/rught keys.
 
