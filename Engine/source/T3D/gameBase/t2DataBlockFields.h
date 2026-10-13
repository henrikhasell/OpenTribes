#ifndef _T2DATABLOCKFIELDS_H_
#define _T2DATABLOCKFIELDS_H_

namespace T2Protocol
{
   struct T2DataBlockFieldRow
   {
      const char              *cls;
      S16                      node;
      U8                       op;
      U8                       bits;
      T2DataBlockField field;
   };

   static const T2DataBlockFieldRow sT2DataBlockFields[] =
   {
      { "PlayerData", 45, OpRaw, 32, T2DbJetForce                   },
      { "PlayerData", 46, OpRaw, 32, T2DbUnderwaterJetForce         },
      { "PlayerData", 47, OpRaw, 32, T2DbUnderwaterVertJetFactor    },
      { "PlayerData", 48, OpRaw, 32, T2DbJetEnergyDrain             },
      { "PlayerData", 49, OpRaw, 32, T2DbUnderwaterJetEnergyDrain   },
      { "PlayerData", 50, OpRaw, 32, T2DbMinJetEnergy               },
      { "PlayerData", 51, OpRaw, 32, T2DbMaxJetForwardSpeed         },
      { "PlayerData", 52, OpRaw, 32, T2DbMaxJetHorizontalPercentage },

   // PlayerData::packData (0x005cf935) writes eleven bare floats here, which is
   // the layout's only run of eleven, at nodes 135 to 145.
      { "PlayerData", 135, OpRaw, 32, T2DbHeatDecayPerSec    },
      { "PlayerData", 136, OpRaw, 32, T2DbHeatIncreasePerSec },

      { "PlayerData", 269, OpRaw, 32, T2DbCameraMaxDist },

   // ShapeBaseImageData::packData writes the mount point as a whole word, which
   // is the layout's node 2.
      { "ShapeBaseImageData", 2, OpRaw, 32, T2DbImageMountPoint },

   // Each of these layouts names its explosion as the fourth of the fifteen
   // datablocks it refers to, which is the eighth flag's id node.
   // The flight of a projectile is not on the wire: the body says where and
   // which way, and the datablock says how fast and for how long. Each of
   // these layouts opens with ProjectileData's twenty-four nodes, so the
   // class's own fields start at node 24, in the order its packData writes
   // them.
      { "LinearProjectileData",      24, OpRaw, 32, T2DbProjectileVelocity },
      { "LinearProjectileData",      27, OpRaw, 32, T2DbProjectileLifetime },
      { "LinearProjectileData",      57, OpRaw, 32, T2DbScaleX             },
      { "LinearProjectileData",      58, OpRaw, 32, T2DbScaleY             },
      { "LinearProjectileData",      59, OpRaw, 32, T2DbScaleZ             },
      { "LinearProjectileData",      53, OpDataBlockId, 11, T2DbExplosion   },

      { "LinearFlareProjectileData", 24, OpRaw, 32, T2DbProjectileVelocity },
      { "LinearFlareProjectileData", 27, OpRaw, 32, T2DbProjectileLifetime },
      { "LinearFlareProjectileData", 64, OpRaw, 32, T2DbScaleX             },
      { "LinearFlareProjectileData", 65, OpRaw, 32, T2DbScaleY             },
      { "LinearFlareProjectileData", 66, OpRaw, 32, T2DbScaleZ             },
      { "LinearFlareProjectileData", 60, OpDataBlockId, 11, T2DbExplosion   },

   // TracerProjectileData adds the streak a bullet is seen as, after the ten
   // fields it inherits from LinearProjectileData.
      { "TracerProjectileData",      24, OpRaw, 32, T2DbProjectileVelocity },
      { "TracerProjectileData",      27, OpRaw, 32, T2DbProjectileLifetime },
      { "TracerProjectileData",      34, OpRaw, 32, T2DbTracerLength       },
      { "TracerProjectileData",      35, OpRaw, 32, T2DbTracerWidth        },
      { "TracerProjectileData",      67, OpRaw, 32, T2DbScaleX             },
      { "TracerProjectileData",      68, OpRaw, 32, T2DbScaleY             },
      { "TracerProjectileData",      69, OpRaw, 32, T2DbScaleZ             },
      { "TracerProjectileData",      63, OpDataBlockId, 11, T2DbExplosion   },

   // EnergyProjectileData is a grenade underneath. GrenadeProjectileData's
   // unpackData at 0x00633cf0 stores in the order 0x148, 0x144, 0x138, 0x13c,
   // 0x14c, 0x150, 0x140, 0x154 -- arming delay, muzzle velocity, elasticity,
   // friction, drag, density, gravity and lifetime -- and the class's own
   // unpackData at 0x00694d80 adds 0x168 to 0x180 after it, which is the
   // cross, the blur and nothing else.
      { "EnergyProjectileData",      24, OpRaw, 32, T2DbArmingDelay        },
      { "EnergyProjectileData",      25, OpRaw, 32, T2DbProjectileVelocity },
      { "EnergyProjectileData",      26, OpRaw, 32, T2DbElasticity         },
      { "EnergyProjectileData",      27, OpRaw, 32, T2DbFriction           },
      { "EnergyProjectileData",      30, OpRaw, 32, T2DbGravityMod         },
      { "EnergyProjectileData",      31, OpRaw, 32, T2DbProjectileLifetime },
      { "EnergyProjectileData",      32, OpRaw, 32, T2DbCrossViewAng       },
      { "EnergyProjectileData",      33, OpRaw, 32, T2DbCrossSize          },
      { "EnergyProjectileData",      34, OpRaw, 32, T2DbBlurLifetime       },
      { "EnergyProjectileData",      35, OpRaw, 32, T2DbBlurWidth          },
      { "EnergyProjectileData",      36, OpRaw, 32, T2DbBlurColorR         },
      { "EnergyProjectileData",      37, OpRaw, 32, T2DbBlurColorG         },
      { "EnergyProjectileData",      38, OpRaw, 32, T2DbBlurColorB         },
      { "EnergyProjectileData",      64, OpRaw, 32, T2DbScaleX             },
      { "EnergyProjectileData",      65, OpRaw, 32, T2DbScaleY             },
      { "EnergyProjectileData",      66, OpRaw, 32, T2DbScaleZ             },
      { "EnergyProjectileData",      60, OpDataBlockId, 11, T2DbExplosion   },
   };
}

#endif
