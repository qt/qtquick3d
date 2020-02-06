varying vec2 center_vec;

void frag()
{
    float dist_to_center = length(center_vec) * 100.0 / radius;

    vec2 texc;
    if (dist_to_center > 1.0) {
        texc = TexCoord;
    } else {
        float r = radians(360.0) * (1.0 - dist_to_center);
        float distortion = sin(r * (100.0 - distortionWidth) + radians(distortionPhase));
        texc = TexCoord - (TexCoord - center) * distortion * distortionHeight / 800.0;
    }

    if (texc.x < 0.0 || texc.x > 1.0 || texc.y < 0.0 || texc.y > 1.0)
        gl_FragColor = vec4(0.0);
    else
        colorOutput(texture2D_0(texc));
}
