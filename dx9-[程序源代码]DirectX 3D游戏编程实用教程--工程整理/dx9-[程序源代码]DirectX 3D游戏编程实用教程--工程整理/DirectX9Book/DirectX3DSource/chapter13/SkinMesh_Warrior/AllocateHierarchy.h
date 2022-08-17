#pragma once
//=============================================================================
// Desc: AllocateHierarchy.h
//=============================================================================

#include <d3d9.h>
#include <d3dx9.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//-----------------------------------------------------------------------------
// Name: struct D3DXFRAME_DERIVED
// Desc: 继承自DXDXFRAME结构的结构
//-----------------------------------------------------------------------------
struct D3DXFRAME_DERIVED: public D3DXFRAME
{
    D3DXMATRIXA16 CombinedTransformationMatrix;
};


//-----------------------------------------------------------------------------
// Name: struct D3DXMESHCONTAINER_DERIVED
// Desc: 继承自D3DXMESHCONTAINER结构的结构
//-----------------------------------------------------------------------------
struct D3DXMESHCONTAINER_DERIVED: public D3DXMESHCONTAINER
{
    LPDIRECT3DTEXTURE9*  ppTextures;            //纹理数组
    LPD3DXMESH           pOrigMesh;             //原始网格
    LPD3DXATTRIBUTERANGE pAttributeTable;
    DWORD                NumAttributeGroups;    //属性组数量,即子网格数量
	DWORD                NumInfl;               //每个顶点最多受多少骨骼的影响
    LPD3DXBUFFER         pBoneCombinationBuf;   //骨骼结合表
    D3DXMATRIX**         ppBoneMatrixPtrs;      //存放骨骼的组合变换矩阵
    D3DXMATRIX*          pBoneOffsetMatrices;   //存放骨骼的初始变换矩阵
	DWORD                NumPaletteEntries;     //骨骼数量上限
	bool                 UseSoftwareVP;         //标识是否使用软件顶点处理
};


//-----------------------------------------------------------------------------
// Name: class CAllocateHierarchy
// Desc: 该类用来从.X文件加载框架层次和网格模型数据
//-----------------------------------------------------------------------------
class CAllocateHierarchy: public ID3DXAllocateHierarchy
{
public:
	STDMETHOD(CreateFrame)(THIS_ LPCSTR Name, LPD3DXFRAME *ppNewFrame);
	STDMETHOD(CreateMeshContainer)( THIS_ LPCSTR              Name, 
                                    CONST D3DXMESHDATA*       pMeshData,
                                    CONST D3DXMATERIAL*       pMaterials, 
                                    CONST D3DXEFFECTINSTANCE* pEffectInstances, 
                                    DWORD                     NumMaterials, 
                                    CONST DWORD *             pAdjacency, 
                                    LPD3DXSKININFO pSkinInfo, 
                                    LPD3DXMESHCONTAINER *ppNewMeshContainer);    
    STDMETHOD(DestroyFrame)(THIS_ LPD3DXFRAME pFrameToFree);
    STDMETHOD(DestroyMeshContainer)(THIS_ LPD3DXMESHCONTAINER pMeshContainerBase);
};


//-----------------------------------------------------------------------------
// Desc: 全局函数
//-----------------------------------------------------------------------------
void DrawFrame( IDirect3DDevice9* pd3dDevice, LPD3DXFRAME pFrame );
void DrawMeshContainer( IDirect3DDevice9* pd3dDevice, LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase );
HRESULT SetupBoneMatrixPointers( LPD3DXFRAME pFrameBase, LPD3DXFRAME pFrameRoot );
void UpdateFrameMatrices( LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix );
void SmoothChangeAnimation( LPD3DXANIMATIONCONTROLLER pAnimController, LPD3DXANIMATIONSET pAnimSet, FLOAT fCurrTime );
