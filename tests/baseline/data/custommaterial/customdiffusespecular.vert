VARYING vec3 vNormal;
VARYING vec3 vViewVec;

void MAIN()
{
    VERTEX.x += sin(uTime * 4.0 + VERTEX.y) * uAmplitude;
    vNormal = normalize(NORMAL_MATRIX * NORMAL);
    vViewVec = CAMERA_POSITION - (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
