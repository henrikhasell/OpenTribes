in vec4 color;
in vec2 texCoord;

uniform sampler2D diffuseMap;

void main()
{
   gl_FragColor = color * texture2D( diffuseMap, texCoord );
}
