#define NSIGHTDECLAREFUNCS // to declare the function pointers
#include "HyperQ.h"
#include "coreRenderer.h"

#define NVFX

#ifndef ONLYGLES
#pragma MESSAGE("************************************************")
#pragma MESSAGE("****** Using OpenGL Full Device ****************")
#pragma MESSAGE("************************************************")
#else
#ifdef OGLES2
#pragma MESSAGE("************************************************")
#pragma MESSAGE("******** using OpenGL ES 2.0 Emulator **********")
#pragma MESSAGE("************************************************")
#else
#pragma MESSAGE("*******************************************************************************")
#pragma MESSAGE("****************** Using OpenGL Device Only with GLES2 functions **************")
#pragma MESSAGE("*******************************************************************************")
#pragma MESSAGE("C:\\p4\\sw\\docs\\gpu\\drivers\\OpenGL\\specs\\proposed\\WGL_EXT_create_context_es2_profile.txt(1) : use it")
#pragma MESSAGE("C:\\p4\\sw\\docs\\gpu\\drivers\\OpenGL\\specs\\proposed\\GL_ARB_ES2_compatibility.txt(1) : we should comply to this extension")
#pragma MESSAGE("*******************************************************************************")
#endif
#endif
#include "FXParser.h"

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------

nvSvc::ISvcFactory *g_pFactUI = NULL;
IWindowHandler     *g_pWinHandler = NULL;
IWindowConsole     *g_pConsole = NULL;
IWindowLog         *g_pLog = NULL;
IWindowFolding     *g_pControls = NULL;
IProgressBar       *g_pProgress = NULL;

HDC       g_hDC       = NULL;
HGLRC     g_hRC       = NULL;
HWND      g_hWnd      = NULL;
bool      g_isFullscreen = false;
RECT      g_windowedRect;
int		  g_winSz[]	  = {1280 + (800-784),1024+(600-562)};
int       g_origScreen[2];
HINSTANCE g_hInstance = NULL;

float   g_moveStep = 0.2f;

#ifdef OGLES2
EGLDisplay g_display;
EGLSurface g_surface;
#endif


//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE g_hInstance,HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND g_hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initGL(char * effectPathName, const char * servername, const int *udpport);
void render(void);
void shutDown(void);
bool updateAnim();


//--------------------------------------------------------------------------------------
// Timer
//--------------------------------------------------------------------------------------
class Timer
{
public:
    Timer();
    ~Timer();

    /**
     * Start the timer (Reset total elapsed time).
     *
     * Stop the timer.
     * Check if the timer is started.
     */
    void start();
    void pause();
    void resume();
    void stop();
    bool isStarted() const { return mStartTime != -1.0; }
    bool isPaused() const { return mPauseTime != -1.0; }

    /** 
     * Get the total timer elapsed time since start [s].
     */
    double getTotalElapsedTime() const;

    /** 
     * Get the timer elapsed time since last 
     * call to the same method or start [s].
     */
    double getElapsedTime() const;
    double getLastQueriedTime() const;

    /**
     * Query the current Application time.
     * It's a date expressed in s.
     */
    static double queryAppTime();

private:
    /**
     * The start time
     * -1 if not started;
     */
    double mStartTime;
    
    mutable double mPauseTime;

    /**
     * The last time queried.
     */
    mutable double mLastQueryTime;

    /**
     * The inverse frequency of the CPU on which the thread is running and used
     * to measure the elapsed time.
     */
    static double msInvFrequency;
    /**
     * initialize the inv frequency (done once on the first timer created.
     */
    static void initializeClass();
};

//--------------------------------------------------------------------------------------
// Timer
//--------------------------------------------------------------------------------------

/**
* The inverse frequency of the CPU on which the thread is running and used
* to measure the elapsed time.
*/
double Timer::msInvFrequency = 0.0;

void Timer::initializeClass()
{
    if (!msInvFrequency )
    {
#ifdef WIN32
        LARGE_INTEGER lFreq;
        QueryPerformanceFrequency(&lFreq);
        msInvFrequency= (double) lFreq.QuadPart;
        msInvFrequency = 1.0/ msInvFrequency;
#endif
    }
}

Timer::Timer() :
    mStartTime(-1),
    mPauseTime(-1),
    mLastQueryTime(-1)
{
    initializeClass();
}

Timer::~Timer()
{
}

