//--------------------------------------------------------------------------------------
// File: SimpleLodTerrain.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件
#include <dinput.h>                 // DirectInput头文件
#include "Camera.h"
#include "LodTerrain.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"SimpleLodTerrain";   // 窗口类名
wchar_t *g_pWindowName = L"LOD地形绘制示例";    // 窗口标题名

CCamera*                g_pCamera              = NULL;
CLodTerrain*            g_pTerrain             = NULL;
LPD3DXFONT              g_pTextStats           = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL;

LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
LPDIRECTINPUTDEVICE8    g_pMouseDevice         = NULL;
char                    g_pKeyStateBuffer[256] = {0};
DIMOUSESTATE            g_diMouseState         = {0};


HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);
VOID Direct3DCleanup();

// 窗口消息处理函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: ReadDevice();
// Desc: 读取设备的输入数据
//--------------------------------------------------------------------------------------
BOOL ReadDevice(IDirectInputDevice8 *pDIDevice, void* pBuffer, long lSize) 
{
    HRESULT hr;
    while (true) 
    {
        pDIDevice->Poll();              // 轮询设备
        pDIDevice->Acquire();           // 获取设备的控制权
        if (SUCCEEDED(hr = pDIDevice->GetDeviceState(lSize, pBuffer))) break;
        if (hr != DIERR_INPUTLOST || hr != DIERR_NOTACQUIRED) return FALSE;
        if (FAILED(pDIDevice->Acquire())) return FALSE;
    }
    return TRUE;
}

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
    InitDirect3D(hWnd, hInstance);

    // 显示、更新窗口
    ShowWindow(hWnd, SW_SHOWDEFAULT); 
    UpdateWindow(hWnd); 

    // 消息循环
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message!=WM_QUIT)
    {
        static FLOAT fLastTime  = (float)::timeGetTime();
        static FLOAT fCurrTime  = (float)::timeGetTime();
        static FLOAT fTimeDelta = 0.0f;
        fCurrTime  = (float)::timeGetTime();
        fTimeDelta = (fCurrTime - fLastTime) / 1000.0f;
        fLastTime  = fCurrTime;

        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Direct3DRender(hWnd, fTimeDelta);   // 绘制3D场景
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
        //Direct3DRender(hWnd, 0.0f); // 渲染图形
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
HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance) 
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

    // 创建键盘输入设备
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboardDevice, NULL);
    g_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
    g_pKeyboardDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    g_pKeyboardDevice->Acquire();

    // 创建鼠标输入设备
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->CreateDevice(GUID_SysMouse, &g_pMouseDevice, NULL);
    g_pMouseDevice->SetDataFormat(&c_dfDIMouse);
    g_pMouseDevice->Acquire();

    // 创建2D字体
    D3DXCreateFont(g_pd3dDevice, 15, 0, 1000, 0, true, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Arial", &g_pTextStats); 

    // 设置环境光
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
    
    // 创建并初始化虚拟摄像机
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(0.0f, 15.0f, 0.0f));
    g_pCamera->ResetViewMatrix();

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 3000.0f);
    g_pCamera->ResetProjMatrix(&matProj);

    // 创建LOD地形
    g_pTerrain = new CLodTerrain(g_pd3dDevice);
    g_pTerrain->InitLodTerrain(1024.0f, 1024.0f, 0.3f, L"HeightMap.raw");

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: 绘制3D场景
//--------------------------------------------------------------------------------------
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta) 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 读取键盘输入
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // 读取鼠标输入
    ::ZeroMemory(&g_diMouseState, sizeof(g_diMouseState));
    ReadDevice(g_pMouseDevice, (LPVOID)&g_diMouseState, sizeof(g_diMouseState));

    // 移动视角
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->MoveAlongRightVec(-0.5f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->MoveAlongRightVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->MoveAlongLookVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->MoveAlongLookVec(-0.5f);
    if (g_pKeyStateBuffer[DIK_R] & 0x80) g_pCamera->MoveAlongUpVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_F] & 0x80) g_pCamera->MoveAlongUpVec(-0.5f);

    // 旋转视角
    if (g_pKeyStateBuffer[DIK_LEFT]  & 0x80) g_pCamera->RotationUpVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_RIGHT] & 0x80) g_pCamera->RotationUpVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_UP]    & 0x80) g_pCamera->RotationRightVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_DOWN]  & 0x80) g_pCamera->RotationRightVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_N]     & 0x80) g_pCamera->RotationLookVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_M]     & 0x80) g_pCamera->RotationLookVec( 0.01f);

    // 鼠标控制视角
    if (g_diMouseState.rgbButtons[0] & 0x80) 
    {
        g_pCamera->RotationUpVec(g_diMouseState.lX * 0.002f);
        g_pCamera->RotationRightVec(g_diMouseState.lY * 0.002f);
    }

    // 设置取景变换矩阵
    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 更新LOD地形
    g_pTerrain->UpdateLodTerrain(g_pTerrain->GetHeadNode());

    // 输出状态信息
    static RECT rect;
    GetClientRect(hWnd, &rect);
    static char strInfo[256] = {0};
    static FLOAT fFrame = 0.0f, fTime = 0.0f, FPS = 0.0f;

    fFrame++, fTime += fTimeDelta;
    if (fTime >= 1.0f) 
    {
        FPS = fFrame / fTime;
        fFrame = fTime = 0.0f;
    }

    rect.top += 5;
    sprintf_s(strInfo, "LOD地形示例, FPS: %.2f", FPS);
    g_pTextStats->DrawTextA(NULL, strInfo, -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));

    rect.top += 20;
    sprintf_s(strInfo, "当前绘制的顶点数: %d", g_pTerrain->GetRenderNodes());
    g_pTextStats->DrawTextA(NULL, strInfo, -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));
    
    // 绘制LOD地形
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    g_pTerrain->RenderLodTerrain(&matWorld);

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_DELETE(g_pCamera);
    SAFE_DELETE(g_pTerrain);
    SAFE_RELEASE(g_pTextStats);

    g_pMouseDevice->Unacquire();
    g_pKeyboardDevice->Unacquire();
    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pMouseDevice);
    SAFE_RELEASE(g_pKeyboardDevice);

    SAFE_RELEASE(g_pd3dDevice);
}
