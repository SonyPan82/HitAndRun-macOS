// The Win32 executable embeds a fallback font for the SuperSprint debug
// overlay. Normal game UI fonts are loaded from the supplied PC assets.
// Reserve the historical payload size so this optional fallback never leaves
// an unresolved external on macOS.
unsigned char gFont[61075] = {};
