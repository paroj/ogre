#ifdef OGRE_HLSL
#define UNIFORM_BUFFER(name, reg) cbuffer name //: register(b##reg)
#else
#define UNIFORM_BUFFER(name, reg) layout(std140, row_major, binding = reg) uniform name
#endif

UNIFORM_BUFFER(OgreCurrentLights, 2)
{
    f32vec4 light_position_view_space_array1[LIGHT_COUNT];
    f32vec4 light_attenuation_array1[LIGHT_COUNT];
    f32vec4 spotlight_params_array1[LIGHT_COUNT];
    f32vec4 light_direction_view_space_array1[LIGHT_COUNT];
    f32vec4 light_diffuse_colour_power_scaled_array1[LIGHT_COUNT];
};