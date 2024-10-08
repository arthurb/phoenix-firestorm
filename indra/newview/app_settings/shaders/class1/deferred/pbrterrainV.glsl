/**
 * @file class1\environment\pbrterrainV.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#define TERRAIN_PBR_DETAIL_EMISSIVE 0
#define TERRAIN_PBR_DETAIL_OCCLUSION -1
#define TERRAIN_PBR_DETAIL_NORMAL -2
#define TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS -3

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

in vec3 position;
in vec3 normal;
in vec4 tangent;
in vec4 diffuse_color;
in vec2 texcoord1;

out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
out vec3 vary_normal;
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
out vec3 vary_tangents[4];
flat out float vary_sign;
#endif
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
out vec4[10] vary_coords;
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
out vec4[2] vary_coords;
#endif
out vec3 vary_position;

// *HACK: Each material uses only one texture transform, but the KHR texture
// transform spec allows handling texture transforms separately for each
// individual texture info.
uniform vec4[5] terrain_texture_transforms;

vec2 terrain_texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform);
vec3 terrain_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform);

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;

    vec3 n = normal_matrix * normal;
    vary_vertex_normal = normal;
    vec3 t = normal_matrix * tangent.xyz;

#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
    {
        vec4[2] ttt;
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_tangents[0] = normalize(terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt));
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_tangents[1] = normalize(terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt));
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_tangents[2] = normalize(terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt));
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_tangents[3] = normalize(terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt));
    }

    vary_sign = tangent.w;
#endif
    vary_normal = normalize(n);

    // Transform and pass tex coords
    {
        vec4[2] ttt;
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Don't care about upside-down (transform_xy_flipped())
#define transform_xy()             terrain_texture_transform(position.xy,               ttt)
#define transform_yz()             terrain_texture_transform(position.yz,               ttt)
#define transform_negx_z()         terrain_texture_transform(position.xz * vec2(-1, 1), ttt)
#define transform_yz_flipped()     terrain_texture_transform(position.yz * vec2(-1, 1), ttt)
#define transform_negx_z_flipped() terrain_texture_transform(position.xz,               ttt)
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = transform_xy();
        vary_coords[0].zw = transform_yz();
        vary_coords[1].xy = transform_negx_z();
        vary_coords[1].zw = transform_yz_flipped();
        vary_coords[2].xy = transform_negx_z_flipped();
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[2].zw = transform_xy();
        vary_coords[3].xy = transform_yz();
        vary_coords[3].zw = transform_negx_z();
        vary_coords[4].xy = transform_yz_flipped();
        vary_coords[4].zw = transform_negx_z_flipped();
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[5].xy = transform_xy();
        vary_coords[5].zw = transform_yz();
        vary_coords[6].xy = transform_negx_z();
        vary_coords[6].zw = transform_yz_flipped();
        vary_coords[7].xy = transform_negx_z_flipped();
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[7].zw = transform_xy();
        vary_coords[8].xy = transform_yz();
        vary_coords[8].zw = transform_negx_z();
        vary_coords[9].xy = transform_yz_flipped();
        vary_coords[9].zw = transform_negx_z_flipped();
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = terrain_texture_transform(position.xy, ttt);
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[0].zw = terrain_texture_transform(position.xy, ttt);
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[1].xy = terrain_texture_transform(position.xy, ttt);
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[1].zw = terrain_texture_transform(position.xy, ttt);
#endif
    }

    vec4 tc = vec4(texcoord1,0,1);
    vary_texcoord0.zw = tc.xy;
    vary_texcoord1.xy = tc.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = tc.xy-vec2(1.0, 0.0);
}
