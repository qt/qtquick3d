varying vec4 TexCoordBLL; // Bottom Left and Bottom Tap
varying vec4 TexCoordTLT; // Top Left and Top Tap
varying vec4 TexCoordTRR; // Upper Right and Right Tap
varying vec4 TexCoordBRB; // Bottom Right and Bottom Tap

void frag (void)
{
    vec4 centerTap = texture2D_0(TexCoord);
    vec4 edgeTap = texture2D_0(TexCoordBLL.xy)
            + texture2D_0(TexCoordBLL.zw)
            + texture2D_0(TexCoordTLT.xy)
            + texture2D_0(TexCoordTLT.zw)
            + texture2D_0(TexCoordTRR.xy)
            + texture2D_0(TexCoordTRR.zw)
            + texture2D_0(TexCoordBRB.xy)
            + texture2D_0(TexCoordBRB.zw);
    vec3 edgeDetect = 8.0 * (centerTap.rgb - (0.125 * edgeTap.rgb));
    edgeDetect = clamp(edgeDetect, 0.0, centerTap.a);

    colorOutput(vec4(mix(centerTap.rgb, edgeDetect, edgeStrength), centerTap.a));
}
