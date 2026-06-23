# OUT EQ SVG wiring — summary

Goal: make wayq's OUT EQ render the same SVG art RhythmEcho uses (oa/ob/fa/fb route faces, yinyang tilt handles) instead of hand-drawn shapes. Only `src/OutFilterRow.cpp` and `src/OutFilterRow.h` were changed.

## Route/position switch (oa/ob/fa/fb)

- `OutFilterRow.h`: `routeBtnA_`/`routeBtnB_` changed from `juce::TextButton` to `juce::DrawableButton` (ImageFitted), matching RhythmEcho.
- `OutFilterRow.cpp` constructor: replaced the TextButton colour-styling loop with RhythmEcho's SVG load. Loads `oa_svg`/`fa_svg` for A and `ob_svg`/`fb_svg` for B via `juce::Drawable::createFromImageData`, transparent backgrounds, and `setImages(O, nullptr, nullptr, nullptr, F)` so the **off** state shows the output face (oa/ob) and the **on** state shows the feedback face (fa/fb). Tooltips and `onClick` handlers unchanged.
- `syncRouteButtonFromParam()`: dropped the `setButtonText("F"/"O")` lines — the DrawableButton swaps faces from toggle state, exactly like RhythmEcho.
- New `paintOverChildren()`: draws the rounded `GC::LABEL` border around the route buttons (the transparent DrawableButtons no longer get a LookAndFeel border the way the sibling delta/EQ text buttons do). Mirrors RhythmEcho's `paintOverChildren`.

## Baxandall tilt handles (yinyang)

- `OutFilterRow.h`: added `std::unique_ptr<juce::Drawable> yinyangDrawable_;` and `paintYinyangHandle(g, childId, x, y)`.
- `OutFilterRow.cpp`: added `kYinyangDispR = 14.0f`; load `yinyangDrawable_` from `yinyang_svg` at end of constructor.
- `paintUnifiedFilterCell` (active-channel modeX): the two `paintTiltDot` calls replaced by `paintYinyangHandle("yin", bassX, bassY)` and `paintYinyangHandle("yang", trebX, trebY)`. yin = gold = bass, yang = cream = treble — same colour mapping as RhythmEcho. The inactive channel keeps `paintTiltDot`, exactly as RhythmEcho does.
- `paintYinyangHandle`: splits the drawable via `findChildWithID("yin"/"yang")`, scales by `kYinyangDispR/95`, and uses an AffineTransform mapping the SVG centre (100,100) to the handle anchor. The yang half's 180° rotation is baked into the SVG (`transform="rotate(180 100 100)"`), applied by JUCE's parser — no rotation in code, matching RhythmEcho.

## Adaptation note (important — please review)

RhythmEcho and wayq use **different tilt interaction geometry**:
- RhythmEcho: the yinyang sits on the cell's **top edge** and controls **frequency only** (two independent halves), while **gain** is a separate vertical edge slider at each side of the cell. It uses an overhang band above the cell and DragModes `YinyangBassHalf` / `YinyangTrebleHalf` / `BassEdgeSlider` / `TrebEdgeSlider`.
- wayq: each tilt handle is a **single combined freq+gain dot** dragged in both axes (`DragMode::BassTiltDot` / `TrebTiltDot`), with no overhang and no edge sliders.

Per the task constraints ("replace only the hand-drawn art / route button faces and tilt handle dots", "do not refactor unrelated drawing or geometry"), I kept wayq's existing combined-dot geometry and hit-testing and swapped **only the art** — drawing the yin/yang halves at wayq's existing dot position (freqToX, gainToY) instead of RhythmEcho's top-edge freq-only position. I did **not** import RhythmEcho's edge-slider / overhang model, since that would have meant rewriting hitZoneModeX, the drag handlers, double-click resets, tooltips, and cellBounds geometry.

If the intent was actually to adopt RhythmEcho's full yinyang-on-top + edge-slider interaction (a much larger change), that's a separate, larger task — flag it and I can do it.

## New/changed user-visible strings

None. No new labels or text were added; route buttons now show SVG faces instead of the letters "O"/"F", and the tooltip text is unchanged.

## Not built

No build, compile, or build script was run — per the constraints, the user owns the build loop.
