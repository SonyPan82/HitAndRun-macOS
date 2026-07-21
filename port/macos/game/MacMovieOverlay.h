#pragma once

// Native fullscreen movie surface hosted above the OpenGL game view.
bool MacMovieOverlayPlay(const char* path, float volume);
void MacMovieOverlayStop();
void MacMovieOverlayPause();
void MacMovieOverlayUnpause();
bool MacMovieOverlayIsPlaying();
float MacMovieOverlayElapsedSeconds();
