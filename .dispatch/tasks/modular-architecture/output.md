# Modular file breakdown — [WayQ [IO [BP [HpLp] [Baxandall]]]]

Posted as comment on rcrath/wayq issue #1: https://github.com/rcrath/wayq/issues/1#issuecomment-4773706524

| File | Status | Layer | Contents |
|------|--------|-------|----------|
| **Build system** | | | |
| `CMakeLists.txt` | new | build | — |
| **Plugin core — WayQ** | | | |
| `src/PluginProcessor.h/.cpp` | new | WayQ | `WayQAudioProcessor` — APVTS owner, plugin boilerplate |
| `src/PluginEditor.h/.cpp` | new | WayQ | `WayQAudioProcessorEditor` — GUI entry point |
| `src/DragTooltipProvider.h` | new | WayQ | `DragTooltipProvider` — abstract drag-tooltip base |
| **IO layer — WayQIO** | | | |
| `src/WayQIO.h/.cpp` | new | IO | `WayQIO` — reads APVTS params, iterates samples, feeds WayQBP |
| **Band processor — WayQBP** | | | |
| `src/WayQBP.h/.cpp` | new | BP | `WayQBP` — routes `processSample` to WayQHpLp or WayQBaxandall based on `modeX`; owns `dcBlock` |
| **HP+Bell+LP chain — WayQHpLp** | | | |
| `src/WayQHpLp.h/.cpp` | new | HpLp | `WayQHpLp`, `OutTwoPoleHP`, `OutTwoPolePeak`, `OutTwoPoleLP` — 4-order-variant HP→Bell→LP chain; structs moved from `OutputFilter.h` |
| **Baxandall chain — WayQBaxandall** | | | |
| `src/WayQBaxandall.h/.cpp` | new | Baxandall | `WayQBaxandall`, `OutTiltBiquad` — BassTilt→Bell→TrebTilt (Mode X); struct moved from `OutputFilter.h` |
| **Source of the split** | | | |
| `src/OutputFilter.h` | split → obsolete | — | Decomposed: `OutTwoPoleHP`, `OutTwoPolePeak`, `OutTwoPoleLP` → `WayQHpLp.h`; `OutTiltBiquad` → `WayQBaxandall.h`; `OutputFilter` class → `WayQBP` |
| **GUI widgets** | | | |
| `src/OutFilterRow.h/.cpp` | unchanged | GUI | `OutFilterRow` — EQ control row widget |
| `src/StereoSliderRow.h/.cpp` | fix: remove stale `#include "PluginProcessor.h"` on .cpp line 3 | GUI | `StereoSliderRow` |
| `src/MonoSliderRow.h/.cpp` | unchanged | GUI | `MonoSliderRow` |
| `src/DualHandleFilterRow.h/.cpp` | unchanged | GUI | `DualHandleFilterRow` |
| `src/DualHandleSlider.h/.cpp` | unchanged | GUI | `DualHandleSlider` |
| **Look and feel / support** | | | |
| `src/WayQLookAndFeel.h` | unchanged | support | `WayQLookAndFeel` |
| `src/WayQSlider.h` | unchanged | support | `WayQSlider` |
| `src/GUIConstants.h` | unchanged | support | layout constants |
| `src/AmberLink.h` | unchanged | support | — |
| `src/CurveCircle.h` | unchanged | support | — |
| `src/DoubleClickTextButton.h` | unchanged | support | — |
| `src/LEDComponent.h` | unchanged | support | — |
| `src/MeterBar.h` | unchanged | support | — |
| `src/DryWet.h/.cpp` | unchanged | support | `DryWet` |
| **Assets** | | | |
| `src/fonts/` (3 TTFs) | unchanged | asset | binary font data |
| `src/` SVGs (5 files) | unchanged | asset | binary SVG data |

**Ownership chain:** `PluginProcessor` owns `WayQIO`; `WayQIO` drives two `WayQBP` instances (L and R); each `WayQBP` owns one `WayQHpLp` and one `WayQBaxandall`.
