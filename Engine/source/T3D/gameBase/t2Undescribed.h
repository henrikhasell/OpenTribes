// HH: Added for Tribes 2 compatibility.
#ifndef _T2UNDESCRIBED_H_
#define _T2UNDESCRIBED_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// Which classes read their datablock body without the table naming its
/// fields, and so fell back to telling fields apart by the shape of a run.
///
/// The fallback is correct and it is worse, which is exactly the combination
/// that goes unnoticed: a body sending two of nine optional floats files none
/// of them, where a described class files two.  It was silence of this kind
/// that let naming the fields *stop* thirteen of them being read for every
/// ShapeBaseData subclass -- the lookup is per concrete class, only
/// ShapeBaseData was described, and ShapeBaseData is never sent.  Every gate
/// stayed green.
///
/// So a fallback is recorded rather than taken quietly.  t2Undescribed() is
/// the list, and Tools/t2/gendatablocknames.py predicts the same set from the
/// tables without running anything, which is what the gate checks.
namespace T2Undescribed
{
   /// Called by a reader the first time it resolves a class and finds no
   /// names.  Cheap and once per class: the node maps are cached.
   void note(const char *className);

   U32 count();
   const char *at(U32 i);
}

#endif
