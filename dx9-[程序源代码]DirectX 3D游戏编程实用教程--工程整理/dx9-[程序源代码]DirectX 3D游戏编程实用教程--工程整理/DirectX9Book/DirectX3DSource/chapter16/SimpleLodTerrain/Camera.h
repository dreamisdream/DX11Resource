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
// Name: class CCamera
// Desc: ���������ƽ�ơ���ת
//--------------------------------------------------------------------------------------
class CCamera
{
private:
    D3DXVECTOR3         m_vRightVec;        // �ҷ���
    D3DXVECTOR3         m_vUpVec;           // �Ϸ���
    D3DXVECTOR3         m_vLookVec;         // �۲췽��
    D3DXVECTOR3         m_vPosition;        // λ��
    D3DXMATRIX          m_matView;          // ȡ���任����
    D3DXMATRIX          m_matProj;          // ͶӰ�任����
    D3DXVECTOR3         m_vLookat;
    LPDIRECT3DDEVICE9   m_pd3dDevice;

public:
    CCamera(IDirect3DDevice9 *pd3dDevice);
    virtual ~CCamera(void);

public:
    VOID GetViewMatrix(D3DXMATRIX *pMatrix);
    VOID GetProjMatrix(D3DXMATRIX *pMatrix)  { *pMatrix = m_matProj; }
    VOID GetCameraPos(D3DXVECTOR3 *pVector)  { *pVector = m_vPosition; }
    VOID GetLookVector(D3DXVECTOR3 *pVector) { *pVector = m_vLookVec; }

    VOID ResetLookatPos(D3DXVECTOR3 *pLookat = NULL);
    VOID ResetCameraPos(D3DXVECTOR3 *pVector = NULL);
    VOID ResetViewMatrix(D3DXMATRIX *pMatrix = NULL);
    VOID ResetProjMatrix(D3DXMATRIX *pMatrix = NULL);

public:
    // �ظ�����ƽ��
    VOID MoveAlongRightVec(FLOAT fUnits);   // ��right�����ƶ�
    VOID MoveAlongUpVec(FLOAT fUnits);      // ��up�����ƶ�
    VOID MoveAlongLookVec(FLOAT fUnits);    // ��look�����ƶ�

    // �Ƹ�������ת
    VOID RotationRightVec(FLOAT fAngle);    // ��right����ѡ��
    VOID RotationUpVec(FLOAT fAngle);       // ��up������ת
    VOID RotationLookVec(FLOAT fAngle);     // ��look������ת

    // �ƿռ����ת
    VOID CircleRotationX(FLOAT fAngle);     // ��X�������ƹ۲����ת
    VOID CircleRotationY(FLOAT fAngle);     // ��Y�������ƹ۲����ת
    VOID CircleRotationZ(FLOAT fAngle);     // ��Z�������ƹ۲����ת
};
