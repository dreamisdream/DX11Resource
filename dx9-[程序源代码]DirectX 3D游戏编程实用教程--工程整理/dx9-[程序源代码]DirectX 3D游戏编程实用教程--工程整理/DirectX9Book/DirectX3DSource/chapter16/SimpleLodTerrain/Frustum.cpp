#include "Frustum.h"

//--------------------------------------------------------------------------------------
// Name: CFrustum::CFrustum()
// Desc: 
//--------------------------------------------------------------------------------------
CFrustum::CFrustum(IDirect3DDevice9 *pd3dDevice)
{
    m_pd3dDevice = pd3dDevice;
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::~CFrustum()
// Desc: 
//--------------------------------------------------------------------------------------
CFrustum::~CFrustum(void)
{
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::UpdateFrustum
// Desc: 根据取景,投影变换矩阵构造视锥体
//--------------------------------------------------------------------------------------
VOID CFrustum::UpdateFrustum(void) 
{
    D3DXMATRIX Matrix, matView, matProj;
    m_pd3dDevice->GetTransform(D3DTS_VIEW, &matView);
    m_pd3dDevice->GetTransform(D3DTS_PROJECTION, &matProj );
    D3DXMatrixMultiply( &Matrix, &matView, &matProj );

    // Calculate the planes
    m_planes[0].a = Matrix._14 + Matrix._13; // Near
    m_planes[0].b = Matrix._24 + Matrix._23;
    m_planes[0].c = Matrix._34 + Matrix._33;
    m_planes[0].d = Matrix._44 + Matrix._43;
    D3DXPlaneNormalize(&m_planes[0], &m_planes[0]);

    m_planes[1].a = Matrix._14 - Matrix._13; // Far 
    m_planes[1].b = Matrix._24 - Matrix._23;
    m_planes[1].c = Matrix._34 - Matrix._33;
    m_planes[1].d = Matrix._44 - Matrix._43;
    D3DXPlaneNormalize(&m_planes[1], &m_planes[1]);

    m_planes[2].a = Matrix._14 - Matrix._11; // Left
    m_planes[2].b = Matrix._24 - Matrix._21;
    m_planes[2].c = Matrix._34 - Matrix._31;
    m_planes[2].d = Matrix._44 - Matrix._41;
    D3DXPlaneNormalize(&m_planes[2], &m_planes[2]);

    m_planes[3].a = Matrix._14 + Matrix._11; // Righ
    m_planes[3].b = Matrix._24 + Matrix._21;
    m_planes[3].c = Matrix._34 + Matrix._31;
    m_planes[3].d = Matrix._44 + Matrix._41;
    D3DXPlaneNormalize(&m_planes[3], &m_planes[3]);

    m_planes[4].a = Matrix._14 - Matrix._12; // Top 
    m_planes[4].b = Matrix._24 - Matrix._22;
    m_planes[4].c = Matrix._34 - Matrix._32;
    m_planes[4].d = Matrix._44 - Matrix._42;
    D3DXPlaneNormalize(&m_planes[4], &m_planes[4]);

    m_planes[5].a = Matrix._14 + Matrix._12; // Bott
    m_planes[5].b = Matrix._24 + Matrix._22;
    m_planes[5].c = Matrix._34 + Matrix._32;
    m_planes[5].d = Matrix._44 + Matrix._42;
    D3DXPlaneNormalize(&m_planes[5], &m_planes[5]);
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::CheckPoint()
// Desc: 点监测
//--------------------------------------------------------------------------------------
BOOL CFrustum::CheckPoint( D3DXVECTOR3 ptPos )
{
    // Make sure point is in frustum
    for(int i=0;i<6;i++)
    {
        if( D3DXPlaneDotCoord(&m_planes[i], &ptPos) < 0.0f )
            return FALSE;
    }
    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::CheckCube()
// Desc: 立方体检测,只要有一个顶点在裁减体之内就认为可见
//--------------------------------------------------------------------------------------
BOOL CFrustum::CheckCube( D3DXVECTOR3 centerPos, float size )
{
    // 定义立方体的八个顶点
    float sizeDivd2 = size/2;
    D3DXVECTOR3 posVertices[8];
    posVertices[0] = centerPos + D3DXVECTOR3(-sizeDivd2,-sizeDivd2,-sizeDivd2 );
    posVertices[1] = centerPos + D3DXVECTOR3(-sizeDivd2,-sizeDivd2, sizeDivd2 );
    posVertices[2] = centerPos + D3DXVECTOR3(-sizeDivd2, sizeDivd2, sizeDivd2 );
    posVertices[3] = centerPos + D3DXVECTOR3(-sizeDivd2, sizeDivd2,-sizeDivd2 );
    posVertices[4] = centerPos + D3DXVECTOR3( sizeDivd2,-sizeDivd2,-sizeDivd2 );
    posVertices[5] = centerPos + D3DXVECTOR3( sizeDivd2,-sizeDivd2, sizeDivd2 );
    posVertices[6] = centerPos + D3DXVECTOR3( sizeDivd2, sizeDivd2,-sizeDivd2 );
    posVertices[7] = centerPos + D3DXVECTOR3( sizeDivd2, sizeDivd2, sizeDivd2 );
    for(int i=0;i<8;i++)
    {
        if( CheckPoint( posVertices[i] ))
            return TRUE;
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::CheckRectangle()
// Desc: 矩形检测,与立方体检测的区别是,它默认矩形与 XOY 平面平行,不考虑垂直(Y)方向
//       上的因素,只要有一个顶点在裁减体之内就认为可见
//--------------------------------------------------------------------------------------
BOOL CFrustum::CheckRectangle( D3DXVECTOR3 centerPos, float size )
{
    float sizeDivd2 = size/2;
    D3DXVECTOR3 posVertices[4];
    posVertices[0] = centerPos + D3DXVECTOR3(-sizeDivd2, 0,-sizeDivd2 );
    posVertices[1] = centerPos + D3DXVECTOR3(-sizeDivd2, 0, sizeDivd2 );
    posVertices[2] = centerPos + D3DXVECTOR3( sizeDivd2, 0, sizeDivd2 );
    posVertices[3] = centerPos + D3DXVECTOR3( sizeDivd2, 0,-sizeDivd2 );
    for(int i=0;i<4;i++)
    {
        if( CheckPoint( posVertices[i] ))
            return TRUE;
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------
// Name: CFrustum::CheckSphere()
// Desc: 球体检测
//--------------------------------------------------------------------------------------
BOOL CFrustum::CheckSphere( D3DXVECTOR3 centerPos, float radius )
{
    for(int i=0;i<6;i++)
    {
        if( D3DXPlaneDotCoord(&m_planes[i], &centerPos) < -radius )
            return FALSE;
    }
    return TRUE;
}
