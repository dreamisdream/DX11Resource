//--------------------------------------------------------------------------------------
// File: GameClient.cpp
//--------------------------------------------------------------------------------------
#include <stdio.h>
#include <dplay8.h>
#include <dpaddr.h>
#include "resource.h"
#include <map>
#include <string>

#include <list>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dplayx.lib")

#define SAFE_DELETE(p)  { if(p) { delete p; (p)=NULL;       } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// 应用程序GUID
GUID g_AppGuid = { 0xababbe60, 0x1ac0, 0x11d5, { 0x90, 0x89, 0x44, 0x45, 0x53, 0x54, 0x0, 0x1 } };

HWND    g_hMainWnd      = NULL;
BOOL    g_bConnected    = FALSE;
std::list<DPNMSG_ENUM_HOSTS_RESPONSE> g_lsHostAddr;

IDirectPlay8Client*         g_pClientPlayer = NULL;
IDirectPlay8Address*        g_pHostAddress  = NULL;
PDPN_SERVICE_PROVIDER_INFO  g_pAdapterList  = NULL;
DPNHANDLE                   g_hAsyncHandle  = NULL;

// 窗口消息处理函数声明
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// 客户端信息处理回调函数
HRESULT WINAPI ClientMsgHandler(PVOID, DWORD, PVOID);

//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: Windows应用程序入口函数
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // 初始化COM
    ::CoInitialize(NULL);

    // 创建对话框
    HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, (DLGPROC)DlgProc);
    g_hMainWnd = hWnd;

    // 显示、更新对话框
    ShowWindow(hWnd, SW_SHOW); 
    UpdateWindow(hWnd); 

    // 消息循环
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message!=WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ::CoUninitialize();             // 关闭COM
    return 0;
}

