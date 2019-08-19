/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QSSGOPENGLTOKENS_H
#define QSSGOPENGLTOKENS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRender/private/qtquick3drenderglobal_p.h>
#include <qopengl.h>

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif

#ifndef GL_IMAGE_2D
#define GL_IMAGE_2D 0x904D
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#endif

#ifndef GL_PATCHES
#define GL_PATCHES 0x000E
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif

#ifndef GL_VERTEX_SHADER_BIT
#define GL_VERTEX_SHADER_BIT 0x00000001
#endif

#ifndef GL_FRAGMENT_SHADER_BIT
#define GL_FRAGMENT_SHADER_BIT 0x00000002
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#define GL_GEOMETRY_SHADER_BIT 0x00000004
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#endif

#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER                 0x8A11
#endif

#ifndef GL_ANY_SAMPLES_PASSED
#define GL_ANY_SAMPLES_PASSED             0x8C2F
#endif

#ifndef GL_QUERY_RESULT_AVAILABLE
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#endif

#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT                   0x8866
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE     0x9117
#endif

#ifndef GL_R8
#define GL_R8                             0x8229
#endif

#ifndef GL_RG8
#define GL_RG8                            0x822B
#endif

#ifndef GL_RG
#define GL_RG                             0x8227
#endif

#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif

#ifndef GL_RGB8
#define GL_RGB8                           0x8051
#endif

#ifndef GL_R16F
#define GL_R16F                           0x822D
#endif

#ifndef GL_R32F
#define GL_R32F                           0x822E
#endif

#ifndef GL_RGBA16F
#define GL_RGBA16F                        0x881A
#endif

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT                     0x140B
#endif

#ifndef GL_RED_INTEGER
#define GL_RED_INTEGER                    0x8D94
#endif

#ifndef GL_R32UI
#define GL_R32UI                          0x8236
#endif

#ifndef GL_BGRA
#define GL_BGRA                           0x80E1
#endif

#ifndef GL_R16
#define GL_R16                            0x822A
#endif

#ifndef GL_RED
#define GL_RED                            0x1903
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F                        0x8814
#endif

#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16              0x81A5
#endif

#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24              0x81A6
#endif

#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F             0x8CAC
#endif

#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX                  0x1901
#endif

#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8                 0x8D48
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif

#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#endif

#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif

#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER               0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#endif

#ifndef GL_MAX_DRAW_BUFFERS
#define GL_MAX_DRAW_BUFFERS               0x8824
#endif

#ifndef GL_TEXTURE_COMPARE_MODE
#define GL_TEXTURE_COMPARE_MODE           0x884C
#endif

#ifndef GL_COMPARE_REF_TO_TEXTURE
#define GL_COMPARE_REF_TO_TEXTURE         0x884E
#endif

#ifndef GL_TEXTURE_COMPARE_FUNC
#define GL_TEXTURE_COMPARE_FUNC           0x884D
#endif

#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES                    0x8D57
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY                      0x88B8
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY                     0x88B9
#endif

#ifndef GL_READ_WRITE
#define GL_READ_WRITE                     0x88BA
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER                 0x91B9
#endif

#ifndef GL_VERTEX_PROGRAM_POINT_SIZE
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#endif

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE                   0x8861
#endif

#ifndef GL_RG16F
#define GL_RG16F                          0x822F
#endif

#ifndef GL_RG32F
#define GL_RG32F                          0x8230
#endif

#ifndef GL_RGB32F
#define GL_RGB32F                         0x8815
#endif

#ifndef GL_R11F_G11F_B10F
#define GL_R11F_G11F_B10F                 0x8C3A
#endif

#ifndef GL_UNSIGNED_INT_10F_11F_11F_REV
#define GL_UNSIGNED_INT_10F_11F_11F_REV   0x8C3B
#endif

#ifndef GL_RGB9_E5
#define GL_RGB9_E5                        0x8C3D
#endif

#ifndef GL_UNSIGNED_INT_5_9_9_9_REV
#define GL_UNSIGNED_INT_5_9_9_9_REV       0x8C3E
#endif

#ifndef GL_SRGB8
#define GL_SRGB8                          0x8C41
#endif

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8                   0x8C43
#endif

#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8              0x84FA
#endif

#ifndef GL_R32I
#define GL_R32I                           0x8235
#endif

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#endif

#ifndef GL_MAP_INVALIDATE_RANGE_BIT
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#endif

#ifndef GL_UNSIGNED_INT_VEC2
#define GL_UNSIGNED_INT_VEC2              0x8DC6
#endif

