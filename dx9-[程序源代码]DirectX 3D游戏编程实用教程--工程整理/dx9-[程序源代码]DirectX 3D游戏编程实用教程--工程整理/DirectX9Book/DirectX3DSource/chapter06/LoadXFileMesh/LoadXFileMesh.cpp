//--------------------------------------------------------------------------------------
// File: LoadXFileMesh.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"LoadXFileMesh";  // 窗口类名
wchar_t *g_pWindowName = L"读取X文件示例";  // 窗口标题名

LPDIRECT3DDEVICE9   g_pd3dDevice    = NULL; // Direct3D设备接口
LPD3DXMESH          g_pMeshTiny     = NULL; // 网格对象
D3DMATERIAL9*       g_pMaterials    = NULL; // 网格的材质信息
LPDIRECT3DTEXTURE9* g_pTextures     = NULL; // 网格的纹理信息
DWORD               g_dwNumMtrls    = 0;    // 材质的数目

/*
//--------------------------------------------------------------------------------------
// Name: LoadMesh();
// Desc: 从X文件中加载网格模型
//--------------------------------------------------------------------------------------
HRESULT LoadMesh(LPDIRECT3DDEVICE9 pd3dDevice, wchar_t *pName, LPD3DXMESH *ppMesh,
    LPDWORD pNumMtrls, D3DMATERIAL9 **ppMaterials, LPDIRECT3DTEXTURE9 **ppTextures)
{
    // 加载网格模型
    LPD3DXBUFFER pAdjBuffer  = NULL;
    LPD3DXBUFFER pMtrlBuffer = NULL;
    HRESULT hr = D3DXLoadMeshFromX( pName, D3DXMESH_MANAGED, pd3dDevice, 
                                     &pAdjBuffer, &pMtrlBuffer, NULL, pNumMtrls, ppMesh );
    if (FAILED(hr)) return hr;

    // 读取材质和纹理数据
    D3DXMATERIAL *pMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
    (*ppMaterials) = new D3DMATERIAL9[*pNumMtrls];
    (*ppTextures)  = new LPDIRECT3DTEXTURE9[*pNumMtrls];

    for (DWORD i=0; i<*pNumMtrls; i++) 
    {
        (*ppMaterials)[i] = pMtrls[i].MatD3D;
        (*ppMaterials)[i].Ambient = (*ppMaterials)[i].Diffuse;
        *ppTextures[i]  = NULL;
        D3DXCreateTextureFromFileA(pd3dDevice, pMtrls[i].pTextureFilename, &(*ppTextures[i]));
    }
    pAdjBuffer->Release();
    pMtrlBuffer->Release();
    return S_OK;
}
*/

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

    // 从X文件中加载网格数据
    LPD3DXBUFFER pAdjBuffer  = NULL;
    LPD3DXBUFFER pMtrlBuffer = NULL;
    D3DXLoadMeshFromX(L"tiny.x", D3DXMESH_MANAGED, g_pd3dDevice, 
        &pAdjBuffer, &pMtrlBuffer, NULL, &g_dwNumMtrls, &g_pMeshTiny);

    // 读取材质和纹理数据
    D3DXMATERIAL *pMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
    g_pMaterials = new D3DMATERIAL9[g_dwNumMtrls];
    g_pTextures  = new LPDIRECT3DTEXTURE9[g_dwNumMtrls];

    for (DWORD i=0; i<g_dwNumMtrls; i++) 
    {
        g_pMaterials[i] = pMtrls[i].MatD3D;
        g_pMaterials[i].Ambient = g_pMaterials[i].Diffuse;
        g_pTextures[i]  = NULL;
        D3DXCreateTextureFromFileA(g_pd3dDevice, pMtrls[i].pTextureFilename, &g_pTextures[i]);
    }
    pAdjBuffer->Release();
    pMtrlBuffer->Release();

    // 设置纹理过滤方式
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    // 设置取景变换矩阵
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 0.0, -1000.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置投影变换矩阵
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 2000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // 设置光照
    D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type          = D3DLIGHT_DIRECTIONAL;
    light.Ambient       = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
    light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Specular      = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
    light.Direction     = D3DXVECTOR3(1.0f, 0.0f, 1.0f);
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
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 设置世界变换矩阵
    D3DXMATRIX matWorld, Rx, Ry;
    D3DXMatrixIdentity(&matWorld);
    D3DXMatrixRotationX(&Rx, -D3DX_PI / 2.0f);
    D3DXMatrixRotationY(&Ry,  D3DX_PI + ::timeGetTime() / 360.0f);
    matWorld = Rx * Ry * matWorld;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 设置填充方式
    if (::GetAsyncKeyState('F') & 0x8000f)
        g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    if (::GetAsyncKeyState('S') & 0x8000f) 
        g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    // 绘制网格
    for (DWORD i = 0; i < g_dwNumMtrls; i++)
    {
        g_pd3dDevice->SetMaterial(&g_pMaterials[i]);
        g_pd3dDevice->SetTexture(0, g_pTextures[i]);
        g_pMeshTiny->DrawSubset(i);
    }

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    for (DWORD i = 0; i<g_dwNumMtrls; i++) 
        SAFE_RELEASE(g_pTextures[i]);
    SAFE_DELETE(g_pTextures); 
    SAFE_DELETE(g_pMaterials); 
    SAFE_RELEASE(g_pMeshTiny);
    SAFE_RELEASE(g_pd3dDevice);
}
