#include "platform/platform.h"

// windows platform layer

// wont compile code if not on windows
#if KPLATFORM_WINDOWS

#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"

#include "containers/darray.h"

// specific include for the win32 platform
#include <windows.h>
#include <windowsx.h>  // param input extraction - parameter
#include <stdlib.h>    // temporary

// for surface creation
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vulkan_types.inl"

// windows specific state
typedef struct platform_state {
    HINSTANCE h_instance;  // handle to the instance of the application
    HWND hwnd;             // handle to the window
    VkSurfaceKHR surface;  // vulkan needs a surface to render to. add it to the windows internal state. surface will be retrieved from the windowing system

    // clock
    f64 clock_frequency;       // multiplier for the clock cycles from OS and multiply by this to get the actual time
    LARGE_INTEGER start_time;  // starting time of the application
} platform_state;

static platform_state *state_ptr;  // store a poiter to where the platform state is being held

// foreward declaration - he calls this windows specific mumbojumbo.
LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);  // win 32 process message- takes in a window

// implementation of platform startup for win32
// initialize the platform subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem
b8 platform_system_startup(
    u64 *memory_requirement,                       // pointer to where the memory requirement field is held
    void *state,                                   // pointer to where the state is being held
    const char *application_name,                  // the name of the application, if windowed will be the window title
    i32 x,                                         // x pos
    i32 y,                                         // y pos
    i32 width,                                     // width
    i32 height) {                                  // height
    *memory_requirement = sizeof(platform_state);  // de reference the memory requirement and set it to the size of the platform state
    if (state == 0) {                              // if no state input
        return true;                               // boot out here
    }
    state_ptr = state;                            // pass the pointer to where the state is being held
    state_ptr->h_instance = GetModuleHandleA(0);  // use a windows function to get a pointer to the handle to the instance

    // NOTE: at some point i am going to read throught the microsoft documentation and learn all of this stuff - search for WNDCLASSA  and that will lead me in the right direction
    // Setup and register the window class
    HICON icon = LoadIcon(state_ptr->h_instance, IDI_APPLICATION);  // i believe this would be a personal icon, for kohi
    WNDCLASSA wc;                                                   // declare widow class
    memset(&wc, 0, sizeof(wc));                                     // allocate the memory for the window class and zero it out
    wc.style = CS_DBLCLKS;                                          // get double clicks -- can pass more flags in here
    wc.lpfnWndProc = win32_process_message;                         // pointer to the window procedure. basically what handles event within the system
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state_ptr->h_instance;  // point to h_instance
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // manage the cursor manually - we will handle the cursor
    wc.hbrBackground = NULL;                   // transparent
    wc.lpszClassName = "kohi_window_class";    // important. what is going to be called when the window is created. name of the class in string form

    // after the window class is created it has to be registered - cannot move foreward if this fails
    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);  // in the case that the registration fails pop up a window and let you know - with an ok button to accept it
        return false;
    }

    // Create window - need to be concerned about 2 sizes here, the inner part with the content(client), and the outer part with file, and the tab bar and all that(window)
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    // inititally these will be the same value
    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    // again need to look the the docs, this part is in window styles
    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;  // overlapped(has a title bar and a border), need to look the rest up, maybe use gpt cause looks like it could be a nightmare
    u32 window_ex_style = WS_EX_APPWINDOW;

    // somewhat describe how the window looks - and the type of buttons it has
    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    // obtain the size of the border
    RECT border_rect = {0, 0, 0, 0};                                     // declare a rectangle for a border with zero value defaulted
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);  // takes all of the styles into consideration

    // in this case, the border rectangle is negative - after adjusting for all the styling and such
    window_x += border_rect.left;
    window_y += border_rect.top;

    // grow by the size of the OS border - operating system?
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    // extended styles for window creation? - this has to match everything that was passed in above or it will fail
    HWND handle = CreateWindowExA(
        window_ex_style, "kohi_window_class", application_name,         // application_name becomes the title on the window
        window_style, window_x, window_y, window_width, window_height,  // declared and resized above
        0, 0, state_ptr->h_instance, 0);

    // here is a check point to make sure that everything has worked properly so far - if not the application cannot proceed
    if (handle == 0) {
        MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);  // creates a pop up window, with a fail message, an exlamation button and an ok button

        KFATAL("Window creation failed!");  // fatal level log message
        return false;
    } else {
        state_ptr->hwnd = handle;  // update the state objects window handle to handle
    }

    // now we need to display the window - this can be complicated
    // show the window
    b32 should_activate = 1;                                                        // TODO: if the window should not accept input, this should be false
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;  // switch whether the window can accept inputs
    // if initially minimized, use  SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // if initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(state_ptr->hwnd, show_window_command_flags);  // show window pass in the handle and the flags

    // clock setup -- setting the start time
    LARGE_INTEGER frequency;                                     // declare
    QueryPerformanceFrequency(&frequency);                       // set using this function - gets from the speed of the processor
    state_ptr->clock_frequency = 1.0 / (f64)frequency.QuadPart;  // 1 divided by frequency, which is converted to a 64 bit floating point integer
    QueryPerformanceCounter(&state_ptr->start_time);             // gives us a snapshot of the current time

    return true;  // successfully initialized the platform
}

