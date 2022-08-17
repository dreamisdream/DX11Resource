#include "Camera.h"
#include <fstream>

CCamera::CCamera(IDirect3DDevice9 *pd3dDevice)
{
    m_pd3dDevice = pd3dDevice;
    m_vRightVec  = D3DXVECTOR3(1.0f, 0.0f, 0.0f);   // 默认右向量与X正半轴重合
    m_vUpVec     = D3DXVECTOR3(0.0f, 1.0f, 0.0f);   // 默认上向量与Y正半轴重合
    m_vLookVec   = D3DXVECTOR3(0.0f, 0.0f, 1.0f);   // 默认观察向量与Z正半轴重合
    m_vPosition  = D3DXVECTOR3(0.0f, 0.0f, 0.0f);   // 默认摄像机的位置为原点
    m_vLookat    = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    GetViewMatrix(&m_matView);                      // 取得取景变换矩阵
    D3DXMatrixPerspectiveFovLH(&m_matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 2000.0f); // 投影变换矩阵
}

CCamera::~CCamera(void)
{
}


VOID CCamera::GetViewMatrix(D3DXMATRIX *pMatrix) 
{
    // 使各分量相互垂直
    D3DXVec3Normalize(&m_vLookVec, &m_vLookVec);

    D3DXVec3Cross(&m_vUpVec, &m_vLookVec, &m_vRightVec);    // 上向量与观察向量垂直
    D3DXVec3Normalize(&m_vUpVec, &m_vUpVec);                // 规格化上向量

    D3DXVec3Cross(&m_vRightVec, &m_vUpVec, &m_vLookVec);    // 右向量与上向量垂直
    D3DXVec3Normalize(&m_vRightVec, &m_vRightVec);          // 规格化右向量

    // 创建取景变换矩阵
    pMatrix->_11 = m_vRightVec.x;           // Rx
    pMatrix->_12 = m_vUpVec.x;              // Ux
    pMatrix->_13 = m_vLookVec.x;            // Lx
    pMatrix->_14 = 0.0f;

    pMatrix->_21 = m_vRightVec.y;           // Ry
    pMatrix->_22 = m_vUpVec.y;              // Uy
    pMatrix->_23 = m_vLookVec.y;            // Ly
    pMatrix->_24 = 0.0f;

    pMatrix->_31 = m_vRightVec.z;           // Rz
    pMatrix->_32 = m_vUpVec.z;              // Uz
    pMatrix->_33 = m_vLookVec.z;            // Lz
    pMatrix->_34 = 0.0f;

    pMatrix->_41 = -D3DXVec3Dot(&m_vRightVec, &m_vPosition);    // -P*R
    pMatrix->_42 = -D3DXVec3Dot(&m_vUpVec, &m_vPosition);       // -P*U
    pMatrix->_43 = -D3DXVec3Dot(&m_vLookVec, &m_vPosition);     // -P*L
    pMatrix->_44 = 1.0f;
}

VOID CCamera::ResetLookatPos(D3DXVECTOR3 *pLookat) 
{
    if (pLookat != NULL)  m_vLookat = (*pLookat);
    else m_vLookat = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

    m_vLookVec = m_vLookat - m_vPosition;
    D3DXVec3Normalize(&m_vLookVec, &m_vLookVec);

    D3DXVec3Cross(&m_vUpVec, &m_vLookVec, &m_vRightVec);
    D3DXVec3Normalize(&m_vUpVec, &m_vUpVec);

    D3DXVec3Cross(&m_vRightVec, &m_vUpVec, &m_vLookVec);
    D3DXVec3Normalize(&m_vRightVec, &m_vRightVec);
}

VOID CCamera::ResetCameraPos(D3DXVECTOR3 *pVector) 
{
    D3DXVECTOR3 V = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_vPosition = pVector ? (*pVector) : V;
}

VOID CCamera::ResetViewMatrix(D3DXMATRIX *pMatrix) 
{
    if (pMatrix) m_matView = *pMatrix;
    else GetViewMatrix(&m_matView);
    m_pd3dDevice->SetTransform(D3DTS_VIEW, &m_matView);
    
    m_vRightVec = D3DXVECTOR3(m_matView._11, m_matView._12, m_matView._13);
    m_vUpVec    = D3DXVECTOR3(m_matView._21, m_matView._22, m_matView._23);
    m_vLookVec  = D3DXVECTOR3(m_matView._31, m_matView._32, m_matView._33);
}

VOID CCamera::ResetProjMatrix(D3DXMATRIX *pMatrix) 
{
    if (pMatrix != NULL) m_matProj = *pMatrix;
    else D3DXMatrixPerspectiveFovLH(&m_matProj, D3DX_PI / 4.0f, 1.0f, 1.0f, 1000.0f);
    m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m_matProj);
}

// 沿右向量平移fUnits个单位
VOID CCamera::MoveAlongRightVec(FLOAT fUnits) 
{
    m_vPosition += m_vRightVec * fUnits;
    m_vLookat   += m_vRightVec * fUnits;
}

// 沿上向量平移fUnits个单位
VOID CCamera::MoveAlongUpVec(FLOAT fUnits) 
{
    m_vPosition += m_vUpVec * fUnits;
    m_vLookat   += m_vUpVec * fUnits;
}

// 沿观察向量平移fUnits个单位
VOID CCamera::MoveAlongLookVec(FLOAT fUnits) 
{
    m_vPosition += m_vLookVec * fUnits;
    m_vLookat   += m_vLookVec * fUnits;
}

VOID CCamera::RotationRightVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vRightVec, fAngle);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vPosition, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);
}

VOID CCamera::RotationUpVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vUpVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);

    D3DXVECTOR3 vNormal;
    D3DXVec3Normalize(&vNormal, &m_vLookat);
}

VOID CCamera::RotationLookVec(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vLookVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);

    m_vLookat = m_vLookVec * D3DXVec3Length(&m_vPosition);
}

VOID CCamera::CircleRotationX(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vRightVec, fAngle);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    float dy = m_vPosition.y - m_vLookat.y;
    float dz = m_vPosition.z - m_vLookat.z;
    m_vPosition.y = m_vLookat.y + dy * cosf(fAngle) - dz * sinf(fAngle);
    m_vPosition.z = m_vLookat.z + dy * sinf(fAngle) + dz * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}

VOID CCamera::CircleRotationY(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vUpVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vLookVec, &m_vLookVec, &R);

    float dx = m_vPosition.x - m_vLookat.x;
    float dz = m_vPosition.z - m_vLookat.z;
    m_vPosition.x = m_vLookat.x + dx * cosf(fAngle) - dz * sinf(fAngle);
    m_vPosition.z = m_vLookat.z + dx * sinf(fAngle) + dz * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}

VOID CCamera::CircleRotationZ(FLOAT fAngle) 
{
    D3DXMATRIX R;
    D3DXMatrixRotationAxis(&R, &m_vLookVec, fAngle);
    D3DXVec3TransformCoord(&m_vRightVec, &m_vRightVec, &R);
    D3DXVec3TransformCoord(&m_vUpVec, &m_vUpVec, &R);

    float dx = m_vPosition.x - m_vLookat.x;
    float dy = m_vPosition.y - m_vLookat.y;
    m_vPosition.x = m_vLookat.x + dx * cosf(fAngle) + dy * sinf(fAngle);
    m_vPosition.y = m_vLookat.y - dx * sinf(fAngle) + dy * cosf(fAngle);

    ResetLookatPos(&m_vLookat);
}
