#include "PSystem.h"

#define MAX_RENDERPARTICLES 100

///////////////////////////////////////////////////////////////////////////////
DWORD FtoDW(FLOAT fValue)
{
    return *((DWORD*)&fValue);
}

FLOAT GetRandomFloat(FLOAT fLow, FLOAT fHigh) 
{
    if (fLow >= fHigh) return fLow;

    FLOAT f = (rand() % 10000) * 0.0001f; 
    return (f * (fHigh - fLow)) + fLow; 
}

D3DXVECTOR3 GetRandomVector(D3DXVECTOR3* vMin,D3DXVECTOR3* vMax) 
{
    D3DXVECTOR3 vOut;
    vOut.x = GetRandomFloat(vMin->x, vMax->x);
    vOut.y = GetRandomFloat(vMin->y, vMax->y);
    vOut.z = GetRandomFloat(vMin->z, vMax->z);
    return vOut;
}

///////////////////////////////////////////////////////////////////////////////
// CPSystem
CPSystem::CPSystem(IDirect3DDevice9 *pd3dDevice) 
{
    m_pd3dDevice    = pd3dDevice;
    m_pParticleTex  = NULL;
    m_pVertexBuffer = NULL;

    m_dwVBufSize    = 3000;
    m_dwVBufOffset  = 0;
    m_dwBatchSize   = 500;
    m_fParticleSize = 1.0f;
}

CPSystem::~CPSystem() 
{
    std::list<PARTICLEATTRIBUTE*>::iterator iter;
    for (iter = m_listParticles.begin(); iter != m_listParticles.end(); iter++)
        delete (PARTICLEATTRIBUTE*)(*iter);
    m_listParticles.clear();

    SAFE_RELEASE(m_pParticleTex);
    SAFE_RELEASE(m_pVertexBuffer);
}

BOOL CPSystem::IsAllParticlesDead() 
{
    std::list<PARTICLEATTRIBUTE*>::iterator i;
    for(i = m_listParticles.begin(); i != m_listParticles.end(); i++)
        if ((*i)->IsAlive) return false;
    return true;
}

BOOL CPSystem::InitPSystem(LPCTSTR lpTexFile) 
{
    m_listParticles.clear();

    if (FAILED(D3DXCreateTextureFromFile(m_pd3dDevice, lpTexFile, &m_pParticleTex)))
        return ::MessageBoxA(NULL, "Create Texture Failed!", "PSystem", 0), false;

    if (FAILED(m_pd3dDevice->CreateVertexBuffer(m_dwVBufSize * sizeof(PARTICLEVERTEX), 
        D3DUSAGE_DYNAMIC | D3DUSAGE_POINTS | D3DUSAGE_WRITEONLY, 
        PARTICLEVERTEX::FVF, D3DPOOL_DEFAULT, &m_pVertexBuffer, 0)))
        return ::MessageBoxA(NULL, "Create VertexBuffer Failed!", "PSystem", 0), false;
    return true;
}

void CPSystem::EmitParticles(INT nParticleNum) 
{
    PARTICLEATTRIBUTE *pParticle = NULL;
    for (int i = 0; i<nParticleNum; i++) 
    {
        pParticle = new PARTICLEATTRIBUTE;
        memset(pParticle, 0, sizeof(PARTICLEATTRIBUTE));
        ResetParticle(pParticle);
        m_listParticles.push_back(pParticle);
    }
}

void CPSystem::RefreshParticles()
{
    std::list<PARTICLEATTRIBUTE*>::iterator i;
    for (i = m_listParticles.begin(); i != m_listParticles.end(); i++) 
        if (!(*i)->IsAlive) ResetParticle((*i));
}

void CPSystem::ResetParticle(PARTICLEATTRIBUTE* pParticle) 
{
}

void CPSystem::UpdatePSystem(FLOAT fTimeDelta) 
{
}

void CPSystem::BeginRenderPSystem(void) 
{
    // ���õ㾫�����Ⱦ״̬
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
    m_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, true);
    m_pd3dDevice->SetRenderState(D3DRS_POINTSCALEENABLE, true);
    m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(m_fParticleSize));
    m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE_MIN, FtoDW(0.0f));
    m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE_MAX, FtoDW(20.0f));

    // ���Ƶ㾫�������仯�ĳߴ��С
    m_pd3dDevice->SetRenderState(D3DRS_POINTSCALE_A, FtoDW(0.0f));
    m_pd3dDevice->SetRenderState(D3DRS_POINTSCALE_B, FtoDW(0.0f));
    m_pd3dDevice->SetRenderState(D3DRS_POINTSCALE_C, FtoDW(1.0f));
}

