#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <list>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#endif 
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//--------------------------------------------------------------------------------------
// Desc: Global functions
//--------------------------------------------------------------------------------------
DWORD FtoDW(FLOAT fValue);
FLOAT GetRandomFloat(FLOAT fLow, FLOAT fHigh);
D3DXVECTOR3 GetRandomVector(D3DXVECTOR3* vMin,D3DXVECTOR3* vMax);


struct PARTICLEVERTEX 
{
    FLOAT  _x, _y, _z;              // λ��
    D3DCOLOR  _color;               // ������ɫ
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};

struct PARTICLEATTRIBUTE 
{
    D3DXVECTOR3 Position0;
    D3DXVECTOR3 Velocity0;
    D3DXVECTOR3 Position;           // ���ӵĵ�ǰλ��
    D3DXVECTOR3 Velocity;           // ���ӵĵ�ǰ�ٶ�
    D3DXVECTOR3 Acceleration;       // ���ӵļ��ٶ�
    D3DXCOLOR   CurrColor;          // ���ӵĵ�ǰ��ɫֵ
    D3DXCOLOR   FadeColor;          // ���ӵ�������ɫֵ
    FLOAT       LifeTime;           // ���ӵ�����ʱ��
    FLOAT       AgeTime;            // ���ӵĵ�ǰ����
    BOOL        IsAlive;
};


//--------------------------------------------------------------------------------------
// Name: class CPSystem
// Desc: ����ϵͳ������
//--------------------------------------------------------------------------------------
class CPSystem 
{
protected:
    LPDIRECT3DDEVICE9               m_pd3dDevice;
    LPDIRECT3DTEXTURE9              m_pParticleTex;
    LPDIRECT3DVERTEXBUFFER9         m_pVertexBuffer;
    std::list<PARTICLEATTRIBUTE*>   m_listParticles;

    DWORD       m_dwVBufSize;           // size of vb
    DWORD       m_dwVBufOffset;         // offset in vb to lock   
    DWORD       m_dwBatchSize;          // number of vertices to lock starting at _vbOffset
    FLOAT       m_fParticleSize;        // particle size

public:
    CPSystem(IDirect3DDevice9* pd3dDevice);
    virtual ~CPSystem(void);

    BOOL  IsAllParticlesDead(void);
    DWORD GetParticleNums(void)  { return (DWORD)m_listParticles.size(); }

    virtual BOOL InitPSystem(LPCTSTR lpTexFile);
    virtual void EmitParticles(INT nParticleNum);
    virtual void RefreshParticles(void);
    virtual void ResetParticle(PARTICLEATTRIBUTE* pParticleAttr);
    virtual void UpdatePSystem(FLOAT fTimeDelta);
    virtual void RenderPSystem(FLOAT fTimeDelta);

protected:
    virtual void BeginRenderPSystem(void);
    virtual void EndRenderPSystem(void);
    virtual void RemoveDeadParticles(void);
};


//--------------------------------------------------------------------------------------
// Name: class CFireworkPSystem
// Desc: �̻�����ϵͳ
//--------------------------------------------------------------------------------------
class CFireworksPSystem : public CPSystem
{
protected:
    D3DXVECTOR3 m_vOrigin;                                          // ����Դ

public:
    CFireworksPSystem(IDirect3DDevice9 *pd3dDevice);
    virtual ~CFireworksPSystem();

    virtual void ResetOriginPos(D3DXVECTOR3 *vOrigin);              // ��������Դ��λ��
    virtual void ResetParticle(PARTICLEATTRIBUTE* pParticleAttr);   // ����������������
    virtual void UpdatePSystem(FLOAT fTimeDelta);                   // ��������ϵͳ

protected:
    virtual void BeginRenderPSystem(void);
    virtual void EndRenderPSystem(void);
};
