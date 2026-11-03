// HH: Added for Tribes 2 compatibility.
// Retail's setGammaCorrection (0x00564850) builds the ramp it hands
// SetDeviceGammaRamp as (i / 256) ^ gamma for each 8-bit level i, the
// same curve for red, green and blue.

uniform sampler2D frame;
uniform float gamma;

varying vec2 uv;

void main()
{
   vec3 level = floor( texture2D( frame, uv ).rgb * 255.0 + 0.5 );
   gl_FragColor = vec4( pow( level / 256.0, vec3( gamma ) ), 1.0 );
}
