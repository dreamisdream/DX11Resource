//--------------------------------------------------------------------------------------
// File: Billboard.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"Billboard";      // ��������
wchar_t *g_pWindowName = L"�����ʾ��";     // ���ڱ�����

struct  CUSTOMVERTEX
{
    FLOAT _x, _y, _z;   // ����λ��
    FLOAT _u, _v ;      // ������������
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v)
        : _x(x), _y(y), _z(z), _u(u), _v(v) {}
};
#define D3DFVF_CUSTOMVERTEX  (D3DFVF_XYZ | D3DFVF_TEX1)

LPDIRECT3DVERTEXBUFFER9     g_pGroundVB  = NULL;    // ���涥�㻺����
LPDIRECT3DTEXTURE9          g_pGroundTex = NULL;    // ��������
LPDIRECT3DVERTEXBUFFER9     g_pTreeVB    = NULL;    // ��ľ���㻺����
LPDIRECT3DTEXTURE9          g_pTreeTex   = NULL;    // ��ľ����
LPDIRECT3DDEVICE9           g_pd3dDevice = NULL;    // D3D�豸�ӿ�

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

    // �������涥�㻺��
    g_pd3dDevice->CreateVertexBuffer( 4 * sizeof(CUSTOMVERTEX), 0,  
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pGroundVB, NULL );

    CUSTOMVERTEX *pVertices = NULL;
    g_pGroundVB->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-32.0f, 0.0f, -32.0f,  0.0f, 16.0f);
    pVertices[1] = CUSTOMVERTEX(-32.0f, 0.0f,  32.0f,  0.0f,  0.0f);
    pVertices[2] = CUSTOMVERTEX( 32.0f, 0.0f, -32.0f, 16.0f, 16.0f); 
    pVertices[3] = CUSTOMVERTEX( 32.0f, 0.0f,  32.0f, 16.0f,  0.0f);
    g_pGroundVB->Unlock();

    // ������ľ���㻺��
    g_pd3dDevice->CreateVertexBuffer( 4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pTreeVB, NULL );
    
    pVertices = NULL;
    g_pTreeVB->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    pVertices[1] = CUSTOMVERTEX(-1.0f, 3.0f, 0.0f, 0.0f, 0.0f); 
    pVertices[2] = CUSTOMVERTEX( 1.0f, 0.0f, 0.0f, 1.0f, 1.0f); 
    pVertices[3] = CUSTOMVERTEX( 1.0f, 3.0f, 0.0f, 1.0f, 0.0f);
    g_pTreeVB->Unlock();

    // ���������������ľ����
    D3DXCreateTextureFromFile( g_pd3dDevice, L"ground.jpg", &g_pGroundTex );
    D3DXCreateTextureFromFile( g_pd3dDevice, L"tree.dds", &g_pTreeTex );

    // ����ͶӰ�任����
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0, 1.0f, 100.0f );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // �رչ���
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, false );
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

    //���ù۲����
    D3DXMATRIXA16 matView;
    FLOAT fAngle = timeGetTime() / 2000.0f;

    D3DXVECTOR3 vEye( 10.0f*sin(fAngle), 5.0f, 10.0f*cos(fAngle));
    D3DXVECTOR3 vAt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUp( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matView, &vEye, &vAt, &vUp );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // ��������״̬
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    // Alpha�������, ���û��ϵ��
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   true );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    
    // ��Ⱦ����
    D3DXMATRIX matGround;
    D3DXMatrixIdentity(&matGround);
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matGround );
    g_pd3dDevice->SetTexture( 0, g_pGroundTex );
    g_pd3dDevice->SetStreamSource( 0, g_pGroundVB, 0, sizeof(CUSTOMVERTEX) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    // ����ȡ���任�������������ƾ���
    D3DXMATRIX matBillboard;
    D3DXMatrixIdentity(&matBillboard);
    matBillboard._11 = matView._11;
    matBillboard._13 = matView._13;
    matBillboard._31 = matView._31;
    matBillboard._33 = matView._33;
    D3DXMatrixInverse(&matBillboard, NULL, &matBillboard);
/*
    // ���ݵ�ǰ�۲췽�����������ƾ���
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

    // ��Ⱦ��ľ
    g_pd3dDevice->SetTexture( 0, g_pTreeTex );
    g_pd3dDevice->SetStreamSource( 0, g_pTreeVB, 0, sizeof(CUSTOMVERTEX) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
    g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pTreeTex);
    SAFE_RELEASE(g_pTreeVB);
    SAFE_RELEASE(g_pGroundTex);
    SAFE_RELEASE(g_pGroundVB);
    SAFE_RELEASE(g_pd3dDevice);
}
