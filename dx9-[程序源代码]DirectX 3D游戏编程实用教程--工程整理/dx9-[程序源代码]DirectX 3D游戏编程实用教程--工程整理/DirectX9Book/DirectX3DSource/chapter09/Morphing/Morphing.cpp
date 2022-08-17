//--------------------------------------------------------------------------------------
// File: Morphing.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3D头文件
#include <d3dx9.h>                  // D3DX库头文件

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"Morphing";               // 窗口类名
wchar_t *g_pWindowName = L"渐变动画示例";           // 窗口标题名

LPDIRECT3DDEVICE9            g_pd3dDevice     = NULL;   // D3D设备接口
LPD3DXMESH                   g_pSourceMesh    = NULL;   //源网格模型
LPD3DXMESH                   g_pTargetMesh    = NULL;   //目标网格模型

LPDIRECT3DVERTEXDECLARATION9 g_pVertexDecl    = NULL;   // 顶点声明
LPDIRECT3DVERTEXSHADER9      g_pVertexShader  = NULL;   // 着色器接口
LPD3DXCONSTANTTABLE          g_pConstantTable = NULL;   // 常量表接口

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

    // 加载源和目标网格模型
    D3DXLoadMeshFromX(L"Source.x", D3DXMESH_MANAGED, g_pd3dDevice, NULL, NULL, NULL, NULL, &g_pSourceMesh);
    D3DXLoadMeshFromX(L"Target.x", D3DXMESH_MANAGED, g_pd3dDevice, NULL, NULL, NULL, NULL, &g_pTargetMesh);

    // 创建顶点声明
    D3DVERTEXELEMENT9  decl[] = 
    {
        // 第一组数据流是第一个网格模型
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0}, 
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   0}, 

        // 第二组数据流是第二个网格模型
        { 1,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 1}, 
        { 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   1}, 
        D3DDECL_END()
    };
    g_pd3dDevice->CreateVertexDeclaration(decl, &g_pVertexDecl);

    // 编译着色器程序
    ID3DXBuffer *pShader = NULL;
    ID3DXBuffer *pErrors = NULL;
    D3DXCompileShaderFromFile(L"Morphing.txt", 0, 0, "vs_main", 
        "vs_2_0", D3DXSHADER_DEBUG, &pShader, &pErrors, &g_pConstantTable); 
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // 创建顶点着色器
    g_pd3dDevice->CreateVertexShader((DWORD*)pShader->GetBufferPointer(), &g_pVertexShader);
    g_pConstantTable->SetDefaults(g_pd3dDevice);
    SAFE_RELEASE(pShader);

    // 设置材质颜色参数
    D3DXVECTOR4 vMtrlsDiffuse(0.4f, 0.4f, 0.4f, 1.0f);
    D3DXVECTOR4 vMtrlsAmbient(0.6f, 0.6f, 0.6f, 1.0f);
    g_pConstantTable->SetVector(g_pd3dDevice, "MtrlsAmbient", &vMtrlsDiffuse);
    g_pConstantTable->SetVector(g_pd3dDevice, "MtrlsDiffuse", &vMtrlsAmbient);

    // 设置光照方向及颜色
    D3DXVECTOR4 vLightDir(1.0f, 0.0f, 1.0f, 0.0f); 
    D3DXVECTOR4 LightDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    g_pConstantTable->SetVector(g_pd3dDevice, "vecLightDir",  &vLightDir);
    g_pConstantTable->SetVector(g_pd3dDevice, "LightDiffuse", &LightDiffuse);

    // 设置取景变换矩阵
    D3DXVECTOR3 vEyePt(0.0f, 0.0f, -10.0f);
    D3DXVECTOR3 vLookAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 设置投影变换矩阵
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 1000.0f);
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

    // 构造世界变换矩阵, 以沿圆周运动
    float fFreq  = timeGetTime() / 500.0f;
    float fPhase = timeGetTime() / 3000.0f;

    D3DXMATRIX matWorld, Ry, Rz, Rt;
    D3DXMatrixScaling(&matWorld, 0.01f, 0.01f, 0.01f);
    D3DXMatrixRotationY(&Ry, fPhase);
    D3DXMatrixRotationZ(&Rz, cosf(fFreq) / -6.0f);
    D3DXMatrixTranslation(&Rt, -5.0f * sinf(fPhase), sinf(fFreq) / 2.0f, 10.0f * (1.0f - cosf(fPhase)));

    matWorld = matWorld * Ry * Rz * Rt;
    g_pConstantTable->SetMatrix(g_pd3dDevice, "matWorld", &matWorld );

    D3DXMATRIX matView, matProj, matWorldViewProj;
    g_pd3dDevice->GetTransform(D3DTS_VIEW, &matView);
    g_pd3dDevice->GetTransform(D3DTS_PROJECTION, &matProj);
    matWorldViewProj = matWorld * matView * matProj;
    g_pConstantTable->SetMatrix(g_pd3dDevice, "matWorldViewProj", &matWorldViewProj );

    // 设置时间比例因子, 以生成渐变网格
    float fTimeFactor = (float)(timeGetTime() % 2000) / 1000.0f;
    float fTimeScalar = (fTimeFactor <= 1.0f) ? fTimeFactor :(2.0f - fTimeFactor);
    g_pConstantTable->SetFloat(g_pd3dDevice, "fTimeScalar", fTimeScalar);

    // 设置顶点着色器和顶点声明
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);

    // 设置源网格模型的数据流
    LPDIRECT3DVERTEXBUFFER9 pVertexBuf = NULL;
    g_pSourceMesh->GetVertexBuffer(&pVertexBuf);
    g_pd3dDevice->SetStreamSource(0, pVertexBuf, 0, D3DXGetFVFVertexSize(g_pSourceMesh->GetFVF()));
    SAFE_RELEASE(pVertexBuf);

    // 设置目标网格模型的数据流
    g_pTargetMesh->GetVertexBuffer(&pVertexBuf);
    g_pd3dDevice->SetStreamSource(1, pVertexBuf, 0, D3DXGetFVFVertexSize(g_pTargetMesh->GetFVF()));
    SAFE_RELEASE(pVertexBuf);

    // 设置索引缓存
    LPDIRECT3DINDEXBUFFER9 pIndexBuf = NULL;
    g_pSourceMesh->GetIndexBuffer(&pIndexBuf);
    g_pd3dDevice->SetIndices(pIndexBuf);
    SAFE_RELEASE(pIndexBuf);

    // 绘制渐变动画
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
        g_pSourceMesh->GetNumVertices(), 0, g_pSourceMesh->GetNumFaces());

    g_pd3dDevice->EndScene();                       // 结束绘制
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // 翻转
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: 清理Direct3D, 并释放COM接口
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pSourceMesh);
    SAFE_RELEASE(g_pTargetMesh);
    SAFE_RELEASE(g_pVertexDecl);
    SAFE_RELEASE(g_pVertexShader);
    SAFE_RELEASE(g_pConstantTable);
    SAFE_RELEASE(g_pd3dDevice);
}
