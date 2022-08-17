#include <math.h>
#include <fstream>
#include "LodTerrain.h"

#include "Camera.h"
extern CCamera* g_pCamera;

//--------------------------------------------------------------------------------------
// Name: log2()
// Desc: 
//--------------------------------------------------------------------------------------
int log2(int num) 
{
    int a = 0;
    while (num > 0) 
        num = num>>1, a++;
    return a - 1;
}

//--------------------------------------------------------------------------------------
// Name: QuadTreeNode::SplitNode()
// Desc: 
//--------------------------------------------------------------------------------------
VOID QuadTreeNode::SplitNode() 
{
    if (m_nNodeLevel == 0) return;
    UINT nSizeDivide2 = m_nNodeSize >> 1;
    UINT nSizeDivide4 = m_nNodeSize >> 2;

    // top left
    m_pTLChildNode = new QuadTreeNode();
    m_pTLChildNode->m_pParentNode   = this;
    m_pTLChildNode->m_nCenterX      = m_nCenterX - nSizeDivide4;
    m_pTLChildNode->m_nCenterY      = m_nCenterY - nSizeDivide4;
    m_pTLChildNode->m_nNodeSize     = nSizeDivide2;
    m_pTLChildNode->m_nNodeLevel    = m_nNodeLevel - 1;
    m_pTLChildNode->m_bIsLeaf       = TRUE;

    // top right
    m_pTRChildNode = new QuadTreeNode();
    m_pTRChildNode->m_pParentNode   = this;
    m_pTRChildNode->m_nCenterX      = m_nCenterX + nSizeDivide4;
    m_pTRChildNode->m_nCenterY      = m_nCenterY - nSizeDivide4;
    m_pTRChildNode->m_nNodeSize     = nSizeDivide2;
    m_pTRChildNode->m_nNodeLevel    = m_nNodeLevel - 1;
    m_pTRChildNode->m_bIsLeaf       = TRUE;

    // bottom left
    m_pBLChildNode = new QuadTreeNode();
    m_pBLChildNode->m_pParentNode   = this;
    m_pBLChildNode->m_nCenterX      = m_nCenterX - nSizeDivide4;
    m_pBLChildNode->m_nCenterY      = m_nCenterY + nSizeDivide4;
    m_pBLChildNode->m_nNodeSize     = nSizeDivide2;
    m_pBLChildNode->m_nNodeLevel    = m_nNodeLevel - 1;
    m_pBLChildNode->m_bIsLeaf       = TRUE;

    // bottom right
    m_pBRChildNode = new QuadTreeNode();
    m_pBRChildNode->m_pParentNode   = this;
    m_pBRChildNode->m_nCenterX      = m_nCenterX + nSizeDivide4;
    m_pBRChildNode->m_nCenterY      = m_nCenterY + nSizeDivide4;
    m_pBRChildNode->m_nNodeSize     = nSizeDivide2;
    m_pBRChildNode->m_nNodeLevel    = m_nNodeLevel - 1;
    m_pBRChildNode->m_bIsLeaf       = TRUE;

    // no leaf node any more
    m_bIsLeaf = FALSE;
}


