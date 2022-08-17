//--------------------------------------------------------------------------------------
// File: DirectMusic.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // D3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�
#include <stdio.h>
#include <dsound.h>
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusics.h>
#include <dmusicc.h>
#include <dmusbuff.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"DirectMusic";            // ��������
wchar_t *g_pWindowName = L"DirectMusicӦ��ʾ��";    // ���ڱ�����

IDirectMusicLoader8*        g_pDirectMusicLoader  = NULL;   // DirectMusic������
IDirectMusicSegment8*       g_pDirectMusicSegment = NULL;   // DirectMusic��
IDirectMusicPerformance8*   g_pDirectMusicPerform = NULL;   // DirectMusic������
LPDIRECT3DDEVICE9           g_pd3dDevice          = NULL;   // D3D�豸�ӿ�


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

    // ��ʼ��COM
    CoInitialize(NULL);

    // ����IDirectMusicPerformace�ӿڶ���
    CoCreateInstance(CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC, 
        IID_IDirectMusicPerformance8, (void**)&g_pDirectMusicPerform);

    // ����DirectMusic����������
    CoCreateInstance(CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, 
        IID_IDirectMusicLoader8, (void**)&g_pDirectMusicLoader);

    // ��ʼ��������
    g_pDirectMusicPerform->InitAudio(NULL, NULL, hWnd, 
        DMUS_APATH_SHARED_STEREOPLUSREVERB, 128, DMUS_AUDIOF_ALL, NULL);

    // ����Ĭ������·��
    wchar_t path[MAX_PATH];
    ::GetCurrentDirectory(MAX_PATH, path);
    g_pDirectMusicLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, path, FALSE);

    // ����MIDI�ļ�
    DMUS_OBJECTDESC dm_obj_desc;
    ZeroMemory(&dm_obj_desc, sizeof(DMUS_OBJECTDESC));
    dm_obj_desc.dwSize      = sizeof(DMUS_OBJECTDESC);
    dm_obj_desc.guidClass   = CLSID_DirectMusicSegment;
    dm_obj_desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
    ::lstrcpy(dm_obj_desc.wszFileName, L"example.mid");

    g_pDirectMusicLoader->GetObject(&dm_obj_desc, IID_IDirectMusicSegment, (void**)&g_pDirectMusicSegment);

    // MIDI��������
    g_pDirectMusicSegment->SetParam(GUID_StandardMIDIFile, 0xFFFFFFFF, 0, 0, NULL);
    g_pDirectMusicSegment->Download(g_pDirectMusicPerform);
    g_pDirectMusicSegment->SetRepeats(DMUS_SEG_REPEAT_INFINITE);

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

    if (::GetAsyncKeyState(VK_SPACE) & 0x8000f)
        g_pDirectMusicPerform->PlaySegment(g_pDirectMusicSegment, 0, 0, NULL);
    if (::GetAsyncKeyState('S') & 0x8000f)
        g_pDirectMusicPerform->Stop(g_pDirectMusicSegment, NULL, 0, 0);

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    g_pDirectMusicSegment->Unload(g_pDirectMusicPerform);   // ж��MIDI����
    g_pDirectMusicPerform->CloseDown();                     // �ر�����������
    SAFE_RELEASE(g_pDirectMusicLoader);                     // �ͷ�MIDI��
    SAFE_RELEASE(g_pDirectMusicSegment);                    // �ͷ�DirectMusic������
    SAFE_RELEASE(g_pDirectMusicPerform);                    // �ͷ�����������
    SAFE_RELEASE(g_pd3dDevice);
    CoUninitialize();                                       // �ر�COM
}
