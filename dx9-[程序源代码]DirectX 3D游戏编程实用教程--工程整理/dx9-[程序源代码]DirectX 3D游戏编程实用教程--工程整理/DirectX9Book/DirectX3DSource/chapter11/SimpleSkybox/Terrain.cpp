#include "Terrain.h"
#include <fstream>

CTerrain::CTerrain(IDirect3DDevice9* pd3dDevice)
{
    m_pd3dDevice    = pd3dDevice;
    m_pTexture      = NULL;
    m_pIndexBuf     = NULL;
    m_pVertexBuf    = NULL;
    m_nCellsPerRow  = 0;
    m_nCellsPerCol  = 0;
    m_nVertsPerRow  = 0;
    m_nVertsPerCol  = 0;
    m_nNumVertices  = 0;
    m_fTerrainWidth = 0.0f;
    m_fTerrainDepth = 0.0f;
    m_fCellSpacing  = 0.0f;
    m_fHeightScale  = 0.0f;
}

CTerrain::~CTerrain(void)
{
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pIndexBuf);
    SAFE_RELEASE(m_pVertexBuf);
}

// ============================================================================
// LoadTerrain()
// 功能: 加载地形高度信息和纹理
//
// 参数:
//  pRawFileName: 保存高度信息的Raw文件名
//  pTextureFile: 地形纹理文件
// 
// 返回:
//  成功返回TRUE, 否则返回FALSE
// ============================================================================
BOOL CTerrain::LoadTerrain(wchar_t *pRawFileName, wchar_t *pTextureFile) 
{
    // 读取高度信息
    std::ifstream inFile;
    inFile.open(pRawFileName, std::ios::binary);

    inFile.seekg(0,std::ios::end);
    std::vector<BYTE> inData(inFile.tellg());

    inFile.seekg(std::ios::beg);
    inFile.read((char*)&inData[0], inData.size());
    inFile.close();

    m_vHeightInfo.resize(inData.size());
    for (unsigned int i=0; i<inData.size(); i++)
        m_vHeightInfo[i] = inData[i];

    // 加载地形纹理
    if (FAILED(D3DXCreateTextureFromFile(m_pd3dDevice, pTextureFile, &m_pTexture)))
        return FALSE;

    return TRUE;
}

// ============================================================================
// InitTerrain()
// 功能: 初始化地形的几何高度, 填充顶点和索引缓存
//
// 参数:
//  nRows: 地形的行数
//  nCols: 地形的列数
//  nSpace: 单元格间距
//  fScale: 高度缩放系数
// 
// 返回:
//  成功返回TRUE, 否则返回FALSE
// ============================================================================
BOOL CTerrain::InitTerrain(INT nRows, INT nCols, FLOAT fSpace, FLOAT fScale) 
{
    m_nCellsPerRow  = nRows;
    m_nCellsPerCol  = nCols;
    m_fCellSpacing  = fSpace;
    m_fHeightScale  = fScale; 
    m_fTerrainWidth = nRows * fSpace;
    m_fTerrainDepth = nCols * fSpace;
    m_nVertsPerRow  = m_nCellsPerCol + 1;
    m_nVertsPerCol  = m_nCellsPerRow + 1;
    m_nNumVertices  = m_nVertsPerRow * m_nVertsPerCol;

    // 计算实际的几何高度
    for(unsigned int i=0; i<m_vHeightInfo.size(); i++)
        m_vHeightInfo[i] *= m_fHeightScale;

    // 计算地形的灵活顶点
    if (FAILED(m_pd3dDevice->CreateVertexBuffer(m_nNumVertices * sizeof(TERRAINVERTEX), 
        D3DUSAGE_WRITEONLY, TERRAINVERTEX::FVF, D3DPOOL_MANAGED, &m_pVertexBuf, 0)))
        return FALSE;

    TERRAINVERTEX *pVertices = NULL;
    m_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);

    FLOAT fStartX = -m_fTerrainWidth / 2.0f, fEndX =  m_fTerrainWidth / 2.0f;
    FLOAT fStartZ =  m_fTerrainDepth / 2.0f, fEndZ = -m_fTerrainDepth / 2.0f;
    FLOAT fCoordU = 1.0f / (FLOAT)m_nCellsPerRow;
    FLOAT fCoordV = 1.0f / (FLOAT)m_nCellsPerCol;

    int nIndex = 0, i = 0, j = 0;
    for (float z = fStartZ; z > fEndZ; z -= m_fCellSpacing, i++)
    {
        j = 0;
        for (float x = fStartX; x < fEndX; x += m_fCellSpacing, j++)
        {
            nIndex = i * m_nCellsPerRow + j;
            pVertices[nIndex] = TERRAINVERTEX(x, m_vHeightInfo[nIndex], z, j*fCoordU, i*fCoordV);
            nIndex++;
        }
    }

    m_pVertexBuf->Unlock();

    // compute the indices
    if (FAILED(m_pd3dDevice->CreateIndexBuffer(m_nNumVertices * 6 *sizeof(WORD), 
        D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndexBuf, 0)))
        return FALSE;

    WORD* pIndices = NULL;
    m_pIndexBuf->Lock(0, 0, (void**)&pIndices, 0);

    nIndex = 0;
    for(int row = 0; row < m_nCellsPerRow-1; row++)
    {
        for(int col = 0; col < m_nCellsPerCol-1; col++)
        {
            pIndices[nIndex]   =   row   * m_nCellsPerRow + col;
            pIndices[nIndex+1] =   row   * m_nCellsPerRow + col + 1;
            pIndices[nIndex+2] = (row+1) * m_nCellsPerRow + col;

            pIndices[nIndex+3] = (row+1) * m_nCellsPerRow + col;
            pIndices[nIndex+4] =   row   * m_nCellsPerRow + col + 1;
            pIndices[nIndex+5] = (row+1) * m_nCellsPerRow + col + 1;

            nIndex += 6;
        }
    }

    m_pIndexBuf->Unlock();

    return TRUE;
}

