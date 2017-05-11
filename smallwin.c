#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<stdio.h>

#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB    0x2093
#define WGL_CONTEXT_FLAGS_ARB          0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB   0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);

typedef struct {
   HWND hwnd;
   HDC dc;
   HGLRC rc;
   int _backup_width;
   int _backup_height;
   int _backup_x;
   int _backup_y;
} window;

static window smallwin_window;
static int smallwin_window_opened = 0;
static int smallwin_initialized = 0;
static HINSTANCE smallwin_instance;
static const char * smallwin_class_name = "smallwin";
static int smallwin_screen_height, smallwin_screen_width;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

void smallwin_error_message(char *message)
{
   MessageBox(NULL, message, "Error!", MB_ICONEXCLAMATION | MB_OK);
   return;
}

void smallwin_swap_buffers(window win)
{
   SwapBuffers(win.dc);
   return;
}

static void load_extensions(window win)
{
   HGLRC temp_rc = wglCreateContext(win.dc);
   wglMakeCurrent(win.dc, temp_rc);

   wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

   wglMakeCurrent(win.dc, NULL);
   wglDeleteContext(temp_rc);
   return;
}

static HGLRC create_context(window win, int major, int minor)
{
   load_extensions(win);
   int attrib_list[] = 
   {
      WGL_CONTEXT_MAJOR_VERSION_ARB, major,
      WGL_CONTEXT_MINOR_VERSION_ARB, minor,
      //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0,0
   };
   HGLRC result = wglCreateContextAttribsARB(win.dc, 0, attrib_list);
   return result;
}

static void make_context_current(window win)
{
   wglMakeCurrent(win.dc, win.rc);
}

void smallwin_close_window(window win)
{
   wglMakeCurrent(0, 0);
   wglDeleteContext(win.rc);
   DestroyWindow(win.hwnd);
   return;
}

int blocking_message_loop()
{
   MSG msg;
   while(GetMessage(&msg, 0, 0, 0) > 0)
   {
      DispatchMessage(&msg);
   }

   return (int)msg.wParam;
}

int non_blocking_message_loop()
{
   MSG msg;
   if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
   {
      if(msg.message == WM_QUIT)
      {
         return 0;
      }else
      {
         DispatchMessage(&msg);
      }

   }
   return 1;
}

void exit_smallwin()
{
   smallwin_close_window(smallwin_window);
   UnregisterClass(smallwin_class_name, smallwin_instance);
   return;
}

static void set_pixel_format(window win)
{
   PIXELFORMATDESCRIPTOR pfd;
   ZeroMemory(&pfd, sizeof(pfd));
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   // Recently changed colorbits to 24 and added alphabits=8
   pfd.cColorBits = 24;
   pfd.cDepthBits = 24;
   pfd.cStencilBits = 8;
   pfd.cAlphaBits = 8;
   pfd.iLayerType = PFD_MAIN_PLANE;

   int format_num = ChoosePixelFormat(win.dc, &pfd);

   if(!format_num)
   {
      smallwin_error_message("Smallwin: Error choosing a pixel format.");
   }

   SetPixelFormat(win.dc, format_num, &pfd);
   return;
}

static void _get_adjusted_size(int width, int height, int *w, int *h)
{
   RECT rect;
   rect.left = 0;
   rect.top = 0;
   rect.right = width;
   rect.bottom = height;

   AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

   *w = rect.right-rect.left;
   *h = rect.bottom-rect.top;
   return;
}

int smallwin_win_width(window win)
{
   RECT rect;
   GetClientRect(win.hwnd, &rect);
   return rect.right;
}

int smallwin_win_height(window win)
{
   RECT rect;
   GetClientRect(win.hwnd, &rect);
   return rect.bottom;
}

static void backup_window(window *win)
{
   RECT rect;
   GetWindowRect(win->hwnd, &rect);
   win->_backup_x = rect.left;
   win->_backup_y = rect.top;
   GetClientRect(win->hwnd, &rect);
   win->_backup_height = rect.bottom;
   win->_backup_width = rect.right;
}

void smallwin_go_fullscreen(window win)
{
   // FIXME: Not using the window passed feels like a
   // kludge. We could pass by reference, but that feels
   // ugly too.
   backup_window(&smallwin_window);

   SetWindowLongPtr(win.hwnd, GWL_STYLE, WS_CLIPSIBLINGS | WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN | WS_SYSMENU);
   SetWindowPos(win.hwnd, HWND_NOTOPMOST, 0, 0, smallwin_screen_width, smallwin_screen_height, SWP_SHOWWINDOW);
   return;
}

void smallwin_go_windowed(window win)
{
   int w,h;
   SetWindowLongPtr(win.hwnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
   _get_adjusted_size(win._backup_width, win._backup_height, &w, &h);
   SetWindowPos(win.hwnd, HWND_NOTOPMOST, win._backup_x, win._backup_y, w, h, SWP_SHOWWINDOW);
   return;
}

void (*smallwin_keydown_callback)(unsigned char);
void (*smallwin_keyup_callback)(unsigned char);

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch(msg)
   {
      case WM_CLOSE:
         exit_smallwin();
         break;
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      case WM_SIZE:
         break;
      case WM_INPUT:
         break;
      case WM_KEYDOWN:
         if(!(lParam&0x40000000)&&smallwin_keydown_callback)
            smallwin_keydown_callback(wParam);
         break;
      case WM_KEYUP:
         if(smallwin_keyup_callback)
            smallwin_keyup_callback(wParam);
         break;
      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
   return 0;
}

void init_smallwin()
{
   smallwin_screen_height = GetSystemMetrics(SM_CYSCREEN);
   smallwin_screen_width = GetSystemMetrics(SM_CXSCREEN);

   smallwin_instance = GetModuleHandle(NULL);

   WNDCLASS wc;

   ZeroMemory(&wc, sizeof(wc));
   wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wc.lpfnWndProc   = window_proc;
   wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.lpszClassName = smallwin_class_name;

   if(!RegisterClass(&wc))
   {
      smallwin_error_message("Smallwin: Error registering window.");
   }
   return;
}

window smallwin_open_window(char* title, int width, int height, int major, int minor)
{
   if(smallwin_window_opened)
   {
      smallwin_error_message("Smallwin: A window is already opened.");
      smallwin_close_window(smallwin_window);
   }

   if(!smallwin_initialized)
   {
      init_smallwin();
   }

   smallwin_window_opened = 1;

   window new_window;

   int w,h;
   _get_adjusted_size(width, height, &w, &h);

   new_window.hwnd = CreateWindow(
         smallwin_class_name,
         title,
         WS_OVERLAPPEDWINDOW,
         CW_USEDEFAULT, CW_USEDEFAULT, w, h,
         NULL, NULL, NULL, NULL);

   if(new_window.hwnd == NULL)
   {
      smallwin_error_message("Smallwin: Error creating window.");
      smallwin_close_window(smallwin_window);
   }

   new_window.dc = GetDC(new_window.hwnd);

   set_pixel_format(new_window);
   new_window.rc = create_context(new_window, major, minor);

   ShowWindow(new_window.hwnd, 1);
   UpdateWindow(new_window.hwnd);

   smallwin_window = new_window;

   make_context_current(new_window);

   return new_window;
}

