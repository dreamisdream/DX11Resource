#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//--------------------------------------------------------------------------------------
// Name: class CSkybox
// Desc: ���ε���
//--------------------------------------------------------------------------------------
class CSkybox
{
private:
    LPDIRECT3DDEVICE9       m_pd3dDevice;
    LPDIRECT3DTEXTURE9      m_pTexture;
    LPDIRECT3DINDEXBUFFER9  m_pIndexBuf;
    LPDIRECT3DVERTEXBUFFER9 m_pVertexBuf;

    INT     m_nNumLatitudes;
    INT     m_nNumLongitudes;
    INT     m_nVertsPerLati;
    INT     m_nVertsPerLongi;
    INT     m_nNumVertices;     // ������
    FLOAT   m_fSkyboxRadius;    // �뾶

    struct SKYBOXVERTEX
    {
        FLOAT _x, _y, _z;
        FLOAT _u, _v;
        SKYBOXVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v) 
            : _x(x), _y(y), _z(z), _u(u), _v(v) {}
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
    };

public:
    CSkybox(IDirect3DDevice9 *pd3dDevice);
    virtual ~CSkybox(void);

public:
    BOOL LoadSkybox(wchar_t *pTextureFile);
    BOOL InitSkybox(INT nAlpha, INT nBeta, FLOAT nRadius);
    BOOL DrawSkybox(D3DXMATRIX *pMatWorld, BOOL bDrawFrame=FALSE);
};
