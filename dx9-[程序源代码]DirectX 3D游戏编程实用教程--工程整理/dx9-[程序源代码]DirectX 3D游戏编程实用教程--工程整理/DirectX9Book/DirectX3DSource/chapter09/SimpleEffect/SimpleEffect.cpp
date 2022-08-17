//--------------------------------------------------------------------------------------
// File: SimpleEffect.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"SimpleEffect";   // ��������
wchar_t *g_pWindowName = L"Ч�����ʾ��";   // ���ڱ�����

LPDIRECT3DDEVICE9       g_pd3dDevice    = NULL;     // D3D�豸�ӿ�
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf    = NULL;     // ���㻺��ӿ�
LPDIRECT3DINDEXBUFFER9  g_pIndexBuf     = NULL;     // ��������ӿ�
LPDIRECT3DTEXTURE9      g_pTexture      = NULL;     // ����ӿ�
LPD3DXEFFECT            g_pEffect       = NULL;     // Ч���ӿ�

// ����ṹ
struct CUSTOMVERTEX 
{
    FLOAT _x, _y, _z;               // �����λ��
    FLOAT _nx, _ny, _nz;            // ���㷨����
    FLOAT _u, _v;                   // ��������
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

    // �������㻺��
    g_pd3dDevice->CreateVertexBuffer(24 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL); 

