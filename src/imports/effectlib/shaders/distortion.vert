varying vec2 center_vec;

void vert ()
{
  center_vec = TexCoord - center;
  // Multiply by x/y ratio to make the distortion round instead of an ellipse
  center_vec.y *= Texture0Info.y / Texture0Info.x;
}
