#include "platform.h"

#include "windows.h"

#include <assert.h>
#include <thread>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <vector>

#include "geometry.h"
#include "lib.h"
#include "matrix.h"

static Input s_input = { 0 };
static short s_lastMoveX = 0;
static short s_lastMoveY = 0;
static HDC s_renderDC;
static HWND s_window;
static int s_windowHeight;
static int s_windowWidth;
static bool s_running = true;

static constexpr float BASIC_CAMERA_SPEED = 30.f;
static const vector3 INITIAL_CAMERA_POS = { 60.f, 15.f, 0.f };
static const vector3 INITIAL_CAMERA_LOOK = { 0.0f, 0.f, 0.0f };
static const vector3 INITIAL_CAMERA_ORBIT = { 0.0f, 0.f, 0.0f };
static vector3 s_cameraPos = INITIAL_CAMERA_POS;
static vector3 s_cameraLook = INITIAL_CAMERA_LOOK;
static vector3 s_cameraOrbit = INITIAL_CAMERA_ORBIT;

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (msg == WM_LBUTTONDOWN)
            {
                s_input.mouseInput |= LEFT_BUTTON;
            }
            else if (msg == WM_MBUTTONDOWN)
            {
                s_input.mouseInput |= MIDDLE_BUTTON;
            }
            else
            {
                s_input.mouseInput |= RIGHT_BUTTON;
            }
            s_lastMoveX = SHORT(LOWORD(lParam));
            s_lastMoveY = SHORT(HIWORD(lParam));
            SetCapture(hWnd);
        }
        break;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            if (msg == WM_LBUTTONUP)
            {
                s_input.mouseInput &= ~LEFT_BUTTON;
            }
            else if (msg == WM_MBUTTONUP)
            {
                s_input.mouseInput &= ~MIDDLE_BUTTON;
            }
            else
            {
                s_input.mouseInput &= ~RIGHT_BUTTON;
            }
            if (!s_input.mouseInput)
            {
                ReleaseCapture();
            }
        }
        break;

        case WM_MOUSEMOVE:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            s_input.x += x - s_lastMoveX;
            s_input.y += y - s_lastMoveY;
            s_lastMoveX = x;
            s_lastMoveY = y;
        }
        break;

        case WM_CHAR:
        {
            // ascii 'a'-'z' is range 97-122, out of the range of our bitmask...
            // so just translate lower-to-upper here
			if (wParam >= 'a' && wParam <= 'z')
			{
				wParam = 'A' + wParam - 'a';
			}
            // constrain the range to fit within the 64 bit bit-mask
            // currently only captures these ranges of key presses

            // The numbers 48-90 correspond to characters and symbols
            // this is the '0' char to the 'Z' char
            if (wParam >= START_CHAR && wParam <= END_CHAR)
            {
                s_input.keyInput |= (1ull << (wParam-START_CHAR));
            }
            break;
        }

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
				case VK_LEFT:   s_input.keyInput |= (1ull << ((END_CHAR - START_CHAR) + ARROW_KEY_LEFT)); break;
				case VK_RIGHT:  s_input.keyInput |= (1ull << ((END_CHAR - START_CHAR) + ARROW_KEY_RIGHT)); break;
				case VK_DOWN:   s_input.keyInput |= (1ull << ((END_CHAR - START_CHAR) + ARROW_KEY_DOWN)); break;
				case VK_UP:     s_input.keyInput |= (1ull << ((END_CHAR - START_CHAR) + ARROW_KEY_UP)); break;
				default: break;
            }
        }
        break;

        case WM_SIZE:
        {
            s_windowWidth = LOWORD(lParam);
            s_windowHeight = HIWORD(lParam);
        }
        break;

        case WM_COMMAND:
            // TODO: from accelerators
            break;

        case WM_CLOSE:
            s_running = false;
            PostQuitMessage(0);
            break;

        default:
            break;
    }
    return (DefWindowProc(hWnd, msg, wParam, lParam));
}
//-------------------------------------------------------------------------------------------------
static void StartDrawFrame()
{
	float ratio = (float)s_windowWidth * (1.0f / (float)s_windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, s_windowWidth, s_windowHeight);
	gluPerspective(45.0f, ratio, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	gluLookAt(s_cameraPos.x, s_cameraPos.y, s_cameraPos.z,
			  s_cameraLook.x, s_cameraLook.y, s_cameraLook.z,
			  0.0, 1.0, 0.0);
	glRotatef(float((int)s_cameraOrbit.x % 360), 1.0, 0.0, 0.0);
	glRotatef(float((int)s_cameraOrbit.y % 360), 0.0, 1.0, 0.0);
	glRotatef(float((int)s_cameraOrbit.z % 360), 0.0, 0.0, 1.0);

    // just assume we want alpha blending and depth testing
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
//-------------------------------------------------------------------------------------------------
void Platform_Run(const PlatformParams& params)
{
    const LPCWSTR className = L"PlatformClassName";

    RECT winrect;
    winrect.left = params.windowPos[0];
    winrect.top = params.windowPos[1];
    winrect.right = winrect.left + params.windowSize[0];
    winrect.bottom = winrect.top + params.windowSize[1];
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    AdjustWindowRect(&winrect, style, 1);

    HMODULE instance = GetModuleHandleA(NULL);

    WNDCLASS wc;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL; //MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = L"PlatformClassName";
    int handle = RegisterClass(&wc);
    assert(handle);

    HWND window = CreateWindowExA(
        0
        , "PlatformClassName"
        , "Window Name"
        , style
        , winrect.left// x
        , winrect.top // y
        , winrect.right - winrect.left // width
        , winrect.bottom - winrect.top // height
        , NULL // parent
        , NULL // menu
        , instance // instance
        , NULL
    );
    assert(window != NULL);

    ShowWindow(window, SW_SHOWNORMAL);
    s_renderDC = GetDC(window);
    assert(s_renderDC != NULL);

    PIXELFORMATDESCRIPTOR pfd = {
          sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd
          1,				     // version number
          PFD_DRAW_TO_WINDOW |	     // support window
          PFD_SUPPORT_OPENGL |	     // support OpenGL
          PFD_DOUBLEBUFFER,		     // double buffered
          PFD_TYPE_RGBA,		     // RGBA type
          24, 			     // 24-bit color depth
          0, 0, 0, 0, 0, 0,		     // color bits ignored
          0,				     // no alpha buffer
          0,				     // shift bit ignored
          0,				     // no accumulation buffer
          0, 0, 0, 0, 		     // accum bits ignored
          32, 			     // 32-bit z-buffer
          0,				     // no stencil buffer
          0,				     // no auxiliary buffer
          PFD_MAIN_PLANE,		     // main layer
          0,				     // reserved
          0, 0, 0			     // layer masks ignored
    };
    // get the best available match of pixel format for the device context
    int iPixelFormat = ChoosePixelFormat(s_renderDC, &pfd);
    assert(iPixelFormat != 0);
    bool success = SetPixelFormat(s_renderDC, iPixelFormat, &pfd);
    assert(success);

    HGLRC glc = wglCreateContext(s_renderDC);
    assert(glc != NULL);
    success = wglMakeCurrent(s_renderDC, glc);
    assert(success == TRUE);

    // TODO: separate render thread?
    std::thread thread = std::thread([params] {
        // init some stuff for our draw
		HGLRC glc = wglCreateContext(s_renderDC);
		assert(glc != NULL);
		bool success = wglMakeCurrent(s_renderDC, glc);
		assert(success == TRUE);

		auto frameEnd = std::chrono::high_resolution_clock::now();
		constexpr float MAX_FPS_MS = (1000.f / 60.f);
		while (s_running)
		{
			auto frameStart = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> delta_ms = (frameStart - frameEnd);
			float dt = min(MAX_FPS_MS, (float)(delta_ms.count())) / 1000.f;

			// Do work
            if (params.updateCallback)
            {
                params.updateCallback(dt);
            }
            if (params.drawCallback)
            {
                StartDrawFrame();
                params.drawCallback();
                SwapBuffers(s_renderDC);
            }
                
			// sleep if necessary
			frameEnd = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> frameTime = frameEnd - frameStart;
			if (frameTime.count() < MAX_FPS_MS)
			{
				std::chrono::duration<double, std::milli> delta_ms(MAX_FPS_MS - frameTime.count());
				auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
				std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
			}
		}
    });

    MSG msg;
    while (GetMessage(&msg, window, 0, 0))
    {
        if (!TranslateAccelerator(window, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    s_running = false;
    thread.join();
    //   renderThread.join();
    DestroyWindow(window);

}
//-------------------------------------------------------------------------------------------------
bool Platform_IsRunning()
{
    return s_running;
}
//-------------------------------------------------------------------------------------------------
Input Platform_ConsumeInput()
{
    // todo: better handling of press-release-press-release behavior in a single update
    Input ret = s_input;
    s_input.keyInput = 0;
    s_input.x = 0;
    s_input.y = 0;
    return ret;
}
//-------------------------------------------------------------------------------------------------
vector3 Platform_GetCameraPos() { return s_cameraPos; }
//-------------------------------------------------------------------------------------------------
vector3 Platform_GetCameraLookAt() { return s_cameraLook; }
//-------------------------------------------------------------------------------------------------
vector3 Platform_GetCameraOrbit() { return s_cameraOrbit; }
//-------------------------------------------------------------------------------------------------
void Platform_SetCameraPos(const vector3& pos)
{
    s_cameraPos = pos;
}
//-------------------------------------------------------------------------------------------------
void Platform_SetCameraLookAt(const vector3& lookAt)
{
    s_cameraLook = lookAt;
}
//-------------------------------------------------------------------------------------------------
void Platform_SetCameraOrbit(const vector3& orbit)
{
    s_cameraOrbit = orbit;
}
//-------------------------------------------------------------------------------------------------
void Platform_ResetCamera()
{
    s_cameraPos = INITIAL_CAMERA_POS;
    s_cameraLook = INITIAL_CAMERA_LOOK;
    s_cameraOrbit = INITIAL_CAMERA_ORBIT;
}
//-------------------------------------------------------------------------------------------------
void Platform_BasicCameraInput(const Input& input, float dt)
{
	if (input.mouseInput & LEFT_BUTTON)
	{
		s_cameraOrbit.y += input.x;
	}
	if (input.mouseInput & RIGHT_BUTTON)
	{
		s_cameraOrbit.z += input.y;
	}
	if (input.mouseInput & MIDDLE_BUTTON)
	{
		s_cameraOrbit.x += input.x;
	}
	if (input.CheckKey('W'))
	{
		s_cameraPos.x -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.x -= BASIC_CAMERA_SPEED * dt;
	}
	if (input.CheckKey('A'))
	{
		s_cameraPos.z += BASIC_CAMERA_SPEED * dt;
		s_cameraLook.z += BASIC_CAMERA_SPEED * dt;
	}
	if (input.CheckKey('S'))
	{
		s_cameraPos.x += BASIC_CAMERA_SPEED * dt;
        s_cameraLook.x += BASIC_CAMERA_SPEED * dt;
	}
	if (input.CheckKey('D'))
	{
		s_cameraPos.z -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.z -= BASIC_CAMERA_SPEED * dt;
	}
	if (input.CheckKey('Q'))
	{
		s_cameraPos.y += BASIC_CAMERA_SPEED * dt;
		s_cameraLook.y += BASIC_CAMERA_SPEED * dt;
	}
	if (input.CheckKey('E'))
	{
		s_cameraPos.y -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.y -= BASIC_CAMERA_SPEED * dt;
	}

}