void Timer::start()
{
    mStartTime = mLastQueryTime = queryAppTime();
    mPauseTime = -1;
}

void Timer::pause()
{
    if ( mPauseTime == -1 )
    {
        mPauseTime = mLastQueryTime = queryAppTime();
    }
}

void Timer::resume()
{
    if ( mPauseTime != -1 )
    {
        mLastQueryTime = queryAppTime();
        mStartTime += (mLastQueryTime - mPauseTime);
        mPauseTime = -1;
    }
}

void Timer::stop()
{
    mStartTime = mLastQueryTime = 0.0;
    mPauseTime = -1;
}

double Timer::getTotalElapsedTime() const
{
    if (isStarted())
    {
        if (isPaused())
        {
            return mPauseTime - mStartTime;
        }
        else
        {
            return queryAppTime() - mStartTime;
        }
    }
    return 0.0;
}

double Timer::getElapsedTime() const
{
    if (isStarted())
    {
        double lTime = mLastQueryTime;
        mLastQueryTime = queryAppTime();
        return mLastQueryTime - lTime;
    }
    return 0.0;
}

double Timer::getLastQueriedTime() const
{
    if (isStarted())
    {
        return mLastQueryTime;
    }
    return 0.0;
}

double Timer::queryAppTime()
{
#ifdef WIN32
        LARGE_INTEGER lCount;
        QueryPerformanceCounter(&lCount);
        return (lCount.QuadPart * msInvFrequency);
#endif
}

Timer timer;

/*----------------------------------------------
  --
  -- Message display
  --
  ----------------------------------------------*/
void PrintMessage(LPCSTR fmt, ...)
{
    static char dest[400];
    LPCSTR *ppc = (LPCSTR*)_ADDRESSOF(fmt);
    vsprintf_s(dest, 399, fmt, (va_list)&(ppc[1]));
    OutputDebugString(dest);
    //OutputDebugString("\n");
}

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void setWindowSize(int x, int y, int w, int h)
{
    if(g_isFullscreen)
        return;
    LONG_PTR wndStyle = GetWindowLongPtr(g_hWnd,GWL_STYLE);
    //put back border and caption
    wndStyle |= WS_BORDER | WS_CAPTION | WS_SIZEBOX;
    SetWindowLongPtr(g_hWnd, GWL_STYLE, wndStyle);
    SetWindowPos(g_hWnd, HWND_NOTOPMOST, x, y, w, h, SWP_SHOWWINDOW);
}
//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void setFullScreen(bool fs, bool changeDisplay)
{
    if (fs)
    {
      if (!g_isFullscreen)
      {
        //if not FS save old rect
         GetWindowRect(g_hWnd, &g_windowedRect);
      }

      LONG_PTR wndStyle = GetWindowLongPtr(g_hWnd,GWL_STYLE);
      //remove border and caption
      wndStyle &= ~WS_BORDER;
      wndStyle &= ~WS_CAPTION;
      wndStyle &= ~WS_SIZEBOX;
      SetWindowLongPtr(g_hWnd, GWL_STYLE, wndStyle);

      //make it always on top and screen-sized
      HMONITOR wndMon = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY);
      MONITORINFOEX monInfo;
      monInfo.cbSize = sizeof(MONITORINFOEX);
      GetMonitorInfo(wndMon, &monInfo);
      if(changeDisplay)
      {
          DEVMODE dm;
          dm.dmSize = sizeof(DEVMODE);
          BOOL bRes = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
          g_origScreen[0] = dm.dmPelsWidth;
          g_origScreen[1] = dm.dmPelsHeight;
          dm.dmPelsWidth = canvas.x;
          dm.dmPelsHeight = canvas.y;
          dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT;
          LONG l = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
          SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, canvas.x, canvas.y, SWP_SHOWWINDOW);
      } else {
          int x = monInfo.rcMonitor.left;
          int y = monInfo.rcMonitor.top;
          int w = monInfo.rcMonitor.right - x;
          int h = monInfo.rcMonitor.bottom - y;
          SetWindowPos(g_hWnd, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW);
      }
      g_isFullscreen = true;
    }
    else if (g_isFullscreen)
    {
      DEVMODE dm;
      dm.dmSize = sizeof(DEVMODE);
      BOOL bRes = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
      dm.dmPelsWidth = g_origScreen[0];
      dm.dmPelsHeight = g_origScreen[1];
      dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT;
      LONG l = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

      LONG_PTR wndStyle = GetWindowLongPtr(g_hWnd,GWL_STYLE);
      //put back border and caption
      wndStyle |= WS_BORDER | WS_CAPTION | WS_SIZEBOX;
      SetWindowLongPtr(g_hWnd, GWL_STYLE, wndStyle);
      int x = g_windowedRect.left;
      int y = g_windowedRect.top;
      int w = g_windowedRect.right-x;
      int h = g_windowedRect.bottom-y;
      SetWindowPos(g_hWnd, HWND_NOTOPMOST, x, y, w, h, SWP_SHOWWINDOW);
      g_isFullscreen = false;
    }
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
int WINAPI WinMain(    HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nCmdShow )
{
    WNDCLASS winClass;
    MSG        uMsg;

    memset(&uMsg,0,sizeof(uMsg));

    //winClass.lpszClassName = "MY_WINDOWS_CLASS";
    //winClass.cbSize        = sizeof(WNDCLASSEX);
    //winClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    //winClass.lpfnWndProc   = WindowProc;
    //winClass.hInstance     = hInstance;
    //winClass.hIcon           = LoadIcon(hInstance, (LPCTSTR)IDI_OPENGL_ICON);
    //winClass.hIconSm       = LoadIcon(hInstance, (LPCTSTR)IDI_OPENGL_ICON);
    //winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    //winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //winClass.lpszMenuName  = NULL;
    //winClass.cbClsExtra    = 0;
    //winClass.cbWndExtra    = 0;
    
    winClass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC ;
    winClass.lpfnWndProc    = WindowProc;
    winClass.cbClsExtra     = 0;
    winClass.cbWndExtra     = 0;
    winClass.hInstance      = hInstance;
    winClass.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_OPENGL_ICON);
    winClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground  = (HBRUSH) GetStockObject (BLACK_BRUSH);
    winClass.lpszMenuName   = NULL;
    winClass.lpszClassName  = TEXT("EGLWndClass");

    if(!RegisterClass(&winClass) )
        return E_FAIL;

