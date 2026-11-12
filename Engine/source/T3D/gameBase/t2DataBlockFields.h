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
   // PlayerData::packData (0x005cf227) opens with this one flag, ahead of the
   // five floats below.
      { "PlayerData", 39, OpFlag,  1, T2DbNoFrictionOnSki  },

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

      { "DecalData", 0, OpRaw, 32, T2DbDecalSizeX },
      { "DecalData", 1, OpRaw, 32, T2DbDecalSizeY },

      { "ExplosionData",   7, OpInt,   14, T2DbPlaySpeed          },
      { "ExplosionData",  16, OpInt,   16, T2DbDelayMS            },
      { "ExplosionData",  17, OpInt,   16, T2DbDelayVariance      },
      { "ExplosionData",  18, OpInt,   16, T2DbExplosionLifetime  },
      { "ExplosionData",  19, OpInt,   16, T2DbLifetimeVariance   },
      { "ExplosionData",  20, OpRaw,   32, T2DbExplosionOffset    },
      { "ExplosionData", 105, OpInt,   16, T2DbExplosionScaleX    },
      { "ExplosionData", 106, OpInt,   16, T2DbExplosionScaleY    },
      { "ExplosionData", 107, OpInt,   16, T2DbExplosionScaleZ    },

   // How many of the time and size keys follow, which the run after it is
   // one of eight variants of.
      { "ExplosionData",  43, OpSelect, 3, T2DbExplosionKeys      },
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
         // Its texture, then the shape, which a read puts back under shapes/.
      { "DebrisData", 22, OpString, 0, T2DbDebrisTexture },
      { "DebrisData", 23, OpString, 0, T2DbDebrisShape   },
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
      { "AudioProfile",  6, OpDataBlockId, 11, T2DbAudioEffect      },
      { "AudioProfile",  3, OpString,       0, T2DbAudioFile        },

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
   // The shape the image is, which a read picks out of the body by its .dts
   // ending and a write has to put at its own node.
      { "ShapeBaseImageData", 1, OpString,  0, T2DbImageShapeFile  },
      { "ShapeBaseImageData", 2, OpRaw,    32, T2DbImageMountPoint },
   // then, unless it is the identity, the offset as writeAffineTransform
   // writes it: a position, a quaternion's x, y and z, and the sign of its w.
      { "ShapeBaseImageData",  4, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 0) },
      { "ShapeBaseImageData",  5, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 1) },
      { "ShapeBaseImageData",  6, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 2) },
      { "ShapeBaseImageData",  7, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 3) },
      { "ShapeBaseImageData",  8, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 4) },
      { "ShapeBaseImageData",  9, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 5) },
      { "ShapeBaseImageData", 10, OpFlag, 1, T2DbImageOffsetWNegative             },
   // and whether a cloak may thin the image as a flag at node 19, just before
   // the light type's ranged write (0x005f585d).
      { "ShapeBaseImageData", 19, OpFlag, 1, T2DbImageCloakable },
   // and whether it reflects the sky's environment map as a bare flag at node
   // 27, just after the casing's datablock id (0x005f59fe).
      { "ShapeBaseImageData", 27, OpFlag, 1, T2DbImageEmap },
   // The light's run returns to node 21 for the shell's exit direction, its
   // variance and its velocity (0x005f592e-0x005f599e); a set flag at node 26
   // sends the casing's datablock id at node 1269 (0x005f59e0).
      { "ShapeBaseImageData", 21, OpRaw, 32, T2DataBlockField(T2DbImageShellExitDir + 0) },
      { "ShapeBaseImageData", 22, OpRaw, 32, T2DataBlockField(T2DbImageShellExitDir + 1) },
      { "ShapeBaseImageData", 23, OpRaw, 32, T2DataBlockField(T2DbImageShellExitDir + 2) },
      { "ShapeBaseImageData", 24, OpRaw, 32, T2DbImageShellExitVariance },
      { "ShapeBaseImageData", 25, OpRaw, 32, T2DbImageShellVelocity },
      { "ShapeBaseImageData", 1269, OpDataBlockId, 11, T2DbImageCasing },
   // A TurretImageData's layout is ShapeBaseImageData's with its own run
   // inserted at node 59, so all of those keep their nodes.
      { "TurretImageData",     2, OpRaw, 32, T2DbImageMountPoint },
      { "TurretImageData",     4, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 0) },
      { "TurretImageData",     5, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 1) },
      { "TurretImageData",     6, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 2) },
      { "TurretImageData",     7, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 3) },
      { "TurretImageData",     8, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 4) },
      { "TurretImageData",     9, OpRaw, 32, T2DataBlockField(T2DbImageOffset + 5) },
   // The same opening run as ShapeBaseImageData's, node for node: its shape,
   // its mount, the eye offset and the shell it throws.
      { "TurretImageData",     1, OpString,  0, T2DbImageShapeFile  },
      { "TurretImageData",     4, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 0) },
      { "TurretImageData",     5, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 1) },
      { "TurretImageData",     6, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 2) },
      { "TurretImageData",     7, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 3) },
      { "TurretImageData",     8, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 4) },
      { "TurretImageData",     9, OpRaw,    32, T2DataBlockField(T2DbImageOffset + 5) },
      { "TurretImageData",    21, OpRaw,    32, T2DataBlockField(T2DbImageShellExitDir + 0) },
      { "TurretImageData",    22, OpRaw,    32, T2DataBlockField(T2DbImageShellExitDir + 1) },
      { "TurretImageData",    23, OpRaw,    32, T2DataBlockField(T2DbImageShellExitDir + 2) },
      { "TurretImageData",    24, OpRaw,    32, T2DbImageShellExitVariance },
      { "TurretImageData",    25, OpRaw,    32, T2DbImageShellVelocity },
      { "TurretImageData",  1276, OpDataBlockId, 11, T2DbImageCasing },

      { "TurretImageData",    10, OpFlag, 1, T2DbImageOffsetWNegative             },
      { "TurretImageData",    19, OpFlag, 1, T2DbImageCloakable },
      { "TurretImageData",    27, OpFlag, 1, T2DbImageEmap },

   // What each projectile class puts between the run every one of them opens
   // with and the tail every one of them ends with, both of which t2Layout's
   // matchProjectileData names by their shape. Settled by declaring probe
   // datablocks at a retail server's console and reading the bodies it sent:
   // twelve of the thirteen classes agreed on the shared parts node for node,
   // and each of these is the class's own middle.
   //
   // The ms fields a class carries are quantised to whole ticks, rounding up,
   // which is why a lifetimeMS of 5000 goes out as 5024.
