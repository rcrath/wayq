#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class DragTooltipProvider
{
public:
    virtual ~DragTooltipProvider() = default;
    virtual bool isDraggingForTooltip() const = 0;
    virtual juce::String getDragTooltip() const = 0;
};
