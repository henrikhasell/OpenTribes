in vec4 vPosition;
in vec4 vColor;

uniform mat4 modelview;

out vec4 color;

void main()
{
   gl_Position = modelview * vPosition;
   color = vColor;
}
