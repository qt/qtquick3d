VARYING vec4 vCustomData;
VARYING vec3 vGlobalPosition;
void MAIN()
{
    vCustomData = INSTANCE_DATA;

    // MODEL_MATRIX does not exist when instancing
    vec4 pos = INSTANCE_MODEL_MATRIX * vec4(VERTEX, 1.0);
    vGlobalPosition = pos.xyz;
}
