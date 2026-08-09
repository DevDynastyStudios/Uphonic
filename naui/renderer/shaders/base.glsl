@stage vertex

in vec2 in_position;
in vec2 in_uv;
in vec4 in_color;
in int in_texture_id;

out vec4 frag_color;
out vec2 frag_uv;
out float frag_texture_id;

layout(binding = 0) uniform ViewData
{
    vec2 u_resolution;
};

void main()
{
    vec2 ndc = (in_position / u_resolution) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    frag_uv = in_uv;
    frag_color = in_color;
    frag_texture_id = float(in_texture_id);
}

@stage fragment

layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform sampler2D texture1;

in vec4 frag_color;
in vec2 frag_uv;
in float frag_texture_id;

out vec4 out_color;

const float sdf_threshold  = 0.4;
const float sharpness_mult = 0.75;

float contour(float d, float w)
{
    return smoothstep(sdf_threshold - w, sdf_threshold + w, d);
}

float samp(sampler2D tex, vec2 uv, float w)
{
    return contour(texture(tex, uv).r, w);
}

void main()
{
    const int texture_id = int(frag_texture_id);
    switch (texture_id)
    {
    case -1: out_color = frag_color; return;
    case  0: out_color = texture(texture0, frag_uv) * frag_color; return;
    case 1:
    {
        vec2 uv = frag_uv;
        float dist = texture(texture1, uv).r;

        float width = sharpness_mult * length(vec2(dFdx(dist), dFdy(dist)));
        width = max(width, 1e-5);

        float alpha = contour(dist, width);

        vec2 duv = 0.354 * (dFdx(uv) + dFdy(uv));
        vec4 box = vec4(uv - duv, uv + duv);

        float asum = samp(texture1, box.xy, width)
                    + samp(texture1, box.zw, width)
                    + samp(texture1, box.xw, width)
                    + samp(texture1, box.zy, width);

        alpha = (alpha + 0.5 * asum) / 3.0;

        out_color = vec4(frag_color.rgb, frag_color.a * alpha);
        return;
    }
    }
}