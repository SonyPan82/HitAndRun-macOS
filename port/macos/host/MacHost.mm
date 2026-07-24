#import <AppKit/AppKit.h>
#import <OpenGL/gl.h>

#include <cctype>
#include <cmath>
#include <cstdlib>

#include "MacInput.h"
#include "../input/MacControlProfile.h"
#include "MacGameData.h"
#include "MacDisplay.h"
#include "../game/MacGameBootstrap.h"
#include <radmemory.hpp>
#include <radthread.hpp>
#include <radtime.hpp>
#include <radfile.hpp>
#include <radload/radload.hpp>
#include <p3d/platform/macos/platform.hpp>
#include <p3d/utility.hpp>

static MacGameAction ActionForKey(unsigned short keyCode, bool* known)
{
    static const MacControlProfile profile = MacControlProfileLoad();
    *known = true;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::MoveUp)]) return MAC_ACTION_ACCELERATE;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::MoveDown)]) return MAC_ACTION_BRAKE;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::MoveLeft)]) return MAC_ACTION_STEER_LEFT;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::MoveRight)]) return MAC_ACTION_STEER_RIGHT;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Jump)]) return MAC_ACTION_JUMP;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Action)]) return MAC_ACTION_ACTION;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Sprint)]) return MAC_ACTION_SPRINT;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Confirm)]) return MAC_ACTION_CONFIRM;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Back)]) return MAC_ACTION_CANCEL;
    if (keyCode == profile.keys[static_cast<unsigned>(MacBindableAction::Pause)]) return MAC_ACTION_PAUSE;
    // Hardware key codes keep mappings independent from the selected layout.
    switch (keyCode)
    {
        case 13: return MAC_ACTION_ACCELERATE;  // W
        case 1:  return MAC_ACTION_BRAKE;       // S
        case 0:  return MAC_ACTION_STEER_LEFT;  // A
        case 2:  return MAC_ACTION_STEER_RIGHT; // D
        case 49: return MAC_ACTION_JUMP;        // Space
        case 14: return MAC_ACTION_ACTION;      // E (interact / enter / exit vehicle)
        case 56: return MAC_ACTION_SPRINT;      // Left Shift
        case 36: return MAC_ACTION_CONFIRM;     // Return
        case 53: return MAC_ACTION_CANCEL;      // Escape
        case 35: return MAC_ACTION_PAUSE;       // P
        case 12: return MAC_ACTION_JUMP;        // Q
        case 123: return MAC_ACTION_MENU_LEFT;  // Left arrow
        case 124: return MAC_ACTION_MENU_RIGHT; // Right arrow
        case 125: return MAC_ACTION_MENU_DOWN;  // Down arrow
        case 126: return MAC_ACTION_MENU_UP;    // Up arrow
        default:
            *known = false;
            return MAC_ACTION_COUNT;
    }
}

// Hardware codes are stable for custom bindings, while the character is what
// a player sees on an AZERTY keyboard.  Support both paths explicitly.
static MacGameAction ActionForEvent(NSEvent* event, bool* known)
{
    MacGameAction action = ActionForKey(event.keyCode, known);
    if (*known)
        return action;

    NSString* characters = event.charactersIgnoringModifiers;
    if (characters == nil || characters.length == 0)
        return action;
    const unichar character = std::tolower([characters characterAtIndex:0]);
    *known = true;
    switch (character)
    {
        case 'w':
        case 'z': return MAC_ACTION_ACCELERATE;
        case 's': return MAC_ACTION_BRAKE;
        case 'a':
        case 'q': return MAC_ACTION_STEER_LEFT;
        case 'd': return MAC_ACTION_STEER_RIGHT;
        case 'e': return MAC_ACTION_ACTION;
        case ' ': return MAC_ACTION_JUMP;
        default:
            *known = false;
            return MAC_ACTION_COUNT;
    }
}

// Keyboard input is captured exactly once, via the app-wide local event
// monitor installed in -applicationDidFinishLaunching. That monitor already
// covers legacy fullscreen/context transitions that can replace the view's
// first responder (it doesn't depend on first-responder status), so no
// NSApplication::sendEvent override or NSView keyDown/keyUp handlers are
// needed here -- keeping a single path avoids the previous double-dispatch
// and makes the capture scope easy to audit.

