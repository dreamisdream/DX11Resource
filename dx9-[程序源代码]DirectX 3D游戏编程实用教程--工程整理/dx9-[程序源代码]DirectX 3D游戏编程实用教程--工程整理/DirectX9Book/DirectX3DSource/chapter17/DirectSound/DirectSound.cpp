//--------------------------------------------------------------------------------------
// File: DirectSound.cpp
//--------------------------------------------------------------------------------------
#include <d3d9.h>                   // D3Dͷ�ļ�
#include <d3dx9.h>                  // D3DX��ͷ�ļ�
#include <stdio.h>
#include <dsound.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

wchar_t *g_pClassName  = L"DirectSound";            // ��������
wchar_t *g_pWindowName = L"DirectSoundӦ��ʾ��";    // ���ڱ�����

LPDIRECTSOUND8          g_pDirectSound      = NULL;
LPDIRECTSOUNDBUFFER     g_pPrimaryBuffer    = NULL;
LPDIRECTSOUNDBUFFER8    g_pSecondaryBuffer  = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice        = NULL; // D3D�豸�ӿ�

// .WAV file header
struct WAVEHEADER
{
    char    riff_sig[4];            // 'RIFF'
    long    waveform_chunk_size;    // 8
    char    wave_sig[4];            // 'WAVE'
    char    format_sig[4];          // 'fmt ' (notice space after)
    long    format_chunk_size;      // 16;
    short   format_tag;             // WAVE_FORMAT_PCM
    short   channels;               // # of channels
    long    sample_rate;            // sampling rate
    long    bytes_per_sec;          // bytes per second
    short   block_align;            // sample block alignment
    short   bits_per_sample;        // bits per second
    char    data_sig[4];            // 'data'
    long    data_size;              // size of waveform data
};

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

    // ����IDirectSound8�ӿ�
    DirectSoundCreate8(NULL, &g_pDirectSound, NULL);
    g_pDirectSound->SetCooperativeLevel(hWnd, DSSCL_NORMAL);

    // ��ȡWAV��Ƶ��Ϣ
    WAVEHEADER wave_header;
    ::ZeroMemory(&wave_header, sizeof(WAVEHEADER));
    FILE* fp = fopen("example.wav", "rb");
    fread((void*)&wave_header, 1, sizeof(WAVEHEADER), fp);

    // ��������Ƶ����͸�����Ƶ����
    WAVEFORMATEX  wave_format;
    ZeroMemory(&wave_format, sizeof(WAVEFORMATEX));
    wave_format.wFormatTag      = WAVE_FORMAT_PCM;
    wave_format.nChannels       = wave_header.channels;
    wave_format.nSamplesPerSec  = wave_header.sample_rate;
    wave_format.wBitsPerSample  = wave_header.bits_per_sample;
    wave_format.nBlockAlign     = wave_format.wBitsPerSample / 8 * wave_format.nChannels;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

    DSBUFFERDESC  ds_buf_desc;
    ZeroMemory(&ds_buf_desc, sizeof(DSBUFFERDESC));
    ds_buf_desc.dwSize          = sizeof(DSBUFFERDESC);
    ds_buf_desc.dwFlags         = DSBCAPS_CTRLVOLUME;
    ds_buf_desc.dwBufferBytes   = wave_header.data_size;
    ds_buf_desc.lpwfxFormat     = &wave_format;

    g_pDirectSound->CreateSoundBuffer(&ds_buf_desc, &g_pPrimaryBuffer, NULL);
    g_pPrimaryBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&g_pSecondaryBuffer);

    // ����WAV��Ƶ����
    char  *ptr1, *ptr2; 
    DWORD size1, size2;

    fseek(fp, sizeof(WAVEHEADER), SEEK_SET);
    g_pSecondaryBuffer->Lock(0, wave_header.data_size, (void**)&ptr1, &size1, (void**)&ptr2, &size2, 0);

    if (ptr1) fread(ptr1, 1, size1, fp);
    if (ptr2) fread(ptr2, 1, size2, fp);

    g_pSecondaryBuffer->Unlock(ptr1, size1, ptr2, size2);
    fclose(fp);

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

    static long lPan = 0, lVolume = DSBVOLUME_MAX;
    if (::GetAsyncKeyState(VK_LEFT) & 0x8000f)
        lPan -= 500, g_pSecondaryBuffer->SetPan(lPan);
    if (::GetAsyncKeyState(VK_RIGHT) & 0x8000f)
        lPan += 500, g_pSecondaryBuffer->SetPan(lPan);

    if (::GetAsyncKeyState(VK_UP) & 0x8000f) 
        lVolume += 500, g_pSecondaryBuffer->SetVolume(lVolume);
    if (::GetAsyncKeyState(VK_DOWN) & 0x8000f) 
        lVolume -= 500, g_pSecondaryBuffer->SetVolume(lVolume);

    if (::GetAsyncKeyState(VK_SPACE) & 0x8000f) 
        g_pSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    if (::GetAsyncKeyState('S') & 0x8000f)
        g_pSecondaryBuffer->Stop();

    g_pd3dDevice->EndScene();                       // ��������
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);  // ��ת
}

//--------------------------------------------------------------------------------------
// Name: Direct3DCleanup()
// Desc: ����Direct3D, ���ͷ�COM�ӿ�
//--------------------------------------------------------------------------------------
VOID Direct3DCleanup() 
{
    SAFE_RELEASE(g_pDirectSound);
    SAFE_RELEASE(g_pd3dDevice);
}
