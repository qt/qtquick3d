// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>

#include <QtGui/private/qshader_p.h>

#include <QtShaderTools/private/qshaderbaker_p.h>

#include <QtQuick3DUtils/private/qqsbcollection_p.h>

static const char *shaderDescription() { return "ShaderDescription"; }
static const char *tempOutFileName() { return "qsbctstfile.qsbc"; }

static const char *vertSource() {
    return "#version 310 es\n"
           "layout (location = 0) in vec3 pos;\n"
           "layout (location = 1) out vec4 vertColor;\n"
           "void main()\n"
           "{\n"
           "    gl_Position = vec4(pos, 1.0);\n"
           "    vertColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
           "}\n";
}
static const char *fragSource() {
    return "#version 310 es\n"
            "precision lowp float;\n"
            "layout (location = 0) out vec4 glColor;\n"
            "layout (location = 1) in vec4 vertColor;\n"
            "void main()\n"
            "{\n"
            "    glColor = vertColor;\n"
            "}\n";
}

class ShaderCollection : public QObject
{
    Q_OBJECT
public:
    ShaderCollection() = default;
    ~ShaderCollection() = default;

private Q_SLOTS:
    void initTestCase();
    void test_readWriteToFile();
    void test_readWriteToBuffer();
    void test_readWriteOpenDevice();
    void test_mapModes();

private:
    QShader vert;
    QShader frag;
    QQsbShaderFeatureSet featureSet;
};

void ShaderCollection::initTestCase()
{
    QShaderBaker baker;
    baker.setGeneratedShaders({{ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) }});
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    baker.setSourceString(vertSource(), QShader::VertexStage);
    vert = baker.bake();
    QVERIFY2(vert.isValid(), qPrintable(baker.errorMessage()));

    baker.setSourceString(fragSource(), QShader::FragmentStage);
    frag = baker.bake();
    QVERIFY2(frag.isValid(), qPrintable(baker.errorMessage()));

    featureSet.insert("feature1", true);
    featureSet.insert("feature2", false);
}

void ShaderCollection::test_readWriteToFile()
{
    const size_t hkey = 99;
    const auto tempFile = QDir::tempPath() + QDir::separator() + tempOutFileName();
    QQsbCollection::Entry entry;
    {
        QQsbCollection qsbc(tempFile);
        QVERIFY(qsbc.map(QQsbCollection::Write));
        entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
        QVERIFY(entry.offset != -1);
        QCOMPARE(entry.hkey, hkey);
        const auto entries = qsbc.getEntries();
        QCOMPARE(entries.size(), 1);
        QCOMPARE(*entries.begin(), entry);
        qsbc.unmap();
    }

    {
        QQsbCollection qsbc(tempFile);
        QVERIFY(qsbc.map(QQsbCollection::Read));
        const auto entries = qsbc.getEntries();
        QCOMPARE(entries.size(), 1);
        QCOMPARE(*entries.begin(), entry);
        QByteArray desc;
        QShader vertShader;
        QShader fragShader;
        QQsbShaderFeatureSet features;
        QVERIFY(qsbc.extractQsbEntry(entry, &desc, &features, &vertShader, &fragShader));
        QCOMPARE(desc, QByteArray(shaderDescription()));
        QCOMPARE(vertShader, vert);
        QCOMPARE(fragShader, frag);
        QCOMPARE(features, featureSet);
        qsbc.unmap();
    }
}

void ShaderCollection::test_readWriteToBuffer()
{
    QBuffer buffer;
    const size_t hkey = 99;
    QQsbCollection::Entry entry;
    {
        QVERIFY(buffer.buffer().isEmpty());
        QQsbCollection qsbc(buffer);
        QVERIFY(qsbc.map(QQsbCollection::Write));
        entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
        QVERIFY(entry.offset != -1);
        QCOMPARE(entry.hkey, hkey);
        qsbc.unmap();
        QVERIFY(!buffer.buffer().isEmpty());
    }

    {
        QQsbCollection qsbc(buffer);
        QVERIFY(qsbc.map(QQsbCollection::Read));
        QByteArray desc;
        QShader vertShader;
        QShader fragShader;
        QQsbShaderFeatureSet features;
        QVERIFY(qsbc.extractQsbEntry(entry, &desc, &features, &vertShader, &fragShader));
        QCOMPARE(desc, QByteArray(shaderDescription()));
        QCOMPARE(vertShader, vert);
        QCOMPARE(fragShader, frag);
        QCOMPARE(features, featureSet);
        qsbc.unmap();
    }
}