static int LegacyKeyForKey(unsigned short keyCode)
{
    switch (keyCode)
    {
        case 0: return 0x1e;  // A
        case 1: return 0x1f;  // S
        case 2: return 0x20;  // D
        case 12: return 0x10; // Q
        case 13: return 0x11; // W
        case 35: return 0x19; // P
        case 36: return 0x1c; // Return
        case 49: return 0x39; // Space
        case 53: return 0x01; // Escape
        case 122: return 0x3b; // F1
        case 56: return 0x2a; // Left Shift
        case 60: return 0x36; // Right Shift
        case 123: return 0xcb; // Left
        case 124: return 0xcd; // Right
        case 125: return 0xd0; // Down
        case 126: return 0xc8; // Up
        default: return -1;
    }
}

// Key codes identify physical keys, which is ideal for a fixed QWERTY
// layout.  The original PC prompts are nevertheless read as characters by
// players, so accept their AZERTY and QWERTY labels as aliases as well.
static int LegacyKeyForCharacters(NSString* characters)
{
    if (characters == nil || characters.length == 0)
        return -1;

    const unichar character = std::tolower([characters characterAtIndex:0]);
    switch (character)
    {
        case 'w':
        case 'z': return 0x11; // W / AZERTY Z: advance or accelerate
        case 's': return 0x1f; // S: reverse or brake
        case 'a':
        case 'q': return 0x1e; // A / AZERTY Q: move or steer left
        case 'd': return 0x20; // D: move or steer right
        case ' ': return 0x39;
        default: return -1;
    }
}

// The PC defaults use WASD, but Mac users naturally try the arrow keys too.
// Feed those as the PC movement scan codes as well, while retaining their
// native navigation codes for the front end.
static int GameplayAliasForKey(unsigned short keyCode)
{
    switch (keyCode)
    {
        case 123: return 0x1e; // Left -> A
        case 124: return 0x20; // Right -> D
        case 125: return 0x1f; // Down -> S
        case 126: return 0x11; // Up -> W
        default: return -1;
    }
}

static int ResolutionIndexForSize(NSSize size)
{
    const NSSize sizes[] = {
        { 640.0, 480.0 }, { 800.0, 600.0 }, { 1024.0, 768.0 },
        { 1152.0, 864.0 }, { 1280.0, 1024.0 }, { 1600.0, 1200.0 }
    };
    int best = 0;
    CGFloat bestDistance = CGFLOAT_MAX;
    for (int i = 0; i < 6; ++i)
    {
        const CGFloat distance = std::abs(size.width - sizes[i].width) + std::abs(size.height - sizes[i].height);
        if (distance < bestDistance) { bestDistance = distance; best = i; }
    }
    return best;
}

static NSSize SizeForResolutionIndex(int resolution)
{
    const NSSize sizes[] = {
        { 640.0, 480.0 }, { 800.0, 600.0 }, { 1024.0, 768.0 },
        { 1152.0, 864.0 }, { 1280.0, 1024.0 }, { 1600.0, 1200.0 }
    };
    return sizes[std::max(0, std::min(resolution, 5))];
}

@interface MacInputView : NSOpenGLView
{
    MacInputManager* _input;
    NSPoint _lastPointer;
    NSTextField* _status;
    tPlatform* _p3dPlatform;
    tContext* _p3dContext;
    bool _gameInitialized;
    unsigned int _remainingSmokeTicks;
    unsigned int _testConfirmTick;
    unsigned int _testMoveTick;
    unsigned int _testJumpTick;
    unsigned int _gameTickCount;
}
- (instancetype)initWithFrame:(NSRect)frame input:(MacInputManager*)input;
- (void)gameTick;
- (void)updateStatus;
@end

@implementation MacInputView

