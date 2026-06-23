/*
  ==============================================================================
    GUIConstants.h
    RhythmEcho VST3 Plugin — v0.9.8 GUI rebuild

    Three-zone layout: Rhythm + Fractions (left) | Center (center) | Echo (right)
    Dark theme, resizable, horizontal stereo sliders throughout.

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once
#include <cstdint>

namespace GUIConstants
{
    //==========================================================================
    // DEFAULT DIMENSIONS — resizable from here
    //==========================================================================
    constexpr int DEFAULT_W = 1340;
    constexpr int DEFAULT_H = 875;
    constexpr int MIN_W     = 900;
    constexpr int MIN_H     = 574;

    // Zone proportions (of total width)
    constexpr float LEFT_ZONE_FRAC    = 0.45f;   // Rhythm + Fractions
    constexpr float CENTER_ZONE_FRAC  = 0.10f;   // Center column (buttons + branding)
    constexpr float RIGHT_ZONE_FRAC   = 0.45f;   // Echo

    // Vertical proportions
    constexpr float TITLE_H_FRAC      = 0.12f;   // title bar area
    constexpr float SANE_H_FRAC       = 0.50f;   // Sane controls region
    constexpr float FAFO_H_FRAC       = 0.44f;   // FAFO expansion region

    // Slider geometry — all x-offsets in 626-unit panel-local space (zone w=626)
    constexpr int   SLIDER_ROW_H      = 50;      // row-h:      stereo slider row height
    constexpr int   SLIDER_HALF_H     = 25;      // half-row-h: one channel height
    constexpr int   LINK_BTN_SIZE     = 28;      // link-size:  link button diameter
    constexpr int   ROW_GAP           = 4;       // row-gap:    vertical gap between rows
    constexpr int   LABEL_TEXT_W      = 113;     // label-w:    SVG text element width (not a layout offset)
    constexpr int   WORKAREA_X        = 135;     // panel-local workarea start (SVG x=137 - zone_left x=2)
    constexpr int   WORKAREA_W        = 445;     // workarea-w: slider/button area width
    constexpr int   WORKAREA_END      = 580;     // panel-local workarea right edge (WORKAREA_X + WORKAREA_W)
    constexpr int   LINK_CX           = 604;     // panel-local link button centre (SVG x=606 - zone_left x=2)
    constexpr int   SECTION_PAD       = 12;      // pad:        zone edge padding
    constexpr int   BTN_W             = 84;      // btn-w:      standard button width
    constexpr int   FRAC_CELL         = 37;      // frac-cell:  fraction grid cell size
    constexpr int   LED_SIZE          = 14;      // led-size:   onset LED square

    //==========================================================================
    // COLORS — dark theme matching wireframe
    //==========================================================================
    namespace Color
    {
        // Background — deep ocean
        constexpr uint32_t BG            = 0xFF0C1318;   // deep dark ocean
        constexpr uint32_t ZONE_BG       = 0xFF121B20;   // zone panel background (10% darker than 0xFF141E24)
        constexpr uint32_t CENTER_BG     = 0xFF10171D;   // center column background (10% darker than 0xFF121A20)

        // Title — teal + gold
        constexpr uint32_t TITLE_RHYTHM      = 0xFFA0D8D0;   // "Rhythm" — light teal
        constexpr uint32_t TITLE_ECHO        = 0xFFE8D090;   // "Echo" — light gold

        // Controls — Sane: teal-based
        constexpr uint32_t SLIDER_BG     = 0xFF2C4450;   // slider track background
        constexpr uint32_t SLIDER_FILL   = 0xFF4A8A8A;   // slider filled portion (teal)
        constexpr uint32_t SLIDER_THUMB  = 0xFF4A8A8A;   // thumb color — matches fill
        constexpr uint32_t SLIDER_NOTCH  = 0xFFE4ECF8;   // 0 dB notch mark — ice blue from logo, +10%
        constexpr uint32_t SLIDER_BORDER = 0xFF385460;   // track border
        constexpr uint32_t METER_GREEN   = 0xFF28522E;   // below -12 dB
        constexpr uint32_t METER_YELLOW  = 0xFF88882E;   // above -12 dB
        constexpr uint32_t METER_ORANGE  = 0xFF885028;   // approaching 0 dB
        constexpr uint32_t METER_RED     = 0xFF882828;   // 0 dB and above (clip)
        constexpr uint32_t METER_BG      = 0xFF121820;   // meter background

        // Buttons — dark teal
        constexpr uint32_t BTN_BG        = 0xFF1C2A32;   // button background
        constexpr uint32_t BTN_BORDER    = 0xFF385460;   // button border
        constexpr uint32_t BTN_ACTIVE    = 0xFF80D8D0;   // active/selected button text (bright teal)
        constexpr uint32_t BTN_TEXT      = 0xFFC8D8D0;   // normal button text
        constexpr uint32_t BTN_LIT       = 0xFF285868;   // lit button background

        // Fractions — teal tones
        constexpr uint32_t FRAC_BG       = 0xFF1C2C34;   // fraction cell normal
        constexpr uint32_t FRAC_SELECT   = 0xFF386878;   // fraction cell selected
        constexpr uint32_t FRAC_BORDER   = 0xFF2E4450;   // cell border
        constexpr uint32_t FRAC_TEXT     = 0xFFE0F0E8;   // fraction cell text

        // Text — warm whites
        constexpr uint32_t LABEL         = 0xFFF0F5F0;   // primary label text
        constexpr uint32_t LABEL_DIM     = 0xFFC8D8D0;   // dimmed/secondary text
        constexpr uint32_t SECTION_HDR   = 0xFFC8D8D0;   // section header text
        constexpr uint32_t FAFO_TEXT     = 0xFFF0E0C0;   // FAFO-specific label color (warm gold)

        // Delay readout
        constexpr uint32_t READOUT_BG    = 0xFF121820;   // readout background
        constexpr uint32_t READOUT_BORDER= 0xFF2E4450;   // readout border
        constexpr uint32_t READOUT_TEXT  = 0xFFE0F0E8;   // readout text

        // LED — functional colors unchanged
        constexpr uint32_t LED_GREEN     = 0xFF3CC850;   // onset LED green (Ext SC)
        constexpr uint32_t LED_RED       = 0xFFC83C3C;   // onset LED red (MIDI)
        constexpr uint32_t LED_BLUE      = 0xFF50A0E0;   // onset LED blue (Direct In)
        constexpr uint32_t LED_OFF       = 0xFF283840;   // LED off state

        // Link button
        constexpr uint32_t LINK_BG       = 0xFF283840;
        constexpr uint32_t LINK_ACTIVE   = 0xFF50A8A0;
        constexpr uint32_t LINK_TEXT     = 0xFFD4A84A;

        // OutFilterRow A/B tabs
        constexpr juce::uint32 TAB_ACTIVE   = 0xffd4a84a;  // active gold
        constexpr juce::uint32 TAB_INACTIVE = 0xffe9d3a4;  // washed lighter gold
        constexpr juce::uint32 TAB_TEXT_OFF = 0xff666666;  // gray text on inactive
        constexpr juce::uint32 TAB_TEXT_ON  = 0xff1a1a1a;  // dark text on active
        constexpr juce::uint32 ROW_LABEL    = 0xffd4a84a;  // matches active gold

        // FAFO gold subtheme
        constexpr uint32_t FAFO_TRACK_BG    = 0xFF3E3418;   // FAFO slider track background (dark gold)
        constexpr uint32_t FAFO_TRACK_BORDER= 0xFF8A7030;   // FAFO slider track fill (gold)
        constexpr uint32_t FAFO_THUMB_FILL  = 0xFF8A7030;   // FAFO thumb — matches fill
        constexpr uint32_t FAFO_THUMB_STROKE= 0xFFE8D090;   // FAFO thumb stroke (light gold)

        // Mode accent — Way Music logo gold
        constexpr uint32_t MODE_ACCENT        = 0xFFE8C840;   // golden yellow from Way Music logo
        constexpr uint32_t MODE_ACCENT_BRIGHT = 0xFFFFE88A;   // MODE_ACCENT lifted toward white for "This Is SANE/FAFO" emphasis

        // Branding
        constexpr uint32_t BRAND_TEXT    = 0xFF90B0A8;
        constexpr uint32_t DIM_LABEL     = 0xFF507068;   // dim-label: "prime" annotations

        // OutFilterRow frequency-cell gradient centre
        constexpr uint32_t FREQ_BRIGHT   = 0xFF80B0C0;   // freq-cell gradient center (brighter than FRAC_SELECT, dimmer than FRAC_TEXT)

        // River (cascade delay meter)
        constexpr uint32_t RIVER_BG      = 0xFF0A1018;   // darker than ZONE_BG
    }

    //==========================================================================
    // RIVER — cascade delay meter (Sane mode, spans all three columns)
    //==========================================================================
    constexpr int   RIVER_Y       = 197;       // top edge (below Feedback row bottom)
    constexpr int   RIVER_H       = 313;       // height: y=197 to y=510 (above Chan A)
    constexpr int   RIVER_NOTCH   = 15;        // px per cascade level at default scale
    constexpr float RIVER_TIME_MS = 3000.0f;   // 3 seconds per half-width

    //==========================================================================
    // FONT SIZES (at default 1400x820)
    //==========================================================================
    namespace Font
    {
        constexpr float TITLE_SIZE     = 47.3f;    // "Rhythm" / "Echo"       (30 * 1.26 * 1.25)
        constexpr float ROW_LABEL_SIZE = 25.5f;    // row labels              (22.1 * 1.15)
        constexpr float SUB_LABEL_SIZE = 17.4f;    // sub-labels, annotations (11 * 1.26 * 1.25)
        constexpr float FRAC_SIZE      = 18.9f;    // fraction cell text      (12 * 1.26 * 1.25)
        constexpr float FRAC_HDR_SIZE  = 15.8f;    // fraction header         (10 * 1.26 * 1.25)
        constexpr float READOUT_SIZE   = 20.5f;    // delay readout text      (13 * 1.26 * 1.25)
        constexpr float BTN_SIZE       = 20.5f;    // button text             (13 * 1.26 * 1.25)
        constexpr float BRAND_SIZE      = 17.4f;    // branding version line   (11 * 1.26 * 1.25)
        constexpr float BRAND_SMALL_SIZE = 11.0f;   // branding long lines — sized to fit 130px center column
        constexpr float LINK_SIZE      = 15.8f;    // link button "L"         (10 * 1.26 * 1.25)
    }

    //==========================================================================
    // FRACTIONS — exact values from PD hradio lists
    //==========================================================================
    namespace Frac
    {
        // Numerator: 13 cells
        constexpr int NUM_COUNT = 13;
        constexpr const char* numLabels[NUM_COUNT] = {
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "12", "15", "16", "17"
        };
        constexpr int numDefaultIndex = 0;

        // Denominator: 11 main + empty + 4 primes = 16 cells
        constexpr int DEN_MAIN_COUNT  = 11;
        constexpr int DEN_PRIME_COUNT = 4;
        constexpr int DEN_TOTAL       = 15;

        constexpr const char* denMainLabels[DEN_MAIN_COUNT] = {
            "1", "2", "3", "4", "8", "12", "16", "27", "32", "48", "64"
        };
        constexpr const char* denPrimeLabels[DEN_PRIME_COUNT] = {
            "7", "11", "13", "17"
        };
        // Combined: 11 main + 1 empty separator + 4 primes = 16 cells
        constexpr int DEN_COMBINED_COUNT = 16;
        constexpr const char* denCombinedLabels[DEN_COMBINED_COUNT] = {
            "1", "2", "3", "4", "8", "12", "16", "27", "32", "48", "64",
            "",   // empty non-clickable separator
            "7", "11", "13", "17"
        };
        // Index of the empty separator cell (non-clickable)
        constexpr int DEN_SEPARATOR_INDEX = 11;
        constexpr int denDefaultIndex = 0;
    }

    //==========================================================================
    // CONTROL RANGES
    //==========================================================================
    namespace Range
    {
        // I/O gains (linear)
        constexpr float IN_DEFAULT  = 1.0f;
        constexpr float SC_DEFAULT  = 1.0f;
        constexpr float OUT_DEFAULT = 1.0f;

        // Dry/Wet
        constexpr float DW_MIN = 0.0f,   DW_MAX = 1.0f,    DW_DEFAULT = 0.5f;

        // Feedback
        constexpr float FB_MIN = 0.0f,   FB_MAX = 1.0f,    FB_DEFAULT = 0.33f;

        // XFeedback
        constexpr float XFB_MIN = 0.0f,  XFB_MAX = 0.9f,   XFB_DEFAULT = 0.0f;

        // Sensitivity
        constexpr float SN_MIN = 0.5f,   SN_MAX = 20.0f,   SN_DEFAULT = 5.0f;

        // Min Del (combined antichatter + comb, Issue #24)
        // Range matches antichatter: 0.5-200ms log scale, default 50ms.
        constexpr float MINDEL_MIN = 0.5f, MINDEL_MAX = 200.0f, MINDEL_DEFAULT = 25.0f;

        // Fade / Swell (FAFO)
        constexpr float FADE_MIN  = 3.0f,  FADE_MAX  = 200.0f, FADE_DEFAULT  = 20.0f;
        constexpr float SWELL_MIN = 3.0f,  SWELL_MAX = 200.0f, SWELL_DEFAULT = 20.0f;

        // SC / Output Filters
        constexpr float FILTER_MIN = 20.0f, FILTER_MAX = 20000.0f;
        constexpr float HP_DEFAULT = 20.0f, LP_DEFAULT = 20000.0f;

        // Slew speed
        constexpr float SLEW_MIN = 0.0f, SLEW_MAX = 1.0f, SLEW_DEFAULT = 0.0f;
        // Throw distance
        constexpr float THROW_MIN = 0.0f, THROW_MAX = 1.0f, THROW_DEFAULT = 0.0f;

        // Balance
        constexpr float BAL_MIN = -1.0f, BAL_MAX = 1.0f, BAL_DEFAULT = 0.0f;

        // Output gain
        constexpr float OUTGAIN_MIN = 0.0f, OUTGAIN_MAX = 2.0f, OUTGAIN_DEFAULT = 1.0f;
    }
}

// Short aliases used by all GUI components
namespace GC = GUIConstants::Color;
namespace GF = GUIConstants::Font;
namespace GR = GUIConstants::Range;