#ifdef OGLES2
    g_hWnd = CreateWindow( 
        TEXT("EGLWndClass"), 
        NULL, 
        WS_OVERLAPPED|WS_SYSMENU|WS_THICKFRAME|WS_DISABLED, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        NULL, 
        NULL, 
        (HINSTANCE)GetModuleHandle(NULL), 
        NULL); 
    EnableWindow(g_hWnd, TRUE); 
    RECT area; 
    area.left = 0; 
    area.top = 0;
    area.right = g_winSz[0]; 
    area.bottom = g_winSz[1]; 
    SetWindowPos(g_hWnd, HWND_TOP, 
                 area.left, area.top, 
                 area.right, area.bottom, 
                 SWP_NOMOVE); 
    SetForegroundWindow(g_hWnd); 
    ShowWindow(g_hWnd, SW_SHOWNORMAL);
#else
    //g_hWnd = CreateWindowEx( NULL, "MY_WINDOWS_CLASS",
    //                         "Mesh Viewer",
    //                         WS_OVERLAPPEDWINDOW, 0, 0, g_winSz[0], g_winSz[1], NULL, NULL, 
    //                         g_hInstance, NULL );
    g_hWnd = CreateWindow(
        TEXT("EGLWndClass"),
        NULL,
        WS_OVERLAPPED|WS_THICKFRAME/*|WS_SYSMENU|WS_THICKFRAME|WS_DISABLED*/,
        0, 0, g_winSz[0], g_winSz[1],//CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL);

#endif
    if( g_hWnd == NULL )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
    UpdateWindow( g_hWnd );

    //--------------------------------------------------------------------------
    UISERVICE_LOAD(g_pFactUI, g_pWinHandler);
    class ControlsEvents : public IEventsWnd
    {
        void ScalarChanged(IControlScalar *pWin, float &v, float prev)
        {
        };
        void Button(IWindow *pWin, int pressed) 
        {
            size_t tag;
            pWin->GetUserData(&tag);
            WindowProc( g_hWnd, WM_KEYDOWN, (WPARAM)tag, 0);
        };
        void ToolbarChanged(IControlToolbar *pWin, int selecteditem, int prev) 
        {
            int states;
            size_t tag;
            int ddsidx;
            pWin->GetItemInfos(selecteditem, states, tag, NULL, 0, NULL, 0, ddsidx);
            WindowProc( g_hWnd, WM_KEYDOWN, (WPARAM)tag, 0);
        }
        void CheckBoxChanged(IControlScalar *pWin, bool &value, bool prev)
        {
        }
	    void ComboSelectionChanged(IControlCombo *pWin, unsigned int selectedidx)
        {
        };
    };
    static ControlsEvents controlsEvents;
    //---------------------------------------------------------------------------
    if(g_pWinHandler)
    {
        //g_pConsole = g_pWinHandler->CreateWindowConsole("CONSOLE", "Log");
        //g_pConsole->SetVisible()->SetLocation(g_winSz[0]+30,0)->SetSize(500,400);
        g_pLog = g_pWinHandler->CreateWindowLog("LOG", "Log");
        g_pLog->SetVisible()->SetLocation(g_winSz[0]+30,0)->SetSize(500,400);
        g_pProgress = g_pWinHandler->CreateWindowProgressBar("PROG", "Loading", NULL);
        g_pProgress->SetVisible(0);
        g_pControls =  g_pWinHandler->CreateWindowFolding("CTRLS", "Controls");
        g_pControls->SetVisible(1);
        g_pWinHandler->VariableBind(g_pWinHandler->CreateCtrlScalar("HEAT", "HeatTest", g_pControls)->SetBounds(0.0,1.0), &heatTest);
        g_pWinHandler->VariableBind(g_pWinHandler->CreateCtrlScalar("NSPEED", "Noise Speed", g_pControls)->SetBounds(0.0,1.0), &noiseSpeed);
        g_pWinHandler->VariableBind(g_pWinHandler->CreateCtrlScalar("HNF", "heat on noise", g_pControls)->SetBounds(0.0,2.0), &heatNoiseFact);

//        g_pWinHandler->VariableBind(g_pWinHandler->CreateCtrlCheck("RT", "Render Realtime", g_pControls), &g_realtime.bNonStopRendering);

        g_pWinHandler->Register(&controlsEvents);
        g_pControls->UnFold(true);
    }
    PRINTF((
        "-----------------------------------------------------------\n\n"
        ));

    //if(*lpCmdLine == '\0')
	char hostname[100];
	int  port;
	int n = sscanf(lpCmdLine, "%s %d", hostname, &port);
	if(!initGL("art_asset", n >= 1 ? hostname : NULL, n>= 2 ? &port : NULL))
        return 1;

    timer.start();
    while( uMsg.message != WM_QUIT )
    {
        if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
        { 
            TranslateMessage( &uMsg );
            DispatchMessage( &uMsg );
        }
        else 
        {
            if(updateAnim() /*|| g_realtime.bNonStopRendering || g_realtime.renderCnt > 0*/)
            {
                render();
                Sleep(1.0);
                float dt = timer.getElapsedTime();
                if(dt > 0.033)
                    dt = 0.033;
                else if(dt < 0.0001)
                    dt = 0.0001;
                /*nves2::*/update(dt);
            }
            else
            {
                Sleep(100);
            }
        }
    }

    shutDown();
    if(g_pFactUI)
    {
        if(g_pWinHandler)
            g_pWinHandler->DestroyAll();
        UISERVICE_UNLOAD(g_pFactUI, g_pWinHandler);
    }
    UnregisterClass( "MY_WINDOWS_CLASS", g_hInstance );

    return (int)uMsg.wParam;
}

