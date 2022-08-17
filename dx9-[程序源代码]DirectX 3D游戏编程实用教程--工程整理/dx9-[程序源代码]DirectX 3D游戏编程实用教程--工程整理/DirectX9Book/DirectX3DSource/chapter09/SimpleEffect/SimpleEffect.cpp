//--------------------------------------------------------------------------------------
// File: SimpleEffect.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"SimpleEffect";   // 窗口类名
wchar_t *g_pWindowName = L"效果框架示例";   // 窗口标题名

LPDIRECT3DDEVICE9       g_pd3dDevice    = NULL;     // D3D设备接口
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf    = NULL;     // 顶点缓存接口
LPDIRECT3DINDEXBUFFER9  g_pIndexBuf     = NULL;     // 索引缓存接口
LPDIRECT3DTEXTURE9      g_pTexture      = NULL;     // 纹理接口
LPD3DXEFFECT            g_pEffect       = NULL;     // 效果接口

// 顶点结构
struct CUSTOMVERTEX 
{
    FLOAT _x, _y, _z;               // 顶点的位置
    FLOAT _nx, _ny, _nz;            // 顶点法向量
    FLOAT _u, _v;                   // 纹理坐标
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, 
                 FLOAT nx, FLOAT ny, FLOAT nz, 
                 FLOAT u, FLOAT v) 
    {
        _x = x, _nx = nx;
        _y = y, _ny = ny;
        _z = z, _nz = nz;
        _u = u, _v  = v;
    }
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)


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

    // 创建顶点缓存
    g_pd3dDevice->CreateVertexBuffer(24 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL); 

    // 填充顶点数据
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    // 正面顶点数据
    pVertices[0] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    pVertices[1] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    pVertices[3] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);

    // 背面顶点数据
    pVertices[4] = CUSTOMVERTEX( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pVertices[5] = CUSTOMVERTEX(-1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    pVertices[6] = CUSTOMVERTEX(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    pVertices[7] = CUSTOMVERTEX( 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    // 顶面顶点数据
    pVertices[8]  = CUSTOMVERTEX(-1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    pVertices[9]  = CUSTOMVERTEX( 1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    pVertices[10] = CUSTOMVERTEX( 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    pVertices[11] = CUSTOMVERTEX(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

    // 底面顶点数据
    pVertices[12] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
    pVertices[13] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
    pVertices[14] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
    pVertices[15] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);

    // 左侧面顶点数据
    pVertices[16] = CUSTOMVERTEX(-1.0f,  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[17] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    pVertices[18] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    pVertices[19] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // 右侧面顶点数据
    pVertices[20] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[21] = CUSTOMVERTEX( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    pVertices[22] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    pVertices[23] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    g_pVertexBuf->Unlock();

    // 创建索引缓存
    g_pd3dDevice->CreateIndexBuffer(36 * sizeof(WORD), 0, 
        D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIndexBuf, NULL);

    // 填充索引数据
    WORD *pIndices = NULL;
    g_pIndexBuf->Lock(0, 0, (void**)&pIndices, 0);
    
    // 正面索引数据
    pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
    pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

    // 背面索引数据
    pIndices[6] = 4; pIndices[7]  = 5; pIndices[8]  = 6;
    pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

    // 顶面索引数据
    pIndices[12] = 8; pIndices[13] =  9; pIndices[14] = 10;
    pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

    // 底面索引数据
    pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
    pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

    // 左侧面索引数据
    pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
    pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

    // 右侧面索引数据
    pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
    pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
    g_pIndexBuf->Unlock();

    // 创建效果对象
    ID3DXBuffer *pErrors = NULL;
    D3DXCreateEffectFromFile(g_pd3dDevice, L"SimpleEffect.fx", NULL, NULL, 
        D3DXSHADER_DEBUG, NULL, &g_pEffect, &pErrors); 

    // 输出错误信息
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // 创建并设置纹理
    D3DXCreateTextureFromFile(g_pd3dDevice, L"crate.jpg", &g_pTexture);
    g_pEffect->SetTexture("Texture0", g_pTexture);

    // 设置材质颜色
    D3DXCOLOR MtrlsDiffuse(1.0f, 1.0f, 1.0f, 1.0f);     // 材质漫反射颜色
    D3DXCOLOR MtrlsAmbient(1.0f, 1.0f, 1.0f, 0.0f);     // 材质环境光颜色
    g_pEffect->SetValue("MtrlsDiffuse", &MtrlsDiffuse, sizeof(D3DXCOLOR));
    g_pEffect->SetValue("MtrlsAmbient", &MtrlsAmbient, sizeof(D3DXCOLOR));

    // 设置光照
    D3DXVECTOR3 vLightDir[3] = 
    {   // 光照方向
        D3DXVECTOR3(-1.0f, 0.0f, -1.0f),
        D3DXVECTOR3( 1.0f, 0.0f, -1.0f),
        D3DXVECTOR3( 0.0f, 1.0f,  0.0f),
    };
    D3DXCOLOR LightDiffuse[3] = 
    {   // 光照漫反射颜色
        D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f),              // 红
        D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f),              // 绿
        D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f),              // 黄
    };
    D3DXCOLOR LightAmbient[3] = 
    {   // 光照环境光颜色
        D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f),              // 红
        D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f),              // 绿
        D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f),              // 黄
    };
    g_pEffect->SetValue("LightDir",     vLightDir,    3 * sizeof(D3DXVECTOR3));
    g_pEffect->SetValue("LightDiffuse", LightDiffuse, 3 * sizeof(D3DXCOLOR));
    g_pEffect->SetValue("LightAmbient", LightAmbient, 3 * sizeof(D3DXCOLOR));

    // 设置取景变换矩阵
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 3.0f, -10.0f);
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
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(45, 50, 170), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // 开始绘制

    // 设置世界变换矩阵
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);

    //D3DXMATRIX Rx, Ry, Rz;
    //D3DXMatrixRotationX(&Rx, timeGetTime() / 1000.0f);
    //D3DXMatrixRotationY(&Ry, timeGetTime() / 1000.0f);
    //D3DXMatrixRotationZ(&Rz, timeGetTime() / 1000.0f);
    //matWorld = Rx * Ry * Rz * matWorld;

    D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);        // 绕Y轴旋转
    g_pEffect->SetMatrix("matWorld", &matWorld);

    // 设置组合变换矩阵
    D3DXMATRIX matView, matProj;
    g_pd3dDevice->GetTransform(D3DTS_VIEW, &matView);               // 取景变换矩阵
    g_pd3dDevice->GetTransform(D3DTS_PROJECTION, &matProj);         // 投影变换矩阵
    D3DXMATRIX matWorldViewProj = matWorld * matView * matProj;     // 组合变换矩阵
    g_pEffect->SetMatrix("matWorldViewProj", &matWorldViewProj);

    // 设置手法
    if (::GetAsyncKeyState(0x31) & 0x8000f)     // 数字键1, 使用1个光源进行渲染
        g_pEffect->SetTechnique("RenderSceneWith1Light");
    if (::GetAsyncKeyState(0x32) & 0x8000f)     // 数字键2, 使用2个光源进行渲染
        g_pEffect->SetTechnique("RenderSceneWith2Light");
    if (::GetAsyncKeyState(0x33) & 0x8000f)     // 数字键3, 使用3个光源进行渲染
        g_pEffect->SetTechnique("RenderSceneWith3Light");

    // 绘制立方体
    g_pd3dDevice->SetIndices(g_pIndexBuf);
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));

    UINT iNumPasses = 0;
    g_pEffect->Begin(&iNumPasses, 0);           // 开始当前活动的手法
    for (UINT i = 0; i<iNumPasses; i++)         // 使用每条通道进行绘制
    {
        g_pEffect->BeginPass( i );              // 使用当前活动的通道
        g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
        g_pEffect->EndPass();                   // 终止当前使用的通道
    }
    g_pEffect->End();                           // 结束当前使用的手法

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pEffect);
    SAFE_RELEASE(g_pTexture);
    SAFE_RELEASE(g_pIndexBuf);
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pd3dDevice);
}
