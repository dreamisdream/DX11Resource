//==============================================================
// File: SimpleEffect.fx
// Desc: 使用3个光照效果的效果文件
//==============================================================
// 全局变量
vector MtrlsDiffuse;                // 材质的漫反射颜色
vector MtrlsAmbient;                // 材质的环境光颜色

float3 LightDir[3];                 // 3个光源的光照方向
float4 LightDiffuse[3];             // 光源的漫反射颜色
float4 LightAmbient[3];             // 光源的环境光严肃

matrix matWorld;                    // 世界变换矩阵
matrix matWorldViewProj;            // 组合矩阵=World*View*Projection

texture Texture0;                   // 几何体的纹理
sampler Sampler0 = sampler_state    // 采样器对象
{
    Texture   = <Texture0>;
    MagFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

// 顶点着色器输入结构
struct VS_INPUT 
{
    float4 Position : POSITION; 
    float4 Normal  : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

// 顶点着色器输出结构
struct VS_OUTPUT
{
    float4 Position : POSITION;
    float4 Diffuse  : COLOR0; 
    float2 TexCoord : TEXCOORD0;
};

// 顶点着色器入口函数
VS_OUTPUT vs_main(VS_INPUT input, uniform int nNumLights) 
{
    VS_OUTPUT Output = (VS_OUTPUT) 0;
    Output.Position = mul(input.Position, matWorldViewProj);    // 经过变换后的顶点位置
    Output.TexCoord = input.TexCoord;                           // 顶点的纹理坐标

    // 计算出顶点在受到nNumLights个光照后的颜色值
    float4 vNormal  = normalize(mul(input.Normal, matWorld));   // 经过变换后的顶点法向量
    float3 vDiffuse = float3(0, 0, 0);                          // 总的漫反射颜色值
    float3 vAmbient = float3(0, 0, 0);                          // 总的环境光颜色值
    for (int i=0; i<nNumLights; i++) 
    {
        vDiffuse += LightDiffuse[i] * saturate(dot(vNormal, LightDir[i]));
        vAmbient += LightAmbient[i] * saturate(dot(vNormal, LightDir[i]));
    }
    Output.Diffuse.rgb = MtrlsDiffuse * vDiffuse + MtrlsAmbient * vAmbient;
    Output.Diffuse.a  = 1.0f;
    return Output;
}

// 像素着色器输出结构
struct PS_OUTPUT
{
    float4 PixelColor : COLOR0;
};

// 像素着色器入口函数
PS_OUTPUT ps_main( VS_OUTPUT input ) 
{ 
    PS_OUTPUT Output  = (PS_OUTPUT) 0;
    Output.PixelColor = tex2D(Sampler0, input.TexCoord) * input.Diffuse;
    return Output;
}

// 第1种手法: 使用1个光源
Technique RenderSceneWith1Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(1);
        PixelShader  = compile ps_2_0 ps_main();
    }
}

// 第2种手法: 使用2个光源
Technique RenderSceneWith2Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(2);
        PixelShader  = compile ps_2_0 ps_main();
    }
}

// 第3种手法: 使用3个光源
Technique RenderSceneWith3Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(3);
        PixelShader  = compile ps_2_0 ps_main();
    }
}
