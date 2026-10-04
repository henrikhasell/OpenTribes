// HH: Added for Tribes 2 compatibility.
#ifndef _RENDERBOOTSTRAP_H_
#define _RENDERBOOTSTRAP_H_

/// What core/scripts would have declared.
///
/// Torque 3D keeps a layer of named declarations in script -- ShaderData,
/// CustomMaterial, GFXStateBlockData, Material -- and the engine reaches them
/// by literal name from C++. The port ships none of core/scripts, so in every
/// one of those places the engine asks for a name that nothing has declared.
/// The shader and texture *files* are staged; it is only the declarations that
/// are absent.
///
/// This is deliberately one place rather than a fix per symptom. Three of
/// these were found separately, each by something failing silently in its own
/// way -- a mesh that submitted nothing, a terrain that drew black, a water
/// bin handed an instance it could not draw -- and a bootstrap that covered
/// only the three would be found wanting by the fourth. The names the engine
/// cannot run without are enumerable: they are the string literals it passes
/// to Sim::findObject, createMatInstance and getMaterialDefinitionByName, and
/// the list is in the .cpp beside what each one costs when it is missing.
///
/// Called from SceneManager::_createDefaultRenderPass(), which is the gate
/// every draw passes through and which cannot be reached without -- the same
/// argument that put the render pass itself there. A declaration needed
/// outside drawing does not belong here: MATMGR::getWarningMaterial() builds
/// the fallback material at its own point of need, because a material can be
/// asked for while a resource loads and long before anything draws.
namespace RenderBootstrap
{
   /// Declare whatever of the below is not already declared. Safe to call more
   /// than once; it declares nothing that exists.
   void declareMissing();
}

#endif // _RENDERBOOTSTRAP_H_
