#if !QSSG_ENABLE_RHI
varying vec4 TexCoordBLL; // Bottom Left and Bottom Tap
varying vec4 TexCoordTLT; // Top Left and Top Tap
varying vec4 TexCoordTRR; // Upper Right and Right Tap
varying vec4 TexCoordBRB; // Bottom Right and Bottom Tap
#endif

#ifdef QQ3D_SHADER_META
/*{
    "outputs": [
        { "type": "vec4", "name": "TexCoordBLL", "stage": "vertex" },
        { "type": "vec4", "name": "TexCoordTLT", "stage": "vertex" },
        { "type": "vec4", "name": "TexCoordTRR", "stage": "vertex" },
        { "type": "vec4", "name": "TexCoordBRB", "stage": "vertex" }
    ]
}*/
#endif // QQ3D_SHADER_META

void vert()
{
    vec2 delta = vec2(1.0 / Texture0Info.x, 1.0 / Texture0Info.y);
    TexCoordBLL = vec4(TexCoord.st, TexCoord.st) + vec4(-delta.xy, -delta.x, 0);
    TexCoordTLT = vec4(TexCoord.st, TexCoord.st) + vec4(-delta.x, delta.y, 0, delta.y);
    TexCoordTRR = vec4(TexCoord.st, TexCoord.st) + vec4(delta.xy, delta.x, 0);
    TexCoordBRB = vec4(TexCoord.st, TexCoord.st) + vec4(delta.x, -delta.y, 0, -delta.y);
}
