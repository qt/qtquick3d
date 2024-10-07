void MAIN()
{
    vec4 tmp = vec4(INSTANCE_MODELVIEWPROJECTION_MATRIX[0][3],
                     INSTANCE_MODELVIEWPROJECTION_MATRIX[1][3],
                     INSTANCE_MODELVIEWPROJECTION_MATRIX[2][3],
                     INSTANCE_MODELVIEWPROJECTION_MATRIX[3][3]);

    float weight = dot(tmp, offset);
    vec3 pos = sin(weight) * MORPH_POSITION0 + cos(weight) * MORPH_POSITION1 +
                (1 - sin(weight) - cos(weight)) * VERTEX;

    POSITION = INSTANCE_MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
