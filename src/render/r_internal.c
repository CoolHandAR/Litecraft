#include "render/r_core.h"

#include "core/core_common.h"
#include "shaders/shader_info.h"

extern RPass_PassData* pass;
extern R_Cvars r_cvars;
extern R_Scene scene;

extern void Render_Cube();
extern void Render_Quad();

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
    float irradiance_sample_delta = (p_fast) ? (0.025 * 12.0) : (0.025);
    int prefilter_brdf_sample_count = (p_fast) ? (24) : (1024);

    if (p_irradiance)
    {
        //CONVULUTE CUBEMAP
        Shader_ResetDefines(&pass->ibl.cubemap_shader);
        Shader_SetDefine(&pass->ibl.cubemap_shader, CUBEMAP_DEFINE_IRRADIANCE_PASS, true);

        Shader_Use(&pass->ibl.cubemap_shader);

        Shader_SetMat4(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_PROJ, pass->ibl.cube_proj);
        Shader_SetFloaty(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_IRRADIANCESAMPLEDELTA, irradiance_sample_delta);

        glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.irr_FBO);
        glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.irr_RBO);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

        glViewport(0, 0, pass->ibl.irr_size, pass->ibl.irr_size);
        for (int i = 0; i < 6; i++)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.irradianceCubemapTexture, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            Shader_SetMat4(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_VIEW, pass->ibl.cube_view_matrixes[i]);
            Render_Cube();
        }
    }
    if (p_prefilter)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.filter_FBO);

        Shader_ResetDefines(&pass->ibl.cubemap_shader);
        Shader_SetDefine(&pass->ibl.cubemap_shader, CUBEMAP_DEFINE_PREFILTER_PASS, true);

        Shader_Use(&pass->ibl.cubemap_shader);

        Shader_SetMat4(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_PROJ, pass->ibl.cube_proj);
        Shader_SetInt(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_PREFILTERSAMPLECOUNT, prefilter_brdf_sample_count);

        //PREFILTER 
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            unsigned int mipWidth = (unsigned)(pass->ibl.filter_size * pow(0.5, mip));
            unsigned int mipHeight = (unsigned)(pass->ibl.filter_size * pow(0.5, mip));

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->ibl.filter_depth_mipmaps[mip], 0);

            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);

            Shader_SetFloaty(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_PREFILTERROUGHNESS, roughness);

            for (unsigned int i = 0; i < 6; ++i)
            {
                Shader_SetMat4(&pass->ibl.cubemap_shader, CUBEMAP_UNIFORM_VIEW, pass->ibl.cube_view_matrixes[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.prefilteredCubemapTexture, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                Render_Cube();
            }
        }
    }
    if (p_brdf)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);

        //BRDF GENERATION
        Shader_Use(&pass->ibl.brdf_shader);

        Shader_SetInt(&pass->ibl.brdf_shader, BRDF_UNIFORM_SAMPLECOUNT, prefilter_brdf_sample_count);

        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_BLEND);
        Render_Quad();
    }
}