//--------------------------------------------------------------------------------------
// File: PixelShader.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"PixelShader";                // ��������
wchar_t *g_pWindowName = L"������ɫ��ʾ��";             // ���ڱ�����

LPDIRECT3DDEVICE9            g_pd3dDevice     = NULL;   // D3D�豸�ӿ�
LPDIRECT3DVERTEXBUFFER9      g_pVertexBuf     = NULL;   // ���㻺��
LPDIRECT3DTEXTURE9           g_pTexture0      = NULL;   // ��1������
LPDIRECT3DTEXTURE9           g_pTexture1      = NULL;   // ��2������
LPDIRECT3DTEXTURE9           g_pTexture2      = NULL;   // ��3������
LPDIRECT3DPIXELSHADER9       g_pPixelShader   = NULL;   // ��ɫ���ӿ�
LPD3DXCONSTANTTABLE          g_pConstantTable = NULL;   // ������ӿ�

// ����ṹ
struct CUSTOMVERTEX 
{
    FLOAT _x, _y, _z;           // �����λ��
    FLOAT _u0, _v0;             // ��1����������
    FLOAT _u1, _v1;             // ��2����������
    FLOAT _u2, _v2;             // ��2����������
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
    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL); 

    // ��䶥������
    CUSTOMVERTEX *pVertices = NULL;
    g_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-5.0f, -5.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    pVertices[1] = CUSTOMVERTEX(-5.0f,  5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 5.0f, -5.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    pVertices[3] = CUSTOMVERTEX( 5.0f,  5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    g_pVertexBuf->Unlock();

    // ��������
    D3DXCreateTextureFromFile(g_pd3dDevice, L"lion.jpg", &g_pTexture0);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"spotlight.bmp", &g_pTexture1);
    D3DXCreateTextureFromFile(g_pd3dDevice, L"text.bmp", &g_pTexture2);

    // ������ɫ������
    LPD3DXBUFFER pShader = NULL;
    LPD3DXBUFFER pErrors = NULL;
    D3DXCompileShaderFromFile(L"MultiTexture.txt", NULL, NULL, "ps_main", "ps_2_0", 
        D3DXSHADER_DEBUG, &pShader, &pErrors, &g_pConstantTable);
    if (pErrors != NULL) 
    {
        ::MessageBoxA(NULL, (char*)pErrors->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(pErrors);
    }

    // ����������ɫ��
    g_pd3dDevice->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &g_pPixelShader);
    g_pConstantTable->SetDefaults(g_pd3dDevice);
    pShader->Release();

    // ����ȡ���任����
    D3DXVECTOR3 vEye(0.0f, 0.0f, -20.0f);
    D3DXVECTOR3 vLookAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMATRIX  matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vLookAt, &vUp);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����ͶӰ�任����
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0, 1.0f, 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // �رչ���
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
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

    // ��������任����
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

    // ȡ�ò��������
    static D3DXHANDLE hSampler0 = g_pConstantTable->GetConstantByName(0, "Sampler0");
    static D3DXHANDLE hSampler1 = g_pConstantTable->GetConstantByName(0, "Sampler1");
    static D3DXHANDLE hSampler2 = g_pConstantTable->GetConstantByName(0, "Sampler2");

    UINT Count;
    D3DXCONSTANT_DESC descSampler0, descSampler1, descSampler2;
    g_pConstantTable->GetConstantDesc(hSampler0, &descSampler0, &Count);
    g_pConstantTable->GetConstantDesc(hSampler1, &descSampler1, &Count);
    g_pConstantTable->GetConstantDesc(hSampler2, &descSampler2, &Count);

    // ��������
    g_pd3dDevice->SetTexture(descSampler0.RegisterIndex, g_pTexture0);
    g_pd3dDevice->SetTexture(descSampler1.RegisterIndex, g_pTexture1);
    g_pd3dDevice->SetTexture(descSampler2.RegisterIndex, g_pTexture2);

    // ���ƶ����
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

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
    SAFE_RELEASE(g_pTexture0);
    SAFE_RELEASE(g_pTexture1);
    SAFE_RELEASE(g_pTexture2);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pConstantTable);
    SAFE_RELEASE(g_pd3dDevice);
}
