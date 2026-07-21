#import <AppKit/AppKit.h>
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>

#include "MacMovieOverlay.h"

namespace
{
AVPlayerView* gMovieView = nil;
AVPlayer* gMoviePlayer = nil;
id gEndObserver = nil;
bool gMovieFinished = false;

void RemoveMovieView()
{
    if (gEndObserver != nil)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:gEndObserver];
        gEndObserver = nil;
    }
    [gMoviePlayer pause];
    gMoviePlayer = nil;
    [gMovieView removeFromSuperview];
    gMovieView = nil;
}
}

bool MacMovieOverlayPlay(const char* path, float volume)
{
    if (path == nullptr || path[0] == '\0') return false;
    NSWindow* window = NSApp.keyWindow;
    if (window == nil) window = NSApp.mainWindow;
    NSView* host = window.contentView;
    NSString* moviePath = [NSString stringWithUTF8String:path];
    if (host == nil || ![[NSFileManager defaultManager] fileExistsAtPath:moviePath]) return false;

    RemoveMovieView();
    gMovieFinished = false;
    AVPlayerItem* item = [AVPlayerItem playerItemWithURL:[NSURL fileURLWithPath:moviePath]];
    gMoviePlayer = [AVPlayer playerWithPlayerItem:item];
    gMoviePlayer.volume = volume;
    gMovieView = [[AVPlayerView alloc] initWithFrame:host.bounds];
    gMovieView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    // AVPlayerView already follows AppKit's coordinate system.  The former
    // OpenGL-style Y flip inverted every FMV image.
    gMovieView.player = gMoviePlayer;
    gMovieView.controlsStyle = AVPlayerViewControlsStyleNone;
    gMovieView.videoGravity = AVLayerVideoGravityResizeAspect;
    [host addSubview:gMovieView positioned:NSWindowAbove relativeTo:nil];
    [window makeFirstResponder:host];
    gEndObserver = [[NSNotificationCenter defaultCenter]
        addObserverForName:AVPlayerItemDidPlayToEndTimeNotification
                    object:item
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification*) { gMovieFinished = true; }];
    [gMoviePlayer play];
    return true;
}

void MacMovieOverlayStop() { RemoveMovieView(); gMovieFinished = true; }
void MacMovieOverlayPause() { [gMoviePlayer pause]; }
void MacMovieOverlayUnpause() { [gMoviePlayer play]; }
bool MacMovieOverlayIsPlaying()
{
    return gMoviePlayer != nil && !gMovieFinished && gMoviePlayer.timeControlStatus != AVPlayerTimeControlStatusPaused;
}
float MacMovieOverlayElapsedSeconds()
{
    return gMoviePlayer != nil ? static_cast<float>(CMTimeGetSeconds(gMoviePlayer.currentTime)) : 0.0f;
}
