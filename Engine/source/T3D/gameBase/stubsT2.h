// HH: Added for Tribes 2 compatibility.
#ifndef _STUBST2_H_
#define _STUBST2_H_

class SimObject;

// HH: Added to warn once per unported Tribes 2 class.
namespace StubT2
{
   void warn(const char *kind, const char *className);

   /// A class this engine has only stubbed keeps its script's values as
   /// dynamic fields, because it declares none of its own. That is still
   /// everything a server has to send, so a stub says which of its script's
   /// fields go on which of its layout's nodes and the body is written from
   /// those.
   enum Kind
   {
      Whole,   ///< a float, sent as all 32 bits of one
      Colour,  ///< four bytes, as the layout's colour op takes them
      Word,    ///< a whole number
      Flag,    ///< a true or a false
      Text,      ///< a string
      DataBlock, ///< the name of another datablock, sent as its id
      Vector3,   ///< three floats in one script field, over three slots
   };

   struct Field
   {
      const char *name;   ///< what the script calls it
      U32         slot;   ///< the T2Layout field it travels in
      Kind        kind;
   };

   void publish(SimObject *object, const Field *fields, U32 count);
}

#endif
