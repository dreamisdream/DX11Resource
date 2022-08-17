//--------------------------------------------------------------------------------------
// File: Morphing.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"Morphing";               // ��������
wchar_t *g_pWindowName = L"���䶯��ʾ��";           // ���ڱ�����

LPDIRECT3DDEVICE9            g_pd3dDevice     = NULL;   // D3D�豸�ӿ�
LPD3DXMESH                   g_pSourceMesh    = NULL;   //Դ����ģ��
LPD3DXMESH                   g_pTargetMesh    = NULL;   //Ŀ������ģ��

LPDIRECT3DVERTEXDECLARATION9 g_pVertexDecl    = NULL;   // ��������
LPDIRECT3DVERTEXSHADER9      g_pVertexShader  = NULL;   // ��ɫ���ӿ�
LPD3DXCONSTANTTABLE          g_pConstantTable = NULL;   // ������ӿ�

HRESULT InitDirect3D(HWND hWnd);    // ��ʼ��Direct3D
VOID Direct3DRender();              // ��Ⱦͼ��
VOID Direct3DCleanup();             // ����Direct3D��Դ

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: WindowsӦ�ó�����ں���
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // ��ʼ��������
    WNDCLASS wndclass;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);  // ���ڱ���
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);          // �����״
    wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);      // ����Сͼ��
    wndclass.hInstance      = hInstance;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.lpszClassName  = g_pClassName;
    wndclass.lpszMenuName   = NULL;
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;

    // ע�ᴰ����
    if (!RegisterClass(&wndclass))
        return 0;

    // ��������
    HWND hWnd = CreateWindow(g_pClassName, g_pWindowName, WS_OVERLAPPEDWINDOW, 
        100, 100, 640, 480, NULL, NULL, wndclass.hInstance, NULL);

    // ��ʼ��Direct3D
    InitDirect3D(hWnd);

    // ��ʾ�����´���
    ShowWindow(hWnd, SW_SHOWDEFAULT); 
    UpdateWindow(hWnd); 

    // ��Ϣѭ��
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
            Direct3DRender();       // ����3D����
        }
    }

    UnregisterClass(g_pClassName, wndclass.hInstance);
    return 0;
}

//--------------------------------------------------------------------------------------
// Name: WndProc()
// Desc: ������Ϣ������
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message) 
    {
    case WM_PAINT:                  // �ͻ����ػ���Ϣ
        Direct3DRender();           // ��Ⱦͼ��
        ValidateRect(hWnd, NULL);   // ���¿ͻ�������ʾ
        break;
    case WM_KEYDOWN:                // ���̰�����Ϣ
        if (wParam == VK_ESCAPE)    // ESC��
            DestroyWindow(hWnd);    // ���ٴ���, ������һ��WM_DESTROY��Ϣ
        break;
    case WM_DESTROY:                // ����������Ϣ
        Direct3DCleanup();          // ����Direct3D
        PostQuitMessage(0);         // �˳�����
        break;
    }
    // Ĭ�ϵ���Ϣ����
    return DefWindowProc( hWnd, message, wParam, lParam );
}

