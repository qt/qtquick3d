#include "depthpass.glsllib"

uniform vec2 CameraClipRange;

void frag()
{
    vec4 depthSample = texture(depthTexture, TexCoord);
    float depthVal = getDepthValue(depthSample, CameraClipRange);
    float rawDepth = depthValueToLinearDistance(depthVal, CameraClipRange);

    float depthScale = abs(CameraClipRange.y - CameraClipRange.x);
    float depthDisp = abs(rawDepth - focusDepth) / depthScale;
    float finalDisperse = aberrationAmount * depthDisp;
    float effectAmt = texture(maskTexture, TexCoord).x;

    gl_FragColor = texture(sourceTexture, TexCoord);

    ivec2 texSize = textureSize(sourceTexture, 0);
    vec2 dispDir = normalize(TexCoord.xy - vec2(0.5)) / (2.0 * vec2(texSize));
    vec3 mixColor;
    mixColor = gl_FragColor.rgb;
    mixColor.r = texture(sourceTexture, TexCoord + dispDir * finalDisperse).r;
    mixColor.b = texture(sourceTexture, TexCoord - dispDir * finalDisperse).b;

    gl_FragColor.rgb = mix(gl_FragColor.rgb, mixColor, effectAmt);
}
