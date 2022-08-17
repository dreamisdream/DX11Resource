//--------------------------------------------------------------------------------------
// File: LightMaterial.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库的头文件

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"LightMaterial";  // 窗口类名
wchar_t *g_pWindowName = L"光照与材质示例"; // 窗口标题名

LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;    // D3D设备接口
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf = NULL;    // 顶点缓存接口

struct CUSTOMVERTEX                 // 顶点结构
{
    FLOAT  _x,  _y,  _z;            // 顶点的位置
    FLOAT _nx, _ny, _nz;            // 法向量
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT nx, FLOAT ny, FLOAT nz)
        : _x(x), _y(y), _z(z), _nx(nx), _ny(ny), _nz(nz) {}
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL)


HRESULT InitDirect3D(HWND hWnd);    // 初始化Direct3D
VOID Direct3DRender();              // 渲染图形
VOID Direct3DCleanup();             // 清理Direct3D资源

//--------------------------------------------------------------------------------------
// Name: SetupLight()
// Desc: 设置光源类型
//--------------------------------------------------------------------------------------
VOID SetupLight(LPDIRECT3DDEVICE9 pd3dDevice, UINT nType)
{
    static D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));

    switch (nType)
    {
    case 1:     //点光源
        light.Type          = D3DLIGHT_POINT;
        light.Ambient       = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
        light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
        light.Specular      = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Position      = D3DXVECTOR3(-300.0f, 0.0f, 0.0f);
        light.Attenuation0  = 1.0f;
        light.Attenuation1  = 0.0f;
        light.Attenuation2  = 0.0f;
        light.Range         = 300.0f;
        break;
    case 2:     //平行光
        light.Type          = D3DLIGHT_DIRECTIONAL;
        light.Ambient       = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
        light.Specular      = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Direction     = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
        break;
    case 3:     //聚光灯
        light.Type          = D3DLIGHT_SPOT;
        light.Position      = D3DXVECTOR3(0.0f, 300.0f, 0.0f);
        light.Direction     = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
        light.Ambient       = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
        light.Specular      = D3DXCOLOR(0.3f, 0.3f, 0.3f, 0.3f);
        light.Attenuation0  = 1.0f; 
        light.Attenuation1  = 0.0f; 
        light.Attenuation2  = 0.0f; 
        light.Range         = 300.0f;
        light.Falloff       = 0.1f;
        light.Phi           = D3DX_PI / 3.0f;
        light.Theta         = D3DX_PI / 6.0f;
        break;
    }

    pd3dDevice->SetLight(0, &light);
    pd3dDevice->LightEnable(0, true);
    pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(92, 92, 92));
}

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

    // 创建顶点缓存
    g_pd3dDevice->CreateVertexBuffer(12 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL);

    // 填充顶点数据
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    // 前面
    pVertices[0] = CUSTOMVERTEX(-1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);
    pVertices[1] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.0f, 0.707f, -0.707f);
    pVertices[2] = CUSTOMVERTEX( 1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);

    // 左侧面
    pVertices[3] = CUSTOMVERTEX(-1.0f, 0.0f,  1.0f, -0.707f, 0.707f, 0.0f);
    pVertices[4] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, -0.707f, 0.707f, 0.0f);
    pVertices[5] = CUSTOMVERTEX(-1.0f, 0.0f, -1.0f, -0.707f, 0.707f, 0.0f);

    // 右侧面
    pVertices[6] = CUSTOMVERTEX( 1.0f, 0.0f, -1.0f, 0.707f, 0.707f, 0.0f);
    pVertices[7] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.707f, 0.707f, 0.0f);
    pVertices[8] = CUSTOMVERTEX( 1.0f, 0.0f,  1.0f, 0.707f, 0.707f, 0.0f);

    // 底面
    pVertices[9]  = CUSTOMVERTEX( 1.0f, 0.0f,  1.0f, 0.0f, 0.707f, 0.707f);
    pVertices[10] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.0f, 0.707f, 0.707f);
    pVertices[11] = CUSTOMVERTEX(-1.0f, 0.0f,  1.0f, 0.0f, 0.707f, 0.707f);
    g_pVertexBuf->Unlock();

    // 设置材质
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Specular = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
    mtrl.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
    g_pd3dDevice->SetMaterial(&mtrl);

    // 设置光照
    SetupLight(g_pd3dDevice, 1);
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, true);
    g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true);
    g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, true);

    // 设置取景变换矩阵
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 1.0, -5.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置投影变换矩阵
    D3DXMATRIX  matProj;
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
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
        D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 重新设置光照
    if (::GetAsyncKeyState(0x31) & 0x8000f)         // 按键1
        SetupLight(g_pd3dDevice, 1);
    if (::GetAsyncKeyState(0x32) & 0x8000f)         // 按键2
        SetupLight(g_pd3dDevice, 2);
    if (::GetAsyncKeyState(0x33) & 0x8000f)         // 按键3
        SetupLight(g_pd3dDevice, 3);

    // 设置世界变换矩阵
    D3DXMATRIX matWorld;
    D3DXMatrixRotationY(&matWorld, ::timeGetTime() / 500.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 绘制四面体
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 4);

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pd3dDevice);
}