//--------------------------------------------------------------------------------------
// Name: DlgProc()
// Desc: 对话框消息处理函数
//--------------------------------------------------------------------------------------
LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message)
    {
      case WM_INITDIALOG:
      {
          // 创建DirectPlay客户端对象
          if (FAILED(CoCreateInstance(CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC, IID_IDirectPlay8Client, (void**)&g_pClientPlayer)))
              return FALSE;

          // 初始化客户端对象
          if (FAILED(g_pClientPlayer->Initialize(NULL, ClientMsgHandler, 0)))
              return FALSE;

          // 枚举服务提供者
          DWORD dwListSize = 0, dwNumAdapters  = 0;
          g_pClientPlayer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, g_pAdapterList, &dwListSize, &dwNumAdapters, 0);

          g_pAdapterList = (PDPN_SERVICE_PROVIDER_INFO) new BYTE[dwListSize];
          g_pClientPlayer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, g_pAdapterList, &dwListSize, &dwNumAdapters, 0);

          // 设置按钮状态
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDMSG),      FALSE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDSESSION),  FALSE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_CLOSESESSION), FALSE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_JOINSESSION),  TRUE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME),   TRUE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_PASSWORD),     TRUE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_ADDRESS),      TRUE);
          EnableWindow(GetDlgItem(g_hMainWnd, IDC_PORT),         TRUE);
          return TRUE;
      }
      case WM_COMMAND:
      {
          if (LOWORD(wParam) == IDC_JOINSESSION)
          {
              char szPlayerName[256] = {0};
              char szPassword[256]   = {0};
              char szAddress[32]     = {0};
              char szPort[32]        = {0};
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME), szPlayerName, 256);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PASSWORD),   szPassword,   256);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_ADDRESS),    szAddress,    256);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PORT),       szPort,       256);

              // 创建DirectPlay客户端对象
              if (FAILED(CoCreateInstance(CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC, IID_IDirectPlay8Client, (void**)&g_pClientPlayer)))
                  return FALSE;

              // 初始化客户端对象
              if (FAILED(g_pClientPlayer->Initialize(NULL, ClientMsgHandler, 0)))
                  return FALSE;

              // 设置玩家信息
              wchar_t wszPlayerName[256] = {0};
              mbstowcs(wszPlayerName, szPlayerName, 256);

              DPN_PLAYER_INFO player_info;
              ZeroMemory(&player_info, sizeof(DPN_PLAYER_INFO));
              player_info.dwSize      = sizeof(DPN_PLAYER_INFO);
              player_info.dwInfoFlags = DPNINFO_NAME | DPNINFO_DATA;
              player_info.pwszName    = wszPlayerName;
              g_pClientPlayer->SetClientInfo(&player_info, NULL, NULL, DPNSETCLIENTINFO_SYNC);

              // 创建地址对象
              IDirectPlay8Address *pHostAddress   = NULL;
              IDirectPlay8Address *pDeviceAddress = NULL;
              CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC, IID_IDirectPlay8Address, (void**)&pHostAddress);
              CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC, IID_IDirectPlay8Address, (void**)&pDeviceAddress);

              // 设置TCP/IP协议
              pHostAddress->SetSP(&CLSID_DP8SP_TCPIP);
              pDeviceAddress->SetSP(&CLSID_DP8SP_TCPIP);

              // 设置端口和IP地址
              DWORD dwPort = atoi(szPort);
              pHostAddress->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(DWORD), DPNA_DATATYPE_DWORD);

              // 设置主机IP地址
              wchar_t wszAddress[128] = {0};
              mbstowcs(wszAddress, szAddress, 128);
              pHostAddress->AddComponent(DPNA_KEY_HOSTNAME, wszAddress, 
                  (DWORD)(lstrlenW(wszAddress)+1) * sizeof(wchar_t), DPNA_DATATYPE_STRING);

              // 设置适配卡为第1块网卡
              pHostAddress->AddComponent(DPNA_KEY_DEVICE, &(g_pAdapterList->guid), sizeof(GUID), DPNA_DATATYPE_GUID);
              pDeviceAddress->AddComponent(DPNA_KEY_DEVICE, &(g_pAdapterList->guid), sizeof(GUID), DPNA_DATATYPE_GUID);

              // 取得连接密码
              wchar_t wszPassword[256]= {0};
              mbstowcs(wszPassword, szPassword, 256);

              // 连接到服务端
              DPN_APPLICATION_DESC app_desc;
              ZeroMemory(&app_desc, sizeof(DPN_APPLICATION_DESC));
              app_desc.dwSize          = sizeof(DPN_APPLICATION_DESC);
              app_desc.dwFlags         = DPNSESSION_CLIENT_SERVER;
              app_desc.guidApplication = g_AppGuid;
              app_desc.pwszSessionName = L"DirectPlayExample";
              if (lstrlenW(wszPassword)) 
              {
                  app_desc.dwFlags      |= DPNSESSION_REQUIREPASSWORD;
                  app_desc.pwszPassword  = wszPassword;
              }

              if (FAILED(g_pClientPlayer->Connect(&app_desc, pHostAddress, pDeviceAddress, NULL, NULL, NULL, 0, NULL, &g_hAsyncHandle, 0)))
                  return FALSE;

              SAFE_RELEASE(pHostAddress);
              SAFE_RELEASE(pDeviceAddress);
          }
          if (LOWORD(wParam) == IDC_SENDSESSION)
          {
              char name[256], msg[256];
              char message[256] = {0};
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME), name, 256);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_SENDMSG), msg, 256);
              sprintf(message, "%s: %s", name, msg);

              DPN_BUFFER_DESC buffer_desc;
              ::ZeroMemory(&buffer_desc, sizeof(DPN_BUFFER_DESC));
              buffer_desc.dwBufferSize = strlen(message)+1;
              buffer_desc.pBufferData  = (BYTE*)message;

              g_pClientPlayer->Send(&buffer_desc, 1, 0, NULL, &g_hAsyncHandle, DPNSEND_NOLOOPBACK);
              SetWindowText(GetDlgItem(g_hMainWnd, IDC_SENDMSG), "");
          }
          if (LOWORD(wParam) == IDC_CLOSESESSION)
          {
              if (g_pClientPlayer) g_pClientPlayer->Close(0);
              SAFE_RELEASE(g_pClientPlayer);

              // 设置按钮状态
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDMSG),      FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDSESSION),  FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_CLOSESESSION), FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_JOINSESSION),  TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME),   TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_PASSWORD),     TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_ADDRESS),      TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd, IDC_PORT),         TRUE);
          }
          if (LOWORD(wParam) == IDC_QUIT)
          {
              PostMessage(g_hMainWnd, WM_CLOSE, 0, 0);
          }
          break;
      }
      case WM_CLOSE:
      {
          if (g_pClientPlayer) 
              g_pClientPlayer->Close(0);
          SAFE_RELEASE(g_pClientPlayer);
          SAFE_DELETE(g_pAdapterList);

          DestroyWindow(hWnd);
          PostQuitMessage(0);
          break;
      }
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------
// Name: ClientMsgHandler()
// Desc: 客户端消息回调函数
//--------------------------------------------------------------------------------------
HRESULT WINAPI ClientMsgHandler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
    if (dwMessageType == DPN_MSGID_ENUM_HOSTS_RESPONSE)
    {
        MessageBox(NULL, "enum hosts response.", 0, 0);
        return S_OK;
    }

    if (dwMessageType == DPN_MSGID_CONNECT_COMPLETE)
    {
        PDPNMSG_CONNECT_COMPLETE pConnectComplete = (PDPNMSG_CONNECT_COMPLETE)pMessage;
        g_bConnected = (pConnectComplete->hResultCode == DPN_OK);

        // 设置按钮状态
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDMSG),      g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDSESSION),  g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_CLOSESESSION), g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_JOINSESSION),  !g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME),   !g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PASSWORD),     !g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_ADDRESS),      !g_bConnected);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PORT),         !g_bConnected);
        return S_OK;
    }

    if (dwMessageType == DPN_MSGID_TERMINATE_SESSION)
    {
        PDPNMSG_TERMINATE_SESSION pTerminateSession = (PDPNMSG_TERMINATE_SESSION)pMessage;
        g_pClientPlayer->Close(0);

        // 设置按钮状态
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDMSG),      FALSE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_SENDSESSION),  FALSE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_CLOSESESSION), FALSE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_JOINSESSION),  TRUE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PLAYERNAME),   TRUE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PASSWORD),     TRUE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_ADDRESS),      TRUE);
        EnableWindow(GetDlgItem(g_hMainWnd, IDC_PORT),         TRUE);

        MessageBox(NULL, "服务器断开连接!", "提示", 0);
        return DPN_OK;
    }

    if (dwMessageType == DPN_MSGID_RECEIVE)
    {
        PDPNMSG_RECEIVE pReceiveData = (PDPNMSG_RECEIVE) pMessage;
        SendMessage(GetDlgItem(g_hMainWnd, IDC_RECVMSG), LB_ADDSTRING, 0, (LPARAM)pReceiveData->pReceiveData);
        return S_OK;
    }
    return S_OK;
}