- (instancetype)initWithFrame:(NSRect)frame input:(MacInputManager*)input
{
    NSOpenGLPixelFormatAttribute attributes[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFADoubleBuffer,
        0
    };
    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    self = [super initWithFrame:frame pixelFormat:format];
    if (self != nil)
    {
        // The original UI is authored in fixed PC pixels.  A Retina drawable
        // silently doubled the framebuffer while the game still supplied
        // 640/1280-era HUD coordinates, producing oversized and clipped
        // mini-map and tutorial sprites.
        self.wantsBestResolutionOpenGLSurface = NO;
        _input = input;
        [[self openGLContext] makeCurrentContext];
        GLint swapInterval = 1;
        [[self openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
        _p3dPlatform = tPlatform::Create((__bridge void*)[self openGLContext]);
        _p3dContext = nullptr;
        _gameInitialized = false;
        _remainingSmokeTicks = 0;
        _testConfirmTick = 0;
        _testMoveTick = 0;
        _testJumpTick = 0;
        _gameTickCount = 0;
        if (const char* smokeTicks = std::getenv("HMR_SMOKE_TICKS"); smokeTicks != nullptr)
            _remainingSmokeTicks = static_cast<unsigned int>(std::strtoul(smokeTicks, nullptr, 10));
        if (const char* confirmTick = std::getenv("HMR_TEST_CONFIRM_TICK"); confirmTick != nullptr)
            _testConfirmTick = static_cast<unsigned int>(std::strtoul(confirmTick, nullptr, 10));
        if (const char* moveTick = std::getenv("HMR_TEST_MOVE_TICK"); moveTick != nullptr)
            _testMoveTick = static_cast<unsigned int>(std::strtoul(moveTick, nullptr, 10));
        if (const char* jumpTick = std::getenv("HMR_TEST_JUMP_TICK"); jumpTick != nullptr)
            _testJumpTick = static_cast<unsigned int>(std::strtoul(jumpTick, nullptr, 10));
        if (_p3dPlatform != nullptr)
        {
            tContextInitData init;
            init.xsize = static_cast<int>(frame.size.width);
            init.ysize = static_cast<int>(frame.size.height);
            init.bpp = 32;
            init.nativeContext = (__bridge void*)[self openGLContext];
            _p3dContext = _p3dPlatform->CreateContext(&init);
            if (_p3dContext != nullptr)
            {
                _p3dPlatform->SetActiveContext(_p3dContext);
                p3d::InstallDefaultLoaders();
                _gameInitialized = MacGameBootstrapInitialize();
                if (_gameInitialized)
                    _status.hidden = YES;
            }
        }
        _lastPointer = NSMakePoint(0.0, 0.0);
        _status = [[NSTextField alloc] initWithFrame:NSMakeRect(24, 110, 560, 42)];
        _status.editable = NO;
        _status.selectable = NO;
        _status.bordered = NO;
        _status.drawsBackground = NO;
        _status.font = [NSFont systemFontOfSize:16.0];
        _status.stringValue = @"Initialisation des entrées macOS…";
        _status.hidden = _gameInitialized;
        [self addSubview:_status];
        [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds
                                                            options:NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
                                                              owner:self
                                                           userInfo:nil]];
    }
    return self;
}

- (void)dealloc
{
    if (_gameInitialized)
    {
        MacGameBootstrapShutdown();
        _gameInitialized = false;
    }
    if (_p3dPlatform != nullptr && _p3dContext != nullptr)
    {
        _p3dPlatform->DestroyContext(_p3dContext);
        _p3dContext = nullptr;
    }
    if (_p3dPlatform != nullptr)
    {
        tPlatform::Destroy(_p3dPlatform);
        _p3dPlatform = nullptr;
    }
    [super dealloc];
}

- (BOOL)acceptsFirstResponder { return YES; }

- (void)mouseMoved:(NSEvent*)event
{
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    MacInputSetTrackpad(_input, point.x, point.y, point.x - _lastPointer.x, point.y - _lastPointer.y);
    _lastPointer = point;
}

- (void)scrollWheel:(NSEvent*)event
{
    MacInputSetTrackpad(_input, _lastPointer.x, _lastPointer.y, event.scrollingDeltaX, event.scrollingDeltaY);
}

- (void)mouseDown:(NSEvent*)event { MacInputSetTrackpadButton(_input, 0, true); }
- (void)mouseUp:(NSEvent*)event { MacInputSetTrackpadButton(_input, 0, false); }
- (void)rightMouseDown:(NSEvent*)event { MacInputSetTrackpadButton(_input, 1, true); }
- (void)rightMouseUp:(NSEvent*)event { MacInputSetTrackpadButton(_input, 1, false); }

- (void)reshape
{
    [super reshape];
    [[self openGLContext] update];
    if (_p3dContext != nullptr && _p3dContext->GetDisplay() != nullptr)
    {
        // wantsBestResolutionOpenGLSurface is NO (see initWithFrame:input:),
        // so the actual GL drawable is sized in points, not backing pixels.
        // convertRectToBacking: reports the screen's Retina backing scale
        // regardless of that per-surface override, so on a Retina display it
        // told the game the framebuffer was 2x larger than what was actually
        // being rendered into. The game then placed corner-anchored HUD
        // elements (mini-map, tutorial prompts) using that inflated canvas
        // size, landing them outside -- or only partially inside -- the real
        // viewport, which is what produced the corrupted-looking patches.
        const NSSize points = self.bounds.size;
        _p3dContext->GetDisplay()->InitDisplay(static_cast<int>(points.width), static_cast<int>(points.height), 32);
        if (std::getenv("HMR_INPUT_TRACE") != nullptr)
        {
            const NSRect backing = [self convertRectToBacking:self.bounds];
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            rReleasePrintf("macOS reshape: points=%dx%d backing=%dx%d scaleFactor=%.2f glViewport=%d,%d,%d,%d\n",
                            static_cast<int>(points.width), static_cast<int>(points.height),
                            static_cast<int>(backing.size.width), static_cast<int>(backing.size.height),
                            self.window != nil ? self.window.backingScaleFactor : -1.0,
                            viewport[0], viewport[1], viewport[2], viewport[3]);
        }
    }
}

- (void)drawRect:(NSRect)dirtyRect
{
    if (_gameInitialized)
    {
        return;
    }
    if (_p3dContext != nullptr && p3d::pddi != nullptr && p3d::display != nullptr)
    {
        p3d::pddi->BeginFrame();
        p3d::pddi->Clear(PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH);
        p3d::pddi->EndFrame();
        p3d::display->SwapBuffers();
        return;
    }
    glClearColor(0.075f, 0.11f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    [[self openGLContext] flushBuffer];
}

- (void)gameTick
{
    if (_gameInitialized)
    {
        ++_gameTickCount;
        if (_testConfirmTick != 0)
        {
            if (_gameTickCount == _testConfirmTick)
                MacInputSetKeyboardAction(_input, MAC_ACTION_CONFIRM, true);
            else if (_gameTickCount == _testConfirmTick + 1)
                MacInputSetKeyboardAction(_input, MAC_ACTION_CONFIRM, false);
        }
        if (_testMoveTick != 0)
        {
            if (_gameTickCount == _testMoveTick)
                MacInputSetKeyboardAction(_input, MAC_ACTION_ACCELERATE, true);
            else if (_gameTickCount == _testMoveTick + 60)
                MacInputSetKeyboardAction(_input, MAC_ACTION_ACCELERATE, false);
        }
        if (_testJumpTick != 0)
        {
            if (_gameTickCount == _testJumpTick)
                MacInputSetKeyboardAction(_input, MAC_ACTION_JUMP, true);
            else if (_gameTickCount == _testJumpTick + 1)
                MacInputSetKeyboardAction(_input, MAC_ACTION_JUMP, false);
        }
        MacGameBootstrapTick();
        [self setNeedsDisplay:YES];
        if (_remainingSmokeTicks != 0 && --_remainingSmokeTicks == 0)
            [NSApp terminate:nil];
    }
}

- (void)updateStatus
{
    if (_gameInitialized)
        return;
    MacInputState state;
    MacInputUpdate(_input, &state);
    std::string missing;
    const std::string dataRoot = MacGameDataRoot();
    if (!MacGameDataIsComplete(dataRoot, &missing))
    {
        _status.stringValue = [NSString stringWithFormat:@"Données PC introuvables : %s (%s)", dataRoot.c_str(), missing.c_str()];
        [self setNeedsDisplay:YES];
        return;
    }
    if (!_gameInitialized)
    {
        _status.stringValue = @"Le moteur de jeu macOS n’a pas pu s’initialiser.";
        return;
    }
    _status.stringValue = [NSString stringWithFormat:@"Données PC détectées — %u manette(s), direction %.2f, accélération %.2f", state.connectedControllerCount, state.steering, state.throttle];
    [self setNeedsDisplay:YES];
}
@end

@interface MacApplicationDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
{
    NSWindow* _window;
    MacInputManager* _input;
    MacInputView* _view;
    id _keyMonitor;
}
- (void)setDisplay720p:(id)sender;
- (void)setDisplay900p:(id)sender;
- (void)setDisplay1080p:(id)sender;
- (void)setGameDisplaySize:(NSSize)size fullscreen:(BOOL)fullscreen;
- (int)gameResolution;
- (BOOL)isGameFullscreen;
@end

static MacApplicationDelegate* gApplicationDelegate = nil;

@implementation MacApplicationDelegate

- (void)setDisplaySize:(NSSize)size
{
    [_window setContentSize:size];
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [_window makeFirstResponder:_view];
}

- (void)setGameDisplaySize:(NSSize)size fullscreen:(BOOL)fullscreen
{
    const BOOL isFullscreen = (_window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if (isFullscreen != fullscreen)
        [_window toggleFullScreen:nil];
    [self setDisplaySize:size];
}

- (int)gameResolution { return ResolutionIndexForSize(_window.contentView.bounds.size); }
- (BOOL)isGameFullscreen { return (_window.styleMask & NSWindowStyleMaskFullScreen) != 0; }

- (void)setDisplay720p:(id)sender { [self setDisplaySize:NSMakeSize(1280, 720)]; }
- (void)setDisplay900p:(id)sender { [self setDisplaySize:NSMakeSize(1600, 900)]; }
- (void)setDisplay1080p:(id)sender { [self setDisplaySize:NSMakeSize(1920, 1080)]; }

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    gApplicationDelegate = self;
    _input = MacInputShared();
    _window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1280, 720)
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"The Simpsons: Hit & Run — macOS (contrôles natifs r7)";
    _view = [[MacInputView alloc] initWithFrame:_window.contentView.bounds input:_input];
    _view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _window.contentView = _view;
    _window.delegate = self;
    [_window makeFirstResponder:_view];
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    // NSOpenGLView can lose first-responder status while the legacy game
    // switches contexts.  Consume recognised game keys at the application's
    // local event boundary and feed the native input state exactly once.
    _keyMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskKeyUp)
                                                          handler:^NSEvent* (NSEvent* event) {
        if ((event.modifierFlags & NSEventModifierFlagCommand) != 0)
            return event;
        if (std::getenv("HMR_INPUT_TRACE") != nullptr)
            rReleasePrintf("macOS event: type %ld key %hu chars %s\n", (long)event.type, event.keyCode,
                           event.characters.UTF8String != nullptr ? event.characters.UTF8String : "");
        bool known = false;
        MacGameAction action = ActionForEvent(event, &known);
        if (!known)
            return event;
        MacInputSetKeyboardAction(_input, action, event.type == NSEventTypeKeyDown);
        return nil;
    }];
    // If the player alt-tabs away (or the window otherwise loses key status)
    // while holding a movement key, the matching keyUp is delivered to
    // whichever app they switched to, not to us. Without this the game would
    // otherwise see that key as held down forever. Resigning key status also
    // guarantees no keyboard state changes while we are not the active app.
    [[NSNotificationCenter defaultCenter] addObserver:self
                                              selector:@selector(resetKeyboardState)
                                                  name:NSApplicationDidResignActiveNotification
                                                object:nil];
    NSMenu* displayMenu = [[NSMenu alloc] initWithTitle:@"Affichage"];
    NSMenuItem* fullscreenItem = [[NSMenuItem alloc] initWithTitle:@"Plein écran"
                                                             action:@selector(toggleFullScreen:)
                                                      keyEquivalent:@"f"];
    fullscreenItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [displayMenu addItem:fullscreenItem];
    [displayMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* size720 = [[NSMenuItem alloc] initWithTitle:@"1280 × 720" action:@selector(setDisplay720p:) keyEquivalent:@""];
    size720.target = self;
    [displayMenu addItem:size720];
    NSMenuItem* size900 = [[NSMenuItem alloc] initWithTitle:@"1600 × 900" action:@selector(setDisplay900p:) keyEquivalent:@""];
    size900.target = self;
    [displayMenu addItem:size900];
    NSMenuItem* size1080 = [[NSMenuItem alloc] initWithTitle:@"1920 × 1080" action:@selector(setDisplay1080p:) keyEquivalent:@""];
    size1080.target = self;
    [displayMenu addItem:size1080];
    NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:@"Affichage" action:nil keyEquivalent:@""];
    menuItem.submenu = displayMenu;
    [NSApp.mainMenu addItem:menuItem];
    [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0 target:_view selector:@selector(gameTick) userInfo:nil repeats:YES];
    [NSTimer scheduledTimerWithTimeInterval:0.25 target:_view selector:@selector(updateStatus) userInfo:nil repeats:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender { return YES; }

// Menu focus, a fullscreen transition, or returning from another app can
// otherwise leave AppKit's responder on the window rather than the OpenGL
// game view.  Keep keyboard events on the game's input bridge.
- (void)windowDidBecomeKey:(NSNotification*)notification
{
    [_window makeFirstResponder:_view];
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    [self resetKeyboardState];
}

- (void)resetKeyboardState
{
    MacInputResetKeyboard(_input);
}

// A game window is reconstructed from the engine state at launch; restoring a
// previous Cocoa window can trigger AppKit's crash-recovery modal dialog and
// prevent the game loop from starting.
- (BOOL)applicationShouldSaveApplicationState:(NSApplication*)application { return NO; }
- (BOOL)applicationShouldRestoreApplicationState:(NSApplication*)application { return NO; }

- (void)applicationWillTerminate:(NSNotification*)notification
{
    rReleasePrintf("macOS: AppKit requested termination\n");
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (_keyMonitor != nil)
    {
        [NSEvent removeMonitor:_keyMonitor];
        _keyMonitor = nil;
    }
    _input = nullptr;
    gApplicationDelegate = nil;
}
@end

int MacDisplayGetResolution(void)
{
    return gApplicationDelegate != nil ? [gApplicationDelegate gameResolution] : 2;
}

int MacDisplayGetBPP(void) { return 32; }

int MacDisplayIsFullscreen(void)
{
    return gApplicationDelegate != nil && [gApplicationDelegate isGameFullscreen];
}

int MacDisplaySetMode(int resolution, int bpp, int fullscreen)
{
    (void)bpp;
    if (gApplicationDelegate == nil)
        return 0;
    [gApplicationDelegate setGameDisplaySize:SizeForResolutionIndex(resolution) fullscreen:fullscreen != 0];
    return 1;
}

int main(int argc, const char* argv[])
{
    // Pure3D allocates through Foundation Tech even while merely creating the
    // first render context.  Initialise its allocator before NSOpenGLView is
    // constructed; otherwise tPlatform::Create dereferences the empty memory
    // manager and the app dies before Cocoa can report an error.
    radThreadInitialize();
    radMemoryInitialize();
    radTimeInitialize();
    radFileInitialize(50, 32, RADMEMORY_ALLOC_DEFAULT);
    radLoadInitialize();

    @autoreleasepool
    {
        NSApplication* application = [NSApplication sharedApplication];
        MacApplicationDelegate* delegate = [[MacApplicationDelegate alloc] init];
        application.delegate = delegate;
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        [application run];
    }
    radLoadTerminate();
    radFileTerminate();
    radTimeTerminate();
    radMemoryTerminate();
    radThreadTerminate();
    return 0;
}