#define T2_PROJECTILE_BALLISTIC_ROWS( cls )                                   \
      { cls, 24, OpRaw, 32, T2DbArmingDelay        },                         \
      { cls, 25, OpRaw, 32, T2DbProjectileVelocity },                         \
      { cls, 26, OpRaw, 32, T2DbElasticity         },                         \
      { cls, 27, OpRaw, 32, T2DbFriction           },                         \
      { cls, 28, OpRaw, 32, T2DbProjectileDrag     },                         \
      { cls, 29, OpRaw, 32, T2DbProjectileDensity  },                         \
      { cls, 30, OpRaw, 32, T2DbGravityMod         },                         \
      { cls, 31, OpRaw, 32, T2DbProjectileLifetime },

   // What a bolt and a bullet carry instead: the two speeds it flies at, the
   // three times it lives by, and how it meets water.
#define T2_PROJECTILE_LINEAR_ROWS( cls )                                      \
      { cls, 24, OpRaw,  32, T2DbProjectileDryVelocity               },       \
      { cls, 25, OpRaw,  32, T2DbProjectileWetVelocity               },       \
      { cls, 26, OpRaw,  32, T2DbProjectileFizzleTime                },       \
      { cls, 27, OpRaw,  32, T2DbProjectileLifetime                  },       \
      { cls, 28, OpFlag,  1, T2DbProjectileExplodeOnDeath            },       \
      { cls, 29, OpInt,   7, T2DbProjectileReflectOnWaterImpactAngle },       \
      { cls, 30, OpInt,   7, T2DbProjectileDeflectionOnWaterImpact   },       \
      { cls, 31, OpRaw,  32, T2DbProjectileFizzleUnderwater          },       \
      { cls, 32, OpRaw,  32, T2DbProjectileActivateDelay             },       \
      { cls, 33, OpFlag,  1, T2DbProjectileDoDynamicClientHits       },

      T2_PROJECTILE_BALLISTIC_ROWS( "BombProjectileData"    )
      T2_PROJECTILE_BALLISTIC_ROWS( "EnergyProjectileData"  )
      T2_PROJECTILE_BALLISTIC_ROWS( "FlareProjectileData"   )
      T2_PROJECTILE_BALLISTIC_ROWS( "GrenadeProjectileData" )

      T2_PROJECTILE_LINEAR_ROWS( "LinearFlareProjectileData" )
      T2_PROJECTILE_LINEAR_ROWS( "LinearProjectileData"      )
      T2_PROJECTILE_LINEAR_ROWS( "TracerProjectileData"      )
