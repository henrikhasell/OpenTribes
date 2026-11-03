// HH: Added for Tribes 2 compatibility.
// The finished frame, drawn back over itself through the gamma ramp.

in vec4 vPosition;
in vec4 vTexCoord0;

varying vec2 uv;

void main()
{
   gl_Position = vPosition;
   uv = vTexCoord0.st;
}
