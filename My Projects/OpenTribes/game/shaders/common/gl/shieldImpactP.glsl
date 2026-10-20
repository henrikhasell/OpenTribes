// HH: Added for Tribes 2 compatibility.

uniform sampler2D shieldMap;

varying vec2 shieldCoord;
varying vec4 shieldColor;

void main()
{
   // The second texture unit interpolates from the lit base to the map by
   // the base's alpha and keeps that alpha (0x006a5c16).
   vec3 shield = texture2D( shieldMap, shieldCoord ).rgb;
   gl_FragColor = vec4( mix( shieldColor.rgb, shield, shieldColor.a ),
                        shieldColor.a );
}
