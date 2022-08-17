//--------------------------------------------------------------------------------------
// File: HelloWorld.cpp
//--------------------------------------------------------------------------------------
#include <windows.h>                        // Ӧ�ó���ͷ�ļ�

wchar_t *g_pClassName = L"HelloWorld";     // ��������
wchar_t *g_pWindowName = L"�ҵĴ���";       // ���ڱ�����

// ������Ϣ����������
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: WindowsӦ�ó�����ں���
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd) {

    // ��ʼ��������
    WNDCLASS wndclass;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // ���ڱ���
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);               // �����״
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);             // ����Сͼ��
    wndclass.hInstance = hInstance;
    wndclass.lpfnWndProc = WndProc;
    wndclass.lpszClassName = g_pClassName;
    wndclass.lpszMenuName = NULL;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;

    // ע�ᴰ����
    if (!RegisterClass(&wndclass))
        return 0;

    // ��������
    HWND hWnd = CreateWindow(
        g_pClassName,               // ��������
        g_pWindowName,              // ���ڱ���
        WS_OVERLAPPEDWINDOW,        // ������ʽ
        CW_USEDEFAULT,              // ���������xλ��
        CW_USEDEFAULT,              // ���������yλ��
        CW_USEDEFAULT,              // ���ڵĿ��
        CW_USEDEFAULT,              // ���ڵĸ߶�
        NULL,                       // �����ھ��
        NULL,                       // ���ڲ˵����
        hInstance,                  // Ӧ�ó���ʵ�����
        NULL);                      // �������ڵĲ���

    ShowWindow(hWnd, nShowCmd);     // ��ʾ����
    UpdateWindow(hWnd);             // ���´���

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))  // ȡ����Ϣ
    {
        TranslateMessage(&msg);           // ת����Ϣ
        DispatchMessage(&msg);            // Ͷ����Ϣ
    }

    UnregisterClass(g_pClassName, wndclass.hInstance);
    return 0;
}

//--------------------------------------------------------------------------------------
// Name: WndProc()
// Desc: ������Ϣ������
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
        case WM_LBUTTONDOWN:            // ������������Ϣ
            MessageBox(NULL, L"��ӭ�߽�Direct3D!", L"�ʺ�", 0);
            break;
        case WM_DESTROY:                // ����������Ϣ
            PostQuitMessage(0);         // �˳�����
            break;
    }
    // Ĭ�ϵ���Ϣ����
    return DefWindowProc(hWnd, message, wParam, lParam);
}
