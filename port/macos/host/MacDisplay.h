#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Bridge used by the original PC display-options screen.  Cocoa owns the
// window, while the legacy menu supplies its persisted resolution index.
int MacDisplayGetResolution(void);
int MacDisplayGetBPP(void);
int MacDisplayIsFullscreen(void);
int MacDisplaySetMode(int resolution, int bpp, int fullscreen);

#ifdef __cplusplus
}
#endif