// shut down the platform system, pass in a pointer to the state
void platform_system_shutdown(void *plat_state) {
    if (state_ptr && state_ptr->hwnd) {  // if there is a state, and it contains a window handle
        DestroyWindow(state_ptr->hwnd);  // destroy the window, pass it the handle
        state_ptr->hwnd = 0;             // reset the pointer to the handle
    }
}

// secondary message handling loop - neccassary for some platform stuff
b8 platform_pump_messages() {
    if (state_ptr) {
        MSG message;                                             // windows processes messages in a stack, so that means that they get dealt with one at a time
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {  // if there is a message on the top of the stack take it and remove it to clear it, too many and window wil be unresponsive, while loop runs constantly looking for messages
            TranslateMessage(&message);                          // perform a translation on the message
            DispatchMessageA(&message);                          // dispatch by whatever method is defined in the wc.lpfnWndProc setting above
        }
    }
    return true;  // when there are no messages returns true
}

// all of the next fuctions are just calling their windows couterpartd for the time being, this will be changing

// this is very temporary
void *platform_allocate(u64 size, b8 aligned) {
    return malloc(size);  // i believe malloc has to do with the heap, need to look up - yes malloc stands for memory allocation, and it is on the heap
}

// dont quite get this one and he soesnt explain look up
void platform_free(void *block, b8 aligned) {
    free(block);
}

// set the entire block of memory to zeros
void *platform_zero_memory(void *block, u64 size) {
    return memset(block, 0, size);
}

// copy the memory
void *platform_copy_memory(void *dest, const void *source, u64 size) {
    return memcpy(dest, source, size);
}

// set the memory
void *platform_set_memory(void *dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

// specific writing command only for windows? -- allows us to choose which stream to output to
void platform_console_write(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};                // an array of values that represent foreground and background color styling
    SetConsoleTextAttribute(console_handle, levels[colour]);  // grab value by the number representation of the level

    OutputDebugStringA(message);   // output stream that only windows has - out puts to the debug console
    u64 length = strlen(message);  // requires a u64
    LPDWORD number_written = 0;    // long pointer to a d word, need to look up
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void platform_console_write_error(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);  //  only difference between the 2 functions -- this sends it to a different stream -- special stream for errors
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};                // an array of values that represent foreground and background color styling
    SetConsoleTextAttribute(console_handle, levels[colour]);  // grab value by the number representation of the level

    OutputDebugStringA(message);   // output stream that only windows has - out puts to the debug console
    u64 length = strlen(message);  // requires a u64
    LPDWORD number_written = 0;    // long pointer to a d word, need to look up
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

// there is more setup for this function above
f64 platform_get_absolute_time() {
    if (state_ptr) {
        LARGE_INTEGER now_time;                                      // declare
        QueryPerformanceCounter(&now_time);                          // gives us a snapshot of the current time compared to the start time - number of cycles dince the application started
        return (f64)now_time.QuadPart * state_ptr->clock_frequency;  //  number of cycles dince the application started times the speed of the processor gives us actual time passed and returns that value
    }
    return 0;
}

// not much to do on this for now
void platform_sleep(u64 ms) {
    Sleep(ms);
}

// from vulcan_platform.h -- to get the platform specific extesion names for windows
void platform_get_required_extension_names(const char ***names_darray) {
    darray_push(*names_darray, &"VK_KHR_win32_surface");  // push in the windows surface extension into the vulkan required estensions array
}

// surface creation for vulkan -- exposes too much vulkan code to the platform, but going to use for now, will probably change
b8 platform_create_vulkan_surface(vulkan_context *context) {
    if (!state_ptr) {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};  // create the info needed to create the new surface
    create_info.hinstance = state_ptr->h_instance;                                                // input a poiter to handel instance into surface info
    create_info.hwnd = state_ptr->hwnd;                                                           // input a pointer to the window handle into surface info

    VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state_ptr->surface);  // create the window surface using the surface info and allocate memory and store results in result
    if (result != VK_SUCCESS) {                                                                                           // if it succeeded
        KFATAL("Vulkan surface creation failed.");
        return false;
    }

    context->surface = state_ptr->surface;
    return true;
}

