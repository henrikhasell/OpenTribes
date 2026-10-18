// HH: Added for Tribes 2 compatibility.
// Tribes 2's hi-res shield effect is a shape drawn over by the fixed-function
// pipeline (0x00694350): lit white, with special/shieldmap sphere mapped over
// it. Both are worked out here per vertex, as GL works them.

#include "lighting.glsl"

in vec4 vPosition;
in vec3 vNormal;

uniform mat4 modelview;
uniform mat4 objTrans;
uniform mat4 worldViewOnly;
uniform float visibility;

varying vec2 shieldCoord;
varying vec4 shieldColor;

void main()
{
   gl_Position = modelview * vec4( vPosition.xyz, 1.0 );

   // Tribes 2 never enables GL_NORMALIZE, so a normal comes through the
   // grown shape's inverse transpose at one over its scale.
   vec3 eyeNormal = ( worldViewOnly * vec4( vNormal, 0.0 ) ).xyz;
   eyeNormal /= dot( eyeNormal, eyeNormal );
   vec3 wsNormal = ( objTrans * vec4( vNormal, 0.0 ) ).xyz;
   wsNormal /= dot( wsNormal, wsNormal );

   // GL_SPHERE_MAP in Tribes 2's eye space, which keeps Torque's axes and
   // leaves the turn to GL's in the projection (0x0044746b).
   vec3 eyeDir = normalize( ( worldViewOnly * vec4( vPosition.xyz, 1.0 ) ).xyz );
   vec3 r = eyeDir - 2.0 * eyeNormal * dot( eyeNormal, eyeDir );
   float m = 2.0 * sqrt( r.x * r.x + r.y * r.y + ( r.z + 1.0 ) * ( r.z + 1.0 ) );
   shieldCoord = vec2( r.x / m + 0.5, r.y / m + 0.5 );

   // A white material under the scene's lights, clamped as GL clamps a
   // vertex's colour.
   vec3 wsPosition = ( objTrans * vec4( vPosition.xyz, 1.0 ) ).xyz;
   vec4 diffuse;
   vec4 specular;
   compute4Lights( vec3( 0.0 ), wsPosition, wsNormal, diffuse, specular );
   shieldColor = vec4( clamp( diffuse.rgb + ambient.rgb, 0.0, 1.0 ),
                       clamp( visibility, 0.0, 1.0 ) );
}
