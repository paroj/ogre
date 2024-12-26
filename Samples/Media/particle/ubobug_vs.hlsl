OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include "OgreUnifiedShader.h"

uniform mat4 modelViewProj;

MAIN_PARAMETERS
IN(vec4 position, POSITION)
IN(vec2 iUV, TEXCOORD0)
OUT(vec3 oVertexPos, TEXCOORD0)
OUT(vec2 oUV, TEXCOORD1)
MAIN_DECLARATION
{
    gl_Position = mul(modelViewProj, position);
    oVertexPos = position.xyz;
    oUV = iUV;
}