//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT CALLBACK WindowProc( HWND   g_hWnd, 
                             UINT   msg, 
                             WPARAM wParam, 
                             LPARAM lParam )
{
    static POINT ptLastMousePosit;
    static POINT ptCurrentMousePosit;
    static bool bCtrl;
    static bool bMousing;
    static bool bRMousing;
    static bool bMMousing;

    bool bRes = false;
    //
    // Pass the messages to our UI, first
    //
    if(!bRes) switch( msg )
    {
        case WM_PAINT:
            break;
        case WM_KEYUP:
        {
            switch( wParam )
            {
            case VK_CONTROL:
                bCtrl = false;
                break;
            }
        }
        break;
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_CONTROL:
                    bCtrl = true;
                    break;
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_SPACE:
                    pingServer();
                    break;
                case VK_LEFT:
                    break;
                case VK_RIGHT:
                    break;
                case VK_UP:
                    break;
                case VK_DOWN:
                    break;
                case VK_PRIOR:
                    break;
                case VK_NEXT:
                    break;
            }
        }
        break;

        case WM_LBUTTONDOWN:
        {
            ptLastMousePosit.x = ptCurrentMousePosit.x = LOWORD (lParam);
            ptLastMousePosit.y = ptCurrentMousePosit.y = HIWORD (lParam);
            bMousing = true;
        }
        break;

        case WM_LBUTTONUP:
        {
            toggleHyperQ();
            bMousing = false;
        }
        break;

        case WM_RBUTTONDOWN:
        {
            ptLastMousePosit.x = ptCurrentMousePosit.x = LOWORD (lParam);
            ptLastMousePosit.y = ptCurrentMousePosit.y = HIWORD (lParam);
            bRMousing = true;
        }
        break;

        case WM_RBUTTONUP:
        {
            toggleHyperQ();
            bRMousing = false;
        }
        break;

        case WM_MBUTTONDOWN:
        {
            ptLastMousePosit.x = ptCurrentMousePosit.x = LOWORD (lParam);
            ptLastMousePosit.y = ptCurrentMousePosit.y = HIWORD (lParam);
            bMMousing = true;
        }
        break;

        case WM_MBUTTONUP:
        {
            bMMousing = false;
        }
        break;

        case WM_MOUSEMOVE:
        {
            ptCurrentMousePosit.x = LOWORD (lParam);
            ptCurrentMousePosit.y = HIWORD (lParam);

            if( bMousing )
            {
                float hval = 2.0f*(float)(ptCurrentMousePosit.x - ptLastMousePosit.x)/(float)g_winSz[0];
                float vval = 2.0f*(float)(ptCurrentMousePosit.y - ptLastMousePosit.y)/(float)g_winSz[1];
            }
            if( bRMousing )
            {
                float hval = 2.0f*(float)(ptCurrentMousePosit.x - ptLastMousePosit.x)/(float)g_winSz[0];
                float vval = -2.0f*(float)(ptCurrentMousePosit.y - ptLastMousePosit.y)/(float)g_winSz[1];
            }
            if( bMMousing )
            {
                float hval = 2.0f*(float)(ptCurrentMousePosit.x - ptLastMousePosit.x)/(float)g_winSz[0];
                float vval = 2.0f*(float)(ptCurrentMousePosit.y - ptLastMousePosit.y)/(float)g_winSz[1];
            }
            
            ptLastMousePosit.x = ptCurrentMousePosit.x;
            ptLastMousePosit.y = ptCurrentMousePosit.y;
        }
        break;

        case WM_SIZE:
        {
            g_winSz[0] = LOWORD(lParam); 
            g_winSz[1] = HIWORD(lParam);
            /*nves2::*/resize(g_winSz[0], g_winSz[1]);
        }
        break;
        
        case WM_CLOSE:
        {
            PostQuitMessage(0);    
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
        
        default:
        {
        }
        break;
    }
    return DefWindowProc( g_hWnd, msg, wParam, lParam );
}
/*************************************************************************/ /**

 */ /*********************************************************************/
