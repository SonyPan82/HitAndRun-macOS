#pragma once

#include <sound/soundfx/reverbcontroller.h>

// CoreAudio integration is supplied by the macOS sound backend.  Until that
// backend exposes an auxiliary reverb bus, gameplay keeps a no-op controller
// so entering interiors never depends on console-specific effects.
class MacReverbController : public ReverbController
{
public:
    void SetReverbOn( reverbSettings* settings ) override;
    void SetReverbOff() override;
};
