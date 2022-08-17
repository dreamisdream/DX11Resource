//--------------------------------------------------------------------------------------
// File: DirectMusic.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // D3D头文件
#include <d3dx9.h>                  // D3DX库头文件
#include <stdio.h>
#include <dsound.h>
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusics.h>
#include <dmusicc.h>
#include <dmusbuff.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"DirectMusic";            // 窗口类名
wchar_t *g_pWindowName = L"DirectMusic应用示例";    // 窗口标题名

IDirectMusicLoader8*        g_pDirectMusicLoader  = NULL;   // DirectMusic加载器
IDirectMusicSegment8*       g_pDirectMusicSegment = NULL;   // DirectMusic段
IDirectMusicPerformance8*   g_pDirectMusicPerform = NULL;   // DirectMusic演奏器
LPDIRECT3DDEVICE9           g_pd3dDevice          = NULL;   // D3D设备接口


HRESULT InitDirect3D(HWND hWnd);    // 初始化Direct3D
VOID Direct3DRender();              // 渲染图形
VOID Direct3DCleanup();             // 清理Direct3D资源

// 窗口消息处理函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: Windows应用程序入口函数
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // 初始化窗口类
    WNDCLASS wndclass;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);  // 窗口背景
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);          // 光标形状
    wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);      // 窗口小图标
    wndclass.hInstance      = hInstance;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.lpszClassName  = g_pClassName;
    wndclass.lpszMenuName   = NULL;
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;

    // 注册窗口类
    if (!RegisterClass(&wndclass))
        return 0;

    // 创建窗口
    HWND hWnd = CreateWindow(g_pClassName, g_pWindowName, WS_OVERLAPPEDWINDOW, 
        100, 100, 640, 480, NULL, NULL, wndclass.hInstance, NULL);

    // 初始化Direct3D
    InitDirect3D(hWnd);

    // 显示、更新窗口
    ShowWindow(hWnd, SW_SHOWDEFAULT); 
    UpdateWindow(hWnd); 

    // 消息循环
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message!=WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Direct3DRender();       // 绘制3D场景
        }
    }

    UnregisterClass(g_pClassName, wndclass.hInstance);
    return 0;
}

//--------------------------------------------------------------------------------------
// Name: WndProc()
// Desc: 窗口消息处理函数
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message) 
    {
    case WM_PAINT:                  // 客户区重绘消息
        Direct3DRender();           // 渲染图形
        ValidateRect(hWnd, NULL);   // 更新客户区的显示
        break;
    case WM_KEYDOWN:                // 键盘按下消息
        if (wParam == VK_ESCAPE)    // ESC键
            DestroyWindow(hWnd);    // 销毁窗口, 并发送一条WM_DESTROY消息
        break;
    case WM_DESTROY:                // 窗口销毁消息
        Direct3DCleanup();          // 清理Direct3D
        PostQuitMessage(0);         // 退出程序
        break;
    }
    // 默认的消息处理
    return DefWindowProc( hWnd, message, wParam, lParam );
}

//--------------------------------------------------------------------------------------
// Name: InitDirect3D()
// Desc: 初始化Direct3D
//--------------------------------------------------------------------------------------
HRESULT InitDirect3D(HWND hWnd) 
{
    // 创建IDirect3D接口
    LPDIRECT3D9 pD3D = NULL;                    // IDirect3D9接口
    pD3D = Direct3DCreate9(D3D_SDK_VERSION);    // 创建IDirect3D9接口对象
    if (pD3D == NULL) return E_FAIL;

    // 获取硬件设备信息
    D3DCAPS9 caps; int vp = 0;
    pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps );
    if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    // 创建Direct3D设备接口
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth            = 640;
    d3dpp.BackBufferHeight           = 480;
    d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount            = 1;
    d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality         = 0;
    d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
    d3dpp.hDeviceWindow              = hWnd;
    d3dpp.Windowed                   = true;
    d3dpp.EnableAutoDepthStencil     = true; 
    d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
    d3dpp.Flags                      = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

    pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
        hWnd, vp, &d3dpp, &g_pd3dDevice);
    pD3D->Release();

    // 初始化COM
    CoInitialize(NULL);

    // 创建IDirectMusicPerformace接口对象
    CoCreateInstance(CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC, 
        IID_IDirectMusicPerformance8, (void**)&g_pDirectMusicPerform);

    // 创建DirectMusic加载器对象
    CoCreateInstance(CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, 
        IID_IDirectMusicLoader8, (void**)&g_pDirectMusicLoader);

    // 初始化演奏器
    g_pDirectMusicPerform->InitAudio(NULL, NULL, hWnd, 
        DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);

    // 设置默认搜索路径
    wchar_t path[MAX_PATH];
    ::GetCurrentDirectory(MAX_PATH, path);
    g_pDirectMusicLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, path, FALSE);

    // 加载MIDI文件
    DMUS_OBJECTDESC dm_obj_desc;
    ZeroMemory(&dm_obj_desc, sizeof(DMUS_OBJECTDESC));
    dm_obj_desc.dwSize      = sizeof(DMUS_OBJECTDESC);
    dm_obj_desc.guidClass   = CLSID_DirectMusicSegment;
    dm_obj_desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
    ::lstrcpy(dm_obj_desc.wszFileName, L"example.mid");

    g_pDirectMusicLoader->GetObject(&dm_obj_desc, IID_IDirectMusicSegment, (void**)&g_pDirectMusicSegment);

    // MIDI播放设置
    g_pDirectMusicSegment->SetParam(GUID_StandardMIDIFile, 0xFFFFFFFF, 0, 0, NULL);
    g_pDirectMusicSegment->Download(g_pDirectMusicPerform);
    g_pDirectMusicSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: 绘制3D场景
//--------------------------------------------------------------------------------------
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(45, 50, 170), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    if (::GetAsyncKeyState(VK_SPACE) & 0x8000f)
        g_pDirectMusicPerform->PlaySegment(g_pDirectMusicSegment, 0, 0, NULL);
    if (::GetAsyncKeyState('S') & 0x8000f)
        g_pDirectMusicPerform->Stop(g_pDirectMusicSegment, NULL, 0, 0);

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    g_pDirectMusicSegment->Unload(g_pDirectMusicPerform);   // 卸载MIDI数据
    g_pDirectMusicPerform->CloseDown();                     // 关闭演奏器对象
    SAFE_RELEASE(g_pDirectMusicLoader);                     // 释放MIDI段
    SAFE_RELEASE(g_pDirectMusicSegment);                    // 释放DirectMusic加载器
    SAFE_RELEASE(g_pDirectMusicPerform);                    // 释放演奏器对象
    SAFE_RELEASE(g_pd3dDevice);
    CoUninitialize();                                       // 关闭COM
}
