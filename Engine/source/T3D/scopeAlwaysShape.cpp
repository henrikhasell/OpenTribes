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

#include "T3D/staticShape.h"

class ScopeAlwaysShape : public StaticShape
{
      typedef StaticShape Parent;

   public:
      ScopeAlwaysShape();
      static void initPersistFields();

      /// HH: Added for Tribes 2 compatibility.  The same body AIObjective
      /// reads, through the same reader: the two layouts are byte-identical
      /// and both are StaticShape's reading unchanged.  See
      /// T3D/gameBase/aiObjectiveT2.h.
      void readT2Fields(T2Reader& reader);

      DECLARE_CONOBJECT(ScopeAlwaysShape);
};

ScopeAlwaysShape::ScopeAlwaysShape()
{
   mNetFlags.set(Ghostable|ScopeAlways);
   mTypeMask |= StaticShapeObjectType;
}

void ScopeAlwaysShape::initPersistFields()
{
   Parent::initPersistFields();
}

void ScopeAlwaysShape::readT2Fields(T2Reader& reader)
{
   // StaticShape's body: ShapeBase's, then where it stands and whether it
   // has power.  Qualified because this class's own override is what is
   // running; Parent::readT2Fields would come straight back here.
   StaticShape::readT2Fields( reader );
}

IMPLEMENT_CO_NETOBJECT_V1(ScopeAlwaysShape);

ConsoleDocClass( ScopeAlwaysShape,
   "@brief StaticShape object which is always scoped.\n\n"

   "@note Exists for backwards compatibility, no real use anymore. Go with TSStatic or a standard StaticShape instead.\n\n"

   "@internal\n"
);
