VARYING vec3 vNormal;
VARYING vec3 vViewVec;

void MAIN()
{
    vec3 pos = VERTEX;
    pos.x += sin(uTime * 4.0 + pos.y) * uAmplitude;
    vNormal = normalize(NORMAL_MATRIX * NORMAL);
    vViewVec = CAMERA_POSITION - (MODEL_MATRIX * vec4(pos, 1.0)).xyz;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
