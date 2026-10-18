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
   // The ShapeBaseData layer a PlayerData opens with, whose values are gated
   // by the flags at nodes 0 to 6 and so sit at the tail of the layout.
      { "PlayerData", 273, OpRaw, 32, T2DbMass      },
      { "PlayerData", 272, OpRaw, 32, T2DbDrag      },
      { "PlayerData", 271, OpRaw, 32, T2DbDensity   },
      { "PlayerData", 270, OpRaw, 32, T2DbMaxEnergy },

      { "PlayerData", 40, OpRaw, 32, T2DbMinLookAngle     },
      { "PlayerData", 41, OpRaw, 32, T2DbMaxLookAngle     },
      { "PlayerData", 42, OpRaw, 32, T2DbMaxFreelookAngle },
      { "PlayerData", 43, OpRaw, 32, T2DbMaxTimeScale     },
      { "PlayerData", 44, OpRaw, 32, T2DbMaxStepHeight    },

      { "PlayerData", 45, OpRaw, 32, T2DbJetForce                   },
      { "PlayerData", 46, OpRaw, 32, T2DbUnderwaterJetForce         },
      { "PlayerData", 47, OpRaw, 32, T2DbUnderwaterVertJetFactor    },
      { "PlayerData", 48, OpRaw, 32, T2DbJetEnergyDrain             },
      { "PlayerData", 49, OpRaw, 32, T2DbUnderwaterJetEnergyDrain   },
      { "PlayerData", 50, OpRaw, 32, T2DbMinJetEnergy               },
      { "PlayerData", 51, OpRaw, 32, T2DbMaxJetForwardSpeed         },
      { "PlayerData", 52, OpRaw, 32, T2DbMaxJetHorizontalPercentage },

      { "PlayerData", 187, OpDataBlockId, 11, T2DbJetEmitter },

      { "PlayerData", 55, OpRaw, 32, T2DbRunForce                   },
      { "PlayerData", 56, OpRaw, 32, T2DbRunEnergyDrain             },
      { "PlayerData", 57, OpRaw, 32, T2DbMinRunEnergy               },
      { "PlayerData", 58, OpRaw, 32, T2DbMaxForwardSpeed            },
      { "PlayerData", 59, OpRaw, 32, T2DbMaxBackwardSpeed           },
      { "PlayerData", 60, OpRaw, 32, T2DbMaxSideSpeed               },
      { "PlayerData", 61, OpRaw, 32, T2DbMaxUnderwaterForwardSpeed  },
      { "PlayerData", 62, OpRaw, 32, T2DbMaxUnderwaterBackwardSpeed },
      { "PlayerData", 63, OpRaw, 32, T2DbMaxUnderwaterSideSpeed     },
      { "PlayerData", 64, OpRaw, 32, T2DbRunSurfaceAngle            },

   // A tick count, so a whole word rather than a float.
      { "PlayerData", 65, OpRaw, 32, T2DbRecoverDelay         },
      { "PlayerData", 66, OpRaw, 32, T2DbRecoverRunForceScale },

      { "PlayerData", 67, OpRaw, 32, T2DbJumpForce        },
      { "PlayerData", 68, OpRaw, 32, T2DbJumpEnergyDrain  },
      { "PlayerData", 69, OpRaw, 32, T2DbMinJumpEnergy    },
      { "PlayerData", 70, OpRaw, 32, T2DbMinJumpSpeed     },
      { "PlayerData", 71, OpRaw, 32, T2DbMaxJumpSpeed     },
      { "PlayerData", 72, OpRaw, 32, T2DbJumpSurfaceAngle },
      { "PlayerData", 73, OpInt,  7, T2DbJumpDelay        },

      { "PlayerData", 74, OpRaw, 32, T2DbHorizMaxSpeed     },
      { "PlayerData", 75, OpRaw, 32, T2DbHorizResistSpeed  },
      { "PlayerData", 76, OpRaw, 32, T2DbHorizResistFactor },
      { "PlayerData", 77, OpRaw, 32, T2DbUpMaxSpeed        },
      { "PlayerData", 78, OpRaw, 32, T2DbUpResistSpeed     },
      { "PlayerData", 79, OpRaw, 32, T2DbUpResistFactor    },

      { "PlayerData", 80, OpRaw, 32, T2DbSplashVelocity     },
      { "PlayerData", 81, OpRaw, 32, T2DbSplashAngle        },
      { "PlayerData", 82, OpRaw, 32, T2DbSplashFreqMod      },
      { "PlayerData", 83, OpRaw, 32, T2DbSplashVelEpsilon   },
      { "PlayerData", 84, OpRaw, 32, T2DbBubbleEmitTime     },
      { "PlayerData", 85, OpRaw, 32, T2DbMedSplashSoundVel  },
      { "PlayerData", 86, OpRaw, 32, T2DbHardSplashSoundVel },
      { "PlayerData", 87, OpRaw, 32, T2DbExitSplashSoundVel },
      { "PlayerData", 88, OpRaw, 32, T2DbFootSplashHeight   },
      { "PlayerData", 89, OpRaw, 32, T2DbMinImpactSpeed     },

   // PlayerData::packData (0x005cf783) writes its 32 sounds in turn, each a
   // flag and an id: the flags are nodes 90 to 121, and each id is the node
   // its flag's set arm names, counting down from 185.
      { "PlayerData", 185, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  0) },
      { "PlayerData", 184, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  1) },
      { "PlayerData", 183, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  2) },
      { "PlayerData", 182, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  3) },
      { "PlayerData", 181, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  4) },
      { "PlayerData", 180, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  5) },
      { "PlayerData", 179, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  6) },
      { "PlayerData", 178, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  7) },
      { "PlayerData", 177, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  8) },
      { "PlayerData", 176, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound +  9) },
      { "PlayerData", 175, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 10) },
      { "PlayerData", 174, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 11) },
      { "PlayerData", 173, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 12) },
      { "PlayerData", 172, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 13) },
      { "PlayerData", 171, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 14) },
      { "PlayerData", 170, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 15) },
      { "PlayerData", 169, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 16) },
      { "PlayerData", 168, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 17) },
      { "PlayerData", 167, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 18) },
      { "PlayerData", 166, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 19) },
      { "PlayerData", 165, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 20) },
      { "PlayerData", 164, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 21) },
      { "PlayerData", 163, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 22) },
      { "PlayerData", 162, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 23) },
      { "PlayerData", 161, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 24) },
      { "PlayerData", 160, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 25) },
      { "PlayerData", 159, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 26) },
      { "PlayerData", 158, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 27) },
      { "PlayerData", 157, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 28) },
      { "PlayerData", 156, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 29) },
      { "PlayerData", 155, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 30) },
      { "PlayerData", 154, OpDataBlockId, 11, T2DataBlockField(T2DbPlayerSound + 31) },

      { "PlayerData", 122, OpRaw, 32, T2DbBoxSizeX },
      { "PlayerData", 123, OpRaw, 32, T2DbBoxSizeY },
      { "PlayerData", 124, OpRaw, 32, T2DbBoxSizeZ },

   // The emitter's flag is node 125, ahead of the count and the radius.
      { "PlayerData", 153, OpDataBlockId, 11, T2DbFootPuffEmitter },

   // A particle count, likewise a word.
      { "PlayerData", 126, OpRaw, 32, T2DbFootPuffNumParts },
      { "PlayerData", 127, OpRaw, 32, T2DbFootPuffRadius   },
      { "PlayerData", 129, OpRaw, 32, T2DbDecalOffset      },

      { "PlayerData", 151, OpDataBlockId, 11, T2DbDustEmitter },

   // PlayerData::packData (0x005cf935) writes eleven bare floats here, which is
   // the layout's only run of eleven, at nodes 135 to 145.
      { "PlayerData", 135, OpRaw, 32, T2DbHeatDecayPerSec           },
      { "PlayerData", 136, OpRaw, 32, T2DbHeatIncreasePerSec        },
      { "PlayerData", 137, OpRaw, 32, T2DbGroundImpactMinSpeed      },
      { "PlayerData", 138, OpRaw, 32, T2DbGroundImpactShakeFreqX    },
      { "PlayerData", 139, OpRaw, 32, T2DbGroundImpactShakeFreqY    },
      { "PlayerData", 140, OpRaw, 32, T2DbGroundImpactShakeFreqZ    },
      { "PlayerData", 141, OpRaw, 32, T2DbGroundImpactShakeAmpX     },
      { "PlayerData", 142, OpRaw, 32, T2DbGroundImpactShakeAmpY     },
      { "PlayerData", 143, OpRaw, 32, T2DbGroundImpactShakeAmpZ     },
      { "PlayerData", 144, OpRaw, 32, T2DbGroundImpactShakeDuration },
      { "PlayerData", 145, OpRaw, 32, T2DbGroundImpactShakeFalloff  },

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

   // The file name opens the body, then the sound and the emitter it names.
      { "ExplosionData", 109, OpDataBlockId, 11, T2DbExplosionSound },
      { "ExplosionData", 108, OpDataBlockId, 11, T2DbExplosionParticleEmitter },
      { "ExplosionData",   3, OpInt,   14, T2DbExplosionParticleDensity   },
      { "ExplosionData",   4, OpRaw,   32, T2DbExplosionParticleRadius    },

   // The debris spread, as ExplosionData::packData writes it after playSpeed.
      { "ExplosionData",   8, OpInt,    8, T2DbExplosionDebrisThetaMin         },
      { "ExplosionData",   9, OpInt,    8, T2DbExplosionDebrisThetaMax         },
      { "ExplosionData",  10, OpInt,    9, T2DbExplosionDebrisPhiMin           },
      { "ExplosionData",  11, OpInt,    9, T2DbExplosionDebrisPhiMax           },
      { "ExplosionData",  12, OpInt,   10, T2DbExplosionDebrisNum              },
      { "ExplosionData",  13, OpInt,   10, T2DbExplosionDebrisNumVariance      },
      { "ExplosionData",  14, OpInt,   14, T2DbExplosionDebrisVelocity         },
      { "ExplosionData",  15, OpInt,   14, T2DbExplosionDebrisVelocityVariance },

   // Node 21 is shockwaveOnTerrain, which only a shockwave reads.
      { "ExplosionData",  22, OpFlag,   1, T2DbExplosionShakeCamera   },
      { "ExplosionData",  23, OpRaw,   32, T2DbExplosionShakeFreqX    },
      { "ExplosionData",  24, OpRaw,   32, T2DbExplosionShakeFreqY    },
      { "ExplosionData",  25, OpRaw,   32, T2DbExplosionShakeFreqZ    },
      { "ExplosionData",  26, OpRaw,   32, T2DbExplosionShakeAmpX     },
      { "ExplosionData",  27, OpRaw,   32, T2DbExplosionShakeAmpY     },
      { "ExplosionData",  28, OpRaw,   32, T2DbExplosionShakeAmpZ     },
      { "ExplosionData",  29, OpRaw,   32, T2DbExplosionShakeDuration },
      { "ExplosionData",  30, OpRaw,   32, T2DbExplosionShakeRadius   },
      { "ExplosionData",  31, OpRaw,   32, T2DbExplosionShakeFalloff  },

   // Eleven flagged ids follow, read from node 104 down: the shockwave, the
   // debris, the four emitters and the five sub-explosions.
      { "ExplosionData", 103, OpDataBlockId, 11, T2DbExplosionDebris   },
      { "ExplosionData", 102, OpDataBlockId, 11, T2DbExplosionEmitter0 },
      { "ExplosionData", 101, OpDataBlockId, 11, T2DbExplosionEmitter1 },
      { "ExplosionData", 100, OpDataBlockId, 11, T2DbExplosionEmitter2 },
      { "ExplosionData",  99, OpDataBlockId, 11, T2DbExplosionEmitter3 },
      { "ExplosionData",  98, OpDataBlockId, 11, T2DbSubExplosion0     },
      { "ExplosionData",  97, OpDataBlockId, 11, T2DbSubExplosion1     },
      { "ExplosionData",  96, OpDataBlockId, 11, T2DbSubExplosion2     },
      { "ExplosionData",  95, OpDataBlockId, 11, T2DbSubExplosion3     },
      { "ExplosionData",  94, OpDataBlockId, 11, T2DbSubExplosion4     },

   // DebrisData::packData, field for field; nodes 6 and 12 to 13 are render2D
   // and a second pair of spin speeds, which nothing here draws with.
      { "DebrisData",  0, OpRaw, 32, T2DbDebrisElasticity         },
      { "DebrisData",  1, OpRaw, 32, T2DbDebrisFriction           },
      { "DebrisData",  2, OpRaw, 32, T2DbDebrisNumBounces         },
      { "DebrisData",  3, OpRaw, 32, T2DbDebrisBounceVariance     },
      { "DebrisData",  4, OpRaw, 32, T2DbDebrisMinSpinSpeed       },
      { "DebrisData",  5, OpRaw, 32, T2DbDebrisMaxSpinSpeed       },
      { "DebrisData",  7, OpRaw,  8, T2DbDebrisExplodeOnMaxBounce },
      { "DebrisData",  8, OpRaw,  8, T2DbDebrisStaticOnMaxBounce  },
      { "DebrisData",  9, OpRaw,  8, T2DbDebrisSnapOnMaxBounce    },
      { "DebrisData", 10, OpRaw, 32, T2DbDebrisLifetime           },
      { "DebrisData", 11, OpRaw, 32, T2DbDebrisLifetimeVariance   },
      { "DebrisData", 14, OpRaw, 32, T2DbDebrisVelocity           },
      { "DebrisData", 15, OpRaw, 32, T2DbDebrisVelocityVariance   },
      { "DebrisData", 16, OpRaw,  8, T2DbDebrisFade               },
      { "DebrisData", 17, OpRaw,  8, T2DbDebrisUseRadiusMass      },
      { "DebrisData", 18, OpRaw, 32, T2DbDebrisBaseRadius         },
      { "DebrisData", 19, OpRaw, 32, T2DbDebrisGravModifier       },
      { "DebrisData", 20, OpRaw, 32, T2DbDebrisTerminalVelocity   },
      { "DebrisData", 21, OpRaw,  8, T2DbDebrisIgnoreWater        },
      { "DebrisData", 30, OpDataBlockId, 11, T2DbDebrisEmitter0   },
      { "DebrisData", 29, OpDataBlockId, 11, T2DbDebrisEmitter1   },
      { "DebrisData", 28, OpDataBlockId, 11, T2DbDebrisExplosion  },

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

   // Every projectile carries the same run of datablock ids, the emitters then
   // the explosions then the splash and the sound, so the sound is three past
   // whichever node that class's explosion sits at.
      { "BombProjectileData",        56, OpDataBlockId, 11, T2DbProjectileSound },
      { "ELFProjectileData",         51, OpDataBlockId, 11, T2DbProjectileSound },
      { "EnergyProjectileData",      57, OpDataBlockId, 11, T2DbProjectileSound },
      { "FlareProjectileData",       52, OpDataBlockId, 11, T2DbProjectileSound },
      { "GrenadeProjectileData",     48, OpDataBlockId, 11, T2DbProjectileSound },
      { "LinearFlareProjectileData", 57, OpDataBlockId, 11, T2DbProjectileSound },
      { "LinearProjectileData",      50, OpDataBlockId, 11, T2DbProjectileSound },
      { "ProjectileData",            40, OpDataBlockId, 11, T2DbProjectileSound },
      { "RepairProjectileData",      50, OpDataBlockId, 11, T2DbProjectileSound },
      { "SeekerProjectileData",      63, OpDataBlockId, 11, T2DbProjectileSound },
      { "ShockLanceProjectileData",  63, OpDataBlockId, 11, T2DbProjectileSound },
      { "SniperProjectileData",      66, OpDataBlockId, 11, T2DbProjectileSound },
      { "TargetProjectileData",      53, OpDataBlockId, 11, T2DbProjectileSound },
      { "TracerProjectileData",      60, OpDataBlockId, 11, T2DbProjectileSound },

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

      { "GrenadeProjectileData", 24, OpRaw, 32, T2DbArmingDelay        },
      { "GrenadeProjectileData", 25, OpRaw, 32, T2DbProjectileVelocity },
      { "GrenadeProjectileData", 26, OpRaw, 32, T2DbElasticity         },
      { "GrenadeProjectileData", 27, OpRaw, 32, T2DbFriction           },
      { "GrenadeProjectileData", 28, OpRaw, 32, T2DbProjectileDrag     },
      { "GrenadeProjectileData", 29, OpRaw, 32, T2DbProjectileDensity  },
      { "GrenadeProjectileData", 30, OpRaw, 32, T2DbGravityMod         },
      { "GrenadeProjectileData", 31, OpRaw, 32, T2DbProjectileLifetime },

      { "GrenadeProjectileData", 51, OpDataBlockId, 11, T2DbExplosion   },

      { "ItemData", 141, OpRaw, 32, T2DbMass      },
      { "ItemData", 140, OpRaw, 32, T2DbDrag      },
      { "ItemData", 139, OpRaw, 32, T2DbDensity   },
      { "ItemData", 138, OpRaw, 32, T2DbMaxEnergy },

      { "ItemData", 125, OpDataBlockId, 11, T2DbShapeExplosion            },
      { "ItemData", 124, OpDataBlockId, 11, T2DbShapeUnderwaterExplosion  },
      { "ItemData", 123, OpDataBlockId, 11, T2DbShapeDebris               },

   // The shield effect's lifetime and scale, which ShapeBaseData::unpackData
   // (0x005e6b8a) reads after its two flags: the scale only when a flag
   // says it is not the default. Every ShapeBaseData layout puts the
   // lifetime at node 29.