void CPSystem::RenderPSystem(FLOAT fTimeDelta) 
{
    if (m_listParticles.empty()) return;                // û�л������
    BeginRenderPSystem();                               // ������Ⱦ״̬

    m_pd3dDevice->SetTexture(0, m_pParticleTex);        // ����
    m_pd3dDevice->SetFVF(PARTICLEVERTEX::FVF);          // �����ʽ
    m_pd3dDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(PARTICLEVERTEX));

    if (m_dwVBufOffset >= m_dwVBufSize)
        m_dwVBufOffset  = 0;

    PARTICLEVERTEX *pVertices = NULL;                   // ���㻺��
    m_pVertexBuffer->Lock(
        m_dwVBufOffset * sizeof(PARTICLEVERTEX), 
        m_dwBatchSize  * sizeof(PARTICLEVERTEX), 
        (void**)&pVertices, 
        m_dwVBufOffset ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD);

    DWORD dwVertices = 0;
    std::list<PARTICLEATTRIBUTE*>::iterator i;          // ������������
    for (i = m_listParticles.begin(); i != m_listParticles.end(); i++)
    {
        if (!(*i)->IsAlive) continue;                   // ��������������
        pVertices->_x = (*i)->Position.x;               // ������λ����䵽���㻺��
        pVertices->_y = (*i)->Position.y;
        pVertices->_z = (*i)->Position.z;
        pVertices->_color = (*i)->CurrColor;            // ��������ɫ��䵽���㻺��
        pVertices++, dwVertices++;

        if (dwVertices >= m_dwBatchSize)                // �������, ������Щ����
        {
            m_pVertexBuffer->Unlock();                  // �������㻺��
            m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, m_dwVBufOffset, m_dwBatchSize);

            dwVertices = 0, m_dwVBufOffset += m_dwBatchSize;    // ׼�����������һ������
            
            if (m_dwVBufOffset >= m_dwVBufSize) 
                m_dwVBufOffset  = 0;

            m_pVertexBuffer->Lock(
                m_dwVBufOffset * sizeof(PARTICLEVERTEX), 
                m_dwBatchSize  * sizeof(PARTICLEVERTEX), 
                (void**)&pVertices, 
                m_dwVBufOffset ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD);
        }
    }

    m_pVertexBuffer->Unlock();
    if (dwVertices != 0)                                // ����ʣ�������
        m_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, m_dwVBufOffset, dwVertices);
    m_dwVBufOffset += m_dwBatchSize;

    EndRenderPSystem();                                 // ����������Ⱦ״̬
}

void CPSystem::EndRenderPSystem(void) 
{
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, true);
    m_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, false);
    m_pd3dDevice->SetRenderState(D3DRS_POINTSCALEENABLE, false);
}

void CPSystem::RemoveDeadParticles(void) 
{
    std::list<PARTICLEATTRIBUTE*>::iterator i;
    i = m_listParticles.begin();
    while (i != m_listParticles.end())
    {
        if ((*i)->IsAlive) i++;
        else i = m_listParticles.erase(i);
    }
}


//--------------------------------------------------------------------------------------
// Name: CFireworkPSystem
// Desc: 
//--------------------------------------------------------------------------------------
CFireworksPSystem::CFireworksPSystem(IDirect3DDevice9 *pd3dDevice) 
    : CPSystem(pd3dDevice)
{
    m_fParticleSize = 0.8f;     // ���ӵĴ�С
    m_dwVBufSize    = 5000;     // ���㻺���С
    m_dwVBufOffset  = 0;
    m_dwBatchSize   = 512;      // ÿ�λ������ӵ���Ŀ
    m_vOrigin       = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
}

CFireworksPSystem::~CFireworksPSystem() 
{
}

void CFireworksPSystem::ResetOriginPos(D3DXVECTOR3 *vOrigin) 
{
    m_vOrigin = *vOrigin;
}

void CFireworksPSystem::ResetParticle(PARTICLEATTRIBUTE *pParticleAttr) 
{
    if (pParticleAttr == NULL) return;
    pParticleAttr->IsAlive  = TRUE;
    pParticleAttr->Position = m_vOrigin;                    // ���ӵĳ�ʼλ��

    D3DXVECTOR3 min = D3DXVECTOR3(-1.0f, -1.0f, -1.0f);
    D3DXVECTOR3 max = D3DXVECTOR3( 1.0f,  1.0f,  1.0f);
    pParticleAttr->Velocity = GetRandomVector(&min, &max);  // ����������ٶȷ���
    D3DXVec3Normalize(&pParticleAttr->Velocity, &pParticleAttr->Velocity);
    pParticleAttr->Velocity *= 3.0f;                        // ���ӵ��ٶȴ�С
    pParticleAttr->CurrColor = D3DXCOLOR(
        GetRandomFloat(0.0f, 1.0f), 
        GetRandomFloat(0.0f, 1.0f), 
        GetRandomFloat(0.0f, 1.0f), 0.5f);                  // �����������ɫ
    pParticleAttr->AgeTime  = 0.0f;                         // ���ӵ�����
    pParticleAttr->LifeTime = GetRandomFloat(1.0f, 2.5f);   // ���ӵ���������
}

void CFireworksPSystem::UpdatePSystem(FLOAT fTimeDelta) 
{
    std::list<PARTICLEATTRIBUTE*>::iterator i;
    for (i = m_listParticles.begin(); i != m_listParticles.end(); i++)
    {
        if (!(*i)->IsAlive) continue;                       // ��������������
        (*i)->Position  += (*i)->Velocity * fTimeDelta;     // �����˶�
        (*i)->AgeTime   += fTimeDelta;
        //(*i)->CurrColor *= 0.996f;                          // ���ӵ���ɫ������
        if ((*i)->AgeTime >= (*i)->LifeTime) 
            (*i)->IsAlive = FALSE;                          // �������ڽ���, ��������
    }

    // �Ƴ�����������
    RemoveDeadParticles();
}

void CFireworksPSystem::BeginRenderPSystem() 
{
    CPSystem::BeginRenderPSystem();

    // ����������Alpha�ںϼ�����
    m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);

    // read, but don't write particles to z-buffer
    m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
}

void CFireworksPSystem::EndRenderPSystem() 
{
    CPSystem::EndRenderPSystem();

    m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
    m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
}
