#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <fstream>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//--------------------------------------------------------------------------------------
// Name: class CTerrain
// Desc: ��ά����
//--------------------------------------------------------------------------------------
class CTerrain
{
private:
    LPDIRECT3DDEVICE9       m_pd3dDevice;
    LPDIRECT3DTEXTURE9      m_pTexture;
    LPDIRECT3DINDEXBUFFER9  m_pIndexBuf;
    LPDIRECT3DVERTEXBUFFER9 m_pVertexBuf;

    INT                     m_nCellsPerRow;     // ÿ�е�Ԫ����
    INT                     m_nCellsPerCol;     // ÿ�е�Ԫ����
    INT                     m_nVertsPerRow;     // ÿ�ж�����
    INT                     m_nVertsPerCol;     // ÿ�ж�����
    INT                     m_nNumVertices;     // ��������
    FLOAT                   m_fTerrainWidth;    // ���εĿ��
    FLOAT                   m_fTerrainDepth;    // ���ε����
    FLOAT                   m_fCellSpacing;     // ��Ԫ��ļ��
    FLOAT                   m_fHeightScale;     // �߶�����ϵ��
    std::vector<FLOAT>      m_vHeightInfo;      // �߶���Ϣ

    struct TERRAINVERTEX
    {
        FLOAT _x, _y, _z;
        FLOAT _u, _v;
        TERRAINVERTEX(FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT v) 
            :_x(x), _y(y), _z(z), _u(u), _v(v) {}

        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
    };

public:
    CTerrain(IDirect3DDevice9 *pd3dDevice);
    virtual ~CTerrain(void);

public:
    BOOL LoadTerrain(wchar_t *pRawFileName, wchar_t *pTextureFile);
    BOOL InitTerrain(INT nRows, INT nCols, FLOAT fSpace, FLOAT fScale);
    BOOL DrawTerrain(D3DXMATRIX *pMatWorld, BOOL bDrawFrame=FALSE);
};
