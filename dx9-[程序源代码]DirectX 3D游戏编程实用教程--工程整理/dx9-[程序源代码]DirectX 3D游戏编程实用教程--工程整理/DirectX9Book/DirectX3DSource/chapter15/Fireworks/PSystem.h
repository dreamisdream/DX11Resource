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
    FLOAT  _x, _y, _z;              // 位置
    D3DCOLOR  _color;               // 粒子颜色
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};

struct PARTICLEATTRIBUTE 
{
    D3DXVECTOR3 Position0;
    D3DXVECTOR3 Velocity0;
    D3DXVECTOR3 Position;           // 粒子的当前位置
    D3DXVECTOR3 Velocity;           // 粒子的当前速度
    D3DXVECTOR3 Acceleration;       // 粒子的加速度
    D3DXCOLOR   CurrColor;          // 粒子的当前颜色值
    D3DXCOLOR   FadeColor;          // 粒子的消退颜色值
    FLOAT       LifeTime;           // 粒子的生存时间
    FLOAT       AgeTime;            // 粒子的当前年龄
    BOOL        IsAlive;
};


//--------------------------------------------------------------------------------------
// Name: class CPSystem
// Desc: 粒子系统抽象类
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
// Desc: 烟花粒子系统
//--------------------------------------------------------------------------------------
class CFireworksPSystem : public CPSystem
{
protected:
    D3DXVECTOR3 m_vOrigin;                                          // 粒子源

public:
    CFireworksPSystem(IDirect3DDevice9 *pd3dDevice);
    virtual ~CFireworksPSystem();

    virtual void ResetOriginPos(D3DXVECTOR3 *vOrigin);              // 设置粒子源的位置
    virtual void ResetParticle(PARTICLEATTRIBUTE* pParticleAttr);   // 重新设置粒子属性
    virtual void UpdatePSystem(FLOAT fTimeDelta);                   // 更新粒子系统

protected:
    virtual void BeginRenderPSystem(void);
    virtual void EndRenderPSystem(void);
};
