/**
 * 绘制 N-Body 模拟结果
 * 使用 WinAPI 和 OpenGL
*/

#include <windows.h>
#include <gl/gl.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <ctime>

using std::vector;
typedef double Real;
struct Vec3 {
    Real x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(Real ix, Real iy, Real iz) : x(ix), y(iy), z(iz) {}
};

LRESULT CALLBACK WndProc (HWND hWnd, UINT message,
WPARAM wParam, LPARAM lParam);
void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC);
void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC);

Real k = 5E-6;
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int iCmdShow)
{
    WNDCLASS wc;
    HWND hWnd;
    HDC hDC;
    HGLRC hRC;        
    MSG msg;
    BOOL bQuit = FALSE;

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "N-Body";
    RegisterClass (&wc);

    hWnd = CreateWindow (
      "N-Body", "N-Body with OpenGL", 
      WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
      0, 0, 1024, 768,
      NULL, NULL, hInstance, NULL);

	/* init */
	FILE *fp = fopen("data.out", "r");
	int num, turns;
	fscanf(fp, "%d %d", &num, &turns);
	vector<vector<Vec3> > data(turns, vector<Vec3>(num));
	for (int T = 0; T < turns; T++) {
		for (int i = 0; i < num; i++) {
			fscanf(fp, "%lf %lf %lf", &data[T][i].x, &data[T][i].y, &data[T][i].z);
		}
	}
	fclose(fp);
	
	int curTurn = 0;
	
    EnableOpenGL (hWnd, &hDC, &hRC);
    glPointSize(1.0);
	
    /* program main loop */
    int timer;
    auto init = [&]() {
		curTurn = 0;
		k = 5E-6;
		timer = clock();
	};
	init();
    while (!bQuit)
    {
        if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */
            
			if (curTurn >= turns) {
				init();
			}
			
            glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
            glClear (GL_COLOR_BUFFER_BIT);
            
            glPushMatrix ();
            glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(30.0f, 1.0f, 0.0f, -1.0f);
            
			glBegin (GL_LINES);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(k * 5E4, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, k * 5E4, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, k * 5E4);
			glEnd ();
			
            glBegin (GL_POINTS);
            for (int i = 0; i < num; i++) {
            	auto *u = &data[curTurn][i];
            	glVertex3f(u->x * k, u->y * k, u->z * k);
			}
            glEnd ();
            
            glPopMatrix ();

            SwapBuffers (hDC);
			
			/* 25 frames per second */
			while (clock() - timer < 40 * curTurn)
            	Sleep (1);
            
            curTurn++;
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL (hWnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow (hWnd);

    return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT message,
                          WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_CREATE:
        return 0;
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;

    case WM_DESTROY:
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case 'I':
        	k *= 1.25;
            return 0;
        case 'O':
        	k *= 0.8;
        	return 0;
        }
        return 0;

    default:
        return DefWindowProc (hWnd, message, wParam, lParam);
    }
}

void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC (hWnd);

    /* set the pixel format for the DC */
    ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | 
      PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFormat = ChoosePixelFormat (*hDC, &pfd);
    SetPixelFormat (*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext( *hDC );
    wglMakeCurrent( *hDC, *hRC );

}

void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent (NULL, NULL);
    wglDeleteContext (hRC);
    ReleaseDC (hWnd, hDC);
}
