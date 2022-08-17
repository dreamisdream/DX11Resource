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
// Name: class CFrustum
// Desc: 
//--------------------------------------------------------------------------------------
class CFrustum
{
protected:
    LPDIRECT3DDEVICE9   m_pd3dDevice;
    D3DXPLANE           m_planes[6];

public:
    CFrustum(IDirect3DDevice9 *pd3dDevice);
    virtual ~CFrustum(void);

    VOID UpdateFrustum(void);
	VOID ConstructPlanes(void); // 构造裁减体的六个平面

	BOOL CheckPoint(D3DXVECTOR3 ptPos);
	BOOL CheckCube(D3DXVECTOR3 centerPos, float size);
	BOOL CheckRectangle(D3DXVECTOR3 centerPos, float size);
	BOOL CheckSphere(D3DXVECTOR3 centerPos, float radius);

};
