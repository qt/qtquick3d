void frag() // Create RGBL buffer
{
    vec4 color = texture2D_0(TexCoord);
    color.a = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    gl_FragColor = color;
}
