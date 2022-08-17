//--------------------------------------------------------------------------------------
// File: GameServer.cpp
//--------------------------------------------------------------------------------------
#include <stdio.h>          // ��׼I/Oͷ�ļ�
#include <dplay8.h>         // DirectPlayͷ�ļ�
#include <dpaddr.h>         // DirectPlayͷ�ļ�
#include "resource.h"       // ��Դͷ�ļ�
#include <map>              // ��׼ģ���(STL)�е�ӳ��ͷ�ļ�
#include <string>           // STL���ַ�����ͷ�ļ�

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dplayx.lib")

#define SAFE_DELETE(p)  { if(p) { delete p; (p)=NULL;       } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

HWND    g_hMainWnd = NULL;
GUID    g_AppGuid  = { 0xababbe60, 0x1ac0, 0x11d5, { 0x90, 0x89, 0x44, 0x45, 0x53, 0x54, 0x0, 0x1 } };

IDirectPlay8Server*             g_pServerPlayer = NULL;
PDPN_SERVICE_PROVIDER_INFO      g_pAdapterList  = NULL;
DPNHANDLE                       g_hAsyncHandle  = NULL;
std::map<DPNID, std::string>    g_mapClients;

// ������Ϣ����������
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// �������Ϣ����ص�����
HRESULT WINAPI ServerMsgHandler(PVOID, DWORD, PVOID);

//--------------------------------------------------------------------------------------
// Name: WinMain();
// Desc: WindowsӦ�ó�����ں���
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // ��ʼ��COM
    ::CoInitialize(NULL);

    // �����Ի���
    HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, (DLGPROC)DlgProc);
    g_hMainWnd = hWnd;


    // ��ʾ�����¶Ի���
    ShowWindow(hWnd, SW_SHOW); 
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
    }

    ::CoUninitialize();             // �ر�COM
    return 0;
}


