//--------------------------------------------------------------------------------------
// File: LightMaterial.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX���ͷ�ļ�

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"LightMaterial";  // ��������
wchar_t *g_pWindowName = L"���������ʾ��"; // ���ڱ�����

LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;    // D3D�豸�ӿ�
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf = NULL;    // ���㻺��ӿ�

struct CUSTOMVERTEX                 // ����ṹ
{
    FLOAT  _x,  _y,  _z;            // �����λ��
    FLOAT _nx, _ny, _nz;            // ������
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT nx, FLOAT ny, FLOAT nz)
        : _x(x), _y(y), _z(z), _nx(nx), _ny(ny), _nz(nz) {}
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL)


HRESULT InitDirect3D(HWND hWnd);    // ��ʼ��Direct3D
VOID Direct3DRender();              // ��Ⱦͼ��
VOID Direct3DCleanup();             // ����Direct3D��Դ

//--------------------------------------------------------------------------------------
// Name: SetupLight()
// Desc: ���ù�Դ����
//--------------------------------------------------------------------------------------
VOID SetupLight(LPDIRECT3DDEVICE9 pd3dDevice, UINT nType)
{
    static D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));

    switch (nType)
    {
    case 1:     //���Դ
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
    case 2:     //ƽ�й�
        light.Type          = D3DLIGHT_DIRECTIONAL;
        light.Ambient       = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
        light.Specular      = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
        light.Direction     = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
        break;
    case 3:     //�۹��
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
    g_pd3dDevice->CreateVertexBuffer(12 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL);

    // ��䶥������
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    // ǰ��
    pVertices[0] = CUSTOMVERTEX(-1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);
    pVertices[1] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.0f, 0.707f, -0.707f);
    pVertices[2] = CUSTOMVERTEX( 1.0f, 0.0f, -1.0f, 0.0f, 0.707f, -0.707f);

    // �����
    pVertices[3] = CUSTOMVERTEX(-1.0f, 0.0f,  1.0f, -0.707f, 0.707f, 0.0f);
    pVertices[4] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, -0.707f, 0.707f, 0.0f);
    pVertices[5] = CUSTOMVERTEX(-1.0f, 0.0f, -1.0f, -0.707f, 0.707f, 0.0f);

    // �Ҳ���
    pVertices[6] = CUSTOMVERTEX( 1.0f, 0.0f, -1.0f, 0.707f, 0.707f, 0.0f);
    pVertices[7] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.707f, 0.707f, 0.0f);
    pVertices[8] = CUSTOMVERTEX( 1.0f, 0.0f,  1.0f, 0.707f, 0.707f, 0.0f);

    // ����
    pVertices[9]  = CUSTOMVERTEX( 1.0f, 0.0f,  1.0f, 0.0f, 0.707f, 0.707f);
    pVertices[10] = CUSTOMVERTEX( 0.0f, 1.0f,  0.0f, 0.0f, 0.707f, 0.707f);
    pVertices[11] = CUSTOMVERTEX(-1.0f, 0.0f,  1.0f, 0.0f, 0.707f, 0.707f);
    g_pVertexBuf->Unlock();

    // ���ò���
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Specular = D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f);
    mtrl.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
    g_pd3dDevice->SetMaterial(&mtrl);

    // ���ù���
    SetupLight(g_pd3dDevice, 1);
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, true);
    g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true);
    g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, true);

    // ����ȡ���任����
    D3DXMATRIX  matView;
    D3DXVECTOR3 vEye(0.0f, 1.0, -5.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����ͶӰ�任����
    D3DXMATRIX  matProj;
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

    // �������ù���
    if (::GetAsyncKeyState(0x31) & 0x8000f)         // ����1
        SetupLight(g_pd3dDevice, 1);
    if (::GetAsyncKeyState(0x32) & 0x8000f)         // ����2
        SetupLight(g_pd3dDevice, 2);
    if (::GetAsyncKeyState(0x33) & 0x8000f)         // ����3
        SetupLight(g_pd3dDevice, 3);

    // ��������任����
    D3DXMATRIX matWorld;
    D3DXMatrixRotationY(&matWorld, ::timeGetTime() / 500.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // ����������
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 4);

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pVertexBuf);
    SAFE_RELEASE(g_pd3dDevice);
}
