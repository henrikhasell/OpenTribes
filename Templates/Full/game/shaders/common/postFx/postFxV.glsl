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

#include "./../gl/hlslCompat.glsl"
#include "./../gl/torque.glsl"

in vec4 vPosition;
in vec4 vTexCoord0;
in vec4 vTexCoord1;


uniform vec4 rtParams0;
uniform vec4 rtParams1;
uniform vec4 rtParams2;
uniform vec4 rtParams3;

varying vec2 uv0;
varying vec2 uv1;
varying vec2 uv2;
varying vec2 uv3;
varying vec3 wsEyeRay;                 


void main()
{
   gl_Position = vPosition;

   // The scene was rendered into a texture, and GL stores that bottom up, so
   // the row order has to be inverted when sampling it back. Flipping the quad
   // instead collapses it, since the effect chain also feeds these coords to
   // the targets it writes.
   vec2 uv = vec2( vTexCoord0.s, 1.0 - vTexCoord0.t );

   uv0 = viewportCoordToRenderTarget( uv, rtParams0 ); 
   uv1 = viewportCoordToRenderTarget( uv, rtParams1 ); 
   uv2 = viewportCoordToRenderTarget( uv, rtParams2 ); 
   uv3 = viewportCoordToRenderTarget( uv, rtParams3 ); 
   
   wsEyeRay = vTexCoord1.xyz;
}
