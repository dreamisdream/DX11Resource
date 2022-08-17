//==============================================================
// File: SimpleEffect.fx
// Desc: ʹ��3������Ч����Ч���ļ�
//==============================================================
// ȫ�ֱ���
vector MtrlsDiffuse;                // ���ʵ���������ɫ
vector MtrlsAmbient;                // ���ʵĻ�������ɫ

float3 LightDir[3];                 // 3����Դ�Ĺ��շ���
float4 LightDiffuse[3];             // ��Դ����������ɫ
float4 LightAmbient[3];             // ��Դ�Ļ���������

matrix matWorld;                    // ����任����
matrix matWorldViewProj;            // ��Ͼ���=World*View*Projection

texture Texture0;                   // �����������
sampler Sampler0 = sampler_state    // ����������
{
    Texture   = <Texture0>;
    MagFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

// ������ɫ������ṹ
struct VS_INPUT 
{
    float4 Position : POSITION; 
    float4 Normal  : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

// ������ɫ������ṹ
struct VS_OUTPUT
{
    float4 Position : POSITION;
    float4 Diffuse  : COLOR0; 
    float2 TexCoord : TEXCOORD0;
};

// ������ɫ����ں���
VS_OUTPUT vs_main(VS_INPUT input, uniform int nNumLights) 
{
    VS_OUTPUT Output = (VS_OUTPUT) 0;
    Output.Position = mul(input.Position, matWorldViewProj);    // �����任��Ķ���λ��
    Output.TexCoord = input.TexCoord;                           // �������������

    // ������������ܵ�nNumLights�����պ����ɫֵ
    float4 vNormal  = normalize(mul(input.Normal, matWorld));   // �����任��Ķ��㷨����
    float3 vDiffuse = float3(0, 0, 0);                          // �ܵ���������ɫֵ
    float3 vAmbient = float3(0, 0, 0);                          // �ܵĻ�������ɫֵ
    for (int i=0; i<nNumLights; i++) 
    {
        vDiffuse += LightDiffuse[i] * saturate(dot(vNormal, LightDir[i]));
        vAmbient += LightAmbient[i] * saturate(dot(vNormal, LightDir[i]));
    }
    Output.Diffuse.rgb = MtrlsDiffuse * vDiffuse + MtrlsAmbient * vAmbient;
    Output.Diffuse.a  = 1.0f;
    return Output;
}

// ������ɫ������ṹ
struct PS_OUTPUT
{
    float4 PixelColor : COLOR0;
};

// ������ɫ����ں���
PS_OUTPUT ps_main( VS_OUTPUT input ) 
{ 
    PS_OUTPUT Output  = (PS_OUTPUT) 0;
    Output.PixelColor = tex2D(Sampler0, input.TexCoord) * input.Diffuse;
    return Output;
}

// ��1���ַ�: ʹ��1����Դ
Technique RenderSceneWith1Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(1);
        PixelShader  = compile ps_2_0 ps_main();
    }
}

// ��2���ַ�: ʹ��2����Դ
Technique RenderSceneWith2Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(2);
        PixelShader  = compile ps_2_0 ps_main();
    }
}

// ��3���ַ�: ʹ��3����Դ
Technique RenderSceneWith3Light
{
    Pass P0
    {
        VertexShader = compile vs_2_0 vs_main(3);
        PixelShader  = compile ps_2_0 ps_main();
    }
}
