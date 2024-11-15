#include "r_core.h"

extern RPass_PassData* pass;
extern R_Cvars r_cvars;
extern R_Scene scene;

#include "core/core_common.h"

void RInternal_UpdatePostProcessShader(bool p_recompile)
{
    if (p_recompile)
    {
        if (pass->post.post_process_shader > 0)
        {
            glDeleteProgram(pass->post.post_process_shader);
        }

        const char* DEFINES[4] =
        {
            (r_cvars.r_useFxaa->int_value) ? "USE_FXAA" : "",
            (r_cvars.r_useDepthOfField->int_value) ? "USE_DEPTH_OF_FIELD" : "",
            (r_cvars.r_TonemapMode->int_value == 0) ? "USE_REINHARD_TONEMAP" : ((r_cvars.r_TonemapMode->int_value == 1) ? "USE_UNCHARTED2_TONEMAP" : "USE_ACES_TONEMAP"),
            (r_cvars.r_useBloom->int_value) ? "USE_BLOOM" : ""
        };
        pass->post.post_process_shader = Shader_CompileFromFileDefine("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/post_process.frag", NULL, DEFINES, 4);
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
            (r_cvars.r_useSsao->int_value) ? "USE_SSAO" : "",
        };
        pass->deferred.shading_shader = Shader_CompileFromFileDefine("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/deferred_scene.frag", NULL, DEFINES, 2);

        Core_Printf("Deferred shading shader recompiled!");   
    }

    glUseProgram(pass->deferred.shading_shader);
    Shader_SetInteger(pass->deferred.shading_shader, "gNormalMetal", 0);
    Shader_SetInteger(pass->deferred.shading_shader, "gColorRough", 1);
    Shader_SetInteger(pass->deferred.shading_shader, "depth_texture", 2);
    Shader_SetInteger(pass->deferred.shading_shader, "SSAO_texture", 3);
    Shader_SetInteger(pass->deferred.shading_shader, "shadowMapsDepth", 4);
    Shader_SetInteger(pass->deferred.shading_shader, "brdfLUT", 5);
    Shader_SetInteger(pass->deferred.shading_shader, "irradianceMap", 6);
    Shader_SetInteger(pass->deferred.shading_shader, "preFilterMap", 7);

    Shader_SetInteger(pass->deferred.shading_shader, "u_shadowSampleAmount", pass->shadow.num_shadow_sample_kernels);
    Shader_SetFloat(pass->deferred.shading_shader, "u_shadowQualityRadius", pass->shadow.quality_radius_scale);
    if (pass->shadow.num_shadow_sample_kernels > 0)
    {
        char buf[64];
        for (int i = 0; i < pass->shadow.num_shadow_sample_kernels; i++)
        {
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "u_shadowSampleKernels[%i]", i);
            Shader_SetVector2f(pass->deferred.shading_shader, buf, pass->shadow.shadow_sample_kernels[i][0], pass->shadow.shadow_sample_kernels[i][1]);
        }
    }
}

void RInternal_GetShadowQualityData(int p_qualityLevel, int p_blurLevel, float* r_shadowMapSize, float* r_kernels, int* r_numKernels, float* r_qualityRadius)
{
    if (r_shadowMapSize)
    {
        *r_shadowMapSize = 256.0 + (256.0 * p_qualityLevel);
    }
    float quality_radius = 1.0;
    int num_kernel_samples = 0;
    
    switch (p_blurLevel)
    {
    case 1:
    {   
        num_kernel_samples = 1;
        quality_radius = 1.5;
        break;
    }
    case 2:
    {
        num_kernel_samples = 4;
        quality_radius = 2.0;
        break;
    }
    case 3:
    {
        num_kernel_samples = 8;
        quality_radius = 2.0;
        break;
    }
    case 4:
    {
        num_kernel_samples = 16;
        quality_radius = 3.0;
        break;
    }
    case 5:
    {
        num_kernel_samples = 32;
        quality_radius = 4.0;
        break;
    }
    default:
        break;
    }

    if (r_kernels && r_numKernels)
    {
        const float golden_angle = 2.4;

        for (int i = 0; i < num_kernel_samples; i++)
        {
            float r = sqrt((float)(i)+0.5) / sqrt((float)(num_kernel_samples));
            float theta = (float)(i)*golden_angle;

            r_kernels[i * 2] = cos(theta) * r;
            r_kernels[i * 2 + 1] = (theta)*r;
        }

        *r_numKernels = num_kernel_samples;
    }
    if (r_qualityRadius)
    {
        *r_qualityRadius = quality_radius;
    }
}
