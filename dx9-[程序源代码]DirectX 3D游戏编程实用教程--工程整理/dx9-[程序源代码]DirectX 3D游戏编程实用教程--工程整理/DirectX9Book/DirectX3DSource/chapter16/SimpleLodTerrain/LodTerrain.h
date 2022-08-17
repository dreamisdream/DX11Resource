#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <list>
#include <vector>
#include "Frustum.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//--------------------------------------------------------------------------------------
// Name: struct QuadTreeNode
// Desc: 
//--------------------------------------------------------------------------------------
struct QuadTreeNode 
{
    INT             m_nCenterX;
    INT             m_nCenterY;
    UINT            m_nNodeSize;        // node size(width and height)
    UINT            m_nNodeLevel;       // the node's level in quad tree

    QuadTreeNode*   m_pParentNode;
    QuadTreeNode*   m_pTLChildNode;
    QuadTreeNode*   m_pBLChildNode;
    QuadTreeNode*   m_pTRChildNode;
    QuadTreeNode*   m_pBRChildNode;

    BOOL            m_bIsLeaf;
    BOOL            m_bIsVisible;
    BOOL            m_bIsTouched;
    BOOL            m_bIsReached;

    VOID SplitNode();
};


//--------------------------------------------------------------------------------------
// Name: class CLodTerrain
// Desc: 
//--------------------------------------------------------------------------------------
class CLodTerrain 
{
protected:
    INT                     m_nTerrSize;    // ���εĳߴ�(�����εı߳�)
    FLOAT                   m_fCellWidth;   // ��Ԫ��Ŀ��
    FLOAT                   m_fCellHeight;  // ��Ԫ��ĸ߶�
    QuadTreeNode*           m_pRootNode;    // ���ڵ�
    std::vector<FLOAT>      m_vHeightInfo;  // �߶�����

    struct TERRAINVERTEX 
    {
        FLOAT _x, _y, _z;
        static const DWORD FVF = D3DFVF_XYZ;
    };

    INT                     m_nVBufIndex;
    INT                     m_nIBufIndex;
    INT                     m_nRenderNodes;
    WORD*                   m_pIndicesPtr;
    TERRAINVERTEX*          m_pVerticesPtr;
    LPDIRECT3DDEVICE9       m_pd3dDevice;
    LPDIRECT3DINDEXBUFFER9  m_pIndexBuffer;
    LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;

    CFrustum*               m_pFrustum;

public:
    QuadTreeNode*  GetHeadNode() { return m_pRootNode;    }
    INT  GetRenderNodes(void)    { return m_nRenderNodes; }

public:
    CLodTerrain(IDirect3DDevice9 *pd3dDevice);
    virtual ~CLodTerrain(void);

    BOOL InitLodTerrain(FLOAT fWidth, FLOAT fHeight, FLOAT fScale, wchar_t *pRawFileName);
    BOOL UpdateLodTerrain(QuadTreeNode *pNode);         // ����LOD����
    BOOL RenderLodTerrain(D3DXMATRIX *pMatWorld);       // ����LOD����

protected:
    BOOL BuildQuadTree(QuadTreeNode *pNode);            // �����Ĳ���
    BOOL IsNodeVisible(QuadTreeNode *pNode);            // �ڵ��Ƿ�λ���ӽ�����
    BOOL IsNodeReached(QuadTreeNode *pNode);            // �ڵ��Ƿ�����ڵ��׼��
    BOOL SimplifyNodes(QuadTreeNode *pNode);            // �򻯽ڵ�
    BOOL GetNodeVertex(QuadTreeNode *pNode);            // ȡ�����пɴ�ڵ�Ķ���
    VOID ResetNodeFlag(QuadTreeNode *pNode);            // ����ڵ�ı�־λ
    VOID FreeTreeNodes(QuadTreeNode *pNode);
};
