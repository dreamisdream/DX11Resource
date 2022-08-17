//--------------------------------------------------------------------------------------
// File: JoystickInput.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#include <dinput.h>                 // DirectInput头文件
#pragma comment(lib, "dinput8.lib")

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"JoystickInput";          // 窗口类名
wchar_t *g_pWindowName = L"游戏杆控制示例";         // 窗口标题名

LPDIRECT3DDEVICE9       g_pd3dDevice        = NULL; // Direct3D设备接口
LPD3DXMESH              g_pMeshTeapot       = NULL;
LPDIRECTINPUT8          g_pDirectInput      = NULL;
LPDIRECTINPUTDEVICE8    g_pJoystickDevice   = NULL;
DIJOYSTATE              g_diJoystickState   = {0};

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender();              // 渲染图形
VOID Direct3DCleanup();             // 清理Direct3D资源

// 窗口消息处理函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: EnumJoysticks();
// Desc: 枚举游戏杆输入设备
//--------------------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticks(LPCDIDEVICEINSTANCE pDIDInstance, LPVOID pContext)
{
    if (FAILED(g_pDirectInput->CreateDevice(pDIDInstance->guidInstance, &g_pJoystickDevice, NULL)))
        return DIENUM_CONTINUE;
    return DIENUM_STOP;
}

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
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, 0);

    // 创建游戏杆输入设备
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->EnumDevices(DI8DEVTYPE_JOYSTICK, EnumJoysticks, NULL, DIEDFL_ATTACHEDONLY);
    if (g_pJoystickDevice == NULL) 
    {
        ::MessageBox(NULL, L"Enum joystick device failed!", 0, 0);
        return E_FAIL;
    }

    // 设置数据格式及协作基本
    g_pJoystickDevice->SetDataFormat(&c_dfDIJoystick);
    g_pJoystickDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

    // 设置游戏杆范围
    DIPROPRANGE prop_range;
    ZeroMemory(&prop_range, sizeof(DIPROPRANGE));
    prop_range.diph.dwSize       = sizeof(DIPROPRANGE);
    prop_range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    prop_range.diph.dwHow        = 0;
    prop_range.diph.dwObj        = DIPH_DEVICE;
    prop_range.lMin              = -120;
    prop_range.lMax              =  120;

    if (FAILED(g_pJoystickDevice->SetProperty(DIPROP_RANGE, &prop_range.diph)))
    {
        ::MessageBox(NULL, L"Set joystick device property failed", 0, 0);
        return E_FAIL;
    }

    // 设置死区范围为5%
    DIPROPDWORD prop_dword;
    ZeroMemory(&prop_dword, sizeof(DIPROPDWORD));
    prop_dword.diph.dwSize       = sizeof(DIPROPDWORD);
    prop_dword.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    prop_dword.diph.dwHow        = 0;
    prop_dword.diph.dwObj        = DIPH_DEVICE;
    prop_dword.dwData            = 500;

    if(FAILED(g_pJoystickDevice->SetProperty(DIPROP_DEADZONE, &prop_dword.diph)))
    {
        ::MessageBox(NULL, L"Set joystick device dead area failed", 0, 0);
        return E_FAIL;
    }

    // 获取游戏杆输入设备的控制权
    g_pJoystickDevice->Acquire();

    // 设置材质
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient    = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    mtrl.Diffuse    = D3DXCOLOR(0.7f, 0.7f, 0.7f, 1.0f);
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
    
    // 设置取景变换矩阵
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0, 0.0, -10.0f );
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置投影变换矩阵
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
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

    // 读取游戏杆输入
    ::ZeroMemory(&g_diJoystickState, sizeof(g_diJoystickState));
    ReadDevice(g_pJoystickDevice, (LPVOID)&g_diJoystickState, sizeof(g_diJoystickState));

    // 游戏杆控制平移
    static FLOAT fPosX = 0.0f, fPosY = 0.0f, fPosZ = 0.0f;
    if (g_diJoystickState.rgbButtons[0] & 0x80) 
    {
        fPosX += g_diJoystickState.lX *  0.0001f;
        fPosY += g_diJoystickState.lY * -0.0001f;
        fPosZ += g_diJoystickState.lZ *  0.0001f;
    }

    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, fPosX, fPosY, fPosZ);

    // 游戏杆控制旋转
    static float fAngleX = 0.0f, fAngleY = 0.0f, fAngleZ = 0.0f;
    if (g_diJoystickState.rgbButtons[1] & 0x80) 
    {
        fAngleX += g_diJoystickState.lY * -0.0001f;
        fAngleY += g_diJoystickState.lX * -0.0001f;
        fAngleZ += g_diJoystickState.lZ * -0.0001f;
    }
    D3DXMATRIX Rx, Ry, Rz;
    D3DXMatrixRotationX(&Rx, fAngleX);
    D3DXMatrixRotationY(&Ry, fAngleY);
    D3DXMatrixRotationZ(&Rz, fAngleZ);

    // 游戏杆滑块控制缩放
    float fScaleFactor = g_diJoystickState.rglSlider[0] * 0.04f;
    if (fScaleFactor <= 0.0f) fScaleFactor = 0.01f;

    D3DXMATRIX Rs;
    D3DXMatrixScaling(&Rs, fScaleFactor, fScaleFactor, fScaleFactor);
    
    matWorld = Rx * Ry * Rz * Rs * matWorld;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 绘制茶壶
    g_pMeshTeapot->DrawSubset(0);

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    g_pJoystickDevice->Unacquire();
    SAFE_RELEASE(g_pJoystickDevice);
    SAFE_RELEASE(g_pMeshTeapot);
    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pd3dDevice);
}
