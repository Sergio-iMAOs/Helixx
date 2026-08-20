#include "HelixxComponent.h"

namespace helixx
{

HelixxComponent::HelixxComponent(Helixx& helixx)
    :
    helixx(helixx)
{
}

HelixxComponent::~HelixxComponent()
{
}

void HelixxComponent::paint(juce::Graphics& /*g*/)
{
}

void HelixxComponent::resized()
{
}

} // namespace helixx