#ifndef GL_UNSIGNED_INT_VEC3
#define GL_UNSIGNED_INT_VEC3              0x8DC7
#endif

#ifndef GL_UNSIGNED_INT_VEC4
#define GL_UNSIGNED_INT_VEC4              0x8DC8
#endif

#ifndef GL_INT_SAMPLER_2D_ARRAY
#define GL_INT_SAMPLER_2D_ARRAY           0x8DCF
#endif

#ifndef GL_SAMPLER_2D_SHADOW
#define GL_SAMPLER_2D_SHADOW              0x8B62
#endif

#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL             0x813C
#endif

#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL              0x813D
#endif

#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R              0x8E42
#endif

#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G              0x8E43
#endif

#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B              0x8E44
#endif

#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A              0x8E45
#endif

#ifndef GL_TEXTURE_MIN_LOD
#define GL_TEXTURE_MIN_LOD                0x813A
#endif

#ifndef GL_TEXTURE_MAX_LOD
#define GL_TEXTURE_MAX_LOD                0x813B
#endif

#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R                 0x8072
#endif

#ifndef GL_SAMPLER_2D_ARRAY
#define GL_SAMPLER_2D_ARRAY               0x8DC1
#endif

#ifndef GL_INVALID_INDEX
#define GL_INVALID_INDEX                  0xFFFFFFFFu
#endif

#ifndef GL_UNIFORM_BLOCK_DATA_SIZE
#define GL_UNIFORM_BLOCK_DATA_SIZE        0x8A40
#endif

#ifndef GL_UNIFORM_BLOCK_NAME_LENGTH
#define GL_UNIFORM_BLOCK_NAME_LENGTH      0x8A41
#endif

#ifndef GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS  0x8A42
#endif

#ifndef GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#endif

#ifndef GL_UNIFORM_TYPE
#define GL_UNIFORM_TYPE                   0x8A37
#endif

#ifndef GL_UNIFORM_SIZE
#define GL_UNIFORM_SIZE                   0x8A38
#endif

#ifndef GL_UNIFORM_NAME_LENGTH
#define GL_UNIFORM_NAME_LENGTH            0x8A39
#endif

#ifndef GL_UNIFORM_BLOCK_INDEX
#define GL_UNIFORM_BLOCK_INDEX            0x8A3A
#endif

#ifndef GL_UNIFORM_OFFSET
#define GL_UNIFORM_OFFSET                 0x8A3B
#endif

#ifndef GL_ACTIVE_UNIFORM_BLOCKS
#define GL_ACTIVE_UNIFORM_BLOCKS          0x8A36
#endif

#ifndef GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#endif

#ifndef GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#endif

#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS                 0x821D
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED                0xFFFFFFFFFFFFFFFFull
#endif

#ifndef GL_MAX_ARRAY_TEXTURE_LAYERS
#define GL_MAX_ARRAY_TEXTURE_LAYERS       0x88FF
#endif

#ifndef GL_MAX_UNIFORM_BUFFER_BINDINGS
#define GL_MAX_UNIFORM_BUFFER_BINDINGS    0x8A2F
#endif

#ifndef GL_MAX_UNIFORM_BLOCK_SIZE
#define GL_MAX_UNIFORM_BLOCK_SIZE         0x8A30
#endif

#ifndef GL_NVIDIA_PLATFORM_BINARY_NV
#define GL_NVIDIA_PLATFORM_BINARY_NV 0x890B
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#endif

