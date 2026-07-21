#import <Foundation/Foundation.h>

#include "MacControlProfile.h"

namespace { NSString* const kProfileKey = @"HitAndRunMacControlProfileV1"; }

MacControlProfile MacControlProfileLoad()
{
    MacControlProfile profile;
    NSDictionary* saved = [[NSUserDefaults standardUserDefaults] dictionaryForKey:kProfileKey];
    if (saved == nil) return profile;
    NSArray* keys = saved[@"keys"];
    if (keys.count == profile.keys.size())
        for (NSUInteger i = 0; i < keys.count; ++i) profile.keys[i] = [[keys objectAtIndex:i] unsignedShortValue];
    profile.trackpadSensitivity = [saved[@"trackpadSensitivity"] floatValue] ?: 1.0f;
    profile.invertTrackpadY = [saved[@"invertTrackpadY"] boolValue];
    return profile;
}

void MacControlProfileSave(const MacControlProfile& profile)
{
    NSMutableArray* keys = [NSMutableArray arrayWithCapacity:profile.keys.size()];
    for (unsigned short key : profile.keys) [keys addObject:@(key)];
    [[NSUserDefaults standardUserDefaults] setObject:@{ @"keys": keys, @"trackpadSensitivity": @(profile.trackpadSensitivity), @"invertTrackpadY": @(profile.invertTrackpadY) } forKey:kProfileKey];
}
