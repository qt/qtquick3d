void AMBIENT_LIGHT()
{
    DIFFUSE += TOTAL_AMBIENT_COLOR;
}

void DIRECTIONAL_LIGHT()
{
    // Uses WORLD_NORMAL because the asset is rendered with no culling so
    // double sidedness needs adjusting the normal, and that adjusted value is in
    // WORLD_NORMAL, not VAR_WORLD_NORMAL (that's the unmodified interpolated normal)
    DIFFUSE += vec3(1.0, 0.0, 0.0) * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(WORLD_NORMAL), TO_LIGHT_DIR)));
}

void SPECULAR_LIGHT()
{
}
