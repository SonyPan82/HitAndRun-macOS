#import <Foundation/Foundation.h>
// GameController is used below only for physical gamepads (joysticks are a
// genuine HID device, so a controller abstraction is appropriate for them).
// The keyboard is intentionally NOT read through GCKeyboard: unlike NSEvent,
// GCKeyboard's coalescedKeyboard reports physical key state independent of
// which window/app is key, so it can observe keystrokes the player types
// into other applications. All keyboard state below is fed exclusively by
// window-scoped NSEvent handling in MacHost.mm.
#import <GameController/GameController.h>

#include "MacInput.h"

#include <algorithm>
#include <cstring>

struct MacInputManager
{
    bool keyboard[MAC_ACTION_COUNT] = {};
    bool legacyKeys[256] = {};
    float trackpadX = 0.0f;
    float trackpadY = 0.0f;
    float trackpadDeltaX = 0.0f;
    float trackpadDeltaY = 0.0f;
    bool trackpadButtons[2] = {};
};

static MacInputManager* gSharedInput = nullptr;

static float ClampAxis(float value)
{
    constexpr float deadZone = 0.12f;
    if (value > -deadZone && value < deadZone)
        return 0.0f;
    return std::clamp(value, -1.0f, 1.0f);
}

static bool Pressed(GCControllerButtonInput* button)
{
    return button != nil && button.isPressed;
}

static float PovFromDirection(float x, float y)
{
    constexpr float threshold = 0.35f;
    const int horizontal = x > threshold ? 1 : (x < -threshold ? -1 : 0);
    const int vertical = y > threshold ? 1 : (y < -threshold ? -1 : 0);
    if (horizontal == 0 && vertical == 0) return 1.0f;
    if (vertical > 0) return horizontal > 0 ? 0.125f : (horizontal < 0 ? 0.875f : 0.0f);
    if (vertical < 0) return horizontal > 0 ? 0.375f : (horizontal < 0 ? 0.625f : 0.5f);
    return horizontal > 0 ? 0.25f : 0.75f;
}

MacInputManager* MacInputCreate(void)
{
    MacInputManager* manager = new MacInputManager();
    return manager;
}

MacInputManager* MacInputShared(void)
{
    if (gSharedInput == nullptr)
        gSharedInput = MacInputCreate();
    return gSharedInput;
}

void MacInputResetKeyboard(MacInputManager* manager)
{
    if (manager == nullptr)
        return;
    // Called when the app/window loses key status. Without this, a key held
    // down at the moment focus is lost never gets its keyUp (that keyUp goes
    // to whichever app the player switched to instead), leaving the game
    // convinced the player is still holding it down indefinitely.
    std::memset(manager->keyboard, 0, sizeof(manager->keyboard));
    std::memset(manager->legacyKeys, 0, sizeof(manager->legacyKeys));
}

void MacInputDestroy(MacInputManager* manager)
{
    if (manager == gSharedInput)
        gSharedInput = nullptr;
    delete manager;
}

void MacInputSetKeyboardAction(MacInputManager* manager, MacGameAction action, bool pressed)
{
    if (manager != nullptr && action >= 0 && action < MAC_ACTION_COUNT)
        manager->keyboard[action] = pressed;
}

void MacInputSetLegacyKey(MacInputManager* manager, int keyCode, bool pressed)
{
    if (manager != nullptr && keyCode >= 0 && keyCode < 256)
        manager->legacyKeys[keyCode] = pressed;
}

void MacInputSetTrackpad(MacInputManager* manager, float x, float y, float deltaX, float deltaY)
{
    if (manager == nullptr)
        return;

    manager->trackpadX = x;
    manager->trackpadY = y;
    manager->trackpadDeltaX += deltaX;
    manager->trackpadDeltaY += deltaY;
}

void MacInputSetTrackpadButton(MacInputManager* manager, int button, bool pressed)
{
    if (manager != nullptr && button >= 0 && button < 2)
        manager->trackpadButtons[button] = pressed;
}

