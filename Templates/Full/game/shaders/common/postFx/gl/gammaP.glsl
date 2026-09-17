//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "../../gl/hlslCompat.glsl"
#include "../../gl/torque.glsl"
#include "shadergen:/autogenConditioners.h"

uniform sampler2D backBuffer;
// The ramp is a 2D image, and GL returns black for a 2D texture bound to a
// 1D sampler. D3D9's loosely typed samplers let the HLSL get away with tex1D.
uniform sampler2D colorCorrectionTex;

uniform float OneOverGamma;
in vec2 uv0;

out vec4 OUT_col;

void main()
{
    vec4 color = texture(backBuffer, uv0.xy);

   // Apply the color correction.
   color.r = texture( colorCorrectionTex, vec2( color.r, 0.0 ) ).r;
   color.g = texture( colorCorrectionTex, vec2( color.g, 0.0 ) ).g;
   color.b = texture( colorCorrectionTex, vec2( color.b, 0.0 ) ).b;

   // Apply gamma correction
   color.rgb = pow( clamp(color.rgb, vec3(0.0),vec3(1.0)), vec3(OneOverGamma) );

   // Apply contrast
 
   // Apply brightness
   
   OUT_col = color;
}