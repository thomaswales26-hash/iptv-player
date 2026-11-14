#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace {

NSString* shellQuoted(NSString* value) {
    NSString* escaped = [value stringByReplacingOccurrencesOfString:@"'"
                                                        withString:@"'\\''"];
    return [NSString stringWithFormat:@"'%@'", escaped];
}

NSString* appleScriptQuoted(NSString* value) {
    NSString* escaped = [value stringByReplacingOccurrencesOfString:@"\\"
                                                        withString:@"\\\\"];
    escaped = [escaped stringByReplacingOccurrencesOfString:@"\""
                                                 withString:@"\\\""];
    escaped = [escaped stringByReplacingOccurrencesOfString:@"\n"
                                                 withString:@"\\n"];
    return [NSString stringWithFormat:@"\"%@\"", escaped];
}

}

int launchTerminalPlayer() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSOpenPanel* panel = [NSOpenPanel openPanel];
        UTType* m3uType = [UTType typeWithFilenameExtension:@"m3u"];
        UTType* m3u8Type = [UTType typeWithFilenameExtension:@"m3u8"];
        panel.allowedContentTypes = @[m3uType, m3u8Type];
        panel.allowsMultipleSelection = NO;
        panel.canChooseDirectories = NO;
        panel.prompt = @"Open Playlist";
        if ([panel runModal] != NSModalResponseOK || panel.URL == nil) {
            return 0;
        }

        NSString* executablePath = NSBundle.mainBundle.executablePath;
        NSString* command = [NSString stringWithFormat:@"%@ %@",
                                                       shellQuoted(executablePath),
                                                       shellQuoted(panel.URL.path)];
        NSString* script = [NSString stringWithFormat:
            @"tell application \"Terminal\" to do script %@\n"
             "tell application \"Terminal\" to activate",
            appleScriptQuoted(command)];

        NSDictionary* errorInfo = nil;
        NSAppleScript* appleScript = [[NSAppleScript alloc] initWithSource:script];
        if ([appleScript executeAndReturnError:&errorInfo] == nil) {
            NSString* message = errorInfo[NSAppleScriptErrorMessage];
            if (message == nil) {
                message = @"Could not open Terminal.";
            }
            NSAlert* alert = [[NSAlert alloc] init];
            alert.messageText = @"Could not launch IPTV Player";
            alert.informativeText = message;
            [alert addButtonWithTitle:@"OK"];
            [alert runModal];
            return 1;
        }
    }
    return 0;
}
