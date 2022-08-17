//--------------------------------------------------------------------------------------
// File: Billboard.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"Billboard";      // 窗口类名
wchar_t *g_pWindowName = L"广告牌示例";     // 窗口标题名

struct  CUSTOMVERTEX
{
    FLOAT _x, _y, _z;   // 顶点位置
    FLOAT _u, _v ;      // 顶点纹理坐标
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v)
        : _x(x), _y(y), _z(z), _u(u), _v(v) {}
};
#define D3DFVF_CUSTOMVERTEX  (D3DFVF_XYZ | D3DFVF_TEX1)

LPDIRECT3DVERTEXBUFFER9     g_pGroundVB  = NULL;    // 地面顶点缓冲区
LPDIRECT3DTEXTURE9          g_pGroundTex = NULL;    // 地面纹理
LPDIRECT3DVERTEXBUFFER9     g_pTreeVB    = NULL;    // 树木顶点缓冲区
LPDIRECT3DTEXTURE9          g_pTreeTex   = NULL;    // 树木纹理
LPDIRECT3DDEVICE9           g_pd3dDevice = NULL;    // D3D设备接口

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

    // 创建地面顶点缓存
    g_pd3dDevice->CreateVertexBuffer( 4 * sizeof(CUSTOMVERTEX), 0,  
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pGroundVB, NULL );

    CUSTOMVERTEX *pVertices = NULL;
    g_pGroundVB->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-32.0f, 0.0f, -32.0f,  0.0f, 16.0f);
    pVertices[1] = CUSTOMVERTEX(-32.0f, 0.0f,  32.0f,  0.0f,  0.0f);
    pVertices[2] = CUSTOMVERTEX( 32.0f, 0.0f, -32.0f, 16.0f, 16.0f); 
    pVertices[3] = CUSTOMVERTEX( 32.0f, 0.0f,  32.0f, 16.0f,  0.0f);
    g_pGroundVB->Unlock();

    // 创建树木顶点缓存
    g_pd3dDevice->CreateVertexBuffer( 4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pTreeVB, NULL );
    
    pVertices = NULL;
    g_pTreeVB->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    pVertices[1] = CUSTOMVERTEX(-1.0f, 3.0f, 0.0f, 0.0f, 0.0f); 
    pVertices[2] = CUSTOMVERTEX( 1.0f, 0.0f, 0.0f, 1.0f, 1.0f); 
    pVertices[3] = CUSTOMVERTEX( 1.0f, 3.0f, 0.0f, 1.0f, 0.0f);
    g_pTreeVB->Unlock();

    // 创建地面纹理和树木纹理
    D3DXCreateTextureFromFile( g_pd3dDevice, L"ground.jpg", &g_pGroundTex );
    D3DXCreateTextureFromFile( g_pd3dDevice, L"tree.dds", &g_pTreeTex );

    // 设置投影变换矩阵
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0, 1.0f, 100.0f );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // 关闭光照
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, false );
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

    //设置观察矩阵
    D3DXMATRIXA16 matView;
    FLOAT fAngle = timeGetTime() / 2000.0f;

    D3DXVECTOR3 vEye( 10.0f*sin(fAngle), 5.0f, 10.0f*cos(fAngle));
    D3DXVECTOR3 vAt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUp( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matView, &vEye, &vAt, &vUp );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // 设置纹理状态
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    // Alpha混合设置, 设置混合系数
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   true );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    
    // 渲染地面
    D3DXMATRIX matGround;
    D3DXMatrixIdentity(&matGround);
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matGround );
    g_pd3dDevice->SetTexture( 0, g_pGroundTex );
    g_pd3dDevice->SetStreamSource( 0, g_pGroundVB, 0, sizeof(CUSTOMVERTEX) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    // 根据取景变换矩阵来构造广告牌矩阵
    D3DXMATRIX matBillboard;
    D3DXMatrixIdentity(&matBillboard);
    matBillboard._11 = matView._11;
    matBillboard._13 = matView._13;
    matBillboard._31 = matView._31;
    matBillboard._33 = matView._33;
    D3DXMatrixInverse(&matBillboard, NULL, &matBillboard);
/*
    // 根据当前观察方向来构造广告牌矩阵
    D3DXMATRIX matBillboard;
    D3DXVECTOR3 vDir = vAt - vEye;
    if( vDir.x > 0.0f )
        D3DXMatrixRotationY( &matBillboard, -atanf(vDir.z/vDir.x)+D3DX_PI/2 );
    else
        D3DXMatrixRotationY( &matBillboard, -atanf(vDir.z/vDir.x)-D3DX_PI/2 );
*/

    D3DXMATRIX matTree;
    D3DXMatrixIdentity(&matTree);
    matTree = matBillboard * matTree;
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matTree );

    // 渲染树木
    g_pd3dDevice->SetTexture( 0, g_pTreeTex );
    g_pd3dDevice->SetStreamSource( 0, g_pTreeVB, 0, sizeof(CUSTOMVERTEX) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pTreeTex);
    SAFE_RELEASE(g_pTreeVB);
    SAFE_RELEASE(g_pGroundTex);
    SAFE_RELEASE(g_pGroundVB);
    SAFE_RELEASE(g_pd3dDevice);
}
