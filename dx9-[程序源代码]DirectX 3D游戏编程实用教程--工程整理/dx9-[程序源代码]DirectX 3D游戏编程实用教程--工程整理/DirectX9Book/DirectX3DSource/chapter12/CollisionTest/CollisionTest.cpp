//--------------------------------------------------------------------------------------
// File: CollisionTest.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // Direct3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�
#include <dinput.h>                 // DirectInputͷ�ļ�
#include <stdio.h>
#include "Camera.h"

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

wchar_t *g_pClassName  = L"CollisionTest";      // ��������
wchar_t *g_pWindowName = L"��ײ���ʾ��";       // ���ڱ�����

// ��Ӻнṹ
struct BOUNDINGBOX
{
    D3DXVECTOR3 vMin;   // ��Ӻе���С����
    D3DXVECTOR3 vMax;   // ��Ӻе���󶥵�
};

CCamera*                g_pCamera              = NULL;
LPD3DXFONT              g_pTextStats           = NULL;  // ״̬��Ϣ��2D�ı�
LPD3DXFONT              g_pTextInfor           = NULL;  // ������Ϣ��2D�ı�

LPD3DXMESH              g_pMeshSphere          = NULL;
LPD3DXMESH              g_pMeshTeapot          = NULL;
LPD3DXMESH              g_pMeshBBoxSphere      = NULL;
LPD3DXMESH              g_pMeshBBoxTeapot      = NULL;
BOUNDINGBOX             g_BBoxSphere;
BOUNDINGBOX             g_BBoxTeapot;
D3DMATERIAL9            g_MtrlSphere;
D3DMATERIAL9            g_MtrlTeapot;
D3DMATERIAL9            g_MtrlBBox;

LPDIRECT3DDEVICE9       g_pd3dDevice           = NULL;  // Direct3D�豸�ӿ�
LPDIRECTINPUT8          g_pDirectInput         = NULL;
LPDIRECTINPUTDEVICE8    g_pKeyboardDevice      = NULL;
char                    g_pKeyStateBuffer[256] = {0};

