//--------------------------------------------------------------------------------------
// File: SimpleCamera.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件
#include <dinput.h>                 // DirectInput头文件
#include "Camera.h"

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"SimpleCamera";       // 窗口类名
wchar_t *g_pWindowName = L"虚拟摄像机示例";     // 窗口标题名

CCamera*                g_pCamera              = NULL;

LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL; // Direct3D设备接口
LPD3DXMESH              g_pMeshTeapot          = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf           = NULL;
LPDIRECT3DTEXTURE9      g_pTexture             = NULL;

LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
LPDIRECTINPUTDEVICE8    g_pMouseDevice         = NULL;
char                    g_pKeyStateBuffer[256] = {0};
DIMOUSESTATE            g_diMouseState         = {0};

struct CUSTOMVERTEX
{
    FLOAT _x,  _y,  _z;
    FLOAT _nx, _ny, _nz;
    FLOAT _u,  _v;
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, 
        FLOAT nx, FLOAT ny, FLOAT nz, FLOAT u, FLOAT v)
    {
        _x  = x,  _y  = y,  _z  = z;
        _nx = nx, _ny = ny, _nz = nz;
        _u  = u,  _v  = v;
    }
};
#define D3DFVF_CUSTOMVERTEX  (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender();              // 渲染图形
VOID Direct3DCleanup();             // 清理Direct3D资源

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

    // 创建茶壶
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, NULL);

    // 创建地板
    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pVertexBuf, 0);

    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-20.0f, -5.0f, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 5.0f);
    pVertices[1] = CUSTOMVERTEX(-20.0f, -5.0f,  20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 20.0f, -5.0f, -20.0f, 0.0f, 1.0f, 0.0f, 5.0f, 5.0f);
    pVertices[3] = CUSTOMVERTEX( 20.0f, -5.0f,  20.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f);
    g_pVertexBuf->Unlock();

    // 创建地板纹理
    D3DXCreateTextureFromFile(g_pd3dDevice, L"floor.jpg", &g_pTexture);

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

    // 设置材质
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient    = D3DXCOLOR(0.4f, 0.4f, 0.4f, 1.0f);
    mtrl.Diffuse    = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
    g_pd3dDevice->SetMaterial(&mtrl);

    // 设置光照
    D3DLIGHT9   light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Ambient   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Direction = D3DXVECTOR3(0.707f, -0.707f, 0.707f);
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, true);
    
    // 创建并初始化虚拟摄像机
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(0.0f, 0.0f, -10.0f));
    g_pCamera->ResetLookatPos(&D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    g_pCamera->ResetViewMatrix();
    g_pCamera->ResetProjMatrix();
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: 绘制3D场景
//--------------------------------------------------------------------------------------
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 读取键盘输入
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // 读取鼠标输入
    ::ZeroMemory(&g_diMouseState, sizeof(g_diMouseState));
    ReadDevice(g_pMouseDevice, (LPVOID)&g_diMouseState, sizeof(g_diMouseState));

    // 沿摄像机各分量移动视角
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->MoveAlongRightVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->MoveAlongRightVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->MoveAlongLookVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->MoveAlongLookVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_R] & 0x80) g_pCamera->MoveAlongUpVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_F] & 0x80) g_pCamera->MoveAlongUpVec(-0.01f);

    // 绕摄像机各分量旋转视角
    if (g_pKeyStateBuffer[DIK_UP]    & 0x80) g_pCamera->RotationRightVec(-0.001f);
    if (g_pKeyStateBuffer[DIK_DOWN]  & 0x80) g_pCamera->RotationRightVec( 0.001f);
    if (g_pKeyStateBuffer[DIK_LEFT]  & 0x80) g_pCamera->RotationUpVec(-0.001f);
    if (g_pKeyStateBuffer[DIK_RIGHT] & 0x80) g_pCamera->RotationUpVec( 0.001f);
    if (g_pKeyStateBuffer[DIK_N]     & 0x80) g_pCamera->RotationLookVec(-0.001f);
    if (g_pKeyStateBuffer[DIK_M]     & 0x80) g_pCamera->RotationLookVec( 0.001f);

    // 绕摄像机观察点旋转视角
    if (g_pKeyStateBuffer[DIK_I] & 0x80) g_pCamera->CircleRotationX(-0.001f);
    if (g_pKeyStateBuffer[DIK_K] & 0x80) g_pCamera->CircleRotationX( 0.001f);
    if (g_pKeyStateBuffer[DIK_J] & 0x80) g_pCamera->CircleRotationY(-0.001f);
    if (g_pKeyStateBuffer[DIK_L] & 0x80) g_pCamera->CircleRotationY( 0.001f);
    if (g_pKeyStateBuffer[DIK_O] & 0x80) g_pCamera->CircleRotationZ(-0.001f);
    if (g_pKeyStateBuffer[DIK_P] & 0x80) g_pCamera->CircleRotationZ( 0.001f);

    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
    
    // 鼠标控制物体的平移
    static float fPosX = 0.0f, fPosY = 0.0f, fPosZ = 0.0f;
    if (g_diMouseState.rgbButtons[0] & 0x80) 
    {
        fPosX += g_diMouseState.lX *  0.008f;
        fPosY += g_diMouseState.lY * -0.008f;
    }
    fPosZ += g_diMouseState.lZ * 0.005f;

    // 绘制茶壶
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, fPosX, fPosY, fPosZ);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetTexture(0, NULL);
    g_pMeshTeapot->DrawSubset(0);

    // 设置纹理过滤和纹理寻址方式
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

    // 绘制地板
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetTexture(0, g_pTexture);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

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

    g_pMouseDevice->Unacquire();
    g_pKeyboardDevice->Unacquire();

    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pMouseDevice);
    SAFE_RELEASE(g_pKeyboardDevice);

    SAFE_RELEASE(g_pMeshTeapot);
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pTexture);
    SAFE_RELEASE(g_pd3dDevice);
}
