# Remove OUT-EQ feedback/post (O/F) route switch + chain-order plumbing

## Chain order — the important finding

The task brief assumed wayq's old default (route param default = 1, i.e. HP→Bell→LP)
was the correct sonic behaviour to make permanent. **It is not.**

Checked rhythmecho's live OUT-filter DSP:
- `E:\Dropbox\audio\git\rhythmecho\src\PluginProcessor.cpp:421-422` hardcodes
  `chainOrderA = 3; chainOrderB = 3;  // fixed: HP==>LP+BP (parallel bell)` and feeds
  that to `delayL/R.setOutputChainOrder(...)`, overriding rhythmecho's own
  `outFiltChainOrder` parameter at runtime.
- rhythmecho's `OutputFilter::processSample` case 3 math:
  `hpOut = hp; lpOut = lp(hpOut); belOut = bell(hpOut); out = lpOut + belOut - hpOut;`
  (HP → LP with Bell in parallel).

wayq's old default (order 1) was **HP → Bell → LP** (serial) — a DIFFERENT ordering
from rhythmecho. They did **not** match.

Per the added requirement, wayq's permanent hardcoded order is pinned to **rhythmecho's
order 3** (HP → LP with Bell in parallel), NOT wayq's old order 1. This is an explicit
behaviour change from wayq's previous default, made deliberately to match rhythmecho's
preferred sound. wayq's existing `HpLp` case-3 math is byte-for-byte identical to
rhythmecho's case-3 math, so pinning to it reproduces rhythmecho's OUT-EQ tone exactly.

## How the fixed order was made permanent

`src/HpLp.cpp` — `HpLp::processSample` lost its `int chainOrder` arg and the `switch`;
it now always runs the order-3 path:
```
float hpOut  = hp_.process (in);
float lpOut  = lp_.process (hpOut);
float belOut = bell_.process (hpOut);
out = lpOut + belOut - hpOut;
```
`src/HpLp.h` — signature changed to `processSample (float in, float& out)`; header
comment updated to state the fixed HP→LP+parallel-Bell order.

Evidence order-3 behaviour is preserved: the hardcoded body is a verbatim copy of the
former `case 3:` block, which equals rhythmecho's `OutputFilter` case 3.

## Files + functions changed

DSP path:
- `src/HpLp.h` / `src/HpLp.cpp` — removed `chainOrder` param from `processSample`;
  hardcoded order-3 path; updated comments.
- `src/BP.h` — removed `setChainOrder` decl and `int chainOrder_ = 1;` member.
- `src/BP.cpp` — removed `BP::setChainOrder` def; `processSample` now calls
  `hpLp_.processSample (in, filtered)`.
- `src/Filter.h` — removed `setChainOrder` decl.
- `src/Filter.cpp` — removed `Filter::setChainOrder` def.
- `src/IO.cpp` — removed `routeId` param from `applyParamsToChannel`, removed the
  `filter.setChainOrder(...)` call, removed `"outRouteA"`/`"outRouteB"` from both id lists.

Parameters:
- `src/PluginProcessor.cpp` — removed `outRouteA` and `outRouteB`
  `AudioParameterInt` definitions from `createParameterLayout()`.

GUI:
- `src/PluginEditor.cpp` — removed the `"outRouteA", "outRouteB"` argument pair from the
  `outFilterRow_` construction.
- `src/OutFilterRow.h` — removed `routeA`/`routeB` ctor params; removed
  `paintOverChildren` decl; removed `routePid_[2]` member; removed
  `syncRouteButtonFromParam` decl; removed `routeBtnA_`/`routeBtnB_` DrawableButton
  members; updated comments.
