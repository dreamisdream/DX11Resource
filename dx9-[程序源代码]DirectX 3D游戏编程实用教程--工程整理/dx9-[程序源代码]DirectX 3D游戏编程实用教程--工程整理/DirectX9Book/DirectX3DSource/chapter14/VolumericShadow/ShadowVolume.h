#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

class CShadowVolume 
{
private:
    LPDIRECT3DDEVICE9       m_pd3dDevice;       // D3D�豸�ӿ�
    LPDIRECT3DVERTEXBUFFER9 m_pVertexBuf;
    LPD3DXMESH              m_pShadowMesh;      // ��Ӱ������ģ��
    
    DWORD                   m_dwVertices;       // ��Ӱ���ʵ�ʶ�����
    D3DXVECTOR3*            m_pvVertices;       // ��Ӱ��Ķ���

    struct SHADOWVERTEX 
    {
        FLOAT x, y, z, rhw;
        D3DCOLOR diffuse;
        SHADOWVERTEX(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _rhw, D3DCOLOR _diffuse) 
            : x(_x), y(_y), z(_z), rhw(_rhw), diffuse(_diffuse) {}
        static const DWORD FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
    };

public:
    CShadowVolume(IDirect3DDevice9 *pd3dDevice);
    virtual ~CShadowVolume();

    HRESULT CreateShadowVolume(LPD3DXMESH  pMesh);
    HRESULT UpdateShadowVolume(D3DXVECTOR3 vLight);
    HRESULT RenderShadowVolume(BOOL bRenderVolume);

protected:
    VOID AddFaceEdge(WORD* pEdges, DWORD& dwNumEdges, WORD v0, WORD v1);
};
