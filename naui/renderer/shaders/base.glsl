@name base
@stage vertex

in vec2 in_position;
in vec2 in_uv;
in vec4 in_color;
in int in_texture_id;
in uvec2 in_clip;

out vec4 frag_color;
out vec2 frag_uv;
out float frag_texture_id;
out vec2 frag_position;
flat out vec4 frag_clip;

layout(binding = 0) uniform ViewData
{
    vec2 u_resolution;
};

void main()
{
    vec2 ndc = (in_position / u_resolution) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    frag_uv = in_uv;
    frag_color = in_color;
    frag_texture_id = float(in_texture_id);
    frag_position = in_position;
    frag_clip = vec4(uvec4(in_clip.x & 0xFFFFu, in_clip.x >> 16, in_clip.y & 0xFFFFu, in_clip.y >> 16));
}

@stage fragment

layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform sampler2D texture1;

in vec4 frag_color;
in vec2 frag_uv;
in float frag_texture_id;
in vec2 frag_position;
flat in vec4 frag_clip;

out vec4 out_color;

void main()
{
    if (any(bvec4(lessThan(frag_position, frag_clip.xy), greaterThanEqual(frag_position, frag_clip.zw))))
        discard;

    const int texture_id = int(frag_texture_id);
    switch (texture_id)
    {
    case -1: out_color = frag_color; return;
    case  0: out_color = texture(texture0, frag_uv) * frag_color; return;
    case 1:
    {
        float coverage = texture(texture1, frag_uv).r;
        float luminance = dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722));
        coverage = pow(coverage, mix(1.15, 0.6, luminance));
        out_color = vec4(frag_color.rgb, frag_color.a * coverage);
        return;
    }
    }
}
