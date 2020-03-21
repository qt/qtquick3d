#if !QSSG_ENABLE_RHI
varying vec2 center_vec;
#endif

#ifdef QQ3D_SHADER_META
/*{
    "inputs": [
        { "type": "vec2", "name": "center_vec", "stage": "fragment" }
    ]
}*/
#endif // QQ3D_SHADER_META

void frag()
{
    float dist_to_center = length(center_vec) / radius;

    vec2 texc;
    if (dist_to_center > 1.0) {
        texc = TexCoord;
    } else {
        float distortion = 1.0 - dist_to_center * dist_to_center;
        texc = TexCoord - (TexCoord - center) * distortion * distortionHeight;
    }

    if (texc.x < 0.0 || texc.x > 1.0 || texc.y < 0.0 || texc.y > 1.0)
        gl_FragColor = vec4(0.0);
    else
        colorOutput(texture2D_0(texc));
}
