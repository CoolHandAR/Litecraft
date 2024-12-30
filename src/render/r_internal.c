#include "r_core.h"

#include "core/core_common.h"

extern RPass_PassData* pass;
extern R_Cvars r_cvars;
extern R_Scene scene;

extern void Render_Cube();


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

void RInternal_ProcessIBLCubemap(bool p_fast, bool p_irradiance, bool p_prefilter, bool p_brdf)
{
    float irradiance_sample_delta = (p_fast) ? (0.025 * 24.0) : (0.025);
    int prefilter_brdf_sample_count = (p_fast) ? (12) : (1024);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
    if (p_irradiance)
    {
        //CONVULUTE CUBEMAP
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
        glUseProgram(pass->ibl.irradiance_convulution_shader);
        Shader_SetInteger(pass->ibl.irradiance_convulution_shader, "environmentMap", 0);
        Shader_SetMatrix4(pass->ibl.irradiance_convulution_shader, "u_proj", pass->ibl.cube_proj);
        Shader_SetFloat(pass->ibl.irradiance_convulution_shader, "u_sampleDelta", irradiance_sample_delta);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

        glViewport(0, 0, 32, 32);
        for (int i = 0; i < 6; i++)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.irradianceCubemapTexture, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            Shader_SetMatrix4(pass->ibl.irradiance_convulution_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
            Render_Cube();
        }
    }
    if (p_prefilter)
    {
        //PREFILTER CUBEMAP
        glUseProgram(pass->ibl.prefilter_cubemap_shader);
        Shader_SetInteger(pass->ibl.prefilter_cubemap_shader, "environmentMap", 0);
        Shader_SetMatrix4(pass->ibl.prefilter_cubemap_shader, "u_proj", pass->ibl.cube_proj);
        Shader_SetInteger(pass->ibl.prefilter_cubemap_shader, "u_sampleCount", prefilter_brdf_sample_count);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            unsigned int mipWidth = (unsigned)(128 * pow(0.5, mip));
            unsigned int mipHeight = (unsigned)(128 * pow(0.5, mip));

            glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            Shader_SetFloat(pass->ibl.prefilter_cubemap_shader, "u_roughness", roughness);

            for (unsigned int i = 0; i < 6; ++i)
            {
                Shader_SetMatrix4(pass->ibl.prefilter_cubemap_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.prefilteredCubemapTexture, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                Render_Cube();
            }
        }
    }
    if (p_brdf)
    {
        //BRDF GENERATION
        glUseProgram(pass->ibl.brdf_shader);
        Shader_SetInteger(pass->ibl.brdf_shader, "u_sampleCount", prefilter_brdf_sample_count);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ibl.brdfLutTexture, 0);
        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_BLEND);
        Render_Quad();
    }
}