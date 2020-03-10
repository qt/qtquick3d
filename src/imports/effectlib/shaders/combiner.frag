void frag()
{
    vec4 sourceColor = texture2D_0(TexCoord);
    vec4 summation = texture2D_downsample2(TexCoord) + texture2D_downsample4(TexCoord);
    vec3 bloom_result = sourceColor.xyz * exposureExp2 + summation.xyz;
    vec3 thresholded = clamp(bloom_result.xyz, 0.0, channelThreshold) / channelThreshold;
    vec3 tonemapped =  pow(thresholded, vec3(1.0 / gamma));
    vec3 final_color = mix(thresholded, tonemapped, tonemappingLerp);
    float resultAlpha = max(summation.a, sourceColor.a);

    fragOutput = vec4(final_color, resultAlpha);
}
