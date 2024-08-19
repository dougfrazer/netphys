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


struct InputMask
{
	union
	{
		struct
		{
			uint64_t mouse : 3;
			uint64_t chars : 26;
			uint64_t numbers : 10;
			uint64_t arrows : 4;

			// for more inputs just take it off the padding
			uint64_t padding : 21;
		};

		uint64_t all_inputs;
	};

};
static_assert(sizeof(InputMask) == sizeof(uint64_t), "check bit packing for keys");

struct Input
{
	InputMask inputMask;
	int x;
	int y;

	void Clear()
	{
		inputMask.all_inputs = 0;
		x = 0;
		y = 0;
	}

	bool CheckKey(char key) const
	{
		if (key >= '0' && key <= '9')
		{
			const int index = key - '0';
			return (inputMask.numbers & (1ull << index)) > 0;
		}

		if (key >= 'a' && key <= 'z')
		{
			// might just be able to assert and make it so the caller has to pass in either always
			// the capital char or always the lower case char, but easy enough to support both
			key = 'A' + key - 'a';
		}

		if (key >= 'A' && key <= 'Z')
		{
			const int index = key - 'A';
			return (inputMask.chars & (1ull << index)) > 0;
		}

		assert(false); // key passed in is neither a number or character, need to set up mapping/storage for this key
		return false;
	}

	bool CheckKey(SPECIAL_INPUT_KEY key) const
	{
		if (key >= MOUSE_INPUT_FIRST && key <= MOUSE_INPUT_LAST)
		{
			const int index = key - MOUSE_INPUT_FIRST;
			return (inputMask.mouse & (1ull << index)) > 0;
		}

		if (key >= ARROW_INPUT_FIRST && key <= ARROW_INPUT_LAST)
		{
			const int index = key - ARROW_INPUT_FIRST;
			return (inputMask.arrows & (1ull << index)) > 0;
		}

		assert(false); // key passed in needs mapping
		return false;
	}
};

