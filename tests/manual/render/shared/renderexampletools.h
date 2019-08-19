/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once
#ifndef QSSG_RENDER_EXAMPLE_TOOLS_H
#define QSSG_RENDER_EXAMPLE_TOOLS_H
#include "renderexample.h"

class QSSGRenderExampleTools
{
public:
    static const char *getSimpleVertShader()
    {
        return "#version 120\n"
               "uniform mat4 mat_mvp;\n"
               "attribute vec3 attr_pos;			// Vertex pos\n"
               "attribute vec3 attr_norm;			// Vertex pos\n"
               "attribute vec2 attr_uv; 			// UV coords\n"
               "varying vec4 color_to_add;\n"
               "varying vec2 uv_coords;\n"
               "void main()\n"
               "{\n"
               "gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
               "color_to_add.xyz = attr_norm * attr_norm;\n"
               "color_to_add.a = 1.0;\n"
               "uv_coords = attr_uv;\n"
               "}\n";
    }

    static const char *getSimpleFragShader()
    {
        return "#version 120\n"
               "varying vec4 color_to_add;\n"
               "void main()\n"
               "{\n"
               "gl_FragColor=color_to_add;\n"
               "}\n";
    }

    static const char *getSimpleFragShaderTex()
    {
        return "#version 120\n"
               "uniform sampler2D image0;\n"
               "varying vec2 uv_coords;\n"
               "void main()\n"
               "{\n"
               "gl_FragColor=vec4(texture2D( image0, uv_coords ).xyz, 1.0 );\n"
               "}\n";
    }

    static QSSGRef<QSSGRenderInputAssembler> createBox(QSSGRef<QSSGRenderContext> context,
                                                                QSSGRef<QSSGRenderVertexBuffer> &outVertexBuffer,
                                                                QSSGRef<QSSGRenderIndexBuffer> &outIndexBuffer);
    static QSSGRef<QSSGRenderShaderProgram> createSimpleShader(QSSGRef<QSSGRenderContext> context);
    static QSSGRef<QSSGRenderShaderProgram> createSimpleShaderTex(QSSGRef<QSSGRenderContext> context);
};

#endif
