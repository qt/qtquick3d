#define FXAA_GATHER4_ALPHA 0
#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_LINEAR 1
#include "Fxaa3_11.glsllib"

void MAIN() // Mix the input blur and the depth texture with the sprite
{
    float sprightA = texture(sprite, INPUT_UV).a;
    vec4 aaPix = FxaaPixelShader( INPUT_UV, vec4(0.0,0.0,0.0,0.0), INPUT, INPUT, INPUT, vec2(1.0 / OUTPUT_SIZE.x, 1.0 / OUTPUT_SIZE.y),
                                  vec4(0,0,0,0), vec4(0,0,0,0), vec4(0,0,0,0), 0.75, 0.166, 0.0833, 8.0,  0.125, 0.05, vec4(0,0,0,0) );
    FRAGCOLOR = vec4(aaPix.rgb, sprightA );
}