void quit()
{
    PostMessage(g_hWnd, WM_QUIT,0,0);
    PostQuitMessage(0);
}
/*************************************************************************/ /**

 */ /*********************************************************************/
bool updateAnim()
{
    return true;
}
#ifndef OGLES2
//-----------------------------------------------------------------------------
// Name: myOpenGLCallback
// Desc: 
//-----------------------------------------------------------------------------
void myOpenGLCallback(  GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        GLvoid* userParam)
{
    //static std::map<GLuint, bool> ignoreMap;
    //if(ignoreMap[id] == true)
    //    return;
    char *strSource = "0";
    char *strType = strSource;
    char *strSeverity = strSource;
    switch(source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
        strSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        strSource = "WINDOWS";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        strSource = "SHADER COMP.";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        strSource = "3RD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        strSource = "APP";
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        strSource = "OTHER";
        break;
    }
    switch(type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
        strType = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        strType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        strType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        strType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        strType = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        strType = "Other";
        break;
    }
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        strSeverity = "High";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        strSeverity = "Medium";
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        strSeverity = "Low";
        break;
    }
    if(g_pConsole) g_pConsole->Printf("%s - %s - %s : %s\n",strSeverity, strSource, strType, message);
    if(g_pLog) g_pLog->AddMessage("%s - %s - %s : %s\n",strSeverity, strSource, strType, message); FLUSH();
        int r = IDIGNORE; 
