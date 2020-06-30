#include "blur.glsllib"

#ifdef QQ3D_SHADER_META
/*{
    "uniforms": [
        { "type": "sampler2D", "name": "glowSampler", "stage": "fragment" }
    ]
}*/
#endif // QQ3D_SHADER_META

void frag()
{
    vec4 src = texture2D_0(TexCoord);

    float trailfade = 1.0 - fadeAmount;
    vec4 outCol = GaussianBlur3TapPremultiplied(glowSampler) * trailfade;

    // Change the color so that it looks different; saturate it a bit
    float srcSum = dot(vec3(1.0), src.rgb);
    src.rgb = src.rgb * 0.7 + vec3(srcSum) * 0.3;
    gl_FragColor.rgb = (1.0 - src.a) * outCol.rgb + src.rgb;
    gl_FragColor.a = src.a + outCol.a;
}
