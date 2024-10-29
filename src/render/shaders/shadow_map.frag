#version 460 core

layout(location = 0) out float FragColor;

void main()
{             
    float depth = gl_FragCoord.z;

    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);

    FragColor = moment2;
}