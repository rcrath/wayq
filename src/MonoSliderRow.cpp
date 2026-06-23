#include "MonoSliderRow.h"
#include "WayQLookAndFeel.h"

MonoSliderRow::MonoSliderRow (const juce::String& labelText,
                              juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramId)
    : labelText_ (labelText)
{
    addAndMakeVisible (slider_);

    att_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider_);

    if (auto* param = apvts.getParameter (paramId))
        slider_.setDefaultValue (param->getDefaultValue() * (slider_.getMaximum() - slider_.getMinimum()) + slider_.getMinimum());
}

void MonoSliderRow::resized()
{
    float kW      = getWidth() / 626.0f;
    int workareaX = (int)(GUIConstants::WORKAREA_X * kW);
    int workareaW = (int)(GUIConstants::WORKAREA_W * kW);
    slider_.setBounds (workareaX, 0, workareaW, getHeight());
}

void MonoSliderRow::paint (juce::Graphics& g)
{
    float kW = getWidth() / 626.0f;
    auto labelArea = juce::Rectangle<int> (0, 0, (int)(GUIConstants::WORKAREA_X * kW), getHeight());

    g.setColour (juce::Colour (GC::LABEL));
    g.setFont (getAveriaFont (GF::ROW_LABEL_SIZE));
    g.drawText (labelText_, labelArea.reduced (4, 0), juce::Justification::centredRight);
}