#undef T2_PROJECTILE_BALLISTIC_ROWS
#undef T2_PROJECTILE_LINEAR_ROWS

   // A vehicle's bomb turns as it falls, each speed a vector of three.
      { "BombProjectileData", 32, OpRaw, 32, T2DbProjectileMinRotSpeedX },
      { "BombProjectileData", 33, OpRaw, 32, T2DbProjectileMinRotSpeedY },
      { "BombProjectileData", 34, OpRaw, 32, T2DbProjectileMinRotSpeedZ },
      { "BombProjectileData", 35, OpRaw, 32, T2DbProjectileMaxRotSpeedX },
      { "BombProjectileData", 36, OpRaw, 32, T2DbProjectileMaxRotSpeedY },
      { "BombProjectileData", 37, OpRaw, 32, T2DbProjectileMaxRotSpeedZ },

   // A bolt is drawn as a blur of its own colour, end on as a cross.
      { "EnergyProjectileData", 32, OpRaw, 32, T2DbCrossViewAng },
      { "EnergyProjectileData", 33, OpRaw, 32, T2DbCrossSize    },
      { "EnergyProjectileData", 34, OpRaw, 32, T2DbBlurLifetime },
      { "EnergyProjectileData", 35, OpRaw, 32, T2DbBlurWidth    },
      { "EnergyProjectileData", 36, OpRaw, 32, T2DbBlurColorR   },
      { "EnergyProjectileData", 37, OpRaw, 32, T2DbBlurColorG   },
      { "EnergyProjectileData", 38, OpRaw, 32, T2DbBlurColorB   },

      { "FlareProjectileData", 32, OpRaw, 32, T2DbProjectileSize        },
      { "FlareProjectileData", 33, OpRaw,  8, T2DbProjectileUseLensFlare },

   // A bullet is seen as the streak it leaves and the cross it is end on.
      { "TracerProjectileData", 34, OpRaw, 32, T2DbTracerLength    },
      { "TracerProjectileData", 35, OpRaw, 32, T2DbTracerWidth     },
      { "TracerProjectileData", 36, OpRaw, 32, T2DbTracerMinPixels },
      { "TracerProjectileData", 37, OpRaw,  8, T2DbTracerAlpha     },
      { "TracerProjectileData", 38, OpColor, 32, T2DbTracerColor   },
      { "TracerProjectileData", 39, OpRaw, 32, T2DbCrossViewAng    },
      { "TracerProjectileData", 40, OpRaw, 32, T2DbCrossSize       },
      { "TracerProjectileData", 41, OpRaw,  8, T2DbRenderCross     },

   // A flare bolt's petals.
      { "LinearFlareProjectileData", 34, OpRaw,   32, T2DbNumFlares  },
      { "LinearFlareProjectileData", 35, OpColor, 32, T2DbFlareColor },
      { "LinearFlareProjectileData", 38, OpRaw,   32, T2DbFlareSize0 },
      { "LinearFlareProjectileData", 39, OpRaw,   32, T2DbFlareSize1 },
      { "LinearFlareProjectileData", 40, OpRaw,   32, T2DbFlareSize2 },

   // A seeker's own flight, and the trail and casing it sheds. Nodes 39 and
   // 40 are strings the shared matcher names, and 45 to 47 are three more
   // datablock ids of its own, each behind a flag.
      { "SeekerProjectileData", 24, OpRaw, 32, T2DbProjectileLifetime         },
      { "SeekerProjectileData", 25, OpRaw, 32, T2DbProjectileVelocity         },
      { "SeekerProjectileData", 26, OpRaw, 32, T2DbSeekerTurning              },
      { "SeekerProjectileData", 27, OpRaw, 32, T2DbSeekerProximity            },
      { "SeekerProjectileData", 28, OpRaw, 32, T2DbSeekerTerrainAvoidanceSpeed},
      { "SeekerProjectileData", 29, OpRaw, 32, T2DbSeekerTerrainScanAhead     },
      { "SeekerProjectileData", 30, OpRaw, 32, T2DbSeekerTerrainHeightFail    },
      { "SeekerProjectileData", 31, OpRaw, 32, T2DbSeekerTerrainAvoidanceRadius},
      { "SeekerProjectileData", 32, OpRaw, 32, T2DbSeekerFlareDistance        },
      { "SeekerProjectileData", 33, OpRaw, 32, T2DbSeekerFlareAngle           },
      { "SeekerProjectileData", 34, OpRaw,  8, T2DbSeekerUseFlechette         },
      { "SeekerProjectileData", 35, OpRaw, 32, T2DbSeekerMaxVelocity          },
      { "SeekerProjectileData", 36, OpRaw, 32, T2DbSeekerAcceleration         },
      { "SeekerProjectileData", 37, OpRaw, 32, T2DbSeekerFlechetteDelay       },
      { "SeekerProjectileData", 38, OpRaw, 32, T2DbSeekerExhaustTime          },
      { "SeekerProjectileData", 45, OpDataBlockId, 11, T2DbSeekerExhaustEmitter},
      { "SeekerProjectileData", 46, OpDataBlockId, 11, T2DbSeekerPuffEmitter  },
      { "SeekerProjectileData", 47, OpDataBlockId, 11, T2DbSeekerCasingDebris },

   // The lance's zap: how long it lasts, how far it reaches, and the two
   // bolts it is drawn as.
      { "ShockLanceProjectileData",  24, OpRaw, 32, T2DbZapDuration           },
      { "ShockLanceProjectileData",  25, OpRaw, 32, T2DbBoltLength            },
      { "ShockLanceProjectileData",  26, OpRaw, 32, T2DbBoltParts             },
      { "ShockLanceProjectileData",  27, OpRaw, 32, T2DbShockLightningFreq    },
      { "ShockLanceProjectileData",  28, OpRaw, 32, T2DbShockLightningDensity },
      { "ShockLanceProjectileData",  29, OpRaw, 32, T2DbShockLightningAmp     },
      { "ShockLanceProjectileData",  30, OpRaw, 32, T2DbShockLightningWidth   },
      { "ShockLanceProjectileData",  32, OpRaw, 32, T2DbBoltStartWidth0       },
      { "ShockLanceProjectileData",  33, OpRaw, 32, T2DbBoltEndWidth0         },
      { "ShockLanceProjectileData",  34, OpRaw, 32, T2DbBoltSpeed0            },
      { "ShockLanceProjectileData",  35, OpRaw, 32, T2DbBoltTexWrap0          },
      { "ShockLanceProjectileData",  36, OpRaw, 32, T2DbBoltStartWidth1       },
      { "ShockLanceProjectileData",  37, OpRaw, 32, T2DbBoltEndWidth1         },
      { "ShockLanceProjectileData",  38, OpRaw, 32, T2DbBoltSpeed1            },
      { "ShockLanceProjectileData",  39, OpRaw, 32, T2DbBoltTexWrap1          },
   // The emitter it sparks with, behind a flag of its own. The shockwave the
   // scripts also name is not on the wire: a retail server sent nothing here
   // for a datablock that set only `shockwave`, and 495 -- the id of what it
   // was given -- for one that set `emitter`.
      { "ShockLanceProjectileData",  46, OpDataBlockId, 11, T2DbShockLanceEmitter },

   // SeekerProjectileData::packData (0x00637b10) writes its own middle between
   // the two runs matchProjectileData names by their shape: the lifetime, the
   // speed it leaves the muzzle at, how fast it turns, the radius it goes off
   // inside, the four terrain-avoidance fields, how far and how wide it looks
   // for a flare, whether it drops a casing, the speed its motor takes it to
   // and how hard, how long the casing takes, how long the launcher's exhaust
   // lasts, the node that exhaust comes out of, the shape the casing is, and
   // the three datablock ids of its own.
      { "SeekerProjectileData", 24, OpRaw, 32, T2DbProjectileLifetime     },
      { "SeekerProjectileData", 25, OpRaw, 32, T2DbSeekerMuzzleVelocity   },
      { "SeekerProjectileData", 26, OpRaw, 32, T2DbSeekerTurningSpeed     },
      { "SeekerProjectileData", 27, OpRaw, 32, T2DbSeekerProximityRadius  },
      { "SeekerProjectileData", 28, OpRaw, 32, T2DbSeekerAvoidanceSpeed   },
      { "SeekerProjectileData", 29, OpRaw, 32, T2DbSeekerScanAhead        },
      { "SeekerProjectileData", 30, OpRaw, 32, T2DbSeekerHeightFail       },
      { "SeekerProjectileData", 31, OpRaw, 32, T2DbSeekerAvoidanceRadius  },
      { "SeekerProjectileData", 32, OpRaw, 32, T2DbSeekerFlareDistance    },
      { "SeekerProjectileData", 33, OpRaw, 32, T2DbSeekerFlareAngle       },
      { "SeekerProjectileData", 34, OpRaw,  8, T2DbSeekerUseFlechette     },
      { "SeekerProjectileData", 35, OpRaw, 32, T2DbSeekerMaxVelocity      },
      { "SeekerProjectileData", 36, OpRaw, 32, T2DbSeekerAcceleration     },
      { "SeekerProjectileData", 37, OpRaw, 32, T2DbSeekerFlechetteDelay   },
      { "SeekerProjectileData", 38, OpRaw, 32, T2DbSeekerExhaustTime      },
      { "SeekerProjectileData", 39, OpString, 0, T2DbSeekerExhaustNode    },
      { "SeekerProjectileData", 40, OpString, 0, T2DbSeekerCasingShape    },
      { "SeekerProjectileData", 47, OpDataBlockId, 11, T2DbSeekerCasingDebris  },
      { "SeekerProjectileData", 46, OpDataBlockId, 11, T2DbSeekerPuffEmitter   },
      { "SeekerProjectileData", 45, OpDataBlockId, 11, T2DbSeekerExhaustEmitter },

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

   // PrecipitationData, settled against demo000's three bodies: Rain's
   // distinct 0.2, 0.45, 0.35, 1.5, 20, 30, 40 and -0.3 pin the run in its
   // script's own order, Sand's maxSize of 2 pins node 2, and the sound is
   // the only thing behind the body's one flag -- Rain names one and Snow
   // does not.
      { "PrecipitationData", 18, OpDataBlockId, 11, T2DbPrecipSound            },
      { "PrecipitationData",  1, OpRaw,         32, T2DbPrecipType             },
      { "PrecipitationData",  2, OpRaw,         32, T2DbPrecipMaxSize          },
      { "PrecipitationData",  3, OpString,       0, T2DbPrecipMaterialList     },
      { "PrecipitationData",  4, OpRaw,         32, T2DbPrecipSizeX            },
      { "PrecipitationData",  5, OpRaw,         32, T2DbPrecipSizeY            },
      { "PrecipitationData",  6, OpRaw,         32, T2DbPrecipMovingBoxPer     },
      { "PrecipitationData",  7, OpRaw,         32, T2DbPrecipDivHeightVal     },
      { "PrecipitationData",  8, OpRaw,         32, T2DbPrecipSizeBigBox       },
      { "PrecipitationData",  9, OpRaw,         32, T2DbPrecipTopBoxSpeed      },
      { "PrecipitationData", 10, OpRaw,         32, T2DbPrecipFrontBoxSpeed    },
      { "PrecipitationData", 11, OpRaw,         32, T2DbPrecipTopBoxDrawPer    },
      { "PrecipitationData", 12, OpRaw,         32, T2DbPrecipBottomDrawHeight },
      { "PrecipitationData", 13, OpRaw,         32, T2DbPrecipSkipIfPer        },
      { "PrecipitationData", 14, OpRaw,         32, T2DbPrecipBottomSpeedPer   },
      { "PrecipitationData", 15, OpRaw,         32, T2DbPrecipFrontSpeedPer    },
      { "PrecipitationData", 16, OpRaw,         32, T2DbPrecipFrontRadiusPer   },

   // ShockwaveData, every node settled against demo004's fourteen bodies:
   // node 7 is 6.0 for the mortar's width and 4.0 for the grenade's, node 10
   // holds each one's velocity, and the five eight-bit flags are told apart by
   // the one body that sets each -- is2D by the concussion grenade,
   // orientToNormal by the chaingun, mapToTerrain by the mortar, renderBottom
   // by the turret and renderSquare by the muzzle flashes. Nodes 0 to 2 are
   // 1.0 and nodes 3, 4 and 6 nought in every one of them, and the three
   // flagged ids are never sent.
      { "ShockwaveData",  0, OpRaw,   32, T2DataBlockField(T2DbShockwaveOne + 0) },
      { "ShockwaveData",  1, OpRaw,   32, T2DataBlockField(T2DbShockwaveOne + 1) },
      { "ShockwaveData",  2, OpRaw,   32, T2DataBlockField(T2DbShockwaveOne + 2) },
      { "ShockwaveData",  5, OpRaw,   32, T2DbShockwaveLifetime       },
      { "ShockwaveData",  7, OpRaw,   32, T2DbShockwaveWidth          },
      { "ShockwaveData",  8, OpRaw,   32, T2DbShockwaveSegments       },
      { "ShockwaveData",  9, OpRaw,   32, T2DbShockwaveVertSegments   },
      { "ShockwaveData", 10, OpRaw,   32, T2DbShockwaveVelocity       },
      { "ShockwaveData", 11, OpRaw,   32, T2DbShockwaveHeight         },
      { "ShockwaveData", 12, OpRaw,   32, T2DbShockwaveVerticalCurve  },
      { "ShockwaveData", 13, OpRaw,   32, T2DbShockwaveAcceleration   },
      { "ShockwaveData", 14, OpRaw,   32, T2DbShockwaveTexWrap        },
      { "ShockwaveData", 15, OpRaw,    8, T2DbShockwaveIs2D           },
      { "ShockwaveData", 16, OpRaw,    8, T2DbShockwaveOrientToNormal },
      { "ShockwaveData", 17, OpRaw,    8, T2DbShockwaveMapToTerrain   },
      { "ShockwaveData", 18, OpRaw,    8, T2DbShockwaveRenderBottom   },
      { "ShockwaveData", 19, OpRaw,    8, T2DbShockwaveRenderSquare   },
      { "ShockwaveData", 23, OpColor, 32, T2DataBlockField(T2DbShockwaveColor + 0) },
      { "ShockwaveData", 24, OpColor, 32, T2DataBlockField(T2DbShockwaveColor + 1) },
      { "ShockwaveData", 25, OpColor, 32, T2DataBlockField(T2DbShockwaveColor + 2) },
      { "ShockwaveData", 26, OpColor, 32, T2DataBlockField(T2DbShockwaveColor + 3) },
      { "ShockwaveData", 27, OpRaw,   32, T2DataBlockField(T2DbShockwaveTime + 0) },
      { "ShockwaveData", 28, OpRaw,   32, T2DataBlockField(T2DbShockwaveTime + 1) },
      { "ShockwaveData", 29, OpRaw,   32, T2DataBlockField(T2DbShockwaveTime + 2) },
      { "ShockwaveData", 30, OpRaw,   32, T2DataBlockField(T2DbShockwaveTime + 3) },
      { "ShockwaveData", 31, OpString, 0, T2DataBlockField(T2DbShockwaveTexture + 0) },
      { "ShockwaveData", 32, OpString, 0, T2DataBlockField(T2DbShockwaveTexture + 1) },

   // EffectProfile, settled against a recorded retail server: node 0 is the
   // minDistance and node 1 the maxDistance of ArmorJetEffect's 5 and 10, and
   // node 4 is the effect it names. Node 2 is 0.125 and node 3 is nought in
   // all 128 bodies of demo000, which is every EffectProfile the game has, so
   // they are values no script reaches rather than fields of their own.
      { "EffectProfile",  0, OpRaw,    32, T2DbEffectMinDistance },
      { "EffectProfile",  1, OpRaw,    32, T2DbEffectMaxDistance },
      { "EffectProfile",  2, OpRaw,    32, T2DbEffectThird       },
      { "EffectProfile",  4, OpString,  0, T2DbEffectName        },

   // RunningLightData, settled against a recorded retail server for its
   // radius, colour, type, length, node name and texture. Its two vectors are
   // the only fields left and go out in the script's own order, direction
   // before offset: no body in the recording tells those two apart, because
   // the one it carries holds values the shipped scripts do not.
      { "RunningLightData",  0, OpRaw,   32, T2DbLightRadius   },
      { "RunningLightData",  1, OpColor, 32, T2DbLightColor    },
      { "RunningLightData",  2, OpRaw,   32, T2DbLightType     },
      { "RunningLightData",  3, OpRaw,   32, T2DbLightLength   },
      { "RunningLightData",  4, OpString, 0, T2DbLightNodeName },
      { "RunningLightData",  5, OpRaw,   32, T2DataBlockField(T2DbLightDirection + 0) },
      { "RunningLightData",  6, OpRaw,   32, T2DataBlockField(T2DbLightDirection + 1) },
      { "RunningLightData",  7, OpRaw,   32, T2DataBlockField(T2DbLightDirection + 2) },
      { "RunningLightData",  8, OpRaw,   32, T2DataBlockField(T2DbLightOffset + 0) },
      { "RunningLightData",  9, OpRaw,   32, T2DataBlockField(T2DbLightOffset + 1) },
      { "RunningLightData", 10, OpRaw,   32, T2DataBlockField(T2DbLightOffset + 2) },
      { "RunningLightData", 13, OpString, 0, T2DbLightTexture  },

   // One node, one field a script sets: ParticleEmissionDummyData carries
   // nothing but its timeMultiple, and FireballAtmosphereData nothing but the
   // debris it throws, behind the flag that says it has any.
      { "ParticleEmissionDummyData", 0, OpRaw,         32, T2DbEmissionDummyTimeMultiple },
      { "FireballAtmosphereData",    2, OpDataBlockId, 11, T2DbFireballDebris            },

      { "JetEffectData",  0, OpColor,  32, T2DbJetCoolColor      },
      { "JetEffectData",  1, OpColor,  32, T2DbJetHotColor       },
      { "JetEffectData",  2, OpRaw,    32, T2DbJetActivateTime   },
      { "JetEffectData",  3, OpRaw,    32, T2DbJetDeactivateTime },
      { "JetEffectData",  4, OpRaw,    32, T2DbJetLength         },
      { "JetEffectData",  5, OpRaw,    32, T2DbJetWidth          },
      { "JetEffectData",  6, OpRaw,    32, T2DbJetSpeed          },
      { "JetEffectData",  7, OpRaw,    32, T2DbJetStretch        },
      { "JetEffectData",  8, OpRaw,    32, T2DbJetYOffset        },
      { "JetEffectData", 11, OpString,  0, T2DbJetTexture        },

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

   // The particles an emitter throws: a count, then a flag and an id apiece.
   // The read side takes the ids off lastDataBlockIds(), which every id in a
   // body lands in; the write side needs them by node.
      { "ParticleEmitterData", 16, OpReadBr,     32, T2DbEmitterParticleCount },
      { "ParticleEmitterData", 21, OpDataBlockId, 11, T2DbEmitterParticle     },

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

   // How many of the four colour/size/time keys follow, one less than the
   // count, and how many texture names end the body. A reader takes these off
   // the run that follows; a writer has to be told.
      { "ParticleData",  10, OpSelect,  2, T2DbParticleKeysLast   },
      { "ParticleData",  17, OpSelect,  6, T2DbParticleTextures   },

   // The texture names the select above counts, which end the body as a chain:
   // for n of them the walk comes down from node 18+n to 19, one name apiece.
      { "ParticleData",  81, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  80, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  79, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  78, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  77, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  76, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  75, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  74, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  73, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  72, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  71, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  70, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  69, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  68, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  67, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  66, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  65, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  64, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  63, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  62, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  61, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  60, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  59, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  58, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  57, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  56, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  55, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  54, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  53, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  52, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  51, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  50, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  49, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  48, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  47, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  46, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  45, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  44, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  43, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  42, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  41, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  40, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  39, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  38, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  37, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  36, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  35, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  34, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  33, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  32, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  31, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  30, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  29, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  28, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  27, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  26, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  25, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  24, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  23, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  22, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  21, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  20, OpString,  0, T2DbParticleTextureName },
      { "ParticleData",  19, OpString,  0, T2DbParticleTextureName },

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

   // ForceFieldBareData::packData (0x00675450) in its order; the five
   // textures that end it are strings.
      { "ForceFieldBareData",  0, OpRaw,   32, T2DbFieldFadeMS               },
      { "ForceFieldBareData",  1, OpRaw,   32, T2DbFieldBaseTranslucency     },
      { "ForceFieldBareData",  2, OpRaw,   32, T2DbFieldPowerOffTranslucency },
      { "ForceFieldBareData",  3, OpFlag,   1, T2DbFieldTeamPermiable        },
      { "ForceFieldBareData",  4, OpFlag,   1, T2DbFieldOtherPermiable       },
      { "ForceFieldBareData",  5, OpColor, 32, T2DbFieldColor                },
      { "ForceFieldBareData",  6, OpColor, 32, T2DbFieldPowerOffColor        },
      { "ForceFieldBareData",  7, OpRaw,   32, T2DbFieldFramesPerSec         },
      { "ForceFieldBareData",  8, OpRaw,   32, T2DbFieldNumFrames            },
      { "ForceFieldBareData",  9, OpRaw,   32, T2DbFieldScrollSpeed          },
      { "ForceFieldBareData", 10, OpRaw,   32, T2DbFieldUMapping             },
      { "ForceFieldBareData", 11, OpRaw,   32, T2DbFieldVMapping             },

   // SniperProjectileData::packData (0x00641bd0) and TargetProjectileData's
   // (0x006476e0), after ProjectileData's run; the textures are strings.
   // RepairProjectileData's own run, read off the wire and checked against
   // DefaultRepairBeam: 10, 0.15, 20, 1, 0.2, 10, 1, 25.
      { "RepairProjectileData", 24, OpRaw, 32, T2DbBeamRange          },
      { "RepairProjectileData", 25, OpRaw, 32, T2DbRepairWidth        },
      { "RepairProjectileData", 26, OpRaw, 32, T2DbRepairSegments     },
      { "RepairProjectileData", 27, OpRaw, 32, T2DbRepairSpeed        },
      { "RepairProjectileData", 28, OpRaw, 32, T2DbRepairTexRepeat    },
      { "RepairProjectileData", 29, OpRaw, 32, T2DbRepairBlurFreq     },
      { "RepairProjectileData", 30, OpRaw, 32, T2DbRepairBlurLifetime },
      { "RepairProjectileData", 31, OpRaw, 32, T2DbRepairCutoffAngle  },

   // ELFProjectileData's run, read off the wire and checked against BasicELF:
   // 38, 0.1, 9, 0.25, 0.1, 0.15.
      { "ELFProjectileData", 24, OpRaw, 32, T2DbBeamRange         },
      { "ELFProjectileData", 25, OpRaw, 32, T2DbElfBeamWidth      },
      { "ELFProjectileData", 26, OpRaw, 32, T2DbElfBeamSpeed      },
      { "ELFProjectileData", 27, OpRaw, 32, T2DbElfBeamRepeat     },
      { "ELFProjectileData", 28, OpRaw, 32, T2DbElfLightningWidth },
      { "ELFProjectileData", 29, OpRaw, 32, T2DbElfLightningDist  },
      { "ELFProjectileData", 35, OpDataBlockId, 11, T2DbElfEmitter   },

      { "SniperProjectileData", 24, OpRaw,   32, T2DbBeamRange              },
      { "SniperProjectileData", 25, OpRaw,   32, T2DbBeamHeadMultiplier     },
      { "SniperProjectileData", 26, OpColor, 32, T2DbBeamColor              },
      { "SniperProjectileData", 27, OpRaw,   32, T2DbBeamFadeTime           },
      { "SniperProjectileData", 28, OpRaw,   32, T2DbBeamStartWidth         },
      { "SniperProjectileData", 29, OpRaw,   32, T2DbBeamEndWidth           },
      { "SniperProjectileData", 30, OpRaw,   32, T2DbBeamPulseWidth         },
      { "SniperProjectileData", 31, OpRaw,   32, T2DbBeamFlareAngle         },
      { "SniperProjectileData", 32, OpRaw,   32, T2DbBeamMinFlareSize       },
      { "SniperProjectileData", 33, OpRaw,   32, T2DbBeamMaxFlareSize       },
      { "SniperProjectileData", 34, OpRaw,   32, T2DbBeamPulseSpeed         },
      { "SniperProjectileData", 35, OpRaw,   32, T2DbBeamPulseLength        },
   // Two a sniper's body carries that no script sets.
      { "SniperProjectileData", 36, OpColor, 32, T2DbBeamHitColor           },
      { "SniperProjectileData", 37, OpRaw,   32, T2DbBeamHitScale           },

      { "TargetProjectileData", 24, OpRaw,   32, T2DbBeamRange              },
      { "TargetProjectileData", 25, OpColor, 32, T2DbBeamColor              },
      { "TargetProjectileData", 26, OpRaw,   32, T2DbBeamStartWidth         },
      { "TargetProjectileData", 27, OpRaw,   32, T2DbBeamPulseWidth         },
      { "TargetProjectileData", 28, OpRaw,   32, T2DbBeamFlareAngle         },
      { "TargetProjectileData", 29, OpRaw,   32, T2DbBeamMinFlareSize       },
      { "TargetProjectileData", 30, OpRaw,   32, T2DbBeamMaxFlareSize       },
      { "TargetProjectileData", 31, OpRaw,   32, T2DbBeamPulseSpeed         },
      { "TargetProjectileData", 32, OpRaw,   32, T2DbBeamPulseLength        },

   // TurretData::packData (0x006540d0), after StaticShapeData's dynamicType.
      { "TurretData", 41, OpRaw,    32, T2DbThetaMin              },
      { "TurretData", 42, OpRaw,    32, T2DbThetaMax              },
      { "TurretData", 43, OpRaw,    32, T2DbThetaNull             },
      { "TurretData", 44, OpFlag,    1, T2DbNeverUpdateControl    },
      { "TurretData", 45, OpSelect,  2, T2DbPrimaryAxis           },
      { "TurretData", 46, OpRaw,    32, T2DbMaxCapacitorEnergy    },
      { "TurretData", 47, OpRaw,    32, T2DbCapacitorRechargeRate },

   // TurretImageData::packData (0x00654760): the two delays as counts of
   // 32 ms, the two rates in whole degrees a second.
      { "TurretImageData", 59, OpInt,   8, T2DbActivationMS               },
      { "TurretImageData", 60, OpInt,   8, T2DbDeactivateDelayMS          },
      { "TurretImageData", 61, OpInt,  11, T2DbDegPerSecTheta             },
      { "TurretImageData", 62, OpInt,  11, T2DbDegPerSecPhi               },
      { "TurretImageData", 63, OpFlag,  1, T2DbDontFireInsideDamageRadius },
      { "TurretImageData", 64, OpRaw,  32, T2DbDamageRadius               },
      { "TurretImageData", 65, OpFlag,  1, T2DbUseCapacitor               },

   // StationFXVehicleData::packData (0x0069d370) in its order: every field
   // a whole word, the colour as four bytes, then eleven strings.
      { "StationFXVehicleData",  0, OpRaw,   32, T2DbStationGlowTopHeight       },
      { "StationFXVehicleData",  1, OpRaw,   32, T2DbStationGlowBottomHeight    },
      { "StationFXVehicleData",  2, OpRaw,   32, T2DbStationGlowTopRadius       },
      { "StationFXVehicleData",  3, OpRaw,   32, T2DbStationGlowBottomRadius    },
      { "StationFXVehicleData",  4, OpRaw,   32, T2DbStationNumGlowSegments     },
      { "StationFXVehicleData",  5, OpRaw,   32, T2DbStationGlowFadeTime        },
      { "StationFXVehicleData",  6, OpRaw,   32, T2DbStationArmLightDelay       },
      { "StationFXVehicleData",  7, OpRaw,   32, T2DbStationArmLightLifetime    },
      { "StationFXVehicleData",  8, OpRaw,   32, T2DbStationArmLightFadeTime    },
      { "StationFXVehicleData",  9, OpRaw,   32, T2DbStationLifetime            },
      { "StationFXVehicleData", 10, OpRaw,   32, T2DbStationNumArcSegments      },
      { "StationFXVehicleData", 11, OpColor, 32, T2DbStationSphereColor         },
      { "StationFXVehicleData", 12, OpRaw,   32, T2DbStationSpherePhiSegments   },
      { "StationFXVehicleData", 13, OpRaw,   32, T2DbStationSphereThetaSegments },
      { "StationFXVehicleData", 14, OpRaw,   32, T2DbStationSphereRadius        },
      { "StationFXVehicleData", 15, OpRaw,   32, T2DbStationSphereScaleX        },
      { "StationFXVehicleData", 16, OpRaw,   32, T2DbStationSphereScaleY        },
      { "StationFXVehicleData", 17, OpRaw,   32, T2DbStationSphereScaleZ        },

   // SplashData::packData: scale and fifteen words, the explosion and three
   // emitters behind flags, four colours as bytes, four times, two strings.
      { "SplashData",  0, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  1, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  2, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  3, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  4, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  5, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  6, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  7, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  8, OpRaw,         32, T2DbSplashWord        },
      { "SplashData",  9, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 10, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 11, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 12, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 13, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 14, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 15, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 16, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 17, OpRaw,         32, T2DbSplashWord        },
      { "SplashData", 22, OpColor,       32, T2DbSplashRingColor   },
      { "SplashData", 23, OpColor,       32, T2DbSplashRingColor   },
      { "SplashData", 24, OpColor,       32, T2DbSplashRingColor   },
      { "SplashData", 25, OpColor,       32, T2DbSplashRingColor   },
      { "SplashData", 26, OpRaw,         32, T2DbSplashRingTime    },
      { "SplashData", 27, OpRaw,         32, T2DbSplashRingTime    },
      { "SplashData", 28, OpRaw,         32, T2DbSplashRingTime    },
      { "SplashData", 29, OpRaw,         32, T2DbSplashRingTime    },
      { "SplashData", 35, OpDataBlockId, 11, T2DbSplashRingEmitter },
      { "SplashData", 34, OpDataBlockId, 11, T2DbSplashRingEmitter },
      { "SplashData", 33, OpDataBlockId, 11, T2DbSplashRingEmitter },
      { "SplashData", 36, OpDataBlockId, 11, T2DbExplosion         },

   // StationFXPersonalData::packData (0x0069bee0) in its order, every field
   // a whole word, then four strings. It sends numArcSegments twice and keeps
   // the second.
      { "StationFXPersonalData",  0, OpRaw,   32, T2DbStationDelay         },
      { "StationFXPersonalData",  1, OpRaw,   32, T2DbStationFadeDelay     },
      { "StationFXPersonalData",  2, OpRaw,   32, T2DbStationLifetime      },
      { "StationFXPersonalData",  3, OpRaw,   32, T2DbStationHeight        },
      { "StationFXPersonalData",  5, OpRaw,   32, T2DbStationNumDegrees    },
      { "StationFXPersonalData",  6, OpRaw,   32, T2DbStationTrailFadeTime },
      { "StationFXPersonalData",  7, OpRaw,   32, T2DbStationLeftRadius    },
      { "StationFXPersonalData",  8, OpRaw,   32, T2DbStationRightRadius   },
      { "StationFXPersonalData",  9, OpRaw,   32, T2DbStationNumArcSegments },
   };
}

#endif
