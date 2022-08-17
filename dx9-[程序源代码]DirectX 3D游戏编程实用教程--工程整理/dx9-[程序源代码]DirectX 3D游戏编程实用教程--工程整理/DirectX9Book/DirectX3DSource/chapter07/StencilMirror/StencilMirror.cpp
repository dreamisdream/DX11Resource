//--------------------------------------------------------------------------------------
// File: StencilMirror.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3D��ͷ�ļ�

#define SAFE_DELETE(p)  { if(p) delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"StencilMirror";  // ��������
wchar_t *g_pWindowName = L"����Ч��ʾ��";   // ���ڱ�����

LPDIRECT3DDEVICE9   g_pd3dDevice    = NULL; // Direct3D�豸�ӿ�
LPD3DXMESH          g_pMeshWall     = NULL; // ǽ���������
LPD3DXMESH          g_pMeshTorus    = NULL; // Բ�����������
LPD3DXMESH          g_pMeshTeapot   = NULL; // ����������
D3DMATERIAL9        g_Materials[3]  = {0};  // ����

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

    // ������
    D3DXCreateBox(g_pd3dDevice, 5.0f, 4.0f, 0.0f, &g_pMeshWall, NULL);
    g_Materials[0].Ambient  = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    g_Materials[0].Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    g_Materials[0].Specular = D3DXCOLOR(0.1f, 0.1f, 0.1f, 1.0f);

    // Բ����
    D3DXCreateTorus(g_pd3dDevice, 0.4f, 0.8f, 20, 20, &g_pMeshTorus, 0);
    g_Materials[1].Ambient  = D3DXCOLOR(0.0f, 0.0f, 1.0f, 0.5f);
    g_Materials[1].Diffuse  = D3DXCOLOR(0.0f, 0.0f, 1.0f, 0.5f);
    g_Materials[1].Specular = D3DXCOLOR(0.1f, 0.1f, 0.1f, 0.5f);

    // ���
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, NULL);
    g_Materials[2].Ambient  = D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.5f);
    g_Materials[2].Diffuse  = D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.5f);
    g_Materials[2].Specular = D3DXCOLOR(0.1f, 0.1f, 0.1f, 0.5f);

    // ����ȡ���任����
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(-3.0f, 2.0, -10.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����ͶӰ�任����
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // ���ù���
    D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type          = D3DLIGHT_DIRECTIONAL;
    light.Ambient       = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Specular      = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Direction     = D3DXVECTOR3(0.707f, -0.707f, 0.707f);
    g_pd3dDevice->SetLight(0, &light);
	g_pd3dDevice->LightEnable(0, true);
	g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, true);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: ����3D����
//--------------------------------------------------------------------------------------
VOID Direct3DRender() 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 
        D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ��������任����
    D3DXMATRIX matWorld, R;
    D3DXMatrixRotationY(&R, ::timeGetTime() / 1000.0f);

    static FLOAT fOffset = 0.0f;
    static BOOL bIncrease = true;
    if (fOffset <= -2.6f) bIncrease = true;
    if (fOffset >=  2.6f) bIncrease = false;
    fOffset = bIncrease ? fOffset+0.01f : fOffset-0.01f;

    // ���ƾ���
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[0]);
    g_pMeshWall->DrawSubset(0);

    // ����Բ����
    D3DXMatrixTranslation(&matWorld, fOffset, 0.0f, -2.0f);
    D3DXMatrixMultiply(&matWorld, &R, &matWorld);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[1]);
    g_pMeshTorus->DrawSubset(0);

    // ���Ʋ��
    D3DXMatrixTranslation(&matWorld, -fOffset, 0.0f, -4.0f);
    D3DXMatrixMultiply(&matWorld, &R, &matWorld);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[2]);
    g_pMeshTeapot->DrawSubset(0);

    // ����ģ�建��
    g_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE,    true);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILFUNC,      D3DCMP_ALWAYS);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILREF,       0x1);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILMASK,      0xffffffff);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILPASS,      D3DSTENCILOP_REPLACE);

    // ��ֹ����Ȼ���ͺ�̨����д����
    g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     false);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,         D3DBLEND_ZERO);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND,        D3DBLEND_ONE);

    // ��������Ƶ�ģ�建����
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[0]);
    g_pMeshWall->DrawSubset(0);

    // ������������Ȼ�����д����,����������λ�ھ���ʱģ����Գɹ�
    g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     true);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILFUNC,      D3DCMP_EQUAL);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILPASS,      D3DSTENCILOP_KEEP);

    // �����Ȼ���,���������뾵������ںϼ���
    g_pd3dDevice->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,         D3DBLEND_DESTCOLOR);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND,        D3DBLEND_ZERO);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE,         D3DCULL_CW);

    // ��������Բ����ķ������
    D3DXMATRIX matTorus, matTeapot;
    D3DXPLANE planeXY(0.0f, 0.0f, 1.0f, 0.0f); // xyƽ��
    D3DXMatrixReflect(&matTorus, &planeXY);
    D3DXMatrixReflect(&matTeapot, &planeXY);

    // ���Ʋ����Բ���徵��
    D3DXMatrixTranslation(&matWorld, fOffset, 0.0f, -2.0f);
    matWorld = R * matWorld * matTorus;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[1]);
    g_pMeshTorus->DrawSubset(0);

    D3DXMatrixTranslation(&matWorld, -fOffset, 0.0f, -4.0f);
    matWorld = R * matWorld * matTeapot;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pd3dDevice->SetMaterial(&g_Materials[2]);
    g_pMeshTeapot->DrawSubset(0);

    // �ָ���Ⱦ״̬
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
    g_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE,   false);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE,         D3DCULL_CCW);

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pMeshWall);
    SAFE_RELEASE(g_pMeshTorus);
    SAFE_RELEASE(g_pMeshTeapot);
    SAFE_RELEASE(g_pd3dDevice);
}
