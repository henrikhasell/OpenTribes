#ifndef _T2GHOSTFIELDS_H_
#define _T2GHOSTFIELDS_H_

namespace T2Protocol
{
   enum T2GhostField
   {
      T2FieldPosition,              ///< a compressed point, world space
      T2FieldYaw,                   ///< readFloat(bits) turns of heading
      T2FieldVelocityHere,          ///< the flag a velocity follows, clear
      T2FieldSpeed,                 ///< readInt(bits) thirty-seconds of a
      T2FieldDirection,             ///< readNormalVector(10), both of its
      T2FieldTransform,             ///< the first of a MatrixF's 16 floats
      T2FieldAffine,                ///< the first of a position and a quat's
      T2FieldQuatSign,              ///< the flag that negates a quat's w
      T2FieldScale,                 ///< the first of a Point3F's three floats
      T2FieldDamage,                ///< readFloat(bits) of the object's
                                    ///< maxDamage
   };

   inline U32 t2FieldWords(U32 field)
   {
      switch(field)
      {
         case T2FieldTransform: return 16;
         case T2FieldAffine:    return 6;
         case T2FieldScale:     return 3;
         default:               return 0;
      }
   }

   struct T2GhostFieldRow
   {
      const char   *cls;
      S16           node;
      U8            op;
      U8            bits;
      T2GhostField  field;
   };

   static const T2GhostFieldRow sT2GhostFields[] =
   {
      { "Player", 25, OpPoint, 0,  T2FieldPosition     },
      { "Player", 24, OpFlag,  1,  T2FieldVelocityHere },
      { "Player", 23, OpRaw,   13, T2FieldSpeed        },
      { "Player", 22, OpRaw,   21, T2FieldDirection    },
      { "Player", 19, OpRaw,   7,  T2FieldYaw          },

   // ShapeBase::unpackUpdate reads the damage at 0x005ef194 as readFloat(6) of
   // the datablock's maxDamage, followed by two bits of state.
      { "Player",         223, OpRaw, 6, T2FieldDamage },
      { "Vehicle",        214, OpRaw, 6, T2FieldDamage },
      { "FlyingVehicle",  217, OpRaw, 6, T2FieldDamage },
      { "HoverVehicle",   215, OpRaw, 6, T2FieldDamage },
      { "WheeledVehicle", 234, OpRaw, 6, T2FieldDamage },

      { "InteriorInstance",      11, OpRaw, 32, T2FieldTransform },
      { "InteriorInstance",      27, OpRaw, 32, T2FieldScale     },
      { "InteriorInstance",      33, OpRaw, 32, T2FieldTransform },
      { "InteriorInstance",      49, OpRaw, 32, T2FieldScale     },

      { "TSStatic",               0, OpRaw, 32, T2FieldTransform },
      { "TSStatic",              16, OpRaw, 32, T2FieldScale     },

      { "ParticleEmissionDummy",  2, OpRaw, 32, T2FieldTransform },
      { "ParticleEmissionDummy", 18, OpRaw, 32, T2FieldScale     },

      { "PhysicalZone",           3, OpRaw, 32, T2FieldTransform },
      { "PhysicalZone",          19, OpRaw, 32, T2FieldScale     },

      { "VehicleBlocker",         0, OpRaw, 32, T2FieldTransform },

      { "StaticShape",           11, OpRaw,  32, T2FieldAffine   },
      { "StaticShape",            5, OpFlag,  1, T2FieldQuatSign },
      { "StaticShape",            4, OpRaw,  32, T2FieldScale    },

      { "ScopeAlwaysShape",       6, OpRaw,  32, T2FieldAffine   },
      { "ScopeAlwaysShape",      12, OpFlag,  1, T2FieldQuatSign },
      { "ScopeAlwaysShape",      13, OpRaw,  32, T2FieldScale    },

      { "BeaconObject",           8, OpRaw,  32, T2FieldAffine   },
      { "BeaconObject",          14, OpFlag,  1, T2FieldQuatSign },
      { "BeaconObject",          15, OpRaw,  32, T2FieldScale    },

      { "Turret",                18, OpRaw,  32, T2FieldAffine   },
      { "Turret",                12, OpFlag,  1, T2FieldQuatSign },
      { "Turret",                11, OpRaw,  32, T2FieldScale    },

      { "MissionMarker",         10, OpRaw,  32, T2FieldAffine   },
      { "MissionMarker",          4, OpFlag,  1, T2FieldQuatSign },
      { "MissionMarker",          3, OpRaw,  32, T2FieldScale    },

      { "WayPoint",              16, OpRaw,  32, T2FieldAffine   },
      { "WayPoint",              10, OpFlag,  1, T2FieldQuatSign },
      { "WayPoint",               9, OpRaw,  32, T2FieldScale    },

      { "SpawnSphere",           15, OpRaw,  32, T2FieldAffine   },
      { "SpawnSphere",            9, OpFlag,  1, T2FieldQuatSign },
      { "SpawnSphere",            8, OpRaw,  32, T2FieldScale    },

      { "AIObjective",            6, OpRaw,  32, T2FieldAffine   },
      { "AIObjective",           12, OpFlag,  1, T2FieldQuatSign },
      { "AIObjective",           13, OpRaw,  32, T2FieldScale    },

      { "ForceFieldBare",         8, OpRaw,  32, T2FieldAffine   },
      { "ForceFieldBare",        14, OpFlag,  1, T2FieldQuatSign },
      { "ForceFieldBare",        15, OpRaw,  32, T2FieldScale    },

      { "WaterBlock",             0, OpRaw,  32, T2FieldAffine   },
      { "WaterBlock",             6, OpFlag,  1, T2FieldQuatSign },
      { "WaterBlock",             7, OpRaw,  32, T2FieldScale    },

      { "AudioEmitter",         150, OpRaw,  32, T2FieldAffine   },
      { "AudioEmitter",         156, OpFlag,  1, T2FieldQuatSign },
   };
}

#endif
