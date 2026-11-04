// HH: Added for Tribes 2 compatibility.
// A shadow's outline as Tribes 2 has it: what the shape covers is a bit
// (0x0065319d), and each bit widens to a byte, full where covered, through
// a 3x3 blur of 1 2 1, 2 3 2, 1 2 1 over 15 at the larger sizes (0x00443cd0).

in vec4 color;
in vec2 texCoord;

uniform sampler2D diffuseMap;
uniform vec2 texelSize;
uniform float blur;

float covered( vec2 offset )
{
   vec2 uv = texCoord + offset * texelSize;
   if ( uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0 )
      return 0.0;

   return texture2D( diffuseMap, uv ).a > 0.0 ? 1.0 : 0.0;
}

void main()
{
   float shade = covered( vec2( 0.0, 0.0 ) );

   if ( blur > 0.5 )
   {
      shade = 3.0 * shade
            + 2.0 * ( covered( vec2( -1.0,  0.0 ) ) + covered( vec2( 1.0, 0.0 ) ) +
                      covered( vec2(  0.0, -1.0 ) ) + covered( vec2( 0.0, 1.0 ) ) )
            + covered( vec2( -1.0, -1.0 ) ) + covered( vec2( 1.0, -1.0 ) )
            + covered( vec2( -1.0,  1.0 ) ) + covered( vec2( 1.0,  1.0 ) );
      shade /= 15.0;
   }

   gl_FragColor = vec4( shade );
}
