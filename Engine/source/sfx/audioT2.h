// HH: Added for Tribes 2 compatibility.
// Tribes 2's names for a sound and its description.  The game's scripts ask
// for these classes and this engine had none, so nothing they declared was
// ever built.

#ifndef _AUDIOT2_H_
#define _AUDIOT2_H_

#ifndef _SFXPROFILE_H_
#include "sfx/sfxProfile.h"
#endif
#ifndef _SFXDESCRIPTION_H_
#include "sfx/sfxDescription.h"
#endif


/// How Tribes 2 describes a sound: its volume, whether it loops and whether it
/// is placed in the world.
///
/// Those three are Torque 3D's own fields under the same names, so there is
/// nothing to map.  Tribes 2 also gives a `type`, its mixer channel, which
/// Torque 3D has no notion of and which is left as a dynamic field.
class AudioDescription : public SFXDescription
{
   typedef SFXDescription Parent;

public:

   DECLARE_CONOBJECT( AudioDescription );
};


/// A sound, as Tribes 2 names one: a file, a description and whether to load
/// it up front.
///
/// All three are Torque 3D's own fields under the same names.  Tribes 2 also
/// gives an `effect`, an EffectProfile, which is left as a dynamic field.
class AudioProfile : public SFXProfile
{
   typedef SFXProfile Parent;

public:

   DECLARE_CONOBJECT( AudioProfile );
};

#endif // _AUDIOT2_H_
