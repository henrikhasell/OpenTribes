/// @file
/// The bit layout of the Tribes 2 event classes this engine does not have.
///
/// An event's body cannot be skipped any more than a datablock's -- nothing on
/// the wire says how long one is -- so a class the client does not implement
/// still has to be read to reach the next event in the packet.  These are the
/// same kind of graph as t2DataBlockTables.h, extracted the same way, and they
/// share its node and op types.

#ifndef _T2EVENTTABLES_H_
#define _T2EVENTTABLES_H_

namespace T2Protocol
{
   // The node and op types are the datablock ones; only the tables differ.

   static const T2DataNode sT2EvNodes_CRCChallengeEvent[] =
   {
      { OpRaw, 32, 1, -1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpFlag, 1, 4, 4 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_CRCChallengeResponseEvent[] =
   {
      { OpRaw, 32, 1, -1 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_FogChallengeEvent[] =
   {
      { OpInt, 16, 1, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const S16 sT2EvSel_GhostingMessageEvent[] = { 2, 2, 2, 2, 2, 2, 2, 2 };
   static const T2DataNode sT2EvNodes_GhostingMessageEvent[] =
   {
      { OpRaw, 32, 1, -1 },
      { OpSelect, 3, 0, -1 },
      { OpInt, 11, 3, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_GravityEvent[] =
   {
      { OpRaw, 32, 1, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_PathManagerEvent[] =
   {
      { OpFlag, 1, 1, 11 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpCount, 32, 4, 0 },
      { OpLoop, 0, 10, 5 },
      { OpRaw, 32, 6, -1 },
      { OpRaw, 32, 7, -1 },
      { OpRaw, 32, 8, -1 },
      { OpRaw, 32, 9, -1 },
      { OpEndBody, 0, -1, -1 },
      { OpEnd, 0, -1, -1 },
      { OpCount, 32, 12, 1 },
      { OpLoop, 1, 13, 9 },
      { OpLoop, 1, 10, 14 },
      { OpRaw, 32, 15, -1 },
      { OpRaw, 32, 16, -1 },
      { OpRaw, 32, 17, -1 },
      { OpRaw, 32, 18, -1 },
      { OpRaw, 32, 19, -1 },
      { OpRaw, 32, 10, -1 },
   };

   static const S16 sT2EvSel_RemoveClientTargetTypeEvent[] = { 1, 1, 1, 1 };
   static const T2DataNode sT2EvNodes_RemoveClientTargetTypeEvent[] =
   {
      { OpSelect, 2, 0, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_ResetClientTargetsEvent[] =
   {
      { OpFlag, 1, 1, 1 },
      { OpEnd, 0, -1, -1 },
   };

   static const S16 sT2EvSel_SensorGroupColorEvent[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
   static const T2DataNode sT2EvNodes_SensorGroupColorEvent[] =
   {
      { OpSelect, 5, 0, -1 },
      { OpRaw, 32, 2, -1 },
      { OpFlag, 1, 3, 54 },
      { OpFlag, 1, 4, 50 },
      { OpFlag, 1, 5, 46 },
      { OpFlag, 1, 6, 42 },
      { OpFlag, 1, 7, 38 },
      { OpFlag, 1, 8, 34 },
      { OpFlag, 1, 9, 30 },
      { OpFlag, 1, 10, 26 },
      { OpFlag, 1, 11, 22 },
      { OpFlag, 1, 12, 18 },
      { OpFlag, 1, 13, 14 },
      { OpEnd, 0, -1, -1 },
      { OpRaw, 8, 15, -1 },
      { OpRaw, 8, 16, -1 },
      { OpRaw, 8, 17, -1 },
      { OpRaw, 8, 13, -1 },
      { OpRaw, 8, 19, -1 },
      { OpRaw, 8, 20, -1 },
      { OpRaw, 8, 21, -1 },
      { OpRaw, 8, 12, -1 },
      { OpRaw, 8, 23, -1 },
      { OpRaw, 8, 24, -1 },
      { OpRaw, 8, 25, -1 },
      { OpRaw, 8, 11, -1 },
      { OpRaw, 8, 27, -1 },
      { OpRaw, 8, 28, -1 },
      { OpRaw, 8, 29, -1 },
      { OpRaw, 8, 10, -1 },
      { OpRaw, 8, 31, -1 },
      { OpRaw, 8, 32, -1 },
      { OpRaw, 8, 33, -1 },
      { OpRaw, 8, 9, -1 },
      { OpRaw, 8, 35, -1 },
      { OpRaw, 8, 36, -1 },
      { OpRaw, 8, 37, -1 },
      { OpRaw, 8, 8, -1 },
      { OpRaw, 8, 39, -1 },
      { OpRaw, 8, 40, -1 },
      { OpRaw, 8, 41, -1 },
      { OpRaw, 8, 7, -1 },
      { OpRaw, 8, 43, -1 },
      { OpRaw, 8, 44, -1 },
      { OpRaw, 8, 45, -1 },
      { OpRaw, 8, 6, -1 },
      { OpRaw, 8, 47, -1 },
      { OpRaw, 8, 48, -1 },
      { OpRaw, 8, 49, -1 },
      { OpRaw, 8, 5, -1 },
      { OpRaw, 8, 51, -1 },
      { OpRaw, 8, 52, -1 },
      { OpRaw, 8, 53, -1 },
      { OpRaw, 8, 4, -1 },
      { OpRaw, 8, 55, -1 },
      { OpRaw, 8, 56, -1 },
      { OpRaw, 8, 57, -1 },
      { OpRaw, 8, 3, -1 },
   };

   static const T2DataNode sT2EvNodes_SetMissionCRCEvent[] =
   {
      { OpRaw, 32, 1, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const S16 sT2EvSel_SetObjectActiveImageEvent[] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
   static const T2DataNode sT2EvNodes_SetObjectActiveImageEvent[] =
   {
      { OpInt, 10, 1, -1 },
      { OpSelect, 4, 0, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const S16 sT2EvSel_SetSensorGroupEvent[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
   static const T2DataNode sT2EvNodes_SetSensorGroupEvent[] =
   {
      { OpSelect, 5, 0, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const T2DataNode sT2EvNodes_SetServerTargetEvent[] =
   {
      { OpFlag, 1, 1, 5 },
      { OpRaw, 32, 2, -1 },
      { OpRaw, 32, 3, -1 },
      { OpRaw, 32, 4, -1 },
      { OpEnd, 0, -1, -1 },
      { OpInt, 9, 1, -1 },
   };

   static const S16 sT2EvSel_SimVoiceStreamEvent[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 4, 37 };
   static const T2DataNode sT2EvNodes_SimVoiceStreamEvent[] =
   {
      { OpSelect, 5, 0, -1 },
      { OpSelect, 6, 32, -1 },
      { OpSelect, 2, 96, -1 },
      { OpFlag, 1, 4, 6 },
      { OpRaw, 32, 5, -1 },
      { OpEnd, 0, -1, -1 },
      { OpSelect, 5, 100, -1 },
      { OpRaw, 0, 5, -1 },
      { OpRaw, 8, 5, -1 },
      { OpRaw, 16, 5, -1 },
      { OpRaw, 24, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
      { OpRaw, 32, 5, -1 },
   };

   static const T2DataNode sT2EvNodes_TargetFreeEvent[] =
   {
      { OpInt, 9, 1, -1 },
      { OpEnd, 0, -1, -1 },
   };

   static const S16 sT2EvSel_TargetInfoEvent[] = { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 };
   static const T2DataNode sT2EvNodes_TargetInfoEvent[] =
   {
      { OpInt, 9, 1, -1 },
      { OpFlag, 1, 2, 24 },
      { OpFlag, 1, 3, 22 },
      { OpFlag, 1, 4, 20 },
      { OpFlag, 1, 5, 18 },
      { OpFlag, 1, 6, 16 },
      { OpFlag, 1, 7, 15 },
      { OpFlag, 1, 8, 13 },
      { OpFlag, 1, 9, 12 },
      { OpFlag, 1, 10, 11 },
      { OpEnd, 0, -1, -1 },
      { OpFloat, 7, 10, -1 },
      { OpInt, 9, 9, -1 },
      { OpFlag, 1, 8, 14 },
      { OpInt, 11, 8, -1 },
      { OpSelect, 5, 0, -1 },
      { OpFlag, 1, 6, 17 },
      { OpInt, 10, 6, -1 },
      { OpFlag, 1, 5, 19 },
      { OpInt, 10, 5, -1 },
      { OpFlag, 1, 4, 21 },
      { OpInt, 10, 4, -1 },
      { OpFlag, 1, 3, 23 },
      { OpInt, 10, 3, -1 },
      { OpFlag, 1, 2, 25 },
      { OpInt, 10, 2, -1 },
   };

   static const T2DataBlockLayout sT2Events[] =
   {
      { "CRCChallengeEvent", sT2EvNodes_CRCChallengeEvent, 5, NULL, 0 },
      { "CRCChallengeResponseEvent", sT2EvNodes_CRCChallengeResponseEvent, 4, NULL, 0 },
      { "FogChallengeEvent", sT2EvNodes_FogChallengeEvent, 2, NULL, 0 },
      { "GhostingMessageEvent", sT2EvNodes_GhostingMessageEvent, 4, sT2EvSel_GhostingMessageEvent, 0 },
      { "GravityEvent", sT2EvNodes_GravityEvent, 2, NULL, 0 },
      { "PathManagerEvent", sT2EvNodes_PathManagerEvent, 20, NULL, 0 },
      { "RemoveClientTargetTypeEvent", sT2EvNodes_RemoveClientTargetTypeEvent, 2, sT2EvSel_RemoveClientTargetTypeEvent, 0 },
      { "ResetClientTargetsEvent", sT2EvNodes_ResetClientTargetsEvent, 2, NULL, 0 },
      { "SensorGroupColorEvent", sT2EvNodes_SensorGroupColorEvent, 58, sT2EvSel_SensorGroupColorEvent, 0 },
      { "SetMissionCRCEvent", sT2EvNodes_SetMissionCRCEvent, 2, NULL, 0 },
      { "SetObjectActiveImageEvent", sT2EvNodes_SetObjectActiveImageEvent, 3, sT2EvSel_SetObjectActiveImageEvent, 0 },
      { "SetSensorGroupEvent", sT2EvNodes_SetSensorGroupEvent, 2, sT2EvSel_SetSensorGroupEvent, 0 },
      { "SetServerTargetEvent", sT2EvNodes_SetServerTargetEvent, 6, NULL, 0 },
      { "SimVoiceStreamEvent", sT2EvNodes_SimVoiceStreamEvent, 38, sT2EvSel_SimVoiceStreamEvent, 0 },
      { "TargetFreeEvent", sT2EvNodes_TargetFreeEvent, 2, NULL, 0 },
      { "TargetInfoEvent", sT2EvNodes_TargetInfoEvent, 26, sT2EvSel_TargetInfoEvent, 0 },
   };
}

#endif