// ============================================================================
// DrawTerrain()
// 功能: 绘制地形
//
// 参数:
//  pMatWorld: 地形的世界矩阵
//  bDrawFrame: 是否绘制地形的框架
// 
// 返回:
//  成功返回TRUE, 否则返回FALSE
// ============================================================================
BOOL CTerrain::DrawTerrain(D3DXMATRIX *pMatWorld, BOOL bDrawFrame) 
{
    m_pd3dDevice->SetStreamSource(0, m_pVertexBuf, 0, sizeof(TERRAINVERTEX));
    m_pd3dDevice->SetFVF(TERRAINVERTEX::FVF);
    m_pd3dDevice->SetIndices(m_pIndexBuf);
    m_pd3dDevice->SetTexture(0, m_pTexture);

    m_pd3dDevice->SetTransform(D3DTS_WORLD, pMatWorld);
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
        m_nNumVertices, 0, m_nNumVertices * 2);

    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    m_pd3dDevice->SetTexture(0, 0);

    if (bDrawFrame)
    {
        m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
            m_nNumVertices, 0, m_nNumVertices * 2);
        m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    }
    return TRUE;
}


FLOAT CTerrain::GetPointHeight(FLOAT x, FLOAT z) 
{
    static FLOAT fWidthDivide2 = m_fTerrainWidth / 2.0f;
    static FLOAT fDepthDivide2 = m_fTerrainDepth / 2.0f;

    x = (fWidthDivide2 + x) / m_fCellSpacing;
    z = (fDepthDivide2 + z) / m_fCellSpacing;

    int col1 = (int)floorf(x), col2 = (int)ceilf(x);
    int row1 = (int)floorf(z), row2 = (int)ceilf(z);

    FLOAT A = m_vHeightInfo[row1 * m_nCellsPerRow + col1];
    FLOAT B = m_vHeightInfo[row1 * m_nCellsPerRow + col2];
    FLOAT C = m_vHeightInfo[row2 * m_nCellsPerRow + col1];
    FLOAT D = m_vHeightInfo[row2 * m_nCellsPerRow + col2];

	float dx = x - col1;
	float dz = z - row1;

    if (dx + dz < 1.0f)
		return (A + (B - A) * dx + (C - A) * dz);
	else
		return (D + (B - D) * (1 - dx) + (C - D) * (1 - dz));
}
