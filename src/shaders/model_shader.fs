#version 460 core

out vec4 f_color;

struct VecOutput
{
   vec3 v_FragPos;
   vec3 v_Normal;
   vec2 v_texCoords;
   vec3 v_tangentLightPos;
   vec3 v_tangentViewPos;
   vec3 v_tangentFragPos;
};

layout (location = 0) in VecOutput v_Output;

uniform vec3 u_viewPos;

uniform sampler2D texture_normal1;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_height1;
uniform sampler2D texture_specular1;
uniform samplerCube skybox_texture;

uniform float u_heightScale;


vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * u_heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(texture_height1, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(texture_height1, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texture_height1, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main()
{
    vec4 tex_color = texture(texture_diffuse1, v_Output.v_texCoords);

    vec3 I = normalize(v_Output.v_FragPos - u_viewPos);
    vec3 R = reflect(I, normalize(v_Output.v_Normal));
    vec3 refracte = refract(I, normalize(v_Output.v_Normal), 1.0/1.33);
    vec4 reflected_color = texture(skybox_texture, R);
    vec4 refraced_color = texture(skybox_texture, refracte);
    vec4 enviroment_color = mix(reflected_color, refraced_color, 0.5);

    
       // obtain normal from normal map in range [0,1]
    vec3 normal = texture(texture_normal1, v_Output.v_texCoords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
   
    // get diffuse color
    vec3 color = tex_color.rgb;
    // ambient
    vec3 ambient = 0.5 * color;
    // diffuse
    vec3 lightDir = normalize(v_Output.v_tangentLightPos - v_Output.v_tangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(v_Output.v_tangentFragPos - v_Output.v_tangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular_tex = texture(texture_specular1, v_Output.v_texCoords).rgb;

    vec3 specular = vec3(1) * spec * specular_tex;
    vec4 light_color = vec4(ambient + diffuse + specular, 1.0);

   // f_color = mix(tex_color, enviroment_color, 0.1);
    f_color = mix(f_color, light_color, 1.0);
}