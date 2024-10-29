#include "r_core.h"

extern R_RenderPassData* pass;
extern R_Cvars r_cvars;
extern R_Scene scene;

#include "core/c_common.h"

void RInternal_UpdatePostProcessShader(bool p_recompile)
{
    if (p_recompile)
    {
        if (pass->post.post_process_shader > 0)
        {
            glDeleteProgram(pass->post.post_process_shader);
        }

        const char* DEFINES[2] =
        {
            (r_cvars.r_useFxaa->int_value) ? "USE_FXAA" : "",
            (r_cvars.r_useDepthOfField->int_value) ? "USE_DEPTH_OF_FIELD" : "",

        };
        pass->post.post_process_shader = Shader_CompileFromFileDefine("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/post_process.frag", NULL, DEFINES, 2);
        Core_Printf("Post process shader recompiled!");
    }

    glUseProgram(pass->post.post_process_shader);
    Shader_SetFloat(pass->post.post_process_shader, "u_Exposure", r_cvars.r_Exposure->float_value);
    Shader_SetFloat(pass->post.post_process_shader, "u_Gamma", r_cvars.r_Gamma->float_value);
    Shader_SetFloat(pass->post.post_process_shader, "u_Brightness", r_cvars.r_Brightness->float_value);
    Shader_SetFloat(pass->post.post_process_shader, "u_Contrast", r_cvars.r_Contrast->float_value);
    Shader_SetFloat(pass->post.post_process_shader, "u_Saturation", r_cvars.r_Saturation->float_value);
    Shader_SetFloat(pass->post.post_process_shader, "u_BloomStrength", r_cvars.r_bloomStrength->float_value);

    //SETUP SHADER UNIFORMS
    Shader_SetInteger(pass->post.post_process_shader, "depth_texture", 0);
    Shader_SetInteger(pass->post.post_process_shader, "MainSceneTexture", 1);
    Shader_SetInteger(pass->post.post_process_shader, "BloomSceneTexture", 2);
    Shader_SetInteger(pass->post.post_process_shader, "DofSceneTexture", 3);
}

void RInternal_UpdateDeferredShadingShader(bool p_recompile)
{
    if (p_recompile)
    {
        if (pass->deferred.shading_shader > 0)
        {
            glDeleteProgram(pass->deferred.shading_shader);
        }

        const char* DEFINES[2] =
        {
            (r_cvars.r_useDirShadowMapping->int_value) ? "USE_DIR_SHADOWS" : "",
            (r_cvars.r_useSsao->int_value) ? "USE_SSAO" : ""
        };
        pass->deferred.shading_shader = Shader_CompileFromFileDefine("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/deferred_scene.frag", NULL, DEFINES, 2);

        Core_Printf("Deferred shading shader recompiled!");   
    }

    glUseProgram(pass->deferred.shading_shader);
    Shader_SetInteger(pass->deferred.shading_shader, "gNormalMetal", 0);
    Shader_SetInteger(pass->deferred.shading_shader, "gColorRough", 1);
    Shader_SetInteger(pass->deferred.shading_shader, "depth_texture", 2);
    Shader_SetInteger(pass->deferred.shading_shader, "SSAO_texture", 3);
    Shader_SetInteger(pass->deferred.shading_shader, "momentMaps", 4);
    Shader_SetInteger(pass->deferred.shading_shader, "shadowMapsDepth", 5);
    Shader_SetInteger(pass->deferred.shading_shader, "brdfLUT", 6);
    Shader_SetInteger(pass->deferred.shading_shader, "irradianceMap", 7);
    Shader_SetInteger(pass->deferred.shading_shader, "preFilterMap", 8);
}