- `src/OutFilterRow.cpp`:
  - ctor: removed `routeA`/`routeB` params and `routePid_` assignment; removed the
    entire route-button setup block (the oa/ob/fa/fb SVG `BinaryData` loads, setImages,
    setColour, setClickingTogglesState, setTooltip, onClick lambdas, addAndMakeVisible);
    removed the two `addParameterListener (routePid_[...])` calls; removed the
    `syncRouteButtonFromParam()` call.
  - `resized()`: button block changed from 3 rows × 2 cols to **2 rows × 2 cols**
    (delta/polarity on top, EQ-mode P/X below), `blockH = 2*sz`, still vertically
    centred, same `sz = workareaH/7`, same left margin; route rows removed.
  - removed `paintOverChildren` definition entirely (it bordered ONLY the route
    buttons).
  - `copyChannelParams`: removed the route param copy.
  - `selectChannel`: removed `syncRouteButtonFromParam()` call.
  - `mirrorAtoB`: removed the route mirror block.
  - destructor: removed the two `removeParameterListener (routePid_[...])` calls.
  - `parameterChanged`: removed the route-id branch.
  - removed `syncRouteButtonFromParam()` definition.
- `src/WayQLookAndFeel.h` — dropped the stale "route" word from a glyph-list comment.

`cellBounds()` verified unchanged — it already uses `leftMargin + 2 * btnSz + gap`
(2 columns), which still holds with the 2×2 block.

Not touched (intentionally out of scope): HP/Bell/LP filtering, Baxandall tilt and the
treble-tilt sign fix, yinyang handles, delta/polarity buttons, EQ-mode P/X buttons,
A/B link, their tooltips.

## Legacy dead code left alone

`src/OutputFilter.h` still declares `class OutputFilter` with its own `setChainOrder` /
`chainOrder_`. That class is **never instantiated** anywhere in wayq (grep confirms only
the declaration; it serves solely as the namespace for filter constants like FREQ_MIN,
HP_DEFAULT). The route param never drove it, and the task scoped DSP edits to
Filter/BP/HpLp/IO. Left untouched to keep edits minimal. This is the only remaining
`setChainOrder`/`chainOrder` reference in the tree and it is unreachable dead code.

## Final clean-grep

Code-identifier grep over `src/` for
`routeBtn|routePid|syncRouteButton|outRoute|setChainOrder|oa_svg|ob_svg|fa_svg|fb_svg|routeFb|routeFeedback`:

    src\OutputFilter.h:419:    void setChainOrder (int order)   <-- legacy dead OutputFilter class, never called

Broad grep over `src/` (excluding the .ttf binary font) for
`route|Route|outRoute|setChainOrder|chainOrder|ChainOrder|routePid|routeBtn|syncRouteButton|paintOverChildren|oa_svg|ob_svg|fa_svg|fb_svg`:

    src\OutFilterRow.h:28:   // ...handlers and routed via Slider::   (word "routed" in a comment)
    src\OutputFilter.h:419:  void setChainOrder (int order)        (legacy dead class)
    src\OutputFilter.h:421:      chainOrder_ = std::max (0, std::min (order, 3));   (legacy)
    src\OutputFilter.h:441:      switch (chainOrder_)                (legacy)
    src\OutputFilter.h:505:  int   chainOrder_   = 1;              (legacy)
    src\StereoSliderRow.h:23:  void paintOverChildren (...) override;   (unrelated component)
    src\StereoSliderRow.cpp:335: void StereoSliderRow::paintOverChildren (...)   (unrelated)

Zero remaining references to the route switch, the `outRoute` params, the route
DrawableButtons, `routePid_`, `syncRouteButtonFromParam`, OutFilterRow's
`paintOverChildren`, or the four route `BinaryData` symbols. The only `setChainOrder`/
`chainOrder` left is the unreachable legacy `OutputFilter` class.

## Now-unused SVG assets

The coordinator reports it has already deleted `oa.svg`, `ob.svg`, `fa.svg`, `fb.svg`
from `src/` and removed their `juce_add_binary_data` entries in CMakeLists.txt (I did
not touch those files). My edits removed the only references to
`BinaryData::oa_svg / ob_svg / fa_svg / fb_svg` (the route-button SVG loads), so the
source no longer references those now-deleted symbols — confirmed by the clean grep
above. `yinyang.svg` is untouched and still used by the tilt handles.

## Build / version

No build run, no commit, no version change — all per constraints. User owns the build loop.
