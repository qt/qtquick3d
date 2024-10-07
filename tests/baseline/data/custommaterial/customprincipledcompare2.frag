void MAIN()
{
    vec3 rgb = uBaseColorSrgb.rgb;
    float C1 = 0.305306011;
    vec3 C2 = vec3(0.682171111, 0.682171111, 0.682171111);
    vec3 C3 = vec3(0.012522878, 0.012522878, 0.012522878);
    BASE_COLOR = vec4(rgb * (rgb * (rgb * C1 + C2) + C3), uBaseColorSrgb.a);
    METALNESS = uMetalness;
    ROUGHNESS = uRoughness;
    SPECULAR_AMOUNT = uSpecularAmount;
}
