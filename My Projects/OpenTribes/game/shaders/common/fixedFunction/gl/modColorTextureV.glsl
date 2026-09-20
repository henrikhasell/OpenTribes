in vec4 vPosition;
in vec4 vColor;
in vec4 vTexCoord0;

uniform mat4 modelview;

out vec4 color;
out vec2 texCoord;

void main()
{
   gl_Position = modelview * vPosition;
   color = vColor;
   texCoord = vTexCoord0.st;
}