    // ��䶥������
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    // ���涥������
    pVertices[0] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    pVertices[1] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    pVertices[3] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[4] = CUSTOMVERTEX( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pVertices[5] = CUSTOMVERTEX(-1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    pVertices[6] = CUSTOMVERTEX(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    pVertices[7] = CUSTOMVERTEX( 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[8]  = CUSTOMVERTEX(-1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    pVertices[9]  = CUSTOMVERTEX( 1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    pVertices[10] = CUSTOMVERTEX( 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    pVertices[11] = CUSTOMVERTEX(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[12] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
    pVertices[13] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
    pVertices[14] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
    pVertices[15] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);

    // ����涥������
    pVertices[16] = CUSTOMVERTEX(-1.0f,  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[17] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    pVertices[18] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    pVertices[19] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // �Ҳ��涥������
    pVertices[20] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[21] = CUSTOMVERTEX( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    pVertices[22] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    pVertices[23] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    g_pVertexBuf->Unlock();

    // ������������
    g_pd3dDevice->CreateIndexBuffer(36 * sizeof(WORD), 0, 
        D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIndexBuf, NULL);

    // �����������
    WORD *pIndices = NULL;
    g_pIndexBuf->Lock(0, 0, (void**)&pIndices, 0);
    
    // ������������
    pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
    pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

    // ������������
    pIndices[6] = 4; pIndices[7]  = 5; pIndices[8]  = 6;
    pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

    // ������������
    pIndices[12] = 8; pIndices[13] =  9; pIndices[14] = 10;
    pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

    // ������������
    pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
    pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

    // �������������
    pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
    pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

    // �Ҳ�����������
    pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
    pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
    g_pIndexBuf->Unlock();

    // ����Ч������
    ID3DXBuffer *pErrors = NULL;
    D3DXCreateEffectFromFile(g_pd3dDevice, L"SimpleEffect.fx", NULL, NULL, 
        D3DXSHADER_DEBUG, NULL, &g_pEffect, &pErrors); 

    // ���������Ϣ
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // ��������������
    D3DXCreateTextureFromFile(g_pd3dDevice, L"crate.jpg", &g_pTexture);
    g_pEffect->SetTexture("Texture0", g_pTexture);

    // ���ò�����ɫ
    D3DXCOLOR MtrlsDiffuse(1.0f, 1.0f, 1.0f, 1.0f);     // ������������ɫ
    D3DXCOLOR MtrlsAmbient(1.0f, 1.0f, 1.0f, 0.0f);     // ���ʻ�������ɫ
    g_pEffect->SetValue("MtrlsDiffuse", &MtrlsDiffuse, sizeof(D3DXCOLOR));
    g_pEffect->SetValue("MtrlsAmbient", &MtrlsAmbient, sizeof(D3DXCOLOR));

    // ���ù���
    D3DXVECTOR3 vLightDir[3] = 
    {   // ���շ���
        D3DXVECTOR3(-1.0f, 0.0f, -1.0f),
        D3DXVECTOR3( 1.0f, 0.0f, -1.0f),
        D3DXVECTOR3( 0.0f, 1.0f,  0.0f),
    };
    D3DXCOLOR LightDiffuse[3] = 
    {   // ������������ɫ
        D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f),              // ��
        D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f),              // ��
        D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f),              // ��
    };
    D3DXCOLOR LightAmbient[3] = 
    {   // ���ջ�������ɫ
        D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f),              // ��
        D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f),              // ��
        D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f),              // ��
    };
    g_pEffect->SetValue("LightDir",     vLightDir,    3 * sizeof(D3DXVECTOR3));
    g_pEffect->SetValue("LightDiffuse", LightDiffuse, 3 * sizeof(D3DXCOLOR));
    g_pEffect->SetValue("LightAmbient", LightAmbient, 3 * sizeof(D3DXCOLOR));

    // ����ȡ���任����
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 3.0f, -10.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����ͶӰ�任����
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: ����3D����
//--------------------------------------------------------------------------------------
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(45, 50, 170), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ��������任����
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);

    //D3DXMATRIX Rx, Ry, Rz;
    //D3DXMatrixRotationX(&Rx, timeGetTime() / 1000.0f);
    //D3DXMatrixRotationY(&Ry, timeGetTime() / 1000.0f);
    //D3DXMatrixRotationZ(&Rz, timeGetTime() / 1000.0f);
    //matWorld = Rx * Ry * Rz * matWorld;

    D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);        // ��Y����ת
    g_pEffect->SetMatrix("matWorld", &matWorld);

    // ������ϱ任����
    D3DXMATRIX matView, matProj;
    g_pd3dDevice->GetTransform(D3DTS_VIEW, &matView);               // ȡ���任����
    g_pd3dDevice->GetTransform(D3DTS_PROJECTION, &matProj);         // ͶӰ�任����
    D3DXMATRIX matWorldViewProj = matWorld * matView * matProj;     // ��ϱ任����
    g_pEffect->SetMatrix("matWorldViewProj", &matWorldViewProj);

    // �����ַ�
    if (::GetAsyncKeyState(0x31) & 0x8000f)     // ���ּ�1, ʹ��1����Դ������Ⱦ
        g_pEffect->SetTechnique("RenderSceneWith1Light");
    if (::GetAsyncKeyState(0x32) & 0x8000f)     // ���ּ�2, ʹ��2����Դ������Ⱦ
        g_pEffect->SetTechnique("RenderSceneWith2Light");
    if (::GetAsyncKeyState(0x33) & 0x8000f)     // ���ּ�3, ʹ��3����Դ������Ⱦ
        g_pEffect->SetTechnique("RenderSceneWith3Light");

    // ����������
    g_pd3dDevice->SetIndices(g_pIndexBuf);
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));

    UINT iNumPasses = 0;
    g_pEffect->Begin(&iNumPasses, 0);           // ��ʼ��ǰ����ַ�
    for (UINT i = 0; i<iNumPasses; i++)         // ʹ��ÿ��ͨ�����л���
    {
        g_pEffect->BeginPass( i );              // ʹ�õ�ǰ���ͨ��
        g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
        g_pEffect->EndPass();                   // ��ֹ��ǰʹ�õ�ͨ��
    }
    g_pEffect->End();                           // ������ǰʹ�õ��ַ�

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pEffect);
    SAFE_RELEASE(g_pTexture);
    SAFE_RELEASE(g_pIndexBuf);
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pd3dDevice);
}
