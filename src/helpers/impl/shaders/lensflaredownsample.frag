void MAIN()
{
    FRAGCOLOR = vec4(max(vec3(0.0), texture(INPUT, INPUT_UV).rgb - lensFlareBloomBias) * lensFlareBloomScale, 1.0);
}