#define T2_SHIELD_EFFECT_ROWS( cls, x )                                       \
      { cls, 29,    OpRaw, 32, T2DbShieldEffectLifetimeMS },                  \
      { cls, x,     OpRaw, 32, T2DbShieldEffectScaleX     },                  \
      { cls, x + 1, OpRaw, 32, T2DbShieldEffectScaleY     },                  \
      { cls, x + 2, OpRaw, 32, T2DbShieldEffectScaleZ     },

      T2_SHIELD_EFFECT_ROWS( "CameraData",         104 )
      T2_SHIELD_EFFECT_ROWS( "FlyingVehicleData",  191 )
      T2_SHIELD_EFFECT_ROWS( "HoverVehicleData",   202 )
      T2_SHIELD_EFFECT_ROWS( "ItemData",           120 )
      T2_SHIELD_EFFECT_ROWS( "MissionMarkerData",  104 )
      T2_SHIELD_EFFECT_ROWS( "PlayerData",         252 )
      T2_SHIELD_EFFECT_ROWS( "ShapeBaseData",      104 )
      T2_SHIELD_EFFECT_ROWS( "StaticShapeData",    106 )
      T2_SHIELD_EFFECT_ROWS( "TurretData",         113 )
      T2_SHIELD_EFFECT_ROWS( "WheeledVehicleData", 193 )
