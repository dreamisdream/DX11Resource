//--------------------------------------------------------------------------------------
// File: PickExample.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�
#include <dinput.h>                 // DirectInputͷ�ļ�
#include <stdio.h>
#include "Camera.h"

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"PickExample";        // ��������
wchar_t *g_pWindowName = L"ʰȡӦ��ʾ��";       // ���ڱ�����

// �����ṹ
struct BOUNDINGSPHERE 
{
    D3DXVECTOR3 vCenter;        // ���������ĵ�
    FLOAT       fRadius;        // �����İ뾶
};

CCamera*                g_pCamera              = NULL;
LPD3DXFONT              g_pTextStats           = NULL;  // ״̬��Ϣ��2D�ı�
LPD3DXFONT              g_pTextInfor           = NULL;  // ������Ϣ��2D�ı�

LPD3DXMESH              g_pMeshTeapot          = NULL;  // ������������
LPD3DXMESH              g_pMeshBound           = NULL;  // ������������
BOUNDINGSPHERE          g_bsBoundSphere;                // ��������

LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL;  // Direct3D�豸�ӿ�
LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
LPDIRECTINPUTDEVICE8    g_pMouseDevice         = NULL;
char                    g_pKeyStateBuffer[256] = {0};
DIMOUSESTATE            g_diMouseState         = {0};


HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);       // ��Ⱦͼ��
VOID Direct3DCleanup();             // ����Direct3D��Դ

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//--------------------------------------------------------------------------------------
// Name: HasPicked()
// Desc: ����Ƿ�ѡ������
//--------------------------------------------------------------------------------------
BOOL HasPicked(LPDIRECT3DDEVICE9 pd3dDevice, BOUNDINGSPHERE *pBoundingSphere, FLOAT x, FLOAT y) 
{
    // ת����Ļ���굽ͶӰ��������
    D3DXMATRIX mProj;
    pd3dDevice->GetTransform(D3DTS_PROJECTION, &mProj);             // ȡ��ͶӰ�任����

    D3DVIEWPORT9 vp;
    pd3dDevice->GetViewport(&vp);                                   // ȡ���ӿ���Ϣ
    FLOAT px = ((( 2.0f * x) / vp.Width)  - 1.0f) / mProj(0, 0);    // ����ͶӰ���ڵ��x����
    FLOAT py = (((-2.0f * y) / vp.Height) + 1.0f) / mProj(1, 1);    // ����ͶӰ���ڵ��y����

    // ����ʰȡ����
    D3DXVECTOR3 vOrigin(0.0f, 0.0f, 0.0f);                          // ����ԭ��
    D3DXVECTOR3 vDirection = D3DXVECTOR3(px, py, 1.0f);             // ���߷���

    D3DXMATRIX mView, mInverse;
    pd3dDevice->GetTransform(D3DTS_VIEW, &mView);                   // ȡ��ȡ���任����
    D3DXMatrixInverse(&mInverse, 0, &mView);                        // �����

    // �����߽��б任
    D3DXVec3TransformCoord(&vOrigin, &vOrigin, &mInverse);
    D3DXVec3TransformNormal(&vDirection, &vDirection, &mInverse);
    D3DXVec3Normalize(&vDirection, &vDirection);

    // �ж���������������Ƿ��ཻ
    D3DXVECTOR3 vValue = vOrigin - pBoundingSphere->vCenter;
    FLOAT b = 2.0f * D3DXVec3Dot(&vDirection, &vValue);
    FLOAT c = D3DXVec3Dot(&vValue, &vValue) - (pBoundingSphere->fRadius * pBoundingSphere->fRadius);

    FLOAT discriminant = (b * b) - (4.0f * c);
    if (discriminant < 0.0f) return false;

    discriminant = sqrtf(discriminant);
    FLOAT s0 = (-b + discriminant) / 2.0f;
    FLOAT s1 = (-b - discriminant) / 2.0f;
    return (s0 >= 0.0f || s1 >= 0.0f);
}

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
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Direct3DRender(hWnd, 0.0f); // ����3D����
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
        Direct3DRender(hWnd, 0.0f); // ��Ⱦͼ��
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

    // �������
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, 0);

    // �����������
    D3DXVECTOR3 *pvPos = 0;
    g_pMeshTeapot->LockVertexBuffer(0, (void**)&pvPos);
    D3DXComputeBoundingSphere(pvPos, g_pMeshTeapot->GetNumVertices(), 
                              D3DXGetFVFVertexSize(g_pMeshTeapot->GetFVF()), 
                              &g_bsBoundSphere.vCenter, &g_bsBoundSphere.fRadius);
    g_pMeshTeapot->UnlockVertexBuffer();
    
    // �������������
    D3DXCreateSphere(g_pd3dDevice, g_bsBoundSphere.fRadius, 20, 20, &g_pMeshBound, 0);

    // ����2D����
    D3DXCreateFont(g_pd3dDevice, 15, 0, 1000, 0, true, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Arial", &g_pTextStats); 
    D3DXCreateFont(g_pd3dDevice, 14, 0, 1000, 0, false, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 0, L"Times New Roman", &g_pTextInfor); 

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

    // ���ò���
    D3DMATERIAL9 mtrl;
    ::ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Ambient    = D3DXCOLOR(0.4f, 0.4f, 0.4f, 0.5f);
    mtrl.Diffuse    = D3DXCOLOR(0.8f, 0.8f, 0.8f, 0.5f);
    g_pd3dDevice->SetMaterial(&mtrl);

    // ���ù���
    D3DLIGHT9   light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Ambient   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Direction = D3DXVECTOR3(0.707f, -0.707f, 0.707f);
    g_pd3dDevice->SetLight(0, &light);
    g_pd3dDevice->LightEnable(0, true);
    
    // ��������ʼ�����������
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(0.0f, 0.0f, -8.0f));
    g_pCamera->ResetLookatPos(&D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    g_pCamera->ResetViewMatrix();
    g_pCamera->ResetProjMatrix();
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Direct3DRender()
// Desc: ����3D����
//--------------------------------------------------------------------------------------
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta) 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ���״̬��Ϣ
    RECT rect;
    GetClientRect(hWnd, &rect);
    rect.top += 5;
    g_pTextStats->DrawText(NULL, L"AppName: PickExample", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
    rect.top += 20;
    g_pTextStats->DrawText(NULL, L"Screen backbuffer(640X480),X8R8G8B8(D24X8)", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));

    // ��ȡ��������
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // ��ȡ�������
    ::ZeroMemory(&g_diMouseState, sizeof(g_diMouseState));
    ReadDevice(g_pMouseDevice, (LPVOID)&g_diMouseState, sizeof(g_diMouseState));

    // ��������������ƶ��ӽ�
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->MoveAlongRightVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->MoveAlongRightVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->MoveAlongLookVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->MoveAlongLookVec(-0.01f);
    if (g_pKeyStateBuffer[DIK_R] & 0x80) g_pCamera->MoveAlongUpVec( 0.01f);
    if (g_pKeyStateBuffer[DIK_F] & 0x80) g_pCamera->MoveAlongUpVec(-0.01f);

    // ����ȡ���任����
    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
    
    // �ж�����Ƿ�ѡ�м�����
    BOOL   bHasPicked = 0;
    static POINT ptCursor;

    ::GetCursorPos(&ptCursor);
    ::ScreenToClient(hWnd, &ptCursor);
    
    if (HasPicked(g_pd3dDevice, &g_bsBoundSphere, ptCursor.x, ptCursor.y))
        ::SetCursor(::LoadCursor(NULL, IDC_HAND)), bHasPicked = 1;

    // ������ƽ��
    static float fPosX = 0.0f, fPosY = 0.0f, fPosZ = 0.0f;
    if ((g_diMouseState.rgbButtons[0] & 0x80) && bHasPicked)
    {
        fPosX += g_diMouseState.lX *  0.008f;
        fPosY += g_diMouseState.lY * -0.008f;
    }
    fPosZ += g_diMouseState.lZ * 0.005f;
    
    // ��������������λ��
    g_bsBoundSphere.vCenter.x = fPosX;
    g_bsBoundSphere.vCenter.y = fPosY;
    g_bsBoundSphere.vCenter.z = fPosZ;

    // ���Ʋ��
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, fPosX, fPosY, fPosZ);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_pMeshTeapot->DrawSubset(0);

    // ����Alpha�ں�
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // ���������
    g_pMeshBound->DrawSubset(0);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

    // ���������Ϣ
    rect.left = 460, rect.top = 80;
    static char strInfo[256] = {0};
    sprintf_s(strInfo, "ģ������: (%.2f, %.2f, %.2f)", matWorld._41, matWorld._42, matWorld._43);
    g_pTextInfor->DrawTextA(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.6f, 0.6f, 1.0f, 1.0f));

    rect.top += 20;
    D3DXVECTOR3 vCameraPos;
    g_pCamera->GetCameraPos(&vCameraPos);
    sprintf_s(strInfo, "�ӵ�λ��: (%.2f, %.2f, %.2f)", vCameraPos.x, vCameraPos.y, vCameraPos.z);
    g_pTextInfor->DrawTextA(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f));

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pTextStats);
    SAFE_RELEASE(g_pTextInfor);
    SAFE_RELEASE(g_pMeshBound);
    SAFE_RELEASE(g_pMeshTeapot);

    g_pMouseDevice->Unacquire();
    g_pKeyboardDevice->Unacquire();

    SAFE_RELEASE(g_pDirectInput);
    SAFE_RELEASE(g_pMouseDevice);
    SAFE_RELEASE(g_pKeyboardDevice);
    SAFE_RELEASE(g_pd3dDevice);
}
