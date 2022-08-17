//--------------------------------------------------------------------------------------
// File: SimpleLodTerrain.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�
#include <dinput.h>                 // DirectInputͷ�ļ�
#include "Camera.h"
#include "LodTerrain.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"SimpleLodTerrain";   // ��������
wchar_t *g_pWindowName = L"LOD���λ���ʾ��";    // ���ڱ�����

CCamera*                g_pCamera              = NULL;
CLodTerrain*            g_pTerrain             = NULL;
LPD3DXFONT              g_pTextStats           = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL;

LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
LPDIRECTINPUTDEVICE8    g_pMouseDevice         = NULL;
char                    g_pKeyStateBuffer[256] = {0};
DIMOUSESTATE            g_diMouseState         = {0};


HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);
VOID Direct3DCleanup();

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: ReadDevice();
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
    InitDirect3D(hWnd, hInstance);

    // ��ʾ�����´���
    ShowWindow(hWnd, SW_SHOWDEFAULT); 
    UpdateWindow(hWnd); 

    // ��Ϣѭ��
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message!=WM_QUIT)
    {
        static FLOAT fLastTime  = (float)::timeGetTime();
        static FLOAT fCurrTime  = (float)::timeGetTime();
        static FLOAT fTimeDelta = 0.0f;
        fCurrTime  = (float)::timeGetTime();
        fTimeDelta = (fCurrTime - fLastTime) / 1000.0f;
        fLastTime  = fCurrTime;

        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Direct3DRender(hWnd, fTimeDelta);   // ����3D����
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
        //Direct3DRender(hWnd, 0.0f); // ��Ⱦͼ��
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

    // ����2D����
    D3DXCreateFont(g_pd3dDevice, 15, 0, 1000, 0, true, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Arial", &g_pTextStats); 

    // ���û�����
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
    
    // ��������ʼ�����������
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(0.0f, 15.0f, 0.0f));
    g_pCamera->ResetViewMatrix();

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 3000.0f);
    g_pCamera->ResetProjMatrix(&matProj);

    // ����LOD����
    g_pTerrain = new CLodTerrain(g_pd3dDevice);
    g_pTerrain->InitLodTerrain(1024.0f, 1024.0f, 0.3f, L"HeightMap.raw");

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: ����3D����
//--------------------------------------------------------------------------------------
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta) 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ��ȡ��������
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // ��ȡ�������
    ::ZeroMemory(&g_diMouseState, sizeof(g_diMouseState));
    ReadDevice(g_pMouseDevice, (LPVOID)&g_diMouseState, sizeof(g_diMouseState));

    // �ƶ��ӽ�
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->MoveAlongRightVec(-0.5f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->MoveAlongRightVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->MoveAlongLookVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->MoveAlongLookVec(-0.5f);
    if (g_pKeyStateBuffer[DIK_R] & 0x80) g_pCamera->MoveAlongUpVec( 0.5f);
    if (g_pKeyStateBuffer[DIK_F] & 0x80) g_pCamera->MoveAlongUpVec(-0.5f);

    // ��ת�ӽ�
    if (g_pKeyStateBuffer[DIK_LEFT]  & 0x80) g_pCamera->RotationUpVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_RIGHT] & 0x80) g_pCamera->RotationUpVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_UP]    & 0x80) g_pCamera->RotationRightVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_DOWN]  & 0x80) g_pCamera->RotationRightVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_N]     & 0x80) g_pCamera->RotationLookVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_M]     & 0x80) g_pCamera->RotationLookVec( 0.01f);

    // �������ӽ�
    if (g_diMouseState.rgbButtons[0] & 0x80) 
    {
        g_pCamera->RotationUpVec(g_diMouseState.lX * 0.002f);
        g_pCamera->RotationRightVec(g_diMouseState.lY * 0.002f);
    }

    // ����ȡ���任����
    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ����LOD����
    g_pTerrain->UpdateLodTerrain(g_pTerrain->GetHeadNode());

    // ���״̬��Ϣ
    static RECT rect;
    GetClientRect(hWnd, &rect);
    static char strInfo[256] = {0};
    static FLOAT fFrame = 0.0f, fTime = 0.0f, FPS = 0.0f;

    fFrame++, fTime += fTimeDelta;
    if (fTime >= 1.0f) 
    {
        FPS = fFrame / fTime;
        fFrame = fTime = 0.0f;
    }

    rect.top += 5;
    sprintf_s(strInfo, "LOD����ʾ��, FPS: %.2f", FPS);
    g_pTextStats->DrawTextA(NULL, strInfo, -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));

    rect.top += 20;
    sprintf_s(strInfo, "��ǰ���ƵĶ�����: %d", g_pTerrain->GetRenderNodes());
    g_pTextStats->DrawTextA(NULL, strInfo, -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));
    
    // ����LOD����
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    g_pTerrain->RenderLodTerrain(&matWorld);

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
    SAFE_DELETE(g_pTerrain);
    SAFE_RELEASE(g_pTextStats);

    g_pMouseDevice->Unacquire();
    g_pKeyboardDevice->Unacquire();
    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pMouseDevice);
    SAFE_RELEASE(g_pKeyboardDevice);

    SAFE_RELEASE(g_pd3dDevice);
}