static Input s_mostRecentInput = { 0 };
static Input s_lastCheckedInput = { 0 };
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
                s_mostRecentInput.inputMask.mouse |= (1ull << LEFT_MOUSE);
            }
            else if (msg == WM_MBUTTONDOWN)
            {
                s_mostRecentInput.inputMask.mouse |= (1ull << MIDDLE_MOUSE);
            }
            else if(msg == WM_RBUTTONDOWN)
            {
                s_mostRecentInput.inputMask.mouse |= (1ull << RIGHT_MOUSE);
            }
            else
            {
                assert(false); // wtf?
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
                s_mostRecentInput.inputMask.mouse &= ~(1ull << LEFT_MOUSE);
            }
            else if (msg == WM_MBUTTONUP)
            {
                s_mostRecentInput.inputMask.mouse &= ~(1ull << MIDDLE_MOUSE);
            }
            else if (msg == WM_RBUTTONUP)
            {
                s_mostRecentInput.inputMask.mouse &= ~(1ull << RIGHT_MOUSE);
            }
			else
			{
				assert(false); // wtf?
			}

            if (!s_mostRecentInput.inputMask.mouse)
            {
                ReleaseCapture();
            }
        }
        break;

        case WM_MOUSEMOVE:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            s_mostRecentInput.x += x - s_lastMoveX;
            s_mostRecentInput.y += y - s_lastMoveY;
            s_lastMoveX = x;
            s_lastMoveY = y;
        }
        break;

        case WM_KEYDOWN:
        {
			char c = (char)(wParam);
			assert(c < 'a' || c > 'z'); // pretty sure we don't get lower case characters

			if (c >= 'A' && c <= 'Z')
			{
				const int index = c - 'A';
				s_mostRecentInput.inputMask.chars |= (1ull << index);
			}
			else if (c >= '0' && c <= '9')
			{
				const int index = c - '0';
				s_mostRecentInput.inputMask.numbers |= (1ull << index);
			}
			else
			{
				switch (wParam)
				{
				case VK_LEFT:   s_mostRecentInput.inputMask.arrows |= (1ull << (ARROW_KEY_LEFT  - ARROW_INPUT_FIRST)); break;
				case VK_RIGHT:  s_mostRecentInput.inputMask.arrows |= (1ull << (ARROW_KEY_RIGHT - ARROW_INPUT_FIRST)); break;
				case VK_DOWN:   s_mostRecentInput.inputMask.arrows |= (1ull << (ARROW_KEY_DOWN  - ARROW_INPUT_FIRST)); break;
				case VK_UP:     s_mostRecentInput.inputMask.arrows |= (1ull << (ARROW_KEY_UP    - ARROW_INPUT_FIRST)); break;
				default: break;
				}
            }
        }
        break;

		case WM_KEYUP:
		{
			char c = (char)(wParam);
            assert(c < 'a' || c > 'z'); // pretty sure we don't get lower case characters

			if (c >= 'A' && c <= 'Z')
			{
				const int index = c - 'A';
				s_mostRecentInput.inputMask.chars &= ~(1ull << index);
			}
			else if (c >= '0' && c <= '9')
			{
				const int index = c - '0';
				s_mostRecentInput.inputMask.numbers &= ~(1ull << index);
			}
            else
            {
				switch (wParam)
				{
				case VK_LEFT:   s_mostRecentInput.inputMask.arrows &= ~(1ull << (ARROW_KEY_LEFT  - ARROW_INPUT_FIRST)); break;
				case VK_RIGHT:  s_mostRecentInput.inputMask.arrows &= ~(1ull << (ARROW_KEY_RIGHT - ARROW_INPUT_FIRST)); break;
				case VK_DOWN:   s_mostRecentInput.inputMask.arrows &= ~(1ull << (ARROW_KEY_DOWN  - ARROW_INPUT_FIRST)); break;
				case VK_UP:     s_mostRecentInput.inputMask.arrows &= ~(1ull << (ARROW_KEY_UP    - ARROW_INPUT_FIRST)); break;
				default: break;
				}
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
static void EndFrame()
{
    s_lastCheckedInput = s_mostRecentInput;
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

    auto frameEnd = std::chrono::high_resolution_clock::now();
    constexpr float MAX_FPS_MS = (1000.f / 60.f);

    // TODO: separate threads?
    while(s_running)
    {
		auto frameStart = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> delta_ms = (frameStart - frameEnd);
		float dt = min(MAX_FPS_MS, (float)(delta_ms.count())) / 1000.f;
        
        // handle windows messages
        MSG msg;
        PeekMessage(&msg, window, 0, 0, PM_REMOVE);
        if (!TranslateAccelerator(window, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

		// Do update/render calls
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
      
        EndFrame();

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

    DestroyWindow(window);
}
//-------------------------------------------------------------------------------------------------
bool Platform_InputIsDown(char key)
{
    return s_mostRecentInput.CheckKey(key);
}
//-------------------------------------------------------------------------------------------------
bool Platform_InputIsDown(SPECIAL_INPUT_KEY key)
{
    return s_mostRecentInput.CheckKey(key);
}
//-------------------------------------------------------------------------------------------------
bool Platform_InputChangedDown(char key)
{
    if (s_mostRecentInput.CheckKey(key))
    {
        if (!s_lastCheckedInput.CheckKey(key))
        {
            return true;
        }
    }
    return false;
}
//-------------------------------------------------------------------------------------------------
bool Platform_InputChangedDown(SPECIAL_INPUT_KEY key)
{
	if (s_mostRecentInput.CheckKey(key))
	{
		if (!s_lastCheckedInput.CheckKey(key))
		{
			return true;
		}
	}
	return false;
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
void Platform_BasicCameraInput(float dt)
{
	if (Platform_InputIsDown(LEFT_MOUSE))
	{
		s_cameraOrbit.y += s_mostRecentInput.x;
	}
	if (Platform_InputIsDown(RIGHT_MOUSE))
	{
		s_cameraOrbit.z += s_mostRecentInput.y;
	}
	if (Platform_InputIsDown(MIDDLE_MOUSE))
	{
		s_cameraOrbit.x += s_mostRecentInput.x;
	}
	if (Platform_InputIsDown('W'))
	{
		s_cameraPos.x -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.x -= BASIC_CAMERA_SPEED * dt;
	}
	if (Platform_InputIsDown('A'))
	{
		s_cameraPos.z += BASIC_CAMERA_SPEED * dt;
		s_cameraLook.z += BASIC_CAMERA_SPEED * dt;
	}
	if (Platform_InputIsDown('S'))
	{
		s_cameraPos.x += BASIC_CAMERA_SPEED * dt;
        s_cameraLook.x += BASIC_CAMERA_SPEED * dt;
	}
	if (Platform_InputIsDown('D'))
	{
		s_cameraPos.z -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.z -= BASIC_CAMERA_SPEED * dt;
	}
	if (Platform_InputIsDown('Q'))
	{
		s_cameraPos.y += BASIC_CAMERA_SPEED * dt;
		s_cameraLook.y += BASIC_CAMERA_SPEED * dt;
	}
	if (Platform_InputIsDown('E'))
	{
		s_cameraPos.y -= BASIC_CAMERA_SPEED * dt;
		s_cameraLook.y -= BASIC_CAMERA_SPEED * dt;
	}

}

