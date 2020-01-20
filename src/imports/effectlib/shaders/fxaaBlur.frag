#define FXAA_VERION 311
#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_LINEAR 1
#if FXAA_VERION >= 311
#include "Fxaa3_11.glsllib"
#else
#include "Fxaa3_8.glsllib"
#endif

void frag() // Mix the input blur and the depth texture with the sprite
{
        float sprightA = texture2D_sprite( TexCoord ).a;
#if FXAA_VERION >= 311
        vec4 aaPix = FxaaPixelShader( TexCoord, vec4(0.0,0.0,0.0,0.0), Texture0, Texture0, Texture0, vec2(1.0 / DestSize.x, 1.0 / DestSize.y),
                                                                  vec4(0,0,0,0), vec4(0,0,0,0), vec4(0,0,0,0), 0.75, 0.166, 0.0833, 8.0,  0.125, 0.05, vec4(0,0,0,0) );
#else
        vec4 aaPix = FxaaPixelShader( TexCoord, vec4(0.0,0.0,0.0,0.0), Texture0, vec2(1.0 / DestSize.x, 1.0 / DestSize.y), vec4(0,0,0,0) );
#endif
        gl_FragColor = vec4(aaPix.rgb, sprightA );
}
