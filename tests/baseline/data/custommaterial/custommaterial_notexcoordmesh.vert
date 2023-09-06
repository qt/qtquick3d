VARYING vec2 texcoord;

void MAIN()
{
    // no UVs in the mesh, assign something to UV0 ourselves
    UV0 = selector * VERTEX.xy + (1.0 - selector) * NORMAL.xy;
    texcoord = UV0;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
