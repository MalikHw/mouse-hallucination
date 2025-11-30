#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <ApplicationServices/ApplicationServices.h>
    #include <Carbon/Carbon.h>
#else
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
#endif

#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> h(false); // hallucinating state
std::atomic<bool> r(true);  // running state

#ifdef _WIN32
void showShit(const char* msg) {
    MessageBoxA(NULL, msg, "Crazy Mouse", MB_OK | MB_TOPMOST);
    std::thread([](){ std::this_thread::sleep_for(std::chrono::seconds(2)); }).detach();
}

void fuckWithMouse() {
    POINT p, lp = {-9999, -9999};
    int i = 0;
    while (r) {
        if (h && GetCursorPos(&p) && (p.x != lp.x || p.y != lp.y)) {
            SetCursorPos(p.x + ((i * 17 + 13) % 31 - 15), p.y + ((i * 23 + 7) % 31 - 15));
            lp = p; i++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(h ? 16 : 100));
    }
}

void listenShit() {
    RegisterHotKey(NULL, 1, MOD_WIN, 0x53); // Win+S
    MSG m;
    while (r && GetMessage(&m, NULL, 0, 0)) {
        if (m.message == WM_HOTKEY) h = !h;
    }
    UnregisterHotKey(NULL, 1);
}

#elif __APPLE__
void showShit(const char* msg) {
    CFStringRef hdr = CFSTR("Crazy Mouse");
    CFStringRef txt = CFStringCreateWithCString(NULL, msg, kCFStringEncodingUTF8);
    CFUserNotificationDisplayAlert(2.0, 0, NULL, NULL, NULL, hdr, txt, NULL, NULL, NULL, NULL);
    CFRelease(txt);
}

void fuckWithMouse() {
    CGPoint lp = {-9999, -9999};
    int i = 0;
    while (r) {
        if (h) {
            CGEventRef e = CGEventCreate(NULL);
            CGPoint p = CGEventGetLocation(e);
            CFRelease(e);
            if (p.x != lp.x || p.y != lp.y) {
                CGWarpMouseCursorPosition(CGPointMake(
                    p.x + ((i * 17 + 13) % 31 - 15),
                    p.y + ((i * 23 + 7) % 31 - 15)
                ));
                lp = p; i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

CGEventRef fuckingCallback(CGEventTapProxy px, CGEventType t, CGEventRef e, void* rc) {
    if (t == kCGEventKeyDown) {
        CGKeyCode k = (CGKeyCode)CGEventGetIntegerValueField(e, kCGKeyboardEventKeycode);
        CGEventFlags f = CGEventGetFlags(e);
        // Cmd+Shift+Option+S
        if (k == 1 && (f & kCGEventFlagMaskCommand) && 
            (f & kCGEventFlagMaskShift) && (f & kCGEventFlagMaskAlternate)) {
            h = !h;
        }
    }
    return e;
}

void listenShit() {
    CFMachPortRef tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
        kCGEventTapOptionDefault, CGEventMaskBit(kCGEventKeyDown), fuckingCallback, NULL);
    if (!tap) return;
    CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);
    CFRunLoopRun();
}

#else // Linux
void showShit(const char* msg) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "notify-send 'Crazy Mouse' '%s' -t 2000 2>/dev/null &", msg);
    system(cmd);
}

void fuckWithMouse() {
    Display* d = XOpenDisplay(NULL);
    if (!d) return;
    Window root = DefaultRootWindow(d);
    int lx = -9999, ly = -9999, i = 0;
    
    while (r) {
        if (h) {
            Window rr, cr;
            int x, y, wx, wy;
            unsigned int mask;
            if (XQueryPointer(d, root, &rr, &cr, &x, &y, &wx, &wy, &mask) && (x != lx || y != ly)) {
                XWarpPointer(d, None, root, 0, 0, 0, 0, 
                    x + ((i * 17 + 13) % 31 - 15), 
                    y + ((i * 23 + 7) % 31 - 15));
                XFlush(d);
                lx = x; ly = y; i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    XCloseDisplay(d);
}

void listenShit() {
    Display* d = XOpenDisplay(NULL);
    if (!d) return;
    Window root = DefaultRootWindow(d);
    KeyCode ks = XKeysymToKeycode(d, XK_s);
    XGrabKey(d, ks, Mod4Mask, root, False, GrabModeAsync, GrabModeAsync);
    XSelectInput(d, root, KeyPressMask);
    
    XEvent ev;
    while (r) {
        if (XCheckWindowEvent(d, root, KeyPressMask, &ev)) {
            if (ev.type == KeyPress && ev.xkey.keycode == ks) h = !h;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    XUngrabKey(d, ks, Mod4Mask, root);
    XCloseDisplay(d);
}
#endif

int main() {
#ifdef __APPLE__
    showShit("Crazy Mouse Is Fucking On! Cmd+Shift+Option+S to toggle");
#else
    showShit("Crazy Mouse Is Fucking On! Win+S (Super+S) to toggle");
#endif
    
    std::thread t1(listenShit);
    std::thread t2(fuckWithMouse);
    t1.join();
    t2.join();
    return 0;
}
