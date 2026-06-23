# impl-filter — completion summary

## Files created

**src/Filter.h / src/Filter.cpp**
- `Filter` owns one `BP bp_` and an `int delta_` (default 0).
- `prepare(float sr)` / `reset()` delegate to `bp_`.
- `setDelta(int d)` stores the mode.
- `processSample`: calls `bp_.processSample(in, fafoActive, processed)`, then applies:
  - delta 0 → `out = processed`
  - delta 1 → `out = in - processed`
  - delta 2 → `out = -processed`
- All 13 BP setters forwarded verbatim (`setModeX`, `setChainOrder`, `setHpFreq`, `setHpQ`, `setMidFreq`, `setMidGain`, `setMidQ`, `setLpFreq`, `setLpQ`, `setBassTiltFreq`, `setBassTiltGain`, `setTrebTiltFreq`, `setTrebTiltGain`).

## Files updated

**src/IO.h**
- `#include "BP.h"` → `#include "Filter.h"`
- `BP bpL_, bpR_` → `Filter filterL_, filterR_`

**src/IO.cpp**
- `applyParamsToChannel` signature changed from `BP&` to `Filter&`; all internal calls updated to `filter.*`.
- `prepare` / `reset` / sample loop updated: `bpL_`/`bpR_` → `filterL_`/`filterR_`.
- After each `applyParamsToChannel` call, added delta param read:
  - `filterL_.setDelta(static_cast<int>(*apvts.getRawParameterValue("outDeltaA")))`
  - `filterR_.setDelta(static_cast<int>(*apvts.getRawParameterValue("outDeltaB")))`

**CMakeLists.txt**
- Added `src/Filter.cpp` to `target_sources`, between `src/BP.cpp` and `src/IO.cpp`.

## No new user-visible GUI strings introduced.
