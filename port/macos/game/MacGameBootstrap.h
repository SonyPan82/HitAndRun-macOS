#pragma once

// Cocoa owns the application loop. These functions bridge it to the original
// Game singleton without giving legacy PC code ownership of the window.
bool MacGameBootstrapCreate();
bool MacGameBootstrapInitialize();
void MacGameBootstrapTick();
void MacGameBootstrapShutdown();
