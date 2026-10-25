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
      T2FieldAxisZSign,             ///< the flag that turns a Z axis around
      T2FieldZAngle,                ///< a raw float of radians about that
                                    ///< axis, which is a heading either way
      T2FieldScale,                 ///< the first of a Point3F's three floats
      T2FieldJetting,               ///< the flag that lights a jet pack
      T2FieldRotate,                ///< the flag that spins an item
      T2FieldStatic,                ///< the flag that says it never moves
      T2FieldAtRest,                ///< the flag that says it has stopped
      T2FieldWarp,                  ///< the flag that asks for the position
                                    ///< to be interpolated to, not snapped to
      T2FieldNormalPhi,
      T2FieldNormalTheta,
      T2FieldImpactPhi,
      T2FieldImpactTheta,
      T2FieldPointWords,
      T2FieldVelocityWords,
      T2FieldExplodeWords,
      T2FieldImpactWords,
      T2FieldCurrTick,
      T2FieldSourceObject,
      T2FieldSourceSlot,
      T2FieldVehicleObject,
      T2FieldTargetHere,            ///< the flag a target id follows
      T2FieldTarget,                ///< readInt(bits), the target manager's
                                    ///< id for the object
      T2FieldQuickSplash,
      T2FieldActionHere,            ///< the flag an action thread follows
      T2FieldAction,                ///< readInt(bits), a sequence number
      T2FieldActionHold,            ///< the flag that holds it at its end
      T2FieldActionAtEnd,           ///< the flag that says it has finished
      T2FieldActionFirstPerson,     ///< the flag that plays it in first
                                    ///< person too
      T2FieldActionPosHere,         ///< the flag a thread position follows
      T2FieldActionPos,             ///< readSignedFloat(bits) of the thread
      T2FieldFieldState,
      T2FieldFieldFade,
      T2FieldZonePoints,
      T2FieldZonePlanes,
      T2FieldZoneEdges,
      T2FieldVelocityMod,
      T2FieldGravityMod,
      T2FieldAppliedForce,
      T2FieldZoneActive,
      T2FieldCloaked,
      T2FieldFading,
      T2FieldFadeVisible,
      T2FieldFadeOut,
      T2FieldFadeTime,
      T2FieldBeamInitial,           ///< the flag that starts a beam's first
                                    ///< body
      T2FieldBeamHit,
      T2FieldBeamWet,
      T2FieldBeamFollow,
      T2FieldBeamLast,              ///< the flag a beam's body ends with:
                                    ///< whether it follows its source on a
                                    ///< first body, whether it hit on any
                                    ///< other
      T2FieldBeamEnergy,            ///< readFloat(bits) of the shot's
                                    ///< strength
   };

   inline U32 t2FieldWords(U32 field)
   {
      switch(field)
      {
         case T2FieldTransform:     return 16;
         case T2FieldAffine:        return 6;
         case T2FieldScale:         return 3;
         case T2FieldPointWords:    return 3;
         case T2FieldVelocityWords: return 3;
         case T2FieldExplodeWords:  return 3;
         case T2FieldImpactWords:   return 3;
         case T2FieldAppliedForce:  return 3;
         default:                   return 0;
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

      { "Player", 26, OpFlag,  1,  T2FieldJetting      },

   // The action thread Player::unpackUpdate reads after the impact sound:
   // the action, then hold, atEnd and firstPerson, then the thread's
   // position only when it has not finished.
      { "Player", 42, OpFlag,  1,  T2FieldActionHere        },
      { "Player", 41, OpRaw,   8,  T2FieldAction            },
      { "Player", 40, OpFlag,  1,  T2FieldActionHold        },
      { "Player", 39, OpFlag,  1,  T2FieldActionAtEnd       },
      { "Player", 38, OpFlag,  1,  T2FieldActionFirstPerson },
      { "Player", 35, OpFlag,  1,  T2FieldActionFirstPerson },
      { "Player", 37, OpFlag,  1,  T2FieldActionPosHere     },
      { "Player", 36, OpRaw,   6,  T2FieldActionPos         },

   // ShapeBase's cloak block (CloakMask, 0x40), inside the group flag at 64:
   // flag 63 for the block, then whether it is cloaked (0x005ef937), then the
   // controlled flag and the fade.
      { "Player", 62, OpFlag,  1,  T2FieldCloaked           },
   // After the controlled flag at 61, the fade (0x005ef9b6): whether one has
   // begun; if not, whether the shape shows at all; if so, whether it goes
   // out and over how long.
      { "Player", 60, OpFlag,  1,  T2FieldFading            },
      { "Player", 59, OpFlag,  1,  T2FieldFadeVisible       },
      { "Player", 58, OpFlag,  1,  T2FieldFadeOut           },
      { "Player", 57, OpRaw,  32,  T2FieldFadeTime          },

   // The tail Item::unpackUpdate adds to ShapeBase's body: what the item is,
      { "Item", 21, OpFlag,  1, T2FieldRotate        },
      { "Item", 20, OpFlag,  1, T2FieldStatic        },
      { "Item", 17, OpRaw,  32, T2FieldScale         },
      { "Item", 13, OpInt,  10, T2FieldSourceObject  },
      { "Item", 11, OpFlag,  1, T2FieldAxisZSign     },
      { "Item", 10, OpRaw,  32, T2FieldZAngle        },
      { "Item",  8, OpRaw,  32, T2FieldPointWords    },
      { "Item",  5, OpFlag,  1, T2FieldAtRest        },
      { "Item",  4, OpRaw,  32, T2FieldVelocityWords },
      { "Item",  1, OpFlag,  1, T2FieldWarp          },

      { "InteriorInstance",      11, OpRaw, 32, T2FieldTransform },
      { "InteriorInstance",      27, OpRaw, 32, T2FieldScale     },
      { "InteriorInstance",      33, OpRaw, 32, T2FieldTransform },
      { "InteriorInstance",      49, OpRaw, 32, T2FieldScale     },

      { "TSStatic",               0, OpRaw, 32, T2FieldTransform },
      { "TSStatic",              16, OpRaw, 32, T2FieldScale     },

      { "ParticleEmissionDummy",  2, OpRaw, 32, T2FieldTransform },
      { "ParticleEmissionDummy", 18, OpRaw, 32, T2FieldScale     },

   // PhysicalZone::unpackUpdate (0x0068b290): under the initial mask the
   // transform, the scale, the polyhedron's three runs, the two mods and the
   // force; then, on every body, the active flag.
      { "PhysicalZone",          42, OpRaw,   32, T2FieldTransform    },
      { "PhysicalZone",          26, OpRaw,   32, T2FieldScale        },
      { "PhysicalZone",          23, OpCount, 32, T2FieldZonePoints   },
      { "PhysicalZone",          18, OpCount, 32, T2FieldZonePlanes   },
      { "PhysicalZone",          12, OpCount, 32, T2FieldZoneEdges    },
      { "PhysicalZone",           6, OpRaw,   32, T2FieldVelocityMod  },
      { "PhysicalZone",           5, OpRaw,   32, T2FieldGravityMod   },
      { "PhysicalZone",           4, OpRaw,   32, T2FieldAppliedForce },
      { "PhysicalZone",           1, OpFlag,   1, T2FieldZoneActive   },

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

   // GameBase's target, which is the team the field lets through, and the
   // state ForceFieldBare::packUpdate (0x00676bf0) sends under its 0x10 mask.
      { "ForceFieldBare",        18, OpFlag,  1, T2FieldTargetHere },
      { "ForceFieldBare",        19, OpReadBr, 9, T2FieldTarget    },
      { "ForceFieldBare",         6, OpSelect, 2, T2FieldFieldState },
      { "ForceFieldBare",         7, OpRaw,  32, T2FieldFieldFade  },

      { "WaterBlock",             0, OpRaw,  32, T2FieldAffine   },
      { "WaterBlock",             6, OpFlag,  1, T2FieldQuatSign },
      { "WaterBlock",             7, OpRaw,  32, T2FieldScale    },

      { "AudioEmitter",         150, OpRaw,  32, T2FieldAffine   },
      { "AudioEmitter",         156, OpFlag,  1, T2FieldQuatSign },

#define T2_LINEAR_PROJECTILE_FIELDS( cls )                             \
      { cls, 17, OpPoint,        0,  T2FieldPosition     },            \
      { cls, 16, OpSignedFloat, 15,  T2FieldNormalPhi    },            \
      { cls, 15, OpSignedFloat, 14,  T2FieldNormalTheta  },            \
      { cls, 14, OpInt,          9,  T2FieldCurrTick     },            \
      { cls, 12, OpInt,         10,  T2FieldSourceObject },            \
      { cls, 11, OpInt,          3,  T2FieldSourceSlot   },            \
      { cls,  5, OpInt,         10,  T2FieldVehicleObject},            \
      { cls, 21, OpPoint,        0,  T2FieldPosition     },            \
      { cls, 20, OpSignedFloat, 15,  T2FieldImpactPhi    },            \
      { cls, 19, OpSignedFloat, 14,  T2FieldImpactTheta  },            \
      { cls,  4, OpPoint,        0,  T2FieldPosition     },            \
      { cls,  3, OpSignedFloat, 15,  T2FieldImpactPhi    },            \
      { cls,  2, OpSignedFloat, 14,  T2FieldImpactTheta  },

      T2_LINEAR_PROJECTILE_FIELDS( "LinearProjectile"      )
      T2_LINEAR_PROJECTILE_FIELDS( "LinearFlareProjectile" )
      T2_LINEAR_PROJECTILE_FIELDS( "TracerProjectile"      )

#undef T2_LINEAR_PROJECTILE_FIELDS

#define T2_GRENADE_PROJECTILE_FIELDS( cls )                            \
      { cls, 34, OpRaw, 32, T2FieldPointWords    },                    \
      { cls, 31, OpRaw, 32, T2FieldVelocityWords },                    \
      { cls, 28, OpInt, 12, T2FieldCurrTick      },                    \
      { cls, 27, OpFlag, 1, T2FieldQuickSplash   },                    \
      { cls, 25, OpRaw, 32, T2FieldExplodeWords  },                    \
      { cls, 22, OpRaw, 32, T2FieldImpactWords   },                    \
      { cls, 18, OpInt, 11, T2FieldSourceObject  },                    \
      { cls, 17, OpInt,  3, T2FieldSourceSlot    },                    \
      { cls, 15, OpInt, 11, T2FieldVehicleObject },                    \
      { cls, 13, OpRaw, 32, T2FieldPointWords    },                    \
      { cls, 10, OpRaw, 32, T2FieldVelocityWords },                    \
      { cls,  6, OpRaw, 32, T2FieldExplodeWords  },                    \
      { cls,  3, OpRaw, 32, T2FieldImpactWords   },

      T2_GRENADE_PROJECTILE_FIELDS( "GrenadeProjectile" )
      T2_GRENADE_PROJECTILE_FIELDS( "EnergyProjectile"  )
      T2_GRENADE_PROJECTILE_FIELDS( "FlareProjectile"   )
      T2_GRENADE_PROJECTILE_FIELDS( "BombProjectile"    )

#undef T2_GRENADE_PROJECTILE_FIELDS

   // SniperProjectile::unpackUpdate (0x00643aa0) and TargetProjectile's
   // (0x00649c70) share every body after the first: the source, or failing
   // one the start, then the end and whether it hit.
#define T2_BEAM_PROJECTILE_FIELDS( cls )                               \
      { cls,  2, OpFlag,    1, T2FieldBeamInitial   },                 \
      { cls,  4, OpRaw,    32, T2FieldPointWords    },                 \
      { cls,  7, OpRaw,    32, T2FieldExplodeWords  },                 \
      { cls, 10, OpFlag,    1, T2FieldBeamLast      },                 \
      { cls, 12, OpInt,    11, T2FieldSourceObject  },                 \
      { cls, 13, OpSelect,  3, T2FieldSourceSlot    },                 \
      { cls, 14, OpFlag,    1, T2FieldBeamFollow    },

      T2_BEAM_PROJECTILE_FIELDS( "SniperProjectile" )
      T2_BEAM_PROJECTILE_FIELDS( "TargetProjectile" )

#undef T2_BEAM_PROJECTILE_FIELDS

   // A first body, which for a sniper shot leads with its strength.
      { "SniperProjectile", 15, OpFloat,   7, T2FieldBeamEnergy    },
      { "SniperProjectile", 16, OpRaw,    32, T2FieldPointWords    },
      { "SniperProjectile", 19, OpRaw,    32, T2FieldExplodeWords  },
      { "SniperProjectile", 22, OpFlag,    1, T2FieldBeamHit       },
      { "SniperProjectile", 23, OpFlag,    1, T2FieldBeamWet       },
      { "SniperProjectile", 25, OpInt,    11, T2FieldSourceObject  },
      { "SniperProjectile", 26, OpSelect,  3, T2FieldSourceSlot    },

      { "TargetProjectile", 15, OpRaw,    32, T2FieldPointWords    },
      { "TargetProjectile", 18, OpRaw,    32, T2FieldExplodeWords  },
      { "TargetProjectile", 21, OpFlag,    1, T2FieldBeamHit       },
      { "TargetProjectile", 23, OpInt,    11, T2FieldSourceObject  },
      { "TargetProjectile", 24, OpSelect,  3, T2FieldSourceSlot    },
   };
}

#endif
