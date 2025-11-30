#ifdef _WIN32
    #include <windows.h>
    #include <thread>
    #include <atomic>
    #include <chrono>
#elif __APPLE__
    #include <ApplicationServices/ApplicationServices.h>
    #include <Carbon/Carbon.h>
    #include <mach-o/dyld.h>
    #include <thread>
    #include <atomic>
    #include <chrono>
    #include <cstdio>
    #include <cstdlib>
#else
    #include <X11/Xlib.h>
    #include <X11/keysym.h>
    #include <linux/uinput.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <cstring>
    #include <thread>
    #include <atomic>
    #include <chrono>
    #include <cstdio>
    #include <cstdlib>
#endif

std::atomic<bool> h(false);
std::atomic<bool> r(true);

#ifdef _WIN32
void showShit(const char* msg) {
    MessageBoxA(NULL, msg, "Mouse Hallucination", MB_OK | MB_TOPMOST);
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
    RegisterHotKey(NULL, 1, MOD_WIN, 0x53);
    MSG m;
    while (r && GetMessage(&m, NULL, 0, 0)) {
        if (m.message == WM_HOTKEY) h = !h;
    }
    UnregisterHotKey(NULL, 1);
}

#elif __APPLE__
void showShit(const char* msg) {
    CFStringRef hdr = CFSTR("Mouse Hallucination");
    CFStringRef txt = CFStringCreateWithCString(NULL, msg, kCFStringEncodingUTF8);
    CFUserNotificationDisplayAlert(2.0, 0, NULL, NULL, NULL, hdr, txt, NULL, NULL, NULL, NULL);
    CFRelease(txt);
}

void askAutoStart() {
    CFStringRef hdr = CFSTR("Mouse Hallucination");
    CFStringRef msg = CFSTR("Want this shit to run on startup?");
    CFStringRef btn1 = CFSTR("Hell Yeah");
    CFStringRef btn2 = CFSTR("Nah");
    
    CFOptionFlags resp = 0;
    CFUserNotificationDisplayAlert(0, 0, NULL, NULL, NULL, hdr, msg, btn1, btn2, NULL, &resp);
    
    if (resp == kCFUserNotificationDefaultResponse) {
        char path[1024], cmd[2048];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0) {
            snprintf(cmd, sizeof(cmd), 
                "osascript -e 'tell application \"System Events\" to make login item at end with properties {path:\"%s\", hidden:false}' 2>/dev/null &",
                path);
            system(cmd);
        }
    }
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
int uinput_fd = -1;

void showShit(const char* msg) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "notify-send 'Mouse Hallucination' '%s' -t 2000 2>/dev/null &", msg);
    system(cmd);
}

int setupUinput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return -1;
    
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Mouse Hallucination Device");
    
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
    
    return fd;
}

void emitMove(int fd, int dx, int dy) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    
    ev.type = EV_REL;
    ev.code = REL_X;
    ev.value = dx;
    write(fd, &ev, sizeof(ev));
    
    ev.code = REL_Y;
    ev.value = dy;
    write(fd, &ev, sizeof(ev));
    
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

void fuckWithMouse() {
    uinput_fd = setupUinput();
    if (uinput_fd < 0) {
        showShit("Failed to setup uinput! Run with sudo or add user to input group");
        r = false;
        return;
    }
    
    Display* d = XOpenDisplay(NULL);
    if (!d) {
        showShit("Can't open X display!");
        r = false;
        return;
    }
    
    Window root = DefaultRootWindow(d);
    int lx = -9999, ly = -9999, i = 0;
    
    while (r) {
        if (h) {
            Window rr, cr;
            int x, y, wx, wy;
            unsigned int mask;
            if (XQueryPointer(d, root, &rr, &cr, &x, &y, &wx, &wy, &mask) && (x != lx || y != ly)) {
                int dx = (i * 17 + 13) % 31 - 15;
                int dy = (i * 23 + 7) % 31 - 15;
                emitMove(uinput_fd, dx, dy);
                lx = x; ly = y; i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    XCloseDisplay(d);
    if (uinput_fd >= 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
    }
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
    showShit("Mouse Hallucination Is Fucking On! Cmd+Shift+Option+S to toggle");
    askAutoStart();
#else
    showShit("Mouse Hallucination Is Fucking On! Win+S (Super+S) to toggle");
#endif
    
    std::thread t1(listenShit);
    std::thread t2(fuckWithMouse);
    t1.join();
    t2.join();
    return 0;
}
