// HH: Added for Tribes 2 compatibility.
// The light retail's interior lighting pass adds over a surface
// (0x00518c10), for the dynamic lights in slots 1 to 3; slot 0 is the sun,
// which the lightmap already holds. Uses the uniforms lighting.glsl
// declares.
//
// For each light it puts special/lightFalloffMono flat on the surface's
// plane, centred under the light and spanning the circle the light's
// sphere cuts from the plane, and adds it in the light's colour at
// (radius - distance to the plane) / radius. A light behind the surface
// lights it as well as one in front.

vec3 interiorLightsT2( vec3 wsPosition, vec3 wsNormal, sampler2D falloff )
{
   vec3 sum = vec3( 0.0 );

   for ( int i = 1; i < 4; i++ )
   {
      if ( inLightInvRadiusSq[i] <= 0.0 )
         continue;

      float radius = inversesqrt( inLightInvRadiusSq[i] );
      vec3 lightPos = vec3( inLightPos[0][i], inLightPos[1][i], inLightPos[2][i] );

      float d = dot( lightPos - wsPosition, wsNormal );
      float ad = abs( d );
      if ( ad >= radius )
         continue;

      vec3 centre = lightPos - wsNormal * d;
      float mr = sqrt( radius * radius - d * d );

      vec3 s = normalize( cross( wsNormal, abs( wsNormal.z ) < 0.9 ? vec3( 0.0, 0.0, 1.0 ) : vec3( 0.0, 1.0, 0.0 ) ) );
      vec3 t = normalize( cross( wsNormal, s ) );

      vec3 rel = wsPosition - centre;
      vec2 uv = ( vec2( dot( rel, s ), dot( rel, t ) ) / mr + 1.0 ) * 0.5;
      if ( any( lessThan( uv, vec2( 0.0 ) ) ) || any( greaterThan( uv, vec2( 1.0 ) ) ) )
         continue;

      float factor = ( radius - ad ) / radius;
      sum += inLightColor[i].rgb * texture2D( falloff, uv ).rgb * factor;
   }

   return sum;
}
