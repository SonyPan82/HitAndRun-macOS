#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MacInputManager MacInputManager;

typedef enum MacGameAction
{
    MAC_ACTION_ACCELERATE,
    MAC_ACTION_BRAKE,
    MAC_ACTION_STEER_LEFT,
    MAC_ACTION_STEER_RIGHT,
    MAC_ACTION_ACTION,
    MAC_ACTION_JUMP,
    MAC_ACTION_CONFIRM,
    MAC_ACTION_CANCEL,
    MAC_ACTION_PAUSE,
    MAC_ACTION_SPRINT,
    MAC_ACTION_MENU_UP,
    MAC_ACTION_MENU_DOWN,
    MAC_ACTION_MENU_LEFT,
    MAC_ACTION_MENU_RIGHT,
    MAC_ACTION_COUNT
} MacGameAction;

typedef struct MacInputState
{
    float steering;
    float gamepadMoveY;
    float throttle;
    float brake;
    float cameraX;
    float cameraY;
    float trackpadX;
    float trackpadY;
    float trackpadDeltaX;
    float trackpadDeltaY;
    bool trackpadButtons[2];
    // Normalised DirectInput-era layout used by the PC game's controller
    // bindings: 0=A/Cross, 1=B/Circle, 2=X/Square, 3=Y/Triangle,
    // 4=L1, 5=R1, 6=L2, 7=R2, 8=Menu/Options.
    bool gamepadButtons[32];
    float gamepadPov0;
    bool actions[MAC_ACTION_COUNT];
    bool legacyKeys[256];
    uint32_t connectedControllerCount;
} MacInputState;

MacInputManager* MacInputCreate(void);
void MacInputDestroy(MacInputManager* manager);
MacInputManager* MacInputShared(void);
void MacInputUpdate(MacInputManager* manager, MacInputState* state);
// Clears all latched key state. Call when the app or window loses key
// status so a key press mid-hold cannot get stuck "on" forever.
void MacInputResetKeyboard(MacInputManager* manager);

// Called by the future Cocoa window bridge for native keyboard/trackpad input.
void MacInputSetKeyboardAction(MacInputManager* manager, MacGameAction action, bool pressed);
void MacInputSetLegacyKey(MacInputManager* manager, int keyCode, bool pressed);
void MacInputSetTrackpad(MacInputManager* manager, float x, float y, float deltaX, float deltaY);
void MacInputSetTrackpadButton(MacInputManager* manager, int button, bool pressed);

#ifdef __cplusplus
}
#endif
