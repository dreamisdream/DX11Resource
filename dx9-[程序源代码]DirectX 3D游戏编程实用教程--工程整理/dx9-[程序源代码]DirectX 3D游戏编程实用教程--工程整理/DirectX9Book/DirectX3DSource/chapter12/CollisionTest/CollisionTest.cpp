//--------------------------------------------------------------------------------------
// File: CollisionTest.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件
#include <dinput.h>                 // DirectInput头文件
#include <stdio.h>
#include "Camera.h"

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"CollisionTest";      // 窗口类名
wchar_t *g_pWindowName = L"碰撞检测示例";       // 窗口标题名

// 外接盒结构
struct BOUNDINGBOX
{
    D3DXVECTOR3 vMin;   // 外接盒的最小顶点
    D3DXVECTOR3 vMax;   // 外接盒的最大顶点
};

CCamera*                g_pCamera              = NULL;
LPD3DXFONT              g_pTextStats           = NULL;  // 状态信息的2D文本
LPD3DXFONT              g_pTextInfor           = NULL;  // 绘制信息的2D文本

LPD3DXMESH              g_pMeshSphere          = NULL;
LPD3DXMESH              g_pMeshTeapot          = NULL;
LPD3DXMESH              g_pMeshBBoxSphere      = NULL;
LPD3DXMESH              g_pMeshBBoxTeapot      = NULL;
BOUNDINGBOX             g_BBoxSphere;
BOUNDINGBOX             g_BBoxTeapot;
D3DMATERIAL9            g_MtrlSphere;
D3DMATERIAL9            g_MtrlTeapot;
D3DMATERIAL9            g_MtrlBBox;

LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL;  // Direct3D设备接口
LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
char                    g_pKeyStateBuffer[256] = {0};

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);       // 渲染图形
VOID Direct3DCleanup();             // 清理Direct3D资源

// 窗口消息处理函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//--------------------------------------------------------------------------------------
// Name: CollisionTestAABB
// Desc: AABB碰撞检测
//--------------------------------------------------------------------------------------
BOOL CollisionTestAABB(D3DXVECTOR3 vMin1, D3DXVECTOR3 vMax1,
                       D3DXVECTOR3 vMin2, D3DXVECTOR3 vMax2 )
{
    if (vMax1.x < vMin2.x || vMin1.x > vMax2.x) return FALSE;   // x方向
    if (vMax1.y < vMin2.y || vMin1.y > vMax2.y) return FALSE;   // y方向
    if (vMax1.z < vMin2.z || vMin1.z > vMax2.z) return FALSE;   // z方向
    return TRUE;    // 产生碰撞
}