// need to learn more about these switch statements - it seems that once they are triggere they just keep moving til they find the first line of code, even if its 3 cases down
// handle events coming in -- need to look at the microsoft documentation on this as well, look for window messages and such
LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_ERASEBKGND:
            // notify th OS that the erasing will be handled by the application to prevent flicker occuring
            return 1;  // in documentation says to return 1
        case WM_CLOSE:
            // TODO:  fire an event for the application to quit -- come back after the event system has been built
            event_context data = {};                           // create empty event context call it data
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);  // fire event with code application quit, leave the sender field empty and pass in the data
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);  // when window is destroyed you have to post a quit message -- posts WM_QUIT
            return 0;
        case WM_SIZE: {  // commented out for now, need to do some more first
            // Get the updated size
            RECT r;                         // think rect is a representation of the window border? the rest is easy to see
            GetClientRect(hwnd, &r);        // call a windows function to get the window rectangel, pass in the handle to the window and the rect r
            u32 width = r.right - r.left;   // width is the right ppoition minus the left position
            u32 height = r.bottom - r.top;  // height is the bottom position  minus the top position

            // fire the event. the application layer should pick this up, but not handle it as it shouldnt be visible to other parts of the application
            event_context context;                       // create an event context struct context
            context.data.u16[0] = (u16)width;            // in the u16 data array input the width converted to a u16 at index 0
            context.data.u16[1] = (u16)height;           // in the u16 data array input the height converted to a u16 at index 1
            event_fire(EVENT_CODE_RESIZED, 0, context);  // fire off the event with the resized code, no sender, and the context just filled out
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // key pressed or released
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);  // if either of these is true, then we are calling pressed true
            keys key = (u16)w_param;                                   // get the key code from w_param which contains the u16 key code

            // check for left and right for keys that have both
            // Check for extended scan code.
            b8 is_extended = (HIWORD(l_param) & KF_EXTENDED) == KF_EXTENDED;

            // Keypress only determines if _any_ alt/ctrl/shift key is pressed. Determine which one if so.
            if (w_param == VK_MENU) {  // if the key pressed was an alt key
                key = is_extended ? KEY_RALT : KEY_LALT;
            } else if (w_param == VK_SHIFT) {  // if the key pressed was an shift key
                // Annoyingly, KF_EXTENDED is not set for shift keys.
                u32 left_shift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
                u32 scancode = ((l_param & (0xFF << 16)) >> 16);
                key = scancode == left_shift ? KEY_LSHIFT : KEY_RSHIFT;
            } else if (w_param == VK_CONTROL) {  // if the key pressed was an control key
                key = is_extended ? KEY_RCONTROL : KEY_LCONTROL;
            }

            // pass to the input subsystem for processing
            input_process_key(key, pressed);
        } break;
        case WM_MOUSEMOVE: {
            // mouse move -- uses macros provided by windows to get the mouse posistion - l_param is a single interger with both x and y coords packed into it
            i32 x_position = GET_X_LPARAM(l_param);  // gets the x coord form the int
            i32 y_position = GET_Y_LPARAM(l_param);  // gets the y coord form the int

            // pass to the input subsystem
            input_process_mouse_move(x_position, y_position);
        } break;
        case WM_MOUSEWHEEL: {                               // commented out for now because dont have input management yet
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);  // pull the z-delta from the w_param
            if (z_delta != 0) {                             // if it isnt zero
                // flatten the input to an OS independent (-1, 1) -- windows provides strange values for the scroll wheel
                z_delta = (z_delta < 0) ? -1 : 1;  // if the delta is less than zero it becomes simply -1, and if it isnt, it simply becomes 1
            }

            // pass to the input subsystem
            input_process_mouse_wheel(z_delta);
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {                                                                       // commented out for now because dont have input management yet
            b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;  // pressed is true, if any of these are true
            buttons mouse_button = BUTTON_MAX_BUTTONS;
            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    mouse_button = BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    mouse_button = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    mouse_button = BUTTON_RIGHT;
                    break;
            }

            // pass to the input subsystem
            if (mouse_button != BUTTON_MAX_BUTTONS) {  // this is a check to make sure it triggered one of the cases
                input_process_button(mouse_button, pressed);
            }
        } break;
    }

    // if an event occurs and nothing is triggered let windows handle the event as it normally would
    return DefWindowProc(hwnd, msg, w_param, l_param);
}

#endif  // KPLATFORM_WINDOWS