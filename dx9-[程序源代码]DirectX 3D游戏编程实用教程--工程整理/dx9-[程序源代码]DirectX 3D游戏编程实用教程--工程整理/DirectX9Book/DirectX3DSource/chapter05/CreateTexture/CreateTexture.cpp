//--------------------------------------------------------------------------------------
// File: CreateTexture.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX���ͷ�ļ�

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"CreateTexture";      // ��������
wchar_t *g_pWindowName = L"��������ʾ��";       // ���ڱ�����

LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;    // D3D�豸�ӿ�
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf = NULL;    // ���㻺��ӿ�
LPDIRECT3DINDEXBUFFER9  g_pIndexBuf  = NULL;    // ��������ӿ�
LPDIRECT3DTEXTURE9      g_pTexture   = NULL;    // ����ӿ�

// ����ṹ
struct CUSTOMVERTEX 
{
    FLOAT _x, _y, _z;               // �����λ��
    FLOAT _u, _v;                   // ��������
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v)
        : _x(x), _y(y), _z(z), _u(u), _v(v) {}
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

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

    // ��������
    D3DXCreateTextureFromFile(g_pd3dDevice, L"crate.jpg", &g_pTexture);

    // �������㻺��
    g_pd3dDevice->CreateVertexBuffer(24 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL); 

    // ��䶥������
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    // ���涥������
    pVertices[0] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f);
    pVertices[1] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 1.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 1.0f, 1.0f);
    pVertices[3] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[4] = CUSTOMVERTEX( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f);
    pVertices[5] = CUSTOMVERTEX(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f);
    pVertices[6] = CUSTOMVERTEX(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
    pVertices[7] = CUSTOMVERTEX( 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[8]  = CUSTOMVERTEX(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f);
    pVertices[9]  = CUSTOMVERTEX( 1.0f, 1.0f,  1.0f, 1.0f, 0.0f);
    pVertices[10] = CUSTOMVERTEX( 1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
    pVertices[11] = CUSTOMVERTEX(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f);

    // ���涥������
    pVertices[12] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f);
    pVertices[13] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 1.0f, 0.0f);
    pVertices[14] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f);
    pVertices[15] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f);

    // ����涥������
    pVertices[16] = CUSTOMVERTEX(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f);
    pVertices[17] = CUSTOMVERTEX(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f);
    pVertices[18] = CUSTOMVERTEX(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f);
    pVertices[19] = CUSTOMVERTEX(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f);

    // �Ҳ��涥������
    pVertices[20] = CUSTOMVERTEX( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f);
    pVertices[21] = CUSTOMVERTEX( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f);
    pVertices[22] = CUSTOMVERTEX( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f);
    pVertices[23] = CUSTOMVERTEX( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f);
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

    // ���ò���
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    g_pd3dDevice->SetMaterial(&mtrl);

    // ���û�����
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));

    // ����ȡ���任����
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 3.0f, -5.0f);
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
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
        D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ��������任����
    FLOAT fAngle = (float)::timeGetTime() / 360.0f;

    D3DXMATRIX matWorld, Rx, Ry, Rz;
    D3DXMatrixIdentity(&matWorld);
    D3DXMatrixRotationX(&Rx, fAngle);
    D3DXMatrixRotationY(&Ry, fAngle);
    D3DXMatrixRotationZ(&Rz, fAngle);
    matWorld = Rx * Ry * Rz * matWorld;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // ����������
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetIndices(g_pIndexBuf);
    g_pd3dDevice->SetTexture(0, g_pTexture);
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pTexture);
    SAFE_RELEASE(g_pIndexBuf);
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pd3dDevice);
}
