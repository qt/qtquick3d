// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include <QtGui/QSurfaceFormat>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
#include <rhi/qshaderbaker.h>
#endif

class tst_ShaderCache : public QObject
{
    Q_OBJECT

private slots:
    void glslTargetsForContext_data();
    void glslTargetsForContext();
    void persistentBakerTargets();
};

// "330, 460" or "300es, 320es". Comparing the rendered form rather than the
// QShaderVersion list keeps the ES flavour visible in the failure message --
// dropping it is exactly the kind of mistake this test is here to catch.
static QByteArray describe(const QList<QShaderVersion> &versions)
{
    QByteArray result;
    for (const QShaderVersion &version : versions) {
        if (!result.isEmpty())
            result += ", ";
        result += QByteArray::number(version.version());
        if (version.flags().testFlag(QShaderVersion::GlslEs))
            result += "es";
    }
    return result;
}

void tst_ShaderCache::glslTargetsForContext_data()
{
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");
    QTest::addColumn<int>("profile");
    QTest::addColumn<int>("renderableType");
    QTest::addColumn<bool>("isGLESModule");
    QTest::addColumn<QByteArray>("expected");

    const int core = QSurfaceFormat::CoreProfile;
    const int compat = QSurfaceFormat::CompatibilityProfile;
    const int noProfile = QSurfaceFormat::NoProfile;
    const int gl = QSurfaceFormat::OpenGL;
    const int gles = QSurfaceFormat::OpenGLES;

    // Core profile: the 330 floor plus the context's own version, and no
    // duplicate 330 when the context is exactly 3.3.
    QTest::newRow("core 3.3") << 3 << 3 << core << gl << false << QByteArray("330");
    QTest::newRow("core 4.3") << 4 << 3 << core << gl << false << QByteArray("330, 430");
    QTest::newRow("core 4.6") << 4 << 6 << core << gl << false << QByteArray("330, 460");

    // Below 3.3, and any non-core profile, fall back to the fixed 140/130.
    QTest::newRow("core 3.2") << 3 << 2 << core << gl << false << QByteArray("140");
    QTest::newRow("compat 4.6") << 4 << 6 << compat << gl << false << QByteArray("140, 460");
    QTest::newRow("desktop 3.0") << 3 << 0 << noProfile << gl << false << QByteArray("130");

    // GLES: same shape, floored at 300, and every version has to keep the es
    // flavour or QRhiGles2 will not find the shader at all.
    QTest::newRow("gles 2.0") << 2 << 0 << noProfile << gles << false << QByteArray("100es");
    QTest::newRow("gles 3.0") << 3 << 0 << noProfile << gles << false << QByteArray("300es");
    QTest::newRow("gles 3.1") << 3 << 1 << noProfile << gles << false << QByteArray("300es, 310es");
    QTest::newRow("gles 3.2") << 3 << 2 << noProfile << gles << false << QByteArray("300es, 320es");

    // A GLES build of Qt takes the same path even when the format does not say
    // so itself.
    QTest::newRow("gles module") << 3 << 1 << noProfile << gl << true << QByteArray("300es, 310es");
}

void tst_ShaderCache::glslTargetsForContext()
{
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);
    QFETCH(int, profile);
    QFETCH(int, renderableType);
    QFETCH(bool, isGLESModule);
    QFETCH(QByteArray, expected);

    QSurfaceFormat format;
    format.setVersion(majorVersion, minorVersion);
    format.setProfile(QSurfaceFormat::OpenGLContextProfile(profile));
    format.setRenderableType(QSurfaceFormat::RenderableType(renderableType));

    const QByteArray actual = describe(QSSGShaderCache::glslTargetsForContext(format, isGLESModule));
    QCOMPARE(actual, expected);
}

void tst_ShaderCache::persistentBakerTargets()
{
#ifndef QT_QUICK3D_HAS_RUNTIME_SHADERS
    QSKIP("Built without QtShaderTools, nothing bakes");
#else
    QShaderBaker baker;
    baker.setSourceString("#version 440\n"
                          "void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }\n",
                          QShader::VertexStage,
                          QLatin1String("tst_shadercache.vert"));
    QSSGShaderCache::initBakerForPersistentUse(&baker, nullptr);

    const QShader shader = baker.bake();
    QVERIFY2(shader.isValid(), qPrintable(baker.errorMessage()));

    // The persistent cache file outlives the run that wrote it and may be
    // carried to another backend or platform, so every target has to be there.
    // A shader this trivial translates everywhere, meaning anything missing is
    // a target that was not asked for rather than one that failed to build.
    const QList<QShaderKey> keys = shader.availableShaders();

#ifndef Q_OS_WASM
    QVERIFY(keys.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    // Compute shaders and shader storage buffers need 4.3.
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(430))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(420))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(140))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(130))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
#endif
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs))));
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(310, QShaderVersion::GlslEs))));
    // Image atomics need GLSL ES 3.20.
    QVERIFY(keys.contains(QShaderKey(QShader::GlslShader, QShaderVersion(320, QShaderVersion::GlslEs))));
#endif
}

QTEST_APPLESS_MAIN(tst_ShaderCache)

#include "tst_shadercache.moc"