#ifndef GL_NV_path_rendering
#define GL_NV_path_rendering 1
#define GL_PATH_FORMAT_SVG_NV 0x9070
#define GL_PATH_FORMAT_PS_NV 0x9071
#define GL_STANDARD_FONT_NAME_NV 0x9072
#define GL_SYSTEM_FONT_NAME_NV 0x9073
#define GL_FILE_NAME_NV 0x9074
#define GL_PATH_STROKE_WIDTH_NV 0x9075
#define GL_PATH_END_CAPS_NV 0x9076
#define GL_PATH_INITIAL_END_CAP_NV 0x9077
#define GL_PATH_TERMINAL_END_CAP_NV 0x9078
#define GL_PATH_JOIN_STYLE_NV 0x9079
#define GL_PATH_MITER_LIMIT_NV 0x907A
#define GL_PATH_DASH_CAPS_NV 0x907B
#define GL_PATH_INITIAL_DASH_CAP_NV 0x907C
#define GL_PATH_TERMINAL_DASH_CAP_NV 0x907D
#define GL_PATH_DASH_OFFSET_NV 0x907E
#define GL_PATH_CLIENT_LENGTH_NV 0x907F
#define GL_PATH_FILL_MODE_NV 0x9080
#define GL_PATH_FILL_MASK_NV 0x9081
#define GL_PATH_FILL_COVER_MODE_NV 0x9082
#define GL_PATH_STROKE_COVER_MODE_NV 0x9083
#define GL_PATH_STROKE_MASK_NV 0x9084
#define GL_COUNT_UP_NV 0x9088
#define GL_COUNT_DOWN_NV 0x9089
#define GL_PATH_OBJECT_BOUNDING_BOX_NV 0x908A
#define GL_CONVEX_HULL_NV 0x908B
#define GL_BOUNDING_BOX_NV 0x908D
#define GL_TRANSLATE_X_NV 0x908E
#define GL_TRANSLATE_Y_NV 0x908F
#define GL_TRANSLATE_2D_NV 0x9090
#define GL_TRANSLATE_3D_NV 0x9091
#define GL_AFFINE_2D_NV 0x9092
#define GL_AFFINE_3D_NV 0x9094
#define GL_TRANSPOSE_AFFINE_2D_NV 0x9096
#define GL_TRANSPOSE_AFFINE_3D_NV 0x9098
#define GL_UTF8_NV 0x909A
#define GL_UTF16_NV 0x909B
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV 0x909C
#define GL_PATH_COMMAND_COUNT_NV 0x909D
#define GL_PATH_COORD_COUNT_NV 0x909E
#define GL_PATH_DASH_ARRAY_COUNT_NV 0x909F
#define GL_PATH_COMPUTED_LENGTH_NV 0x90A0
#define GL_PATH_FILL_BOUNDING_BOX_NV 0x90A1
#define GL_PATH_STROKE_BOUNDING_BOX_NV 0x90A2
#define GL_SQUARE_NV 0x90A3
#define GL_ROUND_NV 0x90A4
#define GL_TRIANGULAR_NV 0x90A5
#define GL_BEVEL_NV 0x90A6
#define GL_MITER_REVERT_NV 0x90A7
#define GL_MITER_TRUNCATE_NV 0x90A8
#define GL_SKIP_MISSING_GLYPH_NV 0x90A9
#define GL_USE_MISSING_GLYPH_NV 0x90AA
#define GL_PATH_ERROR_POSITION_NV 0x90AB
#define GL_PATH_FOG_GEN_MODE_NV 0x90AC
#define GL_ACCUM_ADJACENT_PAIRS_NV 0x90AD
#define GL_ADJACENT_PAIRS_NV 0x90AE
#define GL_FIRST_TO_REST_NV 0x90AF
#define GL_PATH_GEN_MODE_NV 0x90B0
#define GL_PATH_GEN_COEFF_NV 0x90B1
#define GL_PATH_GEN_COLOR_FORMAT_NV 0x90B2
#define GL_PATH_GEN_COMPONENTS_NV 0x90B3
#define GL_PATH_STENCIL_FUNC_NV 0x90B7
#define GL_PATH_STENCIL_REF_NV 0x90B8
#define GL_PATH_STENCIL_VALUE_MASK_NV 0x90B9
#define GL_PATH_STENCIL_DEPTH_OFFSET_FACTOR_NV 0x90BD
#define GL_PATH_STENCIL_DEPTH_OFFSET_UNITS_NV 0x90BE
#define GL_PATH_COVER_DEPTH_FUNC_NV 0x90BF
#define GL_PATH_DASH_OFFSET_RESET_NV 0x90B4
#define GL_MOVE_TO_RESETS_NV 0x90B5
#define GL_MOVE_TO_CONTINUES_NV 0x90B6
#define GL_CLOSE_PATH_NV 0x00
#define GL_MOVE_TO_NV 0x02
#define GL_RELATIVE_MOVE_TO_NV 0x03
#define GL_LINE_TO_NV 0x04
#define GL_RELATIVE_LINE_TO_NV 0x05
#define GL_HORIZONTAL_LINE_TO_NV 0x06
#define GL_RELATIVE_HORIZONTAL_LINE_TO_NV 0x07
#define GL_VERTICAL_LINE_TO_NV 0x08
#define GL_RELATIVE_VERTICAL_LINE_TO_NV 0x09
#define GL_QUADRATIC_CURVE_TO_NV 0x0A
#define GL_RELATIVE_QUADRATIC_CURVE_TO_NV 0x0B
#define GL_CUBIC_CURVE_TO_NV 0x0C
#define GL_RELATIVE_CUBIC_CURVE_TO_NV 0x0D
#define GL_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0E
#define GL_RELATIVE_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0F
#define GL_SMOOTH_CUBIC_CURVE_TO_NV 0x10
#define GL_RELATIVE_SMOOTH_CUBIC_CURVE_TO_NV 0x11
#define GL_SMALL_CCW_ARC_TO_NV 0x12
#define GL_RELATIVE_SMALL_CCW_ARC_TO_NV 0x13
#define GL_SMALL_CW_ARC_TO_NV 0x14
#define GL_RELATIVE_SMALL_CW_ARC_TO_NV 0x15
#define GL_LARGE_CCW_ARC_TO_NV 0x16
#define GL_RELATIVE_LARGE_CCW_ARC_TO_NV 0x17
#define GL_LARGE_CW_ARC_TO_NV 0x18
#define GL_RELATIVE_LARGE_CW_ARC_TO_NV 0x19
#define GL_RESTART_PATH_NV 0xF0
#define GL_DUP_FIRST_CUBIC_CURVE_TO_NV 0xF2
#define GL_DUP_LAST_CUBIC_CURVE_TO_NV 0xF4
#define GL_RECT_NV 0xF6
#define GL_CIRCULAR_CCW_ARC_TO_NV 0xF8
#define GL_CIRCULAR_CW_ARC_TO_NV 0xFA
#define GL_CIRCULAR_TANGENT_ARC_TO_NV 0xFC
#define GL_ARC_TO_NV 0xFE
#define GL_RELATIVE_ARC_TO_NV 0xFF
#define GL_BOLD_BIT_NV 0x01
#define GL_ITALIC_BIT_NV 0x02
#define GL_GLYPH_WIDTH_BIT_NV 0x01
#define GL_GLYPH_HEIGHT_BIT_NV 0x02
#define GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV 0x04
#define GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV 0x08
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV 0x10
#define GL_GLYPH_VERTICAL_BEARING_X_BIT_NV 0x20
#define GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV 0x40
#define GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV 0x80
#define GL_GLYPH_HAS_KERNING_BIT_NV 0x100
#define GL_FONT_X_MIN_BOUNDS_BIT_NV 0x00010000
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV 0x00020000
#define GL_FONT_X_MAX_BOUNDS_BIT_NV 0x00040000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV 0x00080000
#define GL_FONT_UNITS_PER_EM_BIT_NV 0x00100000
#define GL_FONT_ASCENDER_BIT_NV 0x00200000
#define GL_FONT_DESCENDER_BIT_NV 0x00400000
#define GL_FONT_HEIGHT_BIT_NV 0x00800000
#define GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV 0x01000000
#define GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV 0x02000000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV 0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV 0x08000000
#define GL_FONT_HAS_KERNING_BIT_NV 0x10000000
#define GL_PRIMARY_COLOR_NV 0x852C
#define GL_SECONDARY_COLOR_NV 0x852D
#define GL_ROUNDED_RECT_NV 0xE8
#define GL_RELATIVE_ROUNDED_RECT_NV 0xE9
#define GL_ROUNDED_RECT2_NV 0xEA
#define GL_RELATIVE_ROUNDED_RECT2_NV 0xEB
#define GL_ROUNDED_RECT4_NV 0xEC
#define GL_RELATIVE_ROUNDED_RECT4_NV 0xED
#define GL_ROUNDED_RECT8_NV 0xEE
#define GL_RELATIVE_ROUNDED_RECT8_NV 0xEF
#define GL_RELATIVE_RECT_NV 0xF7
#define GL_FONT_GLYPHS_AVAILABLE_NV 0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV 0x9369
#define GL_FONT_UNAVAILABLE_NV 0x936A
#define GL_FONT_UNINTELLIGIBLE_NV 0x936B
#define GL_CONIC_CURVE_TO_NV 0x1A
#define GL_RELATIVE_CONIC_CURVE_TO_NV 0x1B
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV 0x20000000
#define GL_STANDARD_FONT_FORMAT_NV 0x936C
#define GL_2_BYTES_NV 0x1407
#define GL_3_BYTES_NV 0x1408
#define GL_4_BYTES_NV 0x1409
#define GL_EYE_LINEAR_NV 0x2400
#define GL_OBJECT_LINEAR_NV 0x2401
#define GL_CONSTANT_NV 0x8576
#define GL_PATH_PROJECTION_NV 0x1701
#define GL_PATH_MODELVIEW_NV 0x1700
#define GL_PATH_MODELVIEW_STACK_DEPTH_NV 0x0BA3
#define GL_PATH_MODELVIEW_MATRIX_NV 0x0BA6
#define GL_PATH_MAX_MODELVIEW_STACK_DEPTH_NV 0x0D36
#define GL_PATH_TRANSPOSE_MODELVIEW_MATRIX_NV 0x84E3
#define GL_PATH_PROJECTION_STACK_DEPTH_NV 0x0BA4
#define GL_PATH_PROJECTION_MATRIX_NV 0x0BA7
#define GL_PATH_MAX_PROJECTION_STACK_DEPTH_NV 0x0D38
#define GL_PATH_TRANSPOSE_PROJECTION_MATRIX_NV 0x84E4
#define GL_FRAGMENT_INPUT_NV 0x936D
#endif /* GL_NV_path_rendering */

