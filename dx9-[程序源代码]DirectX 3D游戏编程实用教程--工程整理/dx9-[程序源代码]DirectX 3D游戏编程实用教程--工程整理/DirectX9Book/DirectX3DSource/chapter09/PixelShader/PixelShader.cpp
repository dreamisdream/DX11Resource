//--------------------------------------------------------------------------------------
// File: PixelShader.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"PixelShader";                // 窗口类名
wchar_t *g_pWindowName = L"像素着色器示例";             // 窗口标题名

LPDIRECT3DDEVICE9            g_pd3dDevice     = NULL;   // D3D设备接口
LPDIRECT3DVERTEXBUFFER9      g_pVertexBuf     = NULL;   // 顶点缓存
LPDIRECT3DTEXTURE9           g_pTexture0      = NULL;   // 第1层纹理
LPDIRECT3DTEXTURE9           g_pTexture1      = NULL;   // 第2层纹理
LPDIRECT3DTEXTURE9           g_pTexture2      = NULL;   // 第3层纹理
LPDIRECT3DPIXELSHADER9       g_pPixelShader   = NULL;   // 着色器接口
LPD3DXCONSTANTTABLE          g_pConstantTable = NULL;   // 常量表接口

// 顶点结构
struct CUSTOMVERTEX 
{
    FLOAT _x, _y, _z;           // 顶点的位置
    FLOAT _u0, _v0;             // 第1层纹理坐标
    FLOAT _u1, _v1;             // 第2层纹理坐标
    FLOAT _u2, _v2;             // 第2层纹理坐标
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, 
        FLOAT u0, FLOAT v0, 
        FLOAT u1, FLOAT v1,
        FLOAT u2, FLOAT v2) 
    {
        _x = x, _y = y, _z = z;
        _u0 = u0, _v0 = v0;
        _u1 = u1, _v1 = v1;
        _u2 = u2, _v2 = v2;
    }
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX3)


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
    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL); 

    // 填充顶点数据
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-5.0f, -5.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    pVertices[1] = CUSTOMVERTEX(-5.0f,  5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 5.0f, -5.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    pVertices[3] = CUSTOMVERTEX( 5.0f,  5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    g_pVertexBuf->Unlock();

    // 创建纹理
    D3DXCreateTextureFromFile(g_pd3dDevice, L"lion.jpg", &g_pTexture0);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"spotlight.bmp", &g_pTexture1);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"text.bmp", &g_pTexture2);

    // 编译着色器程序
    LPD3DXBUFFER pShader = NULL;
    LPD3DXBUFFER pErrors = NULL;
    D3DXCompileShaderFromFile(L"MultiTexture.txt", NULL, NULL, "ps_main", "ps_2_0", 
        D3DXSHADER_DEBUG, &pShader, &pErrors, &g_pConstantTable);
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // 创建像素着色器
    g_pd3dDevice->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &g_pPixelShader);
    g_pConstantTable->SetDefaults(g_pd3dDevice);
    pShader->Release();

    // 设置取景变换矩阵
    D3DXVECTOR3 vEye(0.0f, 0.0f, -20.0f);
    D3DXVECTOR3 vLookAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMATRIX  matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vLookAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置投影变换矩阵
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // 关闭光照
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
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
    static FLOAT fPosX=0.0f, fPosY=0.0f, fPosZ=0.0f;
    if (::GetAsyncKeyState(VK_LEFT)  & 0x8000f) fPosX += 0.01f;
    if (::GetAsyncKeyState(VK_RIGHT) & 0x8000f) fPosX -= 0.01f;
    if (::GetAsyncKeyState(VK_UP)    & 0x8000f) fPosY -= 0.01f;
    if (::GetAsyncKeyState(VK_DOWN)  & 0x8000f) fPosY += 0.01f;
    if (::GetAsyncKeyState('A') & 0x8000f)      fPosX += 0.01f;
    if (::GetAsyncKeyState('D') & 0x8000f)      fPosX -= 0.01f;
    if (::GetAsyncKeyState('W') & 0x8000f)      fPosZ -= 0.01f;
    if (::GetAsyncKeyState('S') & 0x8000f)      fPosZ += 0.01f;

    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, fPosX, fPosY, fPosZ);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 取得采样器句柄
    static D3DXHANDLE hSampler0 = g_pConstantTable->GetConstantByName(0, "Sampler0");
    static D3DXHANDLE hSampler1 = g_pConstantTable->GetConstantByName(0, "Sampler1");
    static D3DXHANDLE hSampler2 = g_pConstantTable->GetConstantByName(0, "Sampler2");

    UINT Count;
    D3DXCONSTANT_DESC descSampler0, descSampler1, descSampler2;
    g_pConstantTable->GetConstantDesc(hSampler0, &descSampler0, &Count);
    g_pConstantTable->GetConstantDesc(hSampler1, &descSampler1, &Count);
    g_pConstantTable->GetConstantDesc(hSampler2, &descSampler2, &Count);

    // 设置纹理
    g_pd3dDevice->SetTexture(descSampler0.RegisterIndex, g_pTexture0);
    g_pd3dDevice->SetTexture(descSampler1.RegisterIndex, g_pTexture1);
    g_pd3dDevice->SetTexture(descSampler2.RegisterIndex, g_pTexture2);

    // 绘制多边形
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
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
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pTexture0);
    SAFE_RELEASE(g_pTexture1);
    SAFE_RELEASE(g_pTexture2);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pConstantTable);
    SAFE_RELEASE(g_pd3dDevice);
}