//--------------------------------------------------------------------------------------
// Name: ReadDevice()
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
// Name: WinMain()
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
            Direct3DRender(hWnd, 0.0f); // 绘制3D场景
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
        Direct3DRender(hWnd, 0.0f); // 渲染图形
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

    // 创建茶壶和球体几何体
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, NULL);
    D3DXCreateSphere(g_pd3dDevice, 1.0f, 20, 20, &g_pMeshSphere, NULL);

    // 创建外接盒
    D3DXVECTOR3 *pFirstPosition = 0;
    g_pMeshSphere->LockVertexBuffer(0, (void**)&pFirstPosition);
    D3DXComputeBoundingBox(pFirstPosition, g_pMeshSphere->GetNumVertices(), D3DXGetFVFVertexSize(g_pMeshSphere->GetFVF()), &(g_BBoxSphere.vMin), &(g_BBoxSphere.vMax));
    g_pMeshSphere->UnlockVertexBuffer();

    g_pMeshTeapot->LockVertexBuffer(0, (void**)&pFirstPosition);
    D3DXComputeBoundingBox(pFirstPosition, g_pMeshTeapot->GetNumVertices(), D3DXGetFVFVertexSize(g_pMeshTeapot->GetFVF()), &(g_BBoxTeapot.vMin), &(g_BBoxTeapot.vMax));
    g_pMeshTeapot->UnlockVertexBuffer();

    // 创建外接盒网格
    FLOAT fWidth  = g_BBoxSphere.vMax.x - g_BBoxSphere.vMin.x;
    FLOAT fHeight = g_BBoxSphere.vMax.y - g_BBoxSphere.vMin.y;
    FLOAT fDepth  = g_BBoxSphere.vMax.z - g_BBoxSphere.vMin.z;
    D3DXCreateBox(g_pd3dDevice, fWidth, fHeight, fDepth, &g_pMeshBBoxSphere, NULL);

    fWidth  = g_BBoxTeapot.vMax.x - g_BBoxTeapot.vMin.x;
    fHeight = g_BBoxTeapot.vMax.y - g_BBoxTeapot.vMin.y;
    fDepth  = g_BBoxTeapot.vMax.z - g_BBoxTeapot.vMin.z;
    D3DXCreateBox(g_pd3dDevice, fWidth, fHeight, fDepth, &g_pMeshBBoxTeapot, NULL);

    // 创建2D字体
    D3DXCreateFont(g_pd3dDevice, 15, 0, 1000, 0, true, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Arial", &g_pTextStats); 
    D3DXCreateFont(g_pd3dDevice, 14, 0, 1000, 0, false, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Times New Roman", &g_pTextInfor); 

    // 创建键盘输入设备
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboardDevice, NULL);
    g_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
    g_pKeyboardDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    g_pKeyboardDevice->Acquire();

    // 创建材质
    g_MtrlSphere.Ambient  = D3DXCOLOR(1.0f, 0.2f, 0.0f, 1.0f);
    g_MtrlSphere.Diffuse  = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);

    g_MtrlTeapot.Ambient  = D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f);
    g_MtrlTeapot.Diffuse  = D3DXCOLOR(0.8f, 0.8f, 0.0f, 1.0f);

    g_MtrlBBox.Ambient  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 0.5f);
    g_MtrlBBox.Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 0.5f);

    // 创建并初始化虚拟摄像机
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(-0.2f, 0.0f, -12.0f));
    g_pCamera->ResetLookatPos(&D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    g_pCamera->ResetViewMatrix();
    g_pCamera->ResetProjMatrix();

    // 设置光照
    D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Ambient   = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    light.Diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Direction = D3DXVECTOR3(1.0f, 0.0f, 1.0f);
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, true);
    g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true);
    g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, true);
    
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: 绘制3D场景
//--------------------------------------------------------------------------------------
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta) 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 输出状态信息
    RECT rect;
    GetClientRect(hWnd, &rect);
    rect.top += 5;
    g_pTextStats->DrawText(NULL, L"AppName: CollisionTest", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
    rect.top += 20;
    g_pTextStats->DrawText(NULL, L"Screen backbuffer(640X480),X8R8G8B8(D24X8)", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));

    // 读取键盘输入
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // 绕摄像机各分量旋转视角
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->CircleRotationX( 0.001f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->CircleRotationX(-0.001f);
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->CircleRotationY(-0.001f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->CircleRotationY( 0.001f);

    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置世界变换矩阵
    static FLOAT fOffset   = 5.0f;
    static BOOL  bIncrease = true;
    if (fOffset <= -5.0f) bIncrease = true;
    if (fOffset >=  5.0f) bIncrease = false;
    fOffset = bIncrease ? fOffset+0.008f : fOffset-0.01f;

    D3DXMATRIX matSphere, matTeapot;
    D3DXMatrixTranslation(&matSphere, -fOffset, 0.0f, 0.0f);
    D3DXMatrixTranslation(&matTeapot,  fOffset, 0.0f, 0.0f);
   
    // 碰撞检测
    D3DXVECTOR3 vMin1 = D3DXVECTOR3(-fOffset, 0.0f, 0.0f) + g_BBoxSphere.vMin;
    D3DXVECTOR3 vMax1 = D3DXVECTOR3(-fOffset, 0.0f, 0.0f) + g_BBoxSphere.vMax;
    D3DXVECTOR3 vMin2 = D3DXVECTOR3( fOffset, 0.0f, 0.0f) + g_BBoxTeapot.vMin;
    D3DXVECTOR3 vMax2 = D3DXVECTOR3( fOffset, 0.0f, 0.0f) + g_BBoxTeapot.vMax;
    if (CollisionTestAABB(vMin1, vMax1, vMin2, vMax2)) bIncrease = true;

    // 绘制球体和茶壶
    g_pd3dDevice->SetMaterial(&g_MtrlSphere);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matSphere);
    g_pMeshSphere->DrawSubset(0);

    g_pd3dDevice->SetMaterial(&g_MtrlTeapot);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matTeapot);
    g_pMeshTeapot->DrawSubset(0);

    // 开启Alpha融合, 并绘制外接盒
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    g_pd3dDevice->SetMaterial(&g_MtrlBBox);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matSphere);
    g_pMeshBBoxSphere->DrawSubset(0);

    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matTeapot);
    g_pMeshBBoxTeapot->DrawSubset(0);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

    // 输出绘制信息
    rect.left = 460, rect.top = 80;
    static char strInfo[256] = {0};
    sprintf_s(strInfo, "球体坐标: (%.2f, %.2f, %.2f)", matSphere._41, matSphere._42, matSphere._43);
    g_pTextInfor->DrawTextA(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.6f, 0.6f, 1.0f, 1.0f));

    rect.top += 20;
    D3DXVECTOR3 vCameraPos;
    g_pCamera->GetCameraPos(&vCameraPos);
    sprintf_s(strInfo, "茶壶坐标: (%.2f, %.2f, %.2f)", matTeapot._41, matTeapot._42, matTeapot._43);
    g_pTextInfor->DrawTextA(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pMeshSphere);
    SAFE_RELEASE(g_pMeshTeapot);
    SAFE_RELEASE(g_pMeshBBoxSphere);
    SAFE_RELEASE(g_pMeshBBoxTeapot);

    SAFE_DELETE(g_pCamera);
    g_pKeyboardDevice->Unacquire();
    SAFE_RELEASE(g_pKeyboardDevice);
    SAFE_RELEASE(g_pd3dDevice);
}