void ShaderCollection::test_readWriteOpenDevice()
{
    QBuffer buffer;
    const size_t hkey = 99;
    QQsbCollection::Entry entry;
    {
        buffer.open(QIODevice::WriteOnly | QIODevice::Truncate);
        QCOMPARE(buffer.isOpen(), true);
        QVERIFY(buffer.buffer().isEmpty());
        QQsbCollection qsbc(buffer);
        QVERIFY(qsbc.map(QQsbCollection::Write));
        entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
        QVERIFY(entry.offset != -1);
        QCOMPARE(entry.hkey, hkey);
        qsbc.unmap();
        QVERIFY(!buffer.buffer().isEmpty());
    }

    QCOMPARE(buffer.isOpen(), false);

    {
        buffer.open(QIODevice::ReadOnly);
        QCOMPARE(buffer.isOpen(), true);
        QQsbCollection qsbc(buffer);
        QVERIFY(qsbc.map(QQsbCollection::Read));
        QByteArray desc;
        QShader vertShader;
        QShader fragShader;
        QQsbShaderFeatureSet features;
        QVERIFY(qsbc.extractQsbEntry(entry, &desc, &features, &vertShader, &fragShader));
        QCOMPARE(desc, QByteArray(shaderDescription()));
        QCOMPARE(vertShader, vert);
        QCOMPARE(fragShader, frag);
        QCOMPARE(features, featureSet);
        qsbc.unmap();
    }
}

void ShaderCollection::test_mapModes()
{
    {
        QBuffer buffer;
        const size_t hkey = 99;
        QQsbCollection::Entry entry;
        {
            buffer.open(QIODevice::ReadWrite | QIODevice::Truncate);
            QCOMPARE(buffer.isOpen(), true);
            QVERIFY(buffer.buffer().isEmpty());
            QQsbCollection qsbc(buffer);
            QVERIFY(qsbc.map(QQsbCollection::Write));
            entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
            QVERIFY(entry.offset != -1);
            QCOMPARE(entry.hkey, hkey);
            qsbc.unmap();
            QVERIFY(!buffer.buffer().isEmpty());
        }

        QCOMPARE(buffer.isOpen(), false);

        {
            buffer.open(QIODevice::ReadOnly);
            QCOMPARE(buffer.isOpen(), true);
            QQsbCollection qsbc(buffer);
            QVERIFY(qsbc.map(QQsbCollection::Read));
            QByteArray desc;
            QShader vertShader;
            QShader fragShader;
            QQsbShaderFeatureSet features;
            QVERIFY(qsbc.extractQsbEntry(entry, &desc, &features, &vertShader, &fragShader));
            QCOMPARE(desc, QByteArray(shaderDescription()));
            QCOMPARE(vertShader, vert);
            QCOMPARE(fragShader, frag);
            QCOMPARE(features, featureSet);
            qsbc.unmap();
        }
    }

    {
        QBuffer buffer;
        const size_t hkey = 99;
        QQsbCollection::Entry entry;

        { // Test invalid input (missing Truncate)
            buffer.open(QIODevice::ReadWrite);
            QCOMPARE(buffer.isOpen(), true);
            QVERIFY(buffer.buffer().isEmpty());
            QQsbCollection qsbc(buffer);
            QCOMPARE(qsbc.map(QQsbCollection::Write), false);
            entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
            QCOMPARE(entry.isValid(), false);
            qsbc.unmap();
            QVERIFY(buffer.buffer().isEmpty());
        }

        { // Test invalid input (Text)
            buffer.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
            QCOMPARE(buffer.isOpen(), true);
            QVERIFY(buffer.buffer().isEmpty());
            QQsbCollection qsbc(buffer);
            QCOMPARE(qsbc.map(QQsbCollection::Write), false);
            entry = qsbc.addQsbEntry(QByteArray(shaderDescription()), featureSet, vert, frag, hkey);
            QCOMPARE(entry.isValid(), false);
            qsbc.unmap();
            QVERIFY(buffer.buffer().isEmpty());
        }
    }



}

QTEST_APPLESS_MAIN(ShaderCollection)

#include "tst_shadercollection.moc"
