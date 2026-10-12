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

   // The flight of a projectile is not on the wire: the body says where and
   // which way, and the datablock says how fast and for how long. Each of
   // these layouts opens with ProjectileData's twenty-four nodes, so the
   // class's own fields start at node 24, in the order its packData writes
   // them.
      { "LinearProjectileData",      24, OpRaw, 32, T2DbProjectileVelocity },
      { "LinearProjectileData",      27, OpRaw, 32, T2DbProjectileLifetime },

      { "LinearFlareProjectileData", 24, OpRaw, 32, T2DbProjectileVelocity },
      { "LinearFlareProjectileData", 27, OpRaw, 32, T2DbProjectileLifetime },

   // TracerProjectileData adds the streak a bullet is seen as, after the ten
   // fields it inherits from LinearProjectileData.
      { "TracerProjectileData",      24, OpRaw, 32, T2DbProjectileVelocity },
      { "TracerProjectileData",      27, OpRaw, 32, T2DbProjectileLifetime },
      { "TracerProjectileData",      34, OpRaw, 32, T2DbTracerLength       },
      { "TracerProjectileData",      35, OpRaw, 32, T2DbTracerWidth        },

   // EnergyProjectileData is a grenade underneath: its speed is a muzzle
   // velocity at node 25, the three fields a bounce needs follow it, and its
   // bolt is the cross at node 33.
      { "EnergyProjectileData",      25, OpRaw, 32, T2DbProjectileVelocity },
      { "EnergyProjectileData",      26, OpRaw, 32, T2DbElasticity         },
      { "EnergyProjectileData",      27, OpRaw, 32, T2DbFriction           },
      { "EnergyProjectileData",      30, OpRaw, 32, T2DbGravityMod         },
      { "EnergyProjectileData",      31, OpRaw, 32, T2DbProjectileLifetime },
      { "EnergyProjectileData",      33, OpRaw, 32, T2DbCrossSize          },
   };
}

#endif
