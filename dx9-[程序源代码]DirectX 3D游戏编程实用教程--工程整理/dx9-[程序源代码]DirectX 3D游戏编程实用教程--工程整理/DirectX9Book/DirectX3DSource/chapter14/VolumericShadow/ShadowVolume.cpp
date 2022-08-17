#include "ShadowVolume.h"

CShadowVolume::CShadowVolume(IDirect3DDevice9 *pd3dDevice) 
{
    m_pd3dDevice  = pd3dDevice;
    m_pVertexBuf  = NULL;
    m_pShadowMesh = NULL;
    m_pvVertices  = NULL;
    m_dwVertices  = 0;
}

CShadowVolume::~CShadowVolume() 
{
    SAFE_RELEASE(m_pVertexBuf);
    SAFE_RELEASE(m_pShadowMesh);
    if (m_pvVertices != NULL) 
        delete m_pvVertices;
}

HRESULT CShadowVolume::CreateShadowVolume(LPD3DXMESH pMesh) 
{
    // ��¡����Ӱ�����������ģ��
    pMesh->CloneMeshFVF(pMesh->GetOptions(), D3DFVF_XYZ, m_pd3dDevice, &m_pShadowMesh); 
    
    // Ϊ��Ӱ�嶥�����洢�ռ�
    DWORD dwNumFaces = m_pShadowMesh->GetNumFaces();
    m_pvVertices = new D3DXVECTOR3[dwNumFaces * 3 * 6];

    // ������Ӱ���㻺��
    m_pd3dDevice->CreateVertexBuffer(4 * sizeof(SHADOWVERTEX), D3DUSAGE_WRITEONLY, 
        SHADOWVERTEX::FVF, D3DPOOL_MANAGED, &m_pVertexBuf, NULL);

    float sx = 640.0f;
    float sy = 480.0f;

    SHADOWVERTEX *pVertices = NULL;
    m_pVertexBuf->Lock(0, 0, (void**)&pVertices, 0);
    pVertices[0] = SHADOWVERTEX(0.0f,   sy, 0.0f, 1.0f, D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f));
    pVertices[1] = SHADOWVERTEX(0.0f, 0.0f, 0.0f, 1.0f, D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f));
    pVertices[2] = SHADOWVERTEX(  sx,   sy, 0.0f, 1.0f, D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f));
    pVertices[3] = SHADOWVERTEX(  sx, 0.0f, 0.0f, 1.0f, D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f));
    m_pVertexBuf->Unlock();

    return S_OK;
}

HRESULT CShadowVolume::UpdateShadowVolume(D3DXVECTOR3 vLight) 
{
    D3DXVECTOR3* pVertices  = NULL;                     // ָ������ģ�͵Ķ��㻺��
    WORD*        pIndices   = NULL;                     // ָ������ģ�͵���������
    DWORD        dwNumEdges = 0;

    // �����Է�������ģ�͵Ķ��㻺����������������
    m_pShadowMesh->LockVertexBuffer(0, (void**)&pVertices);
    m_pShadowMesh->LockIndexBuffer(0, (void**)&pIndices);

    DWORD dwNumFaces = m_pShadowMesh->GetNumFaces();    // ȡ������ģ�͵�����
    WORD* pEdges = new WORD[dwNumFaces * 6];            // ������Ӱ��������ߵĶ�������

    // ������б������������
    for( DWORD i = 0; i < dwNumFaces; i++ ) 
    {
        WORD index0 = pIndices[3*i+0];                  // ��i�����3�����������
        WORD index1 = pIndices[3*i+1];
        WORD index2 = pIndices[3*i+2];

        D3DXVECTOR3 v0 = pVertices[index0];             // ��i�����3����������
        D3DXVECTOR3 v1 = pVertices[index1];
        D3DXVECTOR3 v2 = pVertices[index2];

        D3DXVECTOR3 vNormal;
        D3DXVec3Cross(&vNormal, &(v2-v1), &(v1-v0));    // ���㵱ǰ��ķ�����
        if (D3DXVec3Dot(&vNormal, &vLight) < 0.0f) continue;    // ����������Դ����

        AddFaceEdge(pEdges, dwNumEdges, index0, index1);        // ��ӵ���ʱ�ı��б���
        AddFaceEdge(pEdges, dwNumEdges, index1, index2);
        AddFaceEdge(pEdges, dwNumEdges, index2, index0);
    }

    // ������Ӱ��
    m_dwVertices = 0; 
    for (DWORD i = 0; i < dwNumEdges; i++) 
    {
        D3DXVECTOR3 v1 = pVertices[pEdges[2*i+0]];
        D3DXVECTOR3 v2 = pVertices[pEdges[2*i+1]];
        D3DXVECTOR3 v3 = v1 - vLight * 500.0f;          // ����1�������
        D3DXVECTOR3 v4 = v2 - vLight * 500.0f;          // ����2�������

        m_pvVertices[m_dwVertices++] = v1;              // ������V1V2V3
        m_pvVertices[m_dwVertices++] = v2;
        m_pvVertices[m_dwVertices++] = v3;

        m_pvVertices[m_dwVertices++] = v2;              // ������V2V4V3
        m_pvVertices[m_dwVertices++] = v4;
        m_pvVertices[m_dwVertices++] = v3;
    }
    delete pEdges;
    m_pShadowMesh->UnlockVertexBuffer();
    m_pShadowMesh->UnlockIndexBuffer();
    return S_OK;
}

