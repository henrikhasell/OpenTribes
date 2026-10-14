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

      { "ExplosionData",   7, OpInt,   14, T2DbPlaySpeed          },
      { "ExplosionData",  16, OpInt,   16, T2DbDelayMS            },
      { "ExplosionData",  17, OpInt,   16, T2DbDelayVariance      },
      { "ExplosionData",  18, OpInt,   16, T2DbExplosionLifetime  },
      { "ExplosionData",  19, OpInt,   16, T2DbLifetimeVariance   },
      { "ExplosionData",  20, OpRaw,   32, T2DbExplosionOffset    },
      { "ExplosionData", 105, OpInt,   16, T2DbExplosionScaleX    },
      { "ExplosionData", 106, OpInt,   16, T2DbExplosionScaleY    },
      { "ExplosionData", 107, OpInt,   16, T2DbExplosionScaleZ    },

      { "ExplosionData",  45, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  49, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  50, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  54, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  55, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  56, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  60, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  61, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  62, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  63, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  67, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  68, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  69, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  70, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  71, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  75, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  76, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  77, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  78, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  79, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  80, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  84, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  85, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  86, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  87, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  88, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  89, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  90, OpFloat,  8, T2DbExplosionTime      },
      { "ExplosionData",  46, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  47, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  48, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  51, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  52, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  53, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  57, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  58, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  59, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  64, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  65, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  66, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  72, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  73, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  74, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  81, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  82, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  83, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  91, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  92, OpInt,   14, T2DbExplosionSize      },
      { "ExplosionData",  93, OpInt,   14, T2DbExplosionSize      },

      { "ExplosionData", 5, OpFlag, 1, T2DbFaceViewer },

      { "AudioProfile", 7, OpDataBlockId, 11, T2DbAudioDescription },

      { "AudioDescription",  0, OpFloat,  6, T2DbAudioVolume             },
      { "AudioDescription",  1, OpFlag,   1, T2DbAudioLooping            },
      { "AudioDescription",  2, OpFlag,   1, T2DbAudio3D                 },
      { "AudioDescription",  3, OpSelect, 3, T2DbAudioChannel            },
      { "AudioDescription",  5, OpRaw,   32, T2DbAudioReferenceDistance  },
      { "AudioDescription",  6, OpRaw,   32, T2DbAudioMaxDistance        },
      { "AudioDescription",  7, OpInt,    9, T2DbAudioConeInsideAngle    },
      { "AudioDescription",  8, OpInt,    9, T2DbAudioConeOutsideAngle   },
      { "AudioDescription",  9, OpFloat,  6, T2DbAudioConeOutsideVolume  },

   // ShapeBaseImageData::packData writes the mount point as a whole word, which
   // is the layout's node 2.
      { "ShapeBaseImageData", 2, OpRaw, 32, T2DbImageMountPoint },

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

      { "TracerProjectileData",      24, OpRaw, 32, T2DbProjectileVelocity },
      { "TracerProjectileData",      27, OpRaw, 32, T2DbProjectileLifetime },
      { "TracerProjectileData",      34, OpRaw, 32, T2DbTracerLength       },
      { "TracerProjectileData",      35, OpRaw, 32, T2DbTracerWidth        },
      { "TracerProjectileData",      67, OpRaw, 32, T2DbScaleX             },
      { "TracerProjectileData",      68, OpRaw, 32, T2DbScaleY             },
      { "TracerProjectileData",      69, OpRaw, 32, T2DbScaleZ             },
      { "TracerProjectileData",      63, OpDataBlockId, 11, T2DbExplosion   },

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