//    r = VSPrintfE("%s - %s - %s : %s\n",strSeverity, strSource, strType, message);
    //if(r == IDIGNORE) ignoreMap[id] = true;
//    if(r == IDABORT) DebugBreak();/*_asm { int 3 };*/
}
#endif
//-----------------------------------------------------------------------------
// Name: initGL()
// for more...
//-----------------------------------------------------------------------------
bool initGL( char * effectPathName, const char * servername, const int *udpport)
{
#ifndef OGLES2
    g_hDC = GetDC( g_hWnd );
    GLuint PixelFormat;
    
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 0;//16;
    
    g_hDC = GetDC( g_hWnd );
    PixelFormat = ChoosePixelFormat( g_hDC, &pfd );
    SetPixelFormat( g_hDC, PixelFormat, &pfd);
    g_hRC = wglCreateContext( g_hDC );
    wglMakeCurrent( g_hDC, g_hRC );
    //wglSwapInterval(0);

    GLenum res = glewInit();
    GLboolean b = glewIsSupported("GL_ARB_vertex_program");
	if(b == GL_FALSE)
	{
		LOGE("this system cannot run decent OpenGL API");
		return false;
	}

    //======================================================================================
#ifdef OGLES2
    GETPROCADDRESS(PFNWGLCREATECONTEXTATTRIBSARB,wglCreateContextAttribsARB)
#endif
    if(wglCreateContextAttribsARB)
    {
        HGLRC hRC = NULL;
        // Creates an OpenGL 3.1 forward compatible rendering context.
        // A forward compatible rendering context will not support any OpenGL 3.0
        // functionality that has been marked as deprecated.
        int attribList[] =
        {
            //WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            //WGL_CONTEXT_MINOR_VERSION_ARB, 1,
            WGL_CONTEXT_FLAGS_ARB, /*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB|*/
#ifdef _DEBUG
            WGL_CONTEXT_DEBUG_BIT_ARB,
#else
            0,
#endif
//            WGL_CONTEXT_PROFILE_MASK_ARB, 0/*WGL_CONTEXT_ES2_PROFILE_BIT_EXT*/,
            0, 0
        };
        // First try creating an OpenGL 3.1 context.
        if (!(hRC = wglCreateContextAttribsARB(g_hDC, 0, attribList)))
        {
            // Fall back to an OpenGL 3.0 context.
            attribList[3] = 0;
            if (!(hRC = wglCreateContextAttribsARB(g_hDC, 0, attribList)))
            {
                PRINTF(("wglCreateContextAttribsARB() failed for OpenGL 3 context.\n"));
                return false;
            }
        }
        if (!wglMakeCurrent(g_hDC, hRC))
        {
            PRINTF(("wglMakeCurrent() failed for OpenGL 3 context."));
            return false;
        } else {
            wglDeleteContext( g_hRC );
            g_hRC = hRC;
#ifdef _DEBUG
        if(glDebugMessageCallbackARB)
        {
            glDebugMessageCallbackARB((GLDEBUGPROCARB)myOpenGLCallback, NULL);
        }
#endif
        }
    } // wglCreateContextAttribARB
    //======================================================================================
#else
    //
    // OpenGL ES 2 : See http://www.khronos.org/opengles/documentation/opengles1_0/html/eglIntro.html
    // For Android, an example here : http://jiggawatt.org/badc0de/android/index.html
    //
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(g_display, 0, 0);
    CHECKEGLERRORS();

    typedef struct
    {
        EGLint redSize;
        EGLint greenSize;
        EGLint blueSize;
        EGLint alphaSize;
        EGLint depthSize;
        EGLint stencilSize;
    } NvTglConfigRequest;
    NvTglConfigRequest request;

    memset(&request, 0, sizeof(request));
    request.redSize = 5;
    request.greenSize = 6;
    request.blueSize = 5;
    request.alphaSize = 0;
    //if (hasDepth)
        request.depthSize = 1;   // accept any size
    //if (hasStencil)
    //    request.stencilSize = 1;   // accept any size

    #define MAX_CONFIG_ATTRS_SIZE 32
    #define MAX_MATCHING_CONFIGS 64
    EGLint Attributes[] = {
        //EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

        //EGL_RED_SIZE, 8,
        //EGL_GREEN_SIZE, 8,
        //EGL_BLUE_SIZE, 8,
        //EGL_NONE

        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BUFFER_SIZE, request.redSize + request.greenSize + request.blueSize + request.alphaSize,
        EGL_RED_SIZE, request.redSize,
        EGL_GREEN_SIZE, request.greenSize,
        EGL_BLUE_SIZE, request.blueSize,
        EGL_ALPHA_SIZE, request.alphaSize,
        EGL_DEPTH_SIZE, request.depthSize,
        EGL_STENCIL_SIZE, request.stencilSize,
        EGL_NONE
    };
    // choose configs
    EGLConfig matchingConfigs[MAX_MATCHING_CONFIGS];
    EGLint numMatchingConfigs = 0;
    eglChooseConfig(g_display, Attributes, matchingConfigs, 1, &numMatchingConfigs);
    CHECKEGLERRORS();
    EGLConfig config;
    // look for exact match
    int i;
    for (i = 0; i < numMatchingConfigs; i++)
    {
        EGLConfig testConfig = matchingConfigs[i];

        // query attributes from config
        NvTglConfigRequest cfg;
        EGLBoolean ok = EGL_TRUE;
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_RED_SIZE, &cfg.redSize);
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_GREEN_SIZE, &cfg.greenSize);
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_BLUE_SIZE, &cfg.blueSize);
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_ALPHA_SIZE, &cfg.alphaSize);
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_DEPTH_SIZE, &cfg.depthSize);
        ok &= eglGetConfigAttrib(g_display, testConfig, EGL_STENCIL_SIZE, &cfg.stencilSize);
        if (!ok)
        {
            printf("eglGetConfigAttrib failed!\n");
            break;
        }
        // depthSize == 1 indicates to accept any non-zero depth
        EGLBoolean depthMatches = ((request.depthSize == 1) && (cfg.depthSize > 0)) ||
                       (cfg.depthSize == request.depthSize);

        // stencilSize == 1 indicates to accept any non-zero stencil
        EGLBoolean stencilMatches = ((request.stencilSize == 1) && (cfg.stencilSize > 0)) ||
                         (cfg.stencilSize == request.stencilSize);
        
        // compare to request
        if (cfg.redSize == request.redSize &&
                cfg.greenSize == request.greenSize &&
                cfg.blueSize == request.blueSize &&
                cfg.alphaSize == request.alphaSize &&
                depthMatches && stencilMatches)
        {
            config = testConfig;
            break;
        }
    }
    if (i >= numMatchingConfigs)
    {
        // no exact match was found
        //if (exactMatchOnly)
        //{
        //    printf("No exactly matching EGL configs found!\n");
        //    return false;
        //}

        // just return any matching config (ie, the first one)
        config = matchingConfigs[0];
    }



    EGLint ContextAttributes[] = {	EGL_CONTEXT_CLIENT_VERSION, 2,	EGL_NONE
    };
    EGLContext Context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, ContextAttributes);
    CHECKEGLERRORS();
    // In Windows System, EGLNativeWindowType == HWND
    g_surface = eglCreateWindowSurface(g_display, config, g_hWnd, NULL);
    CHECKEGLERRORS();
    eglMakeCurrent(g_display, g_surface, g_surface, Context);
    CHECKEGLERRORS();
#endif
    //
    // Now init the 3D stuff
    //
    if(!/*nves2::*/init("art_asset", "effect.glslfx", servername, udpport))
        return false;
    return true;
}
//-----------------------------------------------------------------------------
// Name: shutDown()
// Desc: 
//-----------------------------------------------------------------------------
void shutDown(void)
{
    /*nves2::*/finish();
#ifndef OGLES2
    //if( g_hRC != NULL )
    //{
    //    wglMakeCurrent( NULL, NULL );
    //    wglDeleteContext( g_hRC );
    //    g_hRC = NULL;                            
    //}

    if( g_hRC != NULL )
    {
        ReleaseDC( g_hWnd, g_hDC );
        g_hDC = NULL;
    }
#endif
}
//-----------------------------------------------------------------------------
// Name: render()
// Desc: 
//-----------------------------------------------------------------------------
void render( void )
{
    //
    // OPENGL Stuff for transformations
    //
    /*nves2::*/render_image();

#ifndef OGLES2
    SwapBuffers( g_hDC );
#else
    eglSwapBuffers(g_display, g_surface);
#endif
    CHECKGLERRORS();
}
