void AMBIENT_LIGHT()
{
    DIFFUSE = TOTAL_AMBIENT_COLOR;
}

// BASE_COLOR is ignored below, it's just vec3(1.0) in most cases, unless there are vertex colors
// (and custom materials do not have built in diffuse maps etc.)

void DIRECTIONAL_LIGHT()
{
    DIFFUSE += uDiffuseColorFactor.rgb * LIGHT_COLOR * SHADOW_CONTRIB * vec3(max(0.0, dot(WORLD_NORMAL, -LIGHT_DIRECTION)));
}

void POINT_LIGHT()
{
    DIFFUSE += uDiffuseColorFactor.rgb * LIGHT_COLOR * LIGHT_ATTENUATION * SHADOW_CONTRIB * vec3(max(0.0, dot(WORLD_NORMAL, -LIGHT_DIRECTION)));
}

void MAIN()
{
    FRAGCOLOR = vec4(DIFFUSE + SPECULAR, ALPHA);
}
