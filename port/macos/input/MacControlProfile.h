#pragma once

#include <array>

// Native, layout-independent default bindings.  Values are macOS hardware
// key codes and are persisted separately from the legacy PC configuration.
enum class MacBindableAction : unsigned char { MoveUp, MoveDown, MoveLeft, MoveRight, Jump, Sprint, Confirm, Back, Pause, Count };

struct MacControlProfile
{
    std::array<unsigned short, static_cast<unsigned>(MacBindableAction::Count)> keys =
        { 13, 1, 0, 2, 49, 56, 36, 53, 35 }; // WASD, Space, Shift, Return, Esc, P
    float trackpadSensitivity = 1.0f;
    bool invertTrackpadY = false;
};

MacControlProfile MacControlProfileLoad();
void MacControlProfileSave(const MacControlProfile& profile);
