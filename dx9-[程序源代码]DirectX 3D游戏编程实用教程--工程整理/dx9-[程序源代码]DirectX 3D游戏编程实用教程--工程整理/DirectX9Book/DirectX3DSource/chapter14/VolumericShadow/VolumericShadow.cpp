//--------------------------------------------------------------------------------------
// File: VolumericShadow.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3D��ͷ�ļ�
#include <dinput.h>
#include "Camera.h"
#include "ShadowVolume.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"VolumericShadow";    // ��������
wchar_t *g_pWindowName = L"�����Ӱʾ��";       // ���ڱ�����

CCamera*                g_pCamera       = NULL;
CShadowVolume*          g_pShadows      = NULL;

LPD3DXMESH              g_pMeshLight    = NULL;
D3DMATERIAL9            g_mLightMtrl    = {0};

LPD3DXMESH              g_pMeshModel    = NULL;
D3DMATERIAL9*           g_pModelMtrls   = NULL;
LPDIRECT3DTEXTURE9*     g_pModelTexes   = NULL;
DWORD                   g_dwNumMtrls    = 0;

LPDIRECT3DVERTEXBUFFER9 g_pFloorVBuffer = NULL;
LPDIRECT3DTEXTURE9      g_pFloorTexture = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice    = NULL;

struct CUSTOMVERTEX
{
    FLOAT _x,  _y,  _z;
    FLOAT _u,  _v;
    CUSTOMVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v)
        : _x(x), _y(y), _z(z), _u(u), _v(v) {}
};
#define D3DFVF_CUSTOMVERTEX  (D3DFVF_XYZ | D3DFVF_TEX1)

LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
LPDIRECTINPUTDEVICE8    g_pMouseDevice         = NULL;
char                    g_pKeyStateBuffer[256] = {0};
DIMOUSESTATE            g_diMouseState         = {0};

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender();              // ��Ⱦͼ��
VOID Direct3DCleanup();             // ����Direct3D��Դ

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: ReadDevice()
// Desc: ��ȡ�豸����������
//--------------------------------------------------------------------------------------
BOOL ReadDevice(IDirectInputDevice8 *pDIDevice, void* pBuffer, long lSize) 
{
    HRESULT hr;
    while (true) 
    {
        pDIDevice->Poll();              // ��ѯ�豸
        pDIDevice->Acquire();           // ��ȡ�豸�Ŀ���Ȩ
        if (SUCCEEDED(hr = pDIDevice->GetDeviceState(lSize, pBuffer))) break;
        if (hr != DIERR_INPUTLOST || hr != DIERR_NOTACQUIRED) return FALSE;
        if (FAILED(pDIDevice->Acquire())) return FALSE;
    }
    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: WinMain()
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
    InitDirect3D(hWnd, hInstance);

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
HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance)
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
    D3DXCreateSphere(g_pd3dDevice, 0.5f, 20, 20, &g_pMeshLight, NULL);

    // �����ذ�
    g_pd3dDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX), 0, 
        D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pFloorVBuffer, 0);

    CUSTOMVERTEX *pVertices = NULL;
    g_pFloorVBuffer->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = CUSTOMVERTEX(-20.0f, -5.0f, -20.0f, 0.0f, 5.0f);
    pVertices[1] = CUSTOMVERTEX(-20.0f, -5.0f,  20.0f, 0.0f, 0.0f);
    pVertices[2] = CUSTOMVERTEX( 20.0f, -5.0f, -20.0f, 5.0f, 5.0f);
    pVertices[3] = CUSTOMVERTEX( 20.0f, -5.0f,  20.0f, 5.0f, 0.0f);
    g_pFloorVBuffer->Unlock();

    // �����ذ�����
    D3DXCreateTextureFromFile(g_pd3dDevice, L"floor.jpg", &g_pFloorTexture);

    // ��X�ļ��м�����������
    LPD3DXBUFFER pAdjBuffer  = NULL;
    LPD3DXBUFFER pMtrlBuffer = NULL;
    D3DXLoadMeshFromX(L"bigship.x", D3DXMESH_MANAGED, g_pd3dDevice, 
        &pAdjBuffer, &pMtrlBuffer, NULL, &g_dwNumMtrls, &g_pMeshModel);

    // ��ȡ���ʺ���������
    D3DXMATERIAL *pMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
    g_pModelMtrls = new D3DMATERIAL9[g_dwNumMtrls];
    g_pModelTexes = new LPDIRECT3DTEXTURE9[g_dwNumMtrls];

    for (DWORD i=0; i<g_dwNumMtrls; i++) 
    {
        g_pModelMtrls[i] = pMtrls[i].MatD3D;
        g_pModelMtrls[i].Ambient = g_pModelMtrls[i].Diffuse;
        g_pModelTexes[i] = NULL;
        D3DXCreateTextureFromFileA(g_pd3dDevice, pMtrls[i].pTextureFilename, &g_pModelTexes[i]);
    }
    pAdjBuffer->Release();
    pMtrlBuffer->Release();

    // ������Ӱ��
    g_pShadows = new CShadowVolume(g_pd3dDevice);
    g_pShadows->CreateShadowVolume(g_pMeshModel);

    // �������������豸
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboardDevice, NULL);
    g_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
    g_pKeyboardDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    g_pKeyboardDevice->Acquire();

    // ������������豸
    DirectInput8Create(hInstance, 0x0800, IID_IDirectInput8, (void**)&g_pDirectInput, NULL);
    g_pDirectInput->CreateDevice(GUID_SysMouse, &g_pMouseDevice, NULL);
    g_pMouseDevice->SetDataFormat(&c_dfDIMouse);
    g_pMouseDevice->Acquire();

    // ����������˷�ʽ
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    // �����������
    g_mLightMtrl.Emissive = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

    // ���ù���
    D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type          = D3DLIGHT_POINT;
    light.Ambient       = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    light.Diffuse       = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Position      = D3DXVECTOR3(-8.0f, 10.0f, -2.0f);
    light.Attenuation0  = 1.0f;
    light.Range         = 300.0f;
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, true);
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, true);

    // ��������ʼ�����������
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(0.0f, 0.0f, -30.0f));
    g_pCamera->ResetLookatPos(&D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    g_pCamera->ResetViewMatrix();
    g_pCamera->ResetProjMatrix();
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

    // ��ȡ��������
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // ��ȡ�������
    ::ZeroMemory(&g_diMouseState, sizeof(g_diMouseState));
    ReadDevice(g_pMouseDevice, (LPVOID)&g_diMouseState, sizeof(g_diMouseState));

    // ��������������ƶ��ӽ�
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->MoveAlongRightVec(-0.2f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->MoveAlongRightVec( 0.2f);
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->MoveAlongLookVec( 0.2f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->MoveAlongLookVec(-0.2f);
    if (g_pKeyStateBuffer[DIK_R] & 0x80) g_pCamera->MoveAlongUpVec( 0.2f);
    if (g_pKeyStateBuffer[DIK_F] & 0x80) g_pCamera->MoveAlongUpVec(-0.2f);

    // ���������������ת�ӽ�
    if (g_pKeyStateBuffer[DIK_UP]    & 0x80) g_pCamera->RotationRightVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_DOWN]  & 0x80) g_pCamera->RotationRightVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_LEFT]  & 0x80) g_pCamera->RotationUpVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_RIGHT] & 0x80) g_pCamera->RotationUpVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_N]     & 0x80) g_pCamera->RotationLookVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_M]     & 0x80) g_pCamera->RotationLookVec( 0.01f);

    // ��������ȡ���任����
    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ��������
    D3DXMATRIX matLight;
    D3DXMatrixTranslation(&matLight, -8.0f, 10.0f, -2.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matLight);
    g_pd3dDevice->SetMaterial(&g_mLightMtrl);
    g_pMeshLight->DrawSubset(0);

    // ���Ƶذ�
    D3DXMATRIX matFloor;
    D3DXMatrixTranslation(&matFloor, 0.0f, -3.0f, 0.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matFloor);
    g_pd3dDevice->SetStreamSource(0, g_pFloorVBuffer, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
    g_pd3dDevice->SetTexture(0, g_pFloorTexture);
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // ��������
    D3DXMATRIX matModel, Rs, Rx, Ry;
    D3DXMatrixScaling(&Rs, 0.5f, 0.5f, 0.5f);

    static FLOAT fAngleX = 0.0f, fAngleY = 0.0f;
    if (g_diMouseState.rgbButtons[0] & 0x80)
    {
        fAngleX -= g_diMouseState.lY * 0.01f;
        fAngleY -= g_diMouseState.lX * 0.01f;
    }

    static FLOAT fMoveX = 0.0f, fMoveY = 0.0f;
    if (g_diMouseState.rgbButtons[1] & 0x80)
    {
        fMoveX += g_diMouseState.lX * 0.01f;
        fMoveY -= g_diMouseState.lY * 0.01f;
    }

    D3DXMatrixRotationX(&Rx, fAngleX);
    D3DXMatrixRotationY(&Ry, fAngleY);
    D3DXMatrixTranslation(&matModel, fMoveX, fMoveY, 0.0f);
    matModel = Rs * Rx * Ry * matModel;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matModel);

    for (DWORD i = 0; i<g_dwNumMtrls; i++)
    {
        g_pd3dDevice->SetTexture(0, g_pModelTexes[i]);
        g_pd3dDevice->SetMaterial(&g_pModelMtrls[i]);
        g_pMeshModel->DrawSubset(i);
    }

    // ������Ӱ
    static BOOL bRenderVolume = FALSE;
    if (::GetAsyncKeyState('V') & 0x8000f) bRenderVolume = TRUE;
    if (::GetAsyncKeyState('C') & 0x8000f) bRenderVolume = FALSE;

    D3DXMATRIX matInverse;
    D3DXMatrixInverse(&matInverse, NULL, &matModel);
    D3DXVECTOR3 vLight(-8.0f, 10.0f, -2.0f);
    D3DXVec3TransformCoord(&vLight, &vLight, &matInverse);

    g_pShadows->UpdateShadowVolume(vLight);
    g_pShadows->RenderShadowVolume(bRenderVolume);

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_DELETE(g_pCamera);
    SAFE_DELETE(g_pShadows);
    SAFE_RELEASE(g_pMeshLight);
    SAFE_RELEASE(g_pMeshModel);
    SAFE_RELEASE(g_pFloorVBuffer);
    SAFE_RELEASE(g_pFloorTexture);

    for (DWORD i = 0; i<g_dwNumMtrls; i++)
        SAFE_RELEASE(g_pModelTexes[i]);
    SAFE_DELETE_ARRAY(g_pModelTexes);
    SAFE_DELETE_ARRAY(g_pModelMtrls);

    g_pKeyboardDevice->Unacquire();
    g_pMouseDevice->Unacquire();
    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pKeyboardDevice);
    SAFE_RELEASE(g_pMouseDevice);
    SAFE_RELEASE(g_pd3dDevice);
}
