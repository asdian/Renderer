#pragma once

#ifndef META_RENDERER_STUDIOENV_INTERFACE_H
#define META_RENDERER_STUDIOENV_INTERFACE_H

#include <interface.h>

/*
	IMetaRendererStudioEnv - CSO_ENV provider hook.

	CSO_ENV is the "@NNN" static-cubemap reflection layer on studio textures
	(e.g. "$20@004_beamgunle_v01.bmp" -> tag 4). Renderer still tags the
	textures at model load and still draws the effect (shader variant, cube
	texture binding on STUDIO_BIND_TEXTURE_ENVMAP, r_glow_color uniform), but
	the "tag number -> cubemap + brightness" step belongs to a plugin that
	registers an IMetaRendererStudioEnvProvider. Renderer has no built-in
	fallback: with no provider registered, or when the provider declines a
	texture, that mesh simply draws without the env layer.

	ABI NOTE: this is deliberately NOT a method on IMetaRenderer. A method
	appended to IMetaRenderer does not exist on a Renderer.dll built before
	the append, and the caller cannot tell (g_bLegacyRendererAPI only tells
	API_005 from API_002) - calling it walks off the end of the vtable. This
	interface has its own version string instead, so an older Renderer.dll
	simply returns NULL from its factory and the plugin knows the hook is
	unavailable.
*/

#define METARENDERER_STUDIOENV_VERSION "MetaRendererStudioEnv_API_001"

typedef struct studio_env_result_s
{
	// OpenGL name of a complete GL_TEXTURE_CUBE_MAP texture. 0 = no result.
	// The provider owns it; Renderer never deletes it.
	unsigned int cubemap;

	// Same meaning as "mulBright" / "addBright" in shader/@NNN.txt:
	// color = (diffuse + addBright) * cubemapColor * mulBright
	float mulBright;
	float addBright;
}studio_env_result_t;

/*
	Implemented by the PLUGIN.
	Called by Renderer from inside the studio draw pass, once per drawn mesh
	whose texture is tagged CSO_ENV (not in the depth-only shadow pass), so it
	must be cheap (cache by tagIndex).
*/
class IMetaRendererStudioEnvProvider : public IBaseInterface
{
public:
	/*
		tagIndex    : the NNN of "@NNN" in the texture name.
		textureName : the full texture name as stored in the .mdl.
		out         : filled in on success.

		Return false (or leave out->cubemap 0) to draw this mesh without the
		env layer. Renderer asks before it picks the shader program, so a
		declined texture never compiles or samples the CSO_ENV variant.

		May be called while the studio program is bound: if you create or
		upload GL textures here, leave GL_TEXTURE_CUBE_MAP bound to 0 on the
		active unit when you are done, as Renderer's own loader does.
	*/
	virtual bool ResolveStudioEnv(int tagIndex, const char* textureName, studio_env_result_t* out) = 0;
};

/*
	Implemented by RENDERER, obtained from Renderer's factory with
	METARENDERER_STUDIOENV_VERSION. At most one provider is active; a later
	RegisterProvider() replaces the earlier one. Unregister before the
	provider's DLL unloads.
*/
class IMetaRendererStudioEnv : public IBaseInterface
{
public:
	virtual void RegisterProvider(IMetaRendererStudioEnvProvider* pProvider) = 0;
	virtual void UnregisterProvider(IMetaRendererStudioEnvProvider* pProvider) = 0;
};

#endif