#ifndef GL_NV_blend_equation_advanced
#define GL_NV_blend_equation_advanced 1
#define GL_BLEND_OVERLAP_NV 0x9281
#define GL_BLEND_PREMULTIPLIED_SRC_NV 0x9280
#define GL_BLUE_NV 0x1905
#define GL_COLORBURN_NV 0x929A
#define GL_COLORDODGE_NV 0x9299
#define GL_CONJOINT_NV 0x9284
#define GL_CONTRAST_NV 0x92A1
#define GL_DARKEN_NV 0x9297
#define GL_DIFFERENCE_NV 0x929E
#define GL_DISJOINT_NV 0x9283
#define GL_DST_ATOP_NV 0x928F
#define GL_DST_IN_NV 0x928B
#define GL_DST_NV 0x9287
#define GL_DST_OUT_NV 0x928D
#define GL_DST_OVER_NV 0x9289
#define GL_EXCLUSION_NV 0x92A0
#define GL_GREEN_NV 0x1904
#define GL_HARDLIGHT_NV 0x929B
#define GL_HARDMIX_NV 0x92A9
#define GL_HSL_COLOR_NV 0x92AF
#define GL_HSL_HUE_NV 0x92AD
#define GL_HSL_LUMINOSITY_NV 0x92B0
#define GL_HSL_SATURATION_NV 0x92AE
#define GL_INVERT_OVG_NV 0x92B4
#define GL_INVERT_RGB_NV 0x92A3
#define GL_LIGHTEN_NV 0x9298
#define GL_LINEARBURN_NV 0x92A5
#define GL_LINEARDODGE_NV 0x92A4
#define GL_LINEARLIGHT_NV 0x92A7
#define GL_MINUS_CLAMPED_NV 0x92B3
#define GL_MINUS_NV 0x929F
#define GL_MULTIPLY_NV 0x9294
#define GL_OVERLAY_NV 0x9296
#define GL_PINLIGHT_NV 0x92A8
#define GL_PLUS_CLAMPED_ALPHA_NV 0x92B2
#define GL_PLUS_CLAMPED_NV 0x92B1
#define GL_PLUS_DARKER_NV 0x9292
#define GL_PLUS_NV 0x9291
#define GL_RED_NV 0x1903
#define GL_SCREEN_NV 0x9295
#define GL_SOFTLIGHT_NV 0x929C
#define GL_SRC_ATOP_NV 0x928E
#define GL_SRC_IN_NV 0x928A
#define GL_SRC_NV 0x9286
#define GL_SRC_OUT_NV 0x928C
#define GL_SRC_OVER_NV 0x9288
#define GL_UNCORRELATED_NV 0x9282
#define GL_VIVIDLIGHT_NV 0x92A6
#define GL_XOR_NV 0x1506
#endif /* GL_NV_blend_equation_advanced */

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#endif

#ifndef GL_ALL_BARRIER_BITS
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#define GL_UNIFORM_BARRIER_BIT 0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT 0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#endif

#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#endif

#ifndef GL_UNSIGNED_INT_ATOMIC_COUNTER
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#endif

#ifndef GL_SHADER_STORAGE_BLOCK
#define GL_SHADER_STORAGE_BLOCK 0x92E6
#endif

#ifndef GL_ACTIVE_RESOURCES
#define GL_ACTIVE_RESOURCES 0x92F5
#endif

#ifndef GL_BUFFER_BINDING
#define GL_ATOMIC_COUNTER_BUFFER_INDEX 0x9301
#define GL_BUFFER_BINDING 0x9302
#define GL_BUFFER_DATA_SIZE 0x9303
#define GL_NUM_ACTIVE_VARIABLES 0x9304
#define GL_ACTIVE_VARIABLES 0x9305
#endif

#ifndef GL_UNIFORM
#define GL_UNIFORM 0x92E1
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_LUMINANCE16F_EXT
#define GL_LUMINANCE16F_EXT               0x881E
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

#ifndef GL_TIMESTAMP
#define GL_TIMESTAMP                      0x8E28
#endif


QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

#endif // QSSGOPENGLTOKENS_H