//--------------------------------------------------------------------------------------
// Name: CLodTerrain::CLodTerrain()
// Desc: 
//--------------------------------------------------------------------------------------
CLodTerrain::CLodTerrain(IDirect3DDevice9 *pd3dDevice)
{
    m_pd3dDevice   = pd3dDevice;
    m_nTerrSize    = 0;
    m_fCellWidth   = 0;
    m_fCellHeight  = 0;
    m_nVBufIndex   = 0;
    m_nIBufIndex   = 0;
    m_nRenderNodes = 0;
    m_pRootNode     = NULL;
    m_pIndicesPtr   = NULL;
    m_pVerticesPtr  = NULL;
    m_pVertexBuffer = NULL;
    m_pIndexBuffer  = NULL;

    m_pFrustum = new CFrustum(pd3dDevice);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::~CLodTerrain()
// Desc: 
//--------------------------------------------------------------------------------------
CLodTerrain::~CLodTerrain(void)
{
    SAFE_DELETE(m_pFrustum);
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);
    FreeTreeNodes(m_pRootNode);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::FreeTreeNodes
// Desc: 
//--------------------------------------------------------------------------------------
VOID CLodTerrain::FreeTreeNodes(QuadTreeNode *pNode)
{
    if (pNode == NULL) return;
    FreeTreeNodes(pNode->m_pTLChildNode);
    FreeTreeNodes(pNode->m_pTRChildNode);
    FreeTreeNodes(pNode->m_pBLChildNode);
    FreeTreeNodes(pNode->m_pBRChildNode);
    SAFE_DELETE(pNode);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::InitLodTerrain()
// Desc: 初始化LOD地形
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::InitLodTerrain(FLOAT fWidth, FLOAT fHeight, FLOAT fScale, wchar_t *pRawFile) 
{
    // 读取高度信息
    std::ifstream inFile;
    inFile.open(pRawFile, std::ios::binary);

    inFile.seekg(0, std::ios::end);
    long lMapSize = inFile.tellg();

    inFile.seekg(std::ios::beg);
    std::vector<BYTE> inData(lMapSize);
    inFile.read((char*)&inData[0], inData.size());
    inFile.close();

    m_vHeightInfo.resize(inData.size());
    for (unsigned int i=0; i<inData.size(); i++)
        m_vHeightInfo[i] = inData[i] * fScale;

    // 地形的实际大小
    m_nTerrSize   = (int)floorl(sqrtl(lMapSize));
    m_fCellWidth  = fWidth  / (m_nTerrSize - 1);
    m_fCellHeight = fHeight / (m_nTerrSize - 1);

    // 创建满四叉树
    m_pRootNode = new QuadTreeNode();
    m_pRootNode->m_nNodeLevel  = log2(m_nTerrSize) - 1;
    m_pRootNode->m_nCenterX    = m_nTerrSize >> 1;
    m_pRootNode->m_nCenterY    = m_nTerrSize >> 1;
    m_pRootNode->m_nNodeSize   = m_nTerrSize - 1;
    BuildQuadTree(m_pRootNode);

    // create nodes' vertex buffer
    if (FAILED(m_pd3dDevice->CreateVertexBuffer((m_nTerrSize>>1) * 9 * sizeof(TERRAINVERTEX), 
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, TERRAINVERTEX::FVF, D3DPOOL_SYSTEMMEM, &m_pVertexBuffer, 0)))
        return ::MessageBox(NULL, L"Create vertex buffer failed!", L"Error", 0), FALSE;

    // create nodes' index buffer
    if (FAILED(m_pd3dDevice->CreateIndexBuffer((m_nTerrSize>>1) * 24 * sizeof(WORD), 
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_SYSTEMMEM, &m_pIndexBuffer, 0)))
        return ::MessageBox(NULL, L"Create index buffer failed!", L"Error", 0), FALSE;

    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::BuildQuadTree()
// Desc: 创建一个满四叉树
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::BuildQuadTree(QuadTreeNode *pNode) 
{
    if (pNode == NULL) 
        return TRUE;

    pNode->SplitNode();
    BuildQuadTree(pNode->m_pTLChildNode);
    BuildQuadTree(pNode->m_pTRChildNode);
    BuildQuadTree(pNode->m_pBLChildNode);
    BuildQuadTree(pNode->m_pBRChildNode);
    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::UpdateLodTerrain()
// Desc: 更新四叉树的节点,并标记可见的节点
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::UpdateLodTerrain(QuadTreeNode *pNode)
{
    // 更新视截体
    m_pFrustum->UpdateFrustum();

    // 简化地形节点
    ResetNodeFlag(pNode ? pNode : m_pRootNode);
    SimplifyNodes(pNode ? pNode : m_pRootNode);

    // 取得节点的顶点、索引
    m_pVerticesPtr = NULL, m_pIndicesPtr = NULL;
    m_pVertexBuffer->Lock(0, 0, (void**)&m_pVerticesPtr, 0);
    m_pIndexBuffer->Lock(0, 0, (void**)&m_pIndicesPtr, 0);

    m_nRenderNodes = 0, m_nVBufIndex = 0, m_nIBufIndex = 0;
    GetNodeVertex(pNode ? pNode : m_pRootNode);

    m_pVertexBuffer->Unlock();
    m_pIndexBuffer->Unlock();
    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::ResetNodeFlag()
// Desc: 清除所有节点的标志位
//--------------------------------------------------------------------------------------
VOID CLodTerrain::ResetNodeFlag(QuadTreeNode *pNode) 
{
    if (pNode == NULL) return;
    pNode->m_bIsReached = FALSE;
    pNode->m_bIsTouched = FALSE;
    pNode->m_bIsVisible = FALSE;
    ResetNodeFlag(pNode->m_pTLChildNode);
    ResetNodeFlag(pNode->m_pTRChildNode);
    ResetNodeFlag(pNode->m_pBLChildNode);
    ResetNodeFlag(pNode->m_pBRChildNode);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::SimplifyNodes()
// Desc: 简化节点
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::SimplifyNodes(QuadTreeNode *pNode) 
{
    pNode->m_bIsTouched = TRUE;

    pNode->m_bIsVisible = IsNodeVisible(pNode);
    if (FALSE == pNode->m_bIsVisible) 
        return pNode->m_bIsReached = FALSE, TRUE;

    pNode->m_bIsReached = IsNodeReached(pNode);
    if (pNode->m_bIsReached || pNode->m_bIsLeaf)
        return m_nRenderNodes++, pNode->m_bIsReached=TRUE;

    if (!pNode->m_pTLChildNode->m_bIsTouched)
        SimplifyNodes(pNode->m_pTLChildNode);
    if (!pNode->m_pTRChildNode->m_bIsTouched)
        SimplifyNodes(pNode->m_pTRChildNode);
    if (!pNode->m_pBLChildNode->m_bIsTouched)
        SimplifyNodes(pNode->m_pBLChildNode);
    if (!pNode->m_pBRChildNode->m_bIsTouched)
        SimplifyNodes(pNode->m_pBRChildNode);

    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::IsNodeVisible()
// Desc: 检测节点是否位于视锥体内
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::IsNodeVisible(QuadTreeNode *pNode) 
{
    D3DXVECTOR3 vCenter;
    static INT nSizeDivide2 = m_nTerrSize >> 1;

    vCenter.x = (pNode->m_nCenterX - nSizeDivide2) * m_fCellWidth;
    vCenter.z = (pNode->m_nCenterY - nSizeDivide2) * m_fCellHeight;
    vCenter.y = m_vHeightInfo[pNode->m_nCenterX * m_nTerrSize + pNode->m_nCenterY];

    FLOAT fRadius = (pNode->m_nNodeSize * m_fCellWidth) / 1.414f;
    return m_pFrustum->CheckSphere(vCenter, fRadius);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::IsNodeReached()
// Desc: 检测节点是否满足简化原则
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::IsNodeReached(QuadTreeNode *pNode) 
{
    D3DXVECTOR3 vCenter;
    static INT nSizeDivide2 = m_nTerrSize >> 1;

    vCenter.x = (pNode->m_nCenterX - nSizeDivide2) * m_fCellWidth;
    vCenter.z = (pNode->m_nCenterY - nSizeDivide2) * m_fCellHeight;
    vCenter.y = m_vHeightInfo[pNode->m_nCenterX * m_nTerrSize + pNode->m_nCenterY];
    
    D3DXVECTOR3 vEye, vLook;
    g_pCamera->GetCameraPos(&vEye);
    g_pCamera->GetLookVector(&vLook);

    D3DXVECTOR3 vDir = vCenter - vEye;
    FLOAT fDistance = D3DXVec3Length(&vDir);
    D3DXVec3Normalize(&vLook, &vLook);
    D3DXVec3Normalize(&vDir,  &vDir);
    FLOAT fValue = abs(D3DXVec3Dot(&vDir, &vLook));

    return fDistance * fValue > (pNode->m_nNodeSize<<3);
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::GetNodeVertex()
// Desc: 将所有显示的节点的坐标写入到顶点缓存和索引缓存中
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::GetNodeVertex(QuadTreeNode *pNode) 
{
    if (FALSE == pNode->m_bIsVisible)
        return TRUE;

    if (TRUE == pNode->m_bIsReached)
    {
        // fill vertex buffer
        static INT nSizeDivide2 = m_nTerrSize >> 1;;
        INT L = pNode->m_nCenterX - (pNode->m_nNodeSize >> 1);  // left
        INT R = pNode->m_nCenterX + (pNode->m_nNodeSize >> 1);  // right
        INT X = pNode->m_nCenterX;                              // center X
        INT Y = pNode->m_nCenterY;                              // center Y
        INT T = pNode->m_nCenterY - (pNode->m_nNodeSize >> 1);  // top
        INT B = pNode->m_nCenterY + (pNode->m_nNodeSize >> 1);  // bottom

        //  0   1   2         TL    Top    TR
        //  3   4   5   ->   Left  Center Right
        //  6   7   8         BL   Bottom  BR
        // TL
        m_pVerticesPtr[m_nVBufIndex + 0]._x = (L - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 0]._z = (T - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 0]._y = m_vHeightInfo[L * m_nTerrSize + T];

        // TOP
        m_pVerticesPtr[m_nVBufIndex + 1]._x = (X - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 1]._z = (T - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 1]._y = m_vHeightInfo[X * m_nTerrSize + T];

        // TR
        m_pVerticesPtr[m_nVBufIndex + 2]._x = (R - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 2]._z = (T - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 2]._y = m_vHeightInfo[R * m_nTerrSize + T];

        // LEFT
        m_pVerticesPtr[m_nVBufIndex + 3]._x = (L - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 3]._z = (Y - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 3]._y = m_vHeightInfo[L * m_nTerrSize + Y];

        // CENTER
        m_pVerticesPtr[m_nVBufIndex + 4]._x = (X - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 4]._z = (Y - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 4]._y = m_vHeightInfo[X * m_nTerrSize + Y];

        // RIGHT
        m_pVerticesPtr[m_nVBufIndex + 5]._x = (R - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 5]._z = (Y - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 5]._y = m_vHeightInfo[R * m_nTerrSize + Y];

        // BL
        m_pVerticesPtr[m_nVBufIndex + 6]._x = (L - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 6]._z = (B - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 6]._y = m_vHeightInfo[L * m_nTerrSize + B];

        // BOTTOM
        m_pVerticesPtr[m_nVBufIndex + 7]._x = (X - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 7]._z = (B - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 7]._y = m_vHeightInfo[X * m_nTerrSize + B];

        // BR
        m_pVerticesPtr[m_nVBufIndex + 8]._x = (R - nSizeDivide2) * m_fCellWidth;
        m_pVerticesPtr[m_nVBufIndex + 8]._z = (B - nSizeDivide2) * m_fCellHeight;
        m_pVerticesPtr[m_nVBufIndex + 8]._y = m_vHeightInfo[R * m_nTerrSize + B];

        m_nVBufIndex   += 9;
        m_nRenderNodes += 9;

        // fill index buffer
        //  0   1   2
        //  3   4   5
        //  6   7   8
        m_pIndicesPtr[m_nIBufIndex + 0]  = (WORD)m_nVBufIndex + 0;
        m_pIndicesPtr[m_nIBufIndex + 1]  = (WORD)m_nVBufIndex + 1;
        m_pIndicesPtr[m_nIBufIndex + 2]  = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 3]  = (WORD)m_nVBufIndex + 1;
        m_pIndicesPtr[m_nIBufIndex + 4]  = (WORD)m_nVBufIndex + 2;
        m_pIndicesPtr[m_nIBufIndex + 5]  = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 6]  = (WORD)m_nVBufIndex + 2;
        m_pIndicesPtr[m_nIBufIndex + 7]  = (WORD)m_nVBufIndex + 5;
        m_pIndicesPtr[m_nIBufIndex + 8]  = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 9]  = (WORD)m_nVBufIndex + 5;
        m_pIndicesPtr[m_nIBufIndex + 10] = (WORD)m_nVBufIndex + 8;
        m_pIndicesPtr[m_nIBufIndex + 11] = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 12] = (WORD)m_nVBufIndex + 8;
        m_pIndicesPtr[m_nIBufIndex + 13] = (WORD)m_nVBufIndex + 7;
        m_pIndicesPtr[m_nIBufIndex + 14] = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 15] = (WORD)m_nVBufIndex + 7;
        m_pIndicesPtr[m_nIBufIndex + 16] = (WORD)m_nVBufIndex + 6;
        m_pIndicesPtr[m_nIBufIndex + 17] = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 18] = (WORD)m_nVBufIndex + 6;
        m_pIndicesPtr[m_nIBufIndex + 19] = (WORD)m_nVBufIndex + 3;
        m_pIndicesPtr[m_nIBufIndex + 20] = (WORD)m_nVBufIndex + 4;

        m_pIndicesPtr[m_nIBufIndex + 21] = (WORD)m_nVBufIndex + 3;
        m_pIndicesPtr[m_nIBufIndex + 22] = (WORD)m_nVBufIndex + 0;
        m_pIndicesPtr[m_nIBufIndex + 23] = (WORD)m_nVBufIndex + 4;

        m_nIBufIndex += 24;
    }
    else if (!pNode->m_bIsLeaf)
    {
        GetNodeVertex(pNode->m_pTLChildNode);
        GetNodeVertex(pNode->m_pTRChildNode);
        GetNodeVertex(pNode->m_pBLChildNode);
        GetNodeVertex(pNode->m_pBRChildNode);

    } // end if-else

    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CLodTerrain::RenderLodTerrain()
// Desc: 渲染LOD地形
//--------------------------------------------------------------------------------------
BOOL CLodTerrain::RenderLodTerrain(D3DXMATRIX *pMatWorld) 
{
    //m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

    m_pd3dDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(TERRAINVERTEX));
    m_pd3dDevice->SetFVF(TERRAINVERTEX::FVF);
    m_pd3dDevice->SetIndices(m_pIndexBuffer);
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
        m_nRenderNodes * 9, 0, m_nRenderNodes * 8);

    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, true);
    return TRUE;
}
