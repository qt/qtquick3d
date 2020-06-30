#ifdef QQ3D_SHADER_META
/*{
    "outputs": [
        { "type": "vec2", "name": "center_vec", "stage": "vertex" }
    ]
}*/
#endif // QQ3D_SHADER_META

void vert ()
{
  center_vec = TexCoord - center;
  // Multiply by x/y ratio to make the distortion round instead of an ellipse
  center_vec.y *= Texture0Info.y / Texture0Info.x;
}
