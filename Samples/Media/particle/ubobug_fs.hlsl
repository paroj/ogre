OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include "OgreUnifiedShader.h"

struct LIGHT_OUTPUT
{
    vec3 diffuse;
    vec3 specular;
};

float computeAttenuation(vec3 vertexposition, vec3 lightposition, vec4 attenuation)
{
    float d = distance(vertexposition, lightposition);
    return 1 / (attenuation.y + attenuation.z * d + attenuation.w * d * d);
}

float computeSpotlight(vec3 LightToVertexNorm, vec3 lightdirection, vec3 spotlightparams)
{
    float cosSpotLightAngle = saturate(dot(LightToVertexNorm, lightdirection));
    float spotFactor = pow(saturate(cosSpotLightAngle - spotlightparams.y) / (spotlightparams.x - spotlightparams.y),
                           spotlightparams.z);
    return spotFactor;
}

LIGHT_OUTPUT computeLighting(vec3 lightposition, vec3 P, vec3 N, vec3 V, float materialsh,
                             vec4 lightattenuation, vec3 lightspecular, vec3 lightdiffuse, vec3 spotlightparams,
                             vec3 lightdirection)
{
    vec3 L = normalize(lightposition - P);
    float diffuseLight = max(dot(N, L), 0.);
    vec3 H = normalize(L + V);
    float specularLight = pow(max(dot(N, H), 0.), materialsh);
    if (diffuseLight <= 0.)
        specularLight = 0.;

    float AttenuationFactor = computeAttenuation(P, lightposition, lightattenuation);

    float spotlight = 1.0f;
    if (spotlightparams[0] != 1.0f)
        spotlight = computeSpotlight(normalize((P - lightposition)), lightdirection, spotlightparams);

    LIGHT_OUTPUT Out;
    Out.diffuse = diffuseLight * AttenuationFactor * spotlight * lightdiffuse;
    Out.specular = specularLight * AttenuationFactor * spotlight * lightspecular;

    return Out;
}

uniform vec4 lightDiffuse[20];
uniform vec4 lightSpecular[20];
uniform vec4 lightAttenuation[20];
uniform vec4 lightPosition[20];
uniform vec4 lightDirection[20];
uniform vec4 lightSpotParams[20];

SAMPLER2D(DiffuseMap, 0);

MAIN_PARAMETERS
IN(vec3 position, TEXCOORD0)
IN(vec2 uv, TEXCOORD1)
MAIN_DECLARATION
{
    vec4 color = vec4(0, 0, 0, 0);
    vec3 DiffuseMapColour = texture2D(DiffuseMap, uv).xyz;

    color.xyz += DiffuseMapColour * 0.5;

    color.w = 0.5;

    float asdw = 0.0;

    asdw += position.x * 0.00001;
    asdw += position.y * 0.00001;
    asdw += position.z * 0.00001;

    const float multiplierAsd = 0.01;

    vec3 P = vec3(1, 0, 0);
    vec3 V = vec3(0, 0, 1);
    vec3 N = vec3(0, 1, 0);
    vec3 diffuse = vec3(0, 0, 0);
    vec3 specular = vec3(0, 0, 0);
    float materialsh = 0.5;

#ifdef OGRE_HLSL
    [unroll]
#endif
    for (int i = 0; i < 20; i++)
    {
        LIGHT_OUTPUT tmpLightOut =
            computeLighting(lightPosition[i].xyz, P, N, V, materialsh, lightAttenuation[i], lightSpecular[i].xyz,
                            lightDiffuse[i].xyz, lightSpotParams[i].xyz, lightDirection[i].xyz);
        diffuse += tmpLightOut.diffuse;
        specular += tmpLightOut.specular;
    }

    diffuse = saturate(diffuse);
    specular = saturate(specular);
    asdw += diffuse.x * multiplierAsd;
    asdw += diffuse.y * multiplierAsd;
    asdw += diffuse.z * multiplierAsd;
    asdw += specular.x * multiplierAsd;
    asdw += specular.y * multiplierAsd;
    asdw += specular.z * multiplierAsd;

    color.x += asdw;

    color.x *= 0.000001;
    color.x += lightDiffuse[0].x;

    gl_FragColor = color;
}