void MacInputUpdate(MacInputManager* manager, MacInputState* state)
{
    if (manager == nullptr || state == nullptr)
        return;

    std::memset(state, 0, sizeof(*state));
    state->gamepadPov0 = 1.0f;
    for (int action = 0; action < MAC_ACTION_COUNT; ++action)
        state->actions[action] = manager->keyboard[action];
    std::memcpy(state->legacyKeys, manager->legacyKeys, sizeof(state->legacyKeys));

    state->trackpadX = manager->trackpadX;
    state->trackpadY = manager->trackpadY;
    state->trackpadDeltaX = manager->trackpadDeltaX;
    state->trackpadDeltaY = manager->trackpadDeltaY;
    std::memcpy(state->trackpadButtons, manager->trackpadButtons, sizeof(state->trackpadButtons));
    manager->trackpadDeltaX = 0.0f;
    manager->trackpadDeltaY = 0.0f;

    for (GCController* controller in GCController.controllers)
    {
        GCExtendedGamepad* pad = controller.extendedGamepad;
        if (pad == nil)
            continue;

        ++state->connectedControllerCount;
        // GameController normalises the physical layouts of Xbox, PlayStation
        // and Switch Pro pads into these logical controls.
        const float steering = ClampAxis(pad.leftThumbstick.xAxis.value);
        if (std::abs(steering) > std::abs(state->steering)) state->steering = steering;
        const float moveY = ClampAxis(pad.leftThumbstick.yAxis.value);
        if (std::abs(moveY) > std::abs(state->gamepadMoveY)) state->gamepadMoveY = moveY;
        state->throttle = std::max(state->throttle, pad.rightTrigger.value);
        state->brake = std::max(state->brake, pad.leftTrigger.value);
        state->cameraX = ClampAxis(pad.rightThumbstick.xAxis.value);
        state->cameraY = ClampAxis(pad.rightThumbstick.yAxis.value);
        state->actions[MAC_ACTION_ACCELERATE] |= pad.rightTrigger.value > 0.05f;
        state->actions[MAC_ACTION_BRAKE] |= pad.leftTrigger.value > 0.05f;
        state->actions[MAC_ACTION_STEER_LEFT] |= pad.leftThumbstick.xAxis.value < -0.35f;
        state->actions[MAC_ACTION_STEER_RIGHT] |= pad.leftThumbstick.xAxis.value > 0.35f;
        state->actions[MAC_ACTION_CONFIRM] |= Pressed(pad.buttonA);
        state->actions[MAC_ACTION_CANCEL] |= Pressed(pad.buttonB);
        state->actions[MAC_ACTION_ACTION] |= Pressed(pad.buttonX);
        state->actions[MAC_ACTION_JUMP] |= Pressed(pad.buttonY);
        state->actions[MAC_ACTION_PAUSE] |= Pressed(pad.buttonMenu);
        state->actions[MAC_ACTION_SPRINT] |= Pressed(pad.rightShoulder);
        state->actions[MAC_ACTION_MENU_UP] |= pad.dpad.yAxis.value > 0.35f;
        state->actions[MAC_ACTION_MENU_DOWN] |= pad.dpad.yAxis.value < -0.35f;
        state->actions[MAC_ACTION_MENU_LEFT] |= pad.dpad.xAxis.value < -0.35f;
        state->actions[MAC_ACTION_MENU_RIGHT] |= pad.dpad.xAxis.value > 0.35f;

        state->gamepadButtons[0] |= Pressed(pad.buttonA);
        state->gamepadButtons[1] |= Pressed(pad.buttonB);
        state->gamepadButtons[2] |= Pressed(pad.buttonX);
        state->gamepadButtons[3] |= Pressed(pad.buttonY);
        state->gamepadButtons[4] |= Pressed(pad.leftShoulder);
        state->gamepadButtons[5] |= Pressed(pad.rightShoulder);
        state->gamepadButtons[6] |= pad.leftTrigger.value > 0.05f;
        state->gamepadButtons[7] |= pad.rightTrigger.value > 0.05f;
        state->gamepadButtons[8] |= Pressed(pad.buttonMenu);
        const float pov = PovFromDirection(pad.dpad.xAxis.value, pad.dpad.yAxis.value);
        if (pov < 1.0f) state->gamepadPov0 = pov;
    }
}