//--------------------------------------------------------------------------------------
// Name: DlgProc()
// Desc: �Ի�����Ϣ������
//--------------------------------------------------------------------------------------
LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message)
    {
      case WM_INITDIALOG:
      {
          // ����DirectPlay����˶���
          if (FAILED(CoCreateInstance(CLSID_DirectPlay8Server, NULL, CLSCTX_INPROC, IID_IDirectPlay8Server, (void**)&g_pServerPlayer)))
              return FALSE;

          // ��ʼ������˶���
          if (FAILED(g_pServerPlayer->Initialize(NULL, ServerMsgHandler, 0)))
              return FALSE;

          // ö�ٷ����ṩ��
          DWORD dwListSize = 0, dwNumAdapters  = 0;
          g_pServerPlayer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, g_pAdapterList, &dwListSize, &dwNumAdapters, 0);

          g_pAdapterList = (PDPN_SERVICE_PROVIDER_INFO) new BYTE[dwListSize];
          g_pServerPlayer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, g_pAdapterList, &dwListSize, &dwNumAdapters, 0);
          return TRUE;
      }
      case WM_COMMAND:
      {
          if (LOWORD(wParam) == IDC_CREATESESSION)
          {
              // ȡ��������Ϣ
              char szHostName[256]  = {0};
              char szPort[32]       = {0};
              char szPassword[32]   = {0};
              char szMaxPlayers[32] = {0};
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_HOSTNAME),   szHostName,  256);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PORT),       szPort,       32);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_PASSWORD),   szPassword,   32);
              GetWindowText(GetDlgItem(g_hMainWnd, IDC_MAXPLAYERS), szMaxPlayers, 32);

              // ����DirectPlay����˶���
              if (FAILED(CoCreateInstance(CLSID_DirectPlay8Server, NULL, CLSCTX_INPROC, IID_IDirectPlay8Server, (void**)&g_pServerPlayer)))
                  return FALSE;

              // ��ʼ������˶���
              if (FAILED(g_pServerPlayer->Initialize(NULL, ServerMsgHandler, 0)))
                  return FALSE;

              // ����DirectPlay��ַ����
              IDirectPlay8Address *pAddress = NULL;
              if (FAILED(CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC, IID_IDirectPlay8Address, (void**)&pAddress)))
                  return FALSE;

              // ����Э��ΪTCP/IP
              if (FAILED(pAddress->SetSP(&CLSID_DP8SP_TCPIP)))
                  return pAddress->Release(), FALSE;

              // ���ö˿ں�
              DWORD dwPort = atoi(szPort);
              if (FAILED(pAddress->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(DWORD), DPNA_DATATYPE_DWORD)))
                  return pAddress->Release(), FALSE;

              // ����������
              if (FAILED(pAddress->AddComponent(DPNA_KEY_DEVICE, &g_pAdapterList->guid, sizeof(GUID), DPNA_DATATYPE_GUID)))
                  return pAddress->Release(), FALSE;

              // ����������Ϣ
              wchar_t wszHostName[256] = {0};
              mbstowcs(wszHostName, szHostName, 256);

              DPN_PLAYER_INFO player_info;
              ZeroMemory(&player_info, sizeof(DPN_PLAYER_INFO));
              player_info.dwSize       = sizeof(DPN_PLAYER_INFO);
              player_info.dwInfoFlags  = DPNINFO_NAME | DPNINFO_DATA;
              player_info.pwszName     = wszHostName;
              g_pServerPlayer->SetServerInfo(&player_info, NULL, NULL, DPNSETCLIENTINFO_SYNC);

              // ����һ���»Ự
              wchar_t wszPassword[256] = {0};
              mbstowcs(wszPassword, szPassword, 256);

              DPN_APPLICATION_DESC app_desc;
              ZeroMemory(&app_desc, sizeof(DPN_APPLICATION_DESC));
              app_desc.dwSize          = sizeof(DPN_APPLICATION_DESC);
              app_desc.dwFlags         = DPNSESSION_CLIENT_SERVER;
              app_desc.guidApplication = g_AppGuid;
              app_desc.pwszSessionName = L"DirectPlayExample";
              app_desc.dwMaxPlayers    = atoi(szMaxPlayers);
              if (strlen(szPassword)) 
              {
                  app_desc.dwFlags      |= DPNSESSION_REQUIREPASSWORD;
                  app_desc.pwszPassword  = wszPassword;
              }
              
              if (FAILED(g_pServerPlayer->Host(&app_desc, &pAddress, 1, NULL, NULL, NULL, 0)))
                  return pAddress->Release(), FALSE;
              pAddress->Release();

              // ���ð�ť״̬
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CLOSECONNECT),   TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CLOSESESSION),  TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CREATESESSION), FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_PORT),          FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_HOSTNAME),      FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_PASSWORD),      FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_MAXPLAYERS),    FALSE);
          }
          if (LOWORD(wParam) == IDC_CLOSESESSION)
          {
              if (g_pServerPlayer) g_pServerPlayer->Close(0);

              // ���ð�ť״̬
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CLOSECONNECT),   FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CLOSESESSION),  FALSE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_CREATESESSION), TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_PORT),          TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_HOSTNAME),      TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_PASSWORD),      TRUE);
              EnableWindow(GetDlgItem(g_hMainWnd,IDC_MAXPLAYERS),    TRUE);
          }
          if (LOWORD(wParam) == IDC_CLOSECONNECT)
          {
              int nSelect = (int)SendMessage(GetDlgItem(g_hMainWnd, IDC_PLAYERS), LB_GETCURSEL, 0, 0);
              if (nSelect == LB_ERR) break;

              char szPlayerName[256] = {0};
              SendMessage(GetDlgItem(g_hMainWnd, IDC_PLAYERS), LB_GETTEXT, nSelect, (LPARAM)szPlayerName);
              std::string strPlayerName = szPlayerName;

              std::map<DPNID, std::string>::iterator i;
              for (i = g_mapClients.begin(); i != g_mapClients.end(); i++)
                  if (i->second.compare(szPlayerName) == 0) break;
              if (i == g_mapClients.end()) break;

              g_pServerPlayer->DestroyClient(i->first, NULL, 0, 0);
              g_mapClients.erase(i);
          }
          if (LOWORD(wParam) == IDC_QUIT)
          {
              PostMessage(g_hMainWnd, WM_CLOSE, 0, 0);
          }
          break;
      }
      case WM_CLOSE:
      {
          if (g_pServerPlayer) 
              g_pServerPlayer->Close(0);
          SAFE_RELEASE(g_pServerPlayer);
          SAFE_DELETE(g_pAdapterList);

          DestroyWindow(hWnd);
          PostQuitMessage(0);
          break;
      }
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------
// Name: ServerMsgHandler()
// Desc: �������Ϣ�ص�����
//--------------------------------------------------------------------------------------
HRESULT WINAPI ServerMsgHandler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
    if (dwMessageType == DPN_MSGID_CREATE_PLAYER)
    {
        PDPNMSG_CREATE_PLAYER pCreatePlayer = (PDPNMSG_CREATE_PLAYER)pMessage;

        // ȡ�������Ϣ
        DWORD dwInfoSize = 0;
        if (g_pServerPlayer->GetClientInfo(pCreatePlayer->dpnidPlayer, NULL, &dwInfoSize, 0) == DPNERR_INVALIDPLAYER)
            return DPN_OK;

        PDPN_PLAYER_INFO pPlayerInfor = (PDPN_PLAYER_INFO) new BYTE[dwInfoSize];
        ::ZeroMemory(pPlayerInfor, dwInfoSize);
        pPlayerInfor->dwSize = sizeof(DPN_PLAYER_INFO);

        if (FAILED(g_pServerPlayer->GetClientInfo(pCreatePlayer->dpnidPlayer, pPlayerInfor, &dwInfoSize, 0)))
            return (delete pPlayerInfor), E_FAIL;

        // ��ӵ��б��
        char name[256] = {0}, message[256] = {0};
        wcstombs(name, pPlayerInfor->pwszName, 256);
        sprintf(message, "%s: ������Ϸ", name);
        SendMessage(GetDlgItem(g_hMainWnd, IDC_PLAYERS), LB_ADDSTRING, 0, (LPARAM)name);

        // ��ӵ�ӳ����
        wcstombs(name, pPlayerInfor->pwszName, 256);
        g_mapClients.insert(std::pair<DPNID, std::string>(pCreatePlayer->dpnidPlayer, name));

        // ֪ͨ�������
        DPN_BUFFER_DESC buffer_desc;
        ::ZeroMemory(&buffer_desc, sizeof(DPN_BUFFER_DESC));
        buffer_desc.dwBufferSize = strlen(message)+1;
        buffer_desc.pBufferData  = (BYTE*)message;
        g_pServerPlayer->SendTo(DPNID_ALL_PLAYERS_GROUP, &buffer_desc, 1, 0, NULL, &g_hAsyncHandle, DPNSEND_NOLOOPBACK);
        return S_OK;
    }

    if (dwMessageType == DPN_MSGID_DESTROY_PLAYER)
    {
        PDPNMSG_DESTROY_PLAYER pDestroyPlayer = (PDPNMSG_DESTROY_PLAYER)pMessage;

        // ȡ�������Ϣ
        DWORD dwInfoSize = 0;
        if (g_pServerPlayer->GetClientInfo(pDestroyPlayer->dpnidPlayer, NULL, &dwInfoSize, 0) == DPNERR_INVALIDPLAYER)
            return DPN_OK;

        PDPN_PLAYER_INFO pPlayerInfor = (PDPN_PLAYER_INFO) new BYTE[dwInfoSize];
        ::ZeroMemory(pPlayerInfor, dwInfoSize);
        pPlayerInfor->dwSize = sizeof(DPN_PLAYER_INFO);
        if (FAILED(g_pServerPlayer->GetClientInfo(pDestroyPlayer->dpnidPlayer, pPlayerInfor, &dwInfoSize, 0)))
            return (delete pPlayerInfor), E_FAIL;

        // ���б����ɾ�����
        char name[256] = {0}, message[256] = {0};
        wcstombs(name, pPlayerInfor->pwszName, 256);
        sprintf(message, "%s: �뿪��Ϸ", name);

        int index = (int)SendMessage(GetDlgItem(g_hMainWnd, IDC_PLAYERS), LB_FINDSTRING, -1, (LPARAM)name);
        if (index != LB_ERR) SendMessage(GetDlgItem(g_hMainWnd, IDC_PLAYERS), LB_DELETESTRING, index, 0);

        // ֪ͨ�������
        DPN_BUFFER_DESC buffer_desc;
        ::ZeroMemory(&buffer_desc, sizeof(DPN_BUFFER_DESC));
        buffer_desc.dwBufferSize = strlen(message)+1;
        buffer_desc.pBufferData  = (BYTE*)message;
        g_pServerPlayer->SendTo(DPNID_ALL_PLAYERS_GROUP, &buffer_desc, 1, 0, NULL, &g_hAsyncHandle, DPNSEND_NOLOOPBACK);
        return S_OK;
    }

    if (dwMessageType == DPN_MSGID_RECEIVE)
    {
        PDPNMSG_RECEIVE pReceiveData = (PDPNMSG_RECEIVE)pMessage;

        // ��ʾ��Ϣ
        SendMessage(GetDlgItem(g_hMainWnd, IDC_RECVMSG), LB_ADDSTRING, 0, (LPARAM)pReceiveData->pReceiveData);

        // ����Ϣ���͸��������
        DPN_BUFFER_DESC buffer_desc;
        ::ZeroMemory(&buffer_desc, sizeof(DPN_BUFFER_DESC));
        buffer_desc.dwBufferSize = pReceiveData->dwReceiveDataSize;
        buffer_desc.pBufferData  = pReceiveData->pReceiveData;
        g_pServerPlayer->SendTo(DPNID_ALL_PLAYERS_GROUP, &buffer_desc, 1, 0, NULL, &g_hAsyncHandle, DPNSEND_NOLOOPBACK);
        return S_OK;
    }
    return S_OK;
}