VOID CShadowVolume::AddFaceEdge(WORD* pEdges, DWORD& dwNumEdges, WORD v0, WORD v1)
{
    for( DWORD i=0; i < dwNumEdges; i++ )
    {
        if ( (pEdges[2*i+0] == v0 && pEdges[2*i+1] == v1) || 
           (pEdges[2*i+0] == v1 && pEdges[2*i+1] == v0) )
        {
            pEdges[2*i+0] = pEdges[2*(dwNumEdges-1)+0]; // ����, ���Ƴ�������
            pEdges[2*i+1] = pEdges[2*(dwNumEdges-1)+1];
            dwNumEdges--; 
            return;
        }
    }
    pEdges[2*dwNumEdges+0] = v0;                        // ������, ����ӵ��б���
    pEdges[2*dwNumEdges+1] = v1;
    dwNumEdges++;
}

HRESULT CShadowVolume::RenderShadowVolume(BOOL bRenderVolume) 
{
    // ����z������д����, ������ģ�建����
    m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, false );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, true );

    if (bRenderVolume) 
    {
        // ��ʾ��Ӱ��
        m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
        m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

        // Ϊ��Ӱ�����ò���
        D3DMATERIAL9 mtrl;
        ZeroMemory( &mtrl, sizeof(D3DMATERIAL9) );
        mtrl.Ambient = D3DXCOLOR(1.0f, 1.0f, 0.0f, 0.2f);
        mtrl.Diffuse = D3DXCOLOR(1.0f, 1.0f, 0.0f, 0.2f);
        m_pd3dDevice->SetMaterial( &mtrl );
    }
    else
    {
        // ������Ȳ���
        m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
        m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
        m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
    }

    // ��Ⱦ��Ӱ��ǰ��, ��Ȳ�������ͨ��, �������ֵ��1(Z-Pass)
    m_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR ); 
    m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE,   D3DSHADE_FLAT );

    m_pd3dDevice->SetFVF( D3DFVF_XYZ );
    m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_dwVertices / 3, m_pvVertices, sizeof(D3DXVECTOR3) );

    // ��Ⱦ��Ӱ�屳��, ��Ȳ���ͨ�������ص����ֵ��1(Z-Pass)
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );

    m_pd3dDevice->SetFVF( D3DFVF_XYZ );
    m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, m_dwVertices / 3, m_pvVertices, sizeof(D3DXVECTOR3) );

    // �ָ���Ⱦ״̬
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,  D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, true );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, false );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, false );

    // �ر���Ȳ���, ����Alpha���
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, false );
    m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, true );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // ����ģ�������Ⱦ״̬
    m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, true );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILREF,  0x1 );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );

    // ��Ⱦһ����ɫ����, ֻ��ͨ��ģ����Ե����زŻᱻ��Ⱦ����ɫ������,��ʾ��Ӱ
    m_pd3dDevice->SetFVF( SHADOWVERTEX::FVF );
    m_pd3dDevice->SetStreamSource( 0, m_pVertexBuf, 0, sizeof(SHADOWVERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    // �ָ���Ⱦ״̬
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, true );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, false );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, false );
    m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE,   D3DSHADE_GOURAUD );
    return S_OK;
}