//--------------------------------------------------------------------------------------
// Name: InitDirect3D()
// Desc: ��ʼ��Direct3D
//--------------------------------------------------------------------------------------
HRESULT InitDirect3D(HWND hWnd) 
{
    // ����IDirect3D�ӿ�
    LPDIRECT3D9 pD3D = NULL;                    // IDirect3D9�ӿ�
    pD3D = Direct3DCreate9(D3D_SDK_VERSION);    // ����IDirect3D9�ӿڶ���
    if (pD3D == NULL) return E_FAIL;

    // ��ȡӲ���豸��Ϣ
    D3DCAPS9 caps; int vp = 0;
    pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps );
    if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    // ����Direct3D�豸�ӿ�
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

    // ����Դ��Ŀ������ģ��
    D3DXLoadMeshFromX(L"Source.x", D3DXMESH_MANAGED, g_pd3dDevice, NULL, NULL, NULL, NULL, &g_pSourceMesh);
    D3DXLoadMeshFromX(L"Target.x", D3DXMESH_MANAGED, g_pd3dDevice, NULL, NULL, NULL, NULL, &g_pTargetMesh);

    // ������������
    D3DVERTEXELEMENT9  decl[] = 
    {
        // ��һ���������ǵ�һ������ģ��
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0}, 
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   0}, 

        // �ڶ����������ǵڶ�������ģ��
        { 1,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 1}, 
        { 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   1}, 
        D3DDECL_END()
    };
    g_pd3dDevice->CreateVertexDeclaration(decl, &g_pVertexDecl);

    // ������ɫ������
    ID3DXBuffer *pShader = NULL;
    ID3DXBuffer *pErrors = NULL;
    D3DXCompileShaderFromFile(L"Morphing.txt", 0, 0, "vs_main", 
        "vs_2_0", D3DXSHADER_DEBUG, &pShader, &pErrors, &g_pConstantTable); 
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // ����������ɫ��
    g_pd3dDevice->CreateVertexShader((DWORD*)pShader->GetBufferPointer(), &g_pVertexShader);
    g_pConstantTable->SetDefaults(g_pd3dDevice);
    SAFE_RELEASE(pShader);

    // ���ò�����ɫ����
    D3DXVECTOR4 vMtrlsDiffuse(0.4f, 0.4f, 0.4f, 1.0f);
    D3DXVECTOR4 vMtrlsAmbient(0.6f, 0.6f, 0.6f, 1.0f);
    g_pConstantTable->SetVector(g_pd3dDevice, "MtrlsAmbient", &vMtrlsDiffuse);
    g_pConstantTable->SetVector(g_pd3dDevice, "MtrlsDiffuse", &vMtrlsAmbient);

    // ���ù��շ�����ɫ
    D3DXVECTOR4 vLightDir(1.0f, 0.0f, 1.0f, 0.0f); 
    D3DXVECTOR4 LightDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    g_pConstantTable->SetVector(g_pd3dDevice, "vecLightDir",  &vLightDir);
    g_pConstantTable->SetVector(g_pd3dDevice, "LightDiffuse", &LightDiffuse);

    // ����ȡ���任����
    D3DXVECTOR3 vEyePt(0.0f, 0.0f, -10.0f);
    D3DXVECTOR3 vLookAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����ͶӰ�任����
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: ����3D����
//--------------------------------------------------------------------------------------
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ��������任����, ����Բ���˶�
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

    // ����ʱ���������, �����ɽ�������
    float fTimeFactor = (float)(timeGetTime() % 2000) / 1000.0f;
    float fTimeScalar = (fTimeFactor <= 1.0f) ? fTimeFactor :(2.0f - fTimeFactor);
    g_pConstantTable->SetFloat(g_pd3dDevice, "fTimeScalar", fTimeScalar);

    // ���ö�����ɫ���Ͷ�������
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);

    // ����Դ����ģ�͵�������
    LPDIRECT3DVERTEXBUFFER9 pVertexBuf = NULL;
    g_pSourceMesh->GetVertexBuffer(&pVertexBuf);
    g_pd3dDevice->SetStreamSource(0, pVertexBuf, 0, D3DXGetFVFVertexSize(g_pSourceMesh->GetFVF()));
    SAFE_RELEASE(pVertexBuf);

    // ����Ŀ������ģ�͵�������
    g_pTargetMesh->GetVertexBuffer(&pVertexBuf);
    g_pd3dDevice->SetStreamSource(1, pVertexBuf, 0, D3DXGetFVFVertexSize(g_pTargetMesh->GetFVF()));
    SAFE_RELEASE(pVertexBuf);

    // ������������
    LPDIRECT3DINDEXBUFFER9 pIndexBuf = NULL;
    g_pSourceMesh->GetIndexBuffer(&pIndexBuf);
    g_pd3dDevice->SetIndices(pIndexBuf);
    SAFE_RELEASE(pIndexBuf);

    // ���ƽ��䶯��
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
        g_pSourceMesh->GetNumVertices(), 0, g_pSourceMesh->GetNumFaces());

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
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