#undef T2_SHIELD_EFFECT_ROWS

      { "ItemData",  39, OpFloat, 10, T2DbItemFriction        },
      { "ItemData",  40, OpFloat, 10, T2DbItemElasticity      },
      { "ItemData",  41, OpFlag,   1, T2DbItemSticky          },
      { "ItemData",  55, OpFloat, 10, T2DbItemGravityMod      },
      { "ItemData",  54, OpRaw,   32, T2DbItemMaxVelocity     },
      { "ItemData",  46, OpSelect, 2, T2DbItemLightType       },
      { "ItemData",  47, OpFloat,  7, T2DbItemLightColorR     },
      { "ItemData",  48, OpFloat,  7, T2DbItemLightColorG     },
      { "ItemData",  49, OpFloat,  7, T2DbItemLightColorB     },
      { "ItemData",  50, OpFloat,  7, T2DbItemLightColorA     },
      { "ItemData",  51, OpRaw,   32, T2DbItemLightTime       },
      { "ItemData",  52, OpRaw,   32, T2DbItemLightRadius     },
      { "ItemData",  53, OpFlag,   1, T2DbItemLightOnlyStatic },

      { "ParticleEmitterData",  0, OpInt,    10, T2DbEjectionPeriodMS  },
      { "ParticleEmitterData",  1, OpInt,    10, T2DbPeriodVarianceMS  },
      { "ParticleEmitterData",  2, OpInt,    16, T2DbEjectionVelocity  },
      { "ParticleEmitterData",  3, OpInt,    14, T2DbVelocityVariance  },
      { "ParticleEmitterData", 29, OpInt,    16, T2DbEjectionOffset    },
      { "ParticleEmitterData",  5, OpInt,     8, T2DbThetaMin          },
      { "ParticleEmitterData",  6, OpInt,     8, T2DbThetaMax          },
      { "ParticleEmitterData", 28, OpInt,     9, T2DbPhiReferenceVel   },
      { "ParticleEmitterData", 27, OpInt,     9, T2DbPhiVariance       },
      { "ParticleEmitterData",  9, OpFlag,    1, T2DbOverrideAdvance   },
      { "ParticleEmitterData", 10, OpFlag,    1, T2DbOrientParticles   },
      { "ParticleEmitterData", 11, OpFlag,    1, T2DbOrientOnVelocity  },

      { "ParticleEmitterData", 26, OpInt,    10, T2DbEmitterLifetimeMS },
      { "ParticleEmitterData", 25, OpReadBr, 10, T2DbEmitterLifetimeVarianceMS },
      { "ParticleEmitterData", 12, OpReadBr, 10, T2DbEmitterLifetimeMS },
      { "ParticleEmitterData", 13, OpInt,    10, T2DbEmitterLifetimeVarianceMS },

      { "ParticleEmitterData", 14, OpFlag,    1, T2DbUseEmitterSizes   },
      { "ParticleEmitterData", 15, OpFlag,    1, T2DbUseEmitterColors  },

      { "ParticleData",   0, OpFloat,        10, T2DbDragCoefficient    },
      { "ParticleData", 104, OpRaw,          32, T2DbWindCoefficient    },
      { "ParticleData",   2, OpSignedFloat,  12, T2DbGravityCoefficient },
      { "ParticleData",   3, OpFloat,         9, T2DbInheritedVelFactor },
      { "ParticleData", 103, OpRaw,          32, T2DbConstantAcceleration },
      { "ParticleData",   5, OpInt,          10, T2DbParticleLifetimeMS },
      { "ParticleData",   6, OpInt,          10, T2DbParticleLifetimeVarianceMS },
      { "ParticleData", 102, OpRaw,          32, T2DbSpinSpeed          },
      { "ParticleData", 100, OpInt,          11, T2DbSpinRandomMin      },
      { "ParticleData", 101, OpInt,          11, T2DbSpinRandomMax      },
      { "ParticleData",   9, OpFlag,          1, T2DbUseInvAlpha        },

      { "ParticleData",  94, OpFloat,  7, T2DbParticleColorR },
      { "ParticleData",  95, OpFloat,  7, T2DbParticleColorG },
      { "ParticleData",  96, OpFloat,  7, T2DbParticleColorB },
      { "ParticleData",  97, OpFloat,  7, T2DbParticleColorA },
      { "ParticleData",  98, OpFloat, 14, T2DbParticleSize   },
      { "ParticleData",  99, OpFloat,  8, T2DbParticleTime   },

      { "ParticleData",  88, OpFloat,  7, T2DbParticleColorR },
      { "ParticleData",  89, OpFloat,  7, T2DbParticleColorG },
      { "ParticleData",  90, OpFloat,  7, T2DbParticleColorB },
      { "ParticleData",  91, OpFloat,  7, T2DbParticleColorA },
      { "ParticleData",  92, OpFloat, 14, T2DbParticleSize   },
      { "ParticleData",  93, OpFloat,  8, T2DbParticleTime   },

      { "ParticleData",  82, OpFloat,  7, T2DbParticleColorR },
      { "ParticleData",  83, OpFloat,  7, T2DbParticleColorG },
      { "ParticleData",  84, OpFloat,  7, T2DbParticleColorB },
      { "ParticleData",  85, OpFloat,  7, T2DbParticleColorA },
      { "ParticleData",  86, OpFloat, 14, T2DbParticleSize   },
      { "ParticleData",  87, OpFloat,  8, T2DbParticleTime   },

      { "ParticleData",  11, OpFloat,  7, T2DbParticleColorR },
      { "ParticleData",  12, OpFloat,  7, T2DbParticleColorG },
      { "ParticleData",  13, OpFloat,  7, T2DbParticleColorB },
      { "ParticleData",  14, OpFloat,  7, T2DbParticleColorA },
      { "ParticleData",  15, OpFloat, 14, T2DbParticleSize   },
      { "ParticleData",  16, OpFloat,  8, T2DbParticleTime   },
   };
}

#endif
