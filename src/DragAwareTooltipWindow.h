#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DragTooltipProvider.h"

// TooltipWindow subclass that keeps showing a tip WHILE a mouse button is held,
// but only for components that opt in via DragTooltipProvider (e.g. the EQ graph
// while a handle is being dragged). Stock JUCE suppresses tooltips during a drag.
class DragAwareTooltipWindow : public juce::TooltipWindow
{
public:
    using juce::TooltipWindow::TooltipWindow;

    juce::String getTipFor (juce::Component& c) override
    {
        if (juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        {
            if (auto* dtp = dynamic_cast<DragTooltipProvider*> (&c))
                if (dtp->isDraggingForTooltip())
                    return dtp->getDragTooltip();
            return {};
        }
        return juce::TooltipWindow::getTipFor (c);
    }
};