HRESULT InitDirect3D(HWND hWnd, HINSTANCE hInstance);
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta);       // ��Ⱦͼ��
VOID Direct3DCleanup();             // ����Direct3D��Դ

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//--------------------------------------------------------------------------------------
// Name: CollisionTestAABB
// Desc: AABB��ײ���
//--------------------------------------------------------------------------------------
BOOL CollisionTestAABB(D3DXVECTOR3 vMin1, D3DXVECTOR3 vMax1,
                       D3DXVECTOR3 vMin2, D3DXVECTOR3 vMax2 )
{
    if (vMax1.x < vMin2.x || vMin1.x > vMax2.x) return FALSE;   // x����
    if (vMax1.y < vMin2.y || vMin1.y > vMax2.y) return FALSE;   // y����
    if (vMax1.z < vMin2.z || vMin1.z > vMax2.z) return FALSE;   // z����
    return TRUE;    // ������ײ
}

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

    // ������������弸����
    D3DXCreateTeapot(g_pd3dDevice, &g_pMeshTeapot, NULL);
    D3DXCreateSphere(g_pd3dDevice, 1.0f, 20, 20, &g_pMeshSphere, NULL);

    // ������Ӻ�
    D3DXVECTOR3 *pFirstPosition = 0;
    g_pMeshSphere->LockVertexBuffer(0, (void**)&pFirstPosition);
    D3DXComputeBoundingBox(pFirstPosition, g_pMeshSphere->GetNumVertices(), D3DXGetFVFVertexSize(g_pMeshSphere->GetFVF()), &(g_BBoxSphere.vMin), &(g_BBoxSphere.vMax));
    g_pMeshSphere->UnlockVertexBuffer();

    g_pMeshTeapot->LockVertexBuffer(0, (void**)&pFirstPosition);
    D3DXComputeBoundingBox(pFirstPosition, g_pMeshTeapot->GetNumVertices(), D3DXGetFVFVertexSize(g_pMeshTeapot->GetFVF()), &(g_BBoxTeapot.vMin), &(g_BBoxTeapot.vMax));
    g_pMeshTeapot->UnlockVertexBuffer();

    // ������Ӻ�����
    FLOAT fWidth  = g_BBoxSphere.vMax.x - g_BBoxSphere.vMin.x;
    FLOAT fHeight = g_BBoxSphere.vMax.y - g_BBoxSphere.vMin.y;
    FLOAT fDepth  = g_BBoxSphere.vMax.z - g_BBoxSphere.vMin.z;
    D3DXCreateBox(g_pd3dDevice, fWidth, fHeight, fDepth, &g_pMeshBBoxSphere, NULL);

    fWidth  = g_BBoxTeapot.vMax.x - g_BBoxTeapot.vMin.x;
    fHeight = g_BBoxTeapot.vMax.y - g_BBoxTeapot.vMin.y;
    fDepth  = g_BBoxTeapot.vMax.z - g_BBoxTeapot.vMin.z;
    D3DXCreateBox(g_pd3dDevice, fWidth, fHeight, fDepth, &g_pMeshBBoxTeapot, NULL);

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

    // ��������
    g_MtrlSphere.Ambient  = D3DXCOLOR(1.0f, 0.2f, 0.0f, 1.0f);
    g_MtrlSphere.Diffuse  = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);

    g_MtrlTeapot.Ambient  = D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f);
    g_MtrlTeapot.Diffuse  = D3DXCOLOR(0.8f, 0.8f, 0.0f, 1.0f);

    g_MtrlBBox.Ambient  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 0.5f);
    g_MtrlBBox.Diffuse  = D3DXCOLOR(1.0f, 1.0f, 1.0f, 0.5f);

    // ��������ʼ�����������
    g_pCamera = new CCamera(g_pd3dDevice);
    g_pCamera->ResetCameraPos(&D3DXVECTOR3(-0.2f, 0.0f, -12.0f));
    g_pCamera->ResetLookatPos(&D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    g_pCamera->ResetViewMatrix();
    g_pCamera->ResetProjMatrix();

    // ���ù���
    D3DLIGHT9 light;
    ::ZeroMemory(&light, sizeof(light));
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Ambient   = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
    light.Diffuse   = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    light.Direction = D3DXVECTOR3(1.0f, 0.0f, 1.0f);
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
VOID Direct3DRender(HWND hWnd, FLOAT fTimeDelta) 
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->BeginScene();                     // ��ʼ����

    // ���״̬��Ϣ
    RECT rect;
    GetClientRect(hWnd, &rect);
    rect.top += 5;
    g_pTextStats->DrawText(NULL, L"AppName: CollisionTest", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
    rect.top += 20;
    g_pTextStats->DrawText(NULL, L"Screen backbuffer(640X480),X8R8G8B8(D24X8)", -1, &rect, 
        DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));

    // ��ȡ��������
    ::ZeroMemory(g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));
    ReadDevice(g_pKeyboardDevice, (LPVOID)g_pKeyStateBuffer, sizeof(g_pKeyStateBuffer));

    // ���������������ת�ӽ�
    if (g_pKeyStateBuffer[DIK_W] & 0x80) g_pCamera->CircleRotationX( 0.001f);
    if (g_pKeyStateBuffer[DIK_S] & 0x80) g_pCamera->CircleRotationX(-0.001f);
    if (g_pKeyStateBuffer[DIK_A] & 0x80) g_pCamera->CircleRotationY(-0.001f);
    if (g_pKeyStateBuffer[DIK_D] & 0x80) g_pCamera->CircleRotationY( 0.001f);

    D3DXMATRIX matView;
    g_pCamera->GetViewMatrix(&matView);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // ��������任����
    static FLOAT fOffset   = 5.0f;
    static BOOL  bIncrease = true;
    if (fOffset <= -5.0f) bIncrease = true;
    if (fOffset >=  5.0f) bIncrease = false;
    fOffset = bIncrease ? fOffset+0.008f : fOffset-0.01f;

    D3DXMATRIX matSphere, matTeapot;
    D3DXMatrixTranslation(&matSphere, -fOffset, 0.0f, 0.0f);
    D3DXMatrixTranslation(&matTeapot,  fOffset, 0.0f, 0.0f);
   
    // ��ײ���
    D3DXVECTOR3 vMin1 = D3DXVECTOR3(-fOffset, 0.0f, 0.0f) + g_BBoxSphere.vMin;
    D3DXVECTOR3 vMax1 = D3DXVECTOR3(-fOffset, 0.0f, 0.0f) + g_BBoxSphere.vMax;
    D3DXVECTOR3 vMin2 = D3DXVECTOR3( fOffset, 0.0f, 0.0f) + g_BBoxTeapot.vMin;
    D3DXVECTOR3 vMax2 = D3DXVECTOR3( fOffset, 0.0f, 0.0f) + g_BBoxTeapot.vMax;
    if (CollisionTestAABB(vMin1, vMax1, vMin2, vMax2)) bIncrease = true;

    // ��������Ͳ��
    g_pd3dDevice->SetMaterial(&g_MtrlSphere);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matSphere);
    g_pMeshSphere->DrawSubset(0);

    g_pd3dDevice->SetMaterial(&g_MtrlTeapot);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matTeapot);
    g_pMeshTeapot->DrawSubset(0);

    // ����Alpha�ں�, ��������Ӻ�
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    g_pd3dDevice->SetMaterial(&g_MtrlBBox);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matSphere);
    g_pMeshBBoxSphere->DrawSubset(0);

    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matTeapot);
    g_pMeshBBoxTeapot->DrawSubset(0);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

    // ���������Ϣ
    rect.left = 460, rect.top = 80;
    static char strInfo[256] = {0};
    sprintf_s(strInfo, "��������: (%.2f, %.2f, %.2f)", matSphere._41, matSphere._42, matSphere._43);
    g_pTextInfor->DrawTextA(NULL, strInfo, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DXCOLOR(0.6f, 0.6f, 1.0f, 1.0f));

    rect.top += 20;
    D3DXVECTOR3 vCameraPos;
    g_pCamera->GetCameraPos(&vCameraPos);
    sprintf_s(strInfo, "�������: (%.2f, %.2f, %.2f)", matTeapot._41, matTeapot._42, matTeapot._43);
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
    SAFE_RELEASE(g_pMeshSphere);
    SAFE_RELEASE(g_pMeshTeapot);
    SAFE_RELEASE(g_pMeshBBoxSphere);
    SAFE_RELEASE(g_pMeshBBoxTeapot);

    SAFE_DELETE(g_pCamera);
    g_pKeyboardDevice->Unacquire();
    SAFE_RELEASE(g_pKeyboardDevice);
    SAFE_RELEASE(g_pd3dDevice);
}
