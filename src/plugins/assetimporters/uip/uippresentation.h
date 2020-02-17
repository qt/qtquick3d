/****************************************************************************
**
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

#ifndef UIPPRESENTATION_H
#define UIPPRESENTATION_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>
#include <QXmlStreamAttributes>

#include <functional>

QT_BEGIN_NAMESPACE

namespace Q3DS {

enum PropertyType {     // value format
    Unknown = 0,
    StringList,         // String
    FloatRange,         // Float
    LongRange,          // Long
    Float,              // Float
    Long,               // Long
    Float2,             // Float2
    Vector,             // Float3
    Scale,              // Float3
    Rotation,           // Float3
    Color,              // Float4
    Boolean,            // Bool
    Slide,              // String
    Font,               // String
    FontSize,           // Float
    String,             // String
    MultiLineString,    // String
    ObjectRef,          // ObjectRef
    Image,              // String
    Mesh,               // String
    Import,             // String
    Texture,            // String
    Image2D,            // String
    Buffer,             // String
    Guid,               // Long4
    StringListOrInt,    // StringOrInt
    Renderable,         // String
    PathBuffer,         // String
    Enum,               // depends on name; data model only
    Matrix4x4           // Matrix4x4
};

bool convertToPropertyType(const QStringRef &value, Q3DS::PropertyType *type, int *componentCount, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToFloat(const QStringRef &value, float *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToInt(const QStringRef &value, int *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToInt32(const QStringRef &value, qint32 *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToBool(const QStringRef &value, bool *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToVector2D(const QStringRef &value, QVector2D *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToVector3D(const QStringRef &value, QVector3D *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToVector4D(const QStringRef &value, QVector4D *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
bool convertToMatrix4x4(const QStringRef &value, QMatrix4x4 *v, const char *desc = nullptr, QXmlStreamReader *reader = nullptr);
int animatablePropertyTypeToMetaType(Q3DS::PropertyType type);
QVariant convertToVariant(const QString &value, Q3DS::PropertyType type);
QString convertFromVariant(const QVariant &value);

} // namespace Q3DS

class PropertyChange
{
public:
    PropertyChange() = default;

    // When the new value is already set via a member or static setter.
    // Used by animations and any external call to a member setter.
    PropertyChange(const QString &name_)
        : m_name(name_)
    { }

    // Value included.
    // Used by slides (on-enter property changes) and data input.
    // High frequency usage should be avoided.
    PropertyChange(const QString &name_, const QString &value_)
        : m_name(name_), m_value(value_), m_hasValue(true)
    { }

    static PropertyChange fromVariant(const QString &name, const QVariant &value);

    // name() and value() must be source compatible with QXmlStreamAttribute
    QStringRef name() const { return QStringRef(&m_name); }
    QStringRef value() const { Q_ASSERT(m_hasValue); return QStringRef(&m_value); }

    QString nameStr() const { return m_name; }
    QString valueStr() const { Q_ASSERT(m_hasValue); return m_value; }

    // A setter can return an invalid change when the new value is the same as
    // before. Such changes are ignored by the changelist.
    bool isValid() const { return !m_name.isEmpty(); }

    // A change without value can only be used with notifyPropertyChanges, not
    // with applyPropertyChanges.
    bool hasValue() const { return m_hasValue; }

private:
    QString m_name;
    QString m_value;
    bool m_hasValue = false;
};

Q_DECLARE_TYPEINFO(PropertyChange, Q_MOVABLE_TYPE);

class PropertyChangeList
{
public:
    typedef const PropertyChange *const_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    PropertyChangeList() { }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    PropertyChangeList(std::initializer_list<PropertyChange> args);
#endif

    const_iterator begin() const Q_DECL_NOTHROW { return m_changes.begin(); }
    const_iterator cbegin() const Q_DECL_NOTHROW { return begin(); }
    const_iterator end() const Q_DECL_NOTHROW { return m_changes.end(); }
    const_iterator cend() const Q_DECL_NOTHROW { return end(); }
    const_reverse_iterator rbegin() const Q_DECL_NOTHROW { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const Q_DECL_NOTHROW { return rbegin(); }
    const_reverse_iterator rend() const Q_DECL_NOTHROW { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const Q_DECL_NOTHROW { return rend(); }

    bool isEmpty() const { return m_changes.isEmpty(); }
    int count() const { return m_changes.count(); }
    void clear() { m_changes.clear(); m_keys.clear(); }
    void append(const PropertyChange &change);
    QSet<QString> keys() const { return m_keys; }

    typedef PropertyChange value_type;

private:
    QVector<PropertyChange> m_changes;
    QSet<QString> m_keys;
};

class GraphObject
{
public:
    enum Type {
        Asset = 0,
        // direct subtypes
        Scene,
        Slide,
        Image,
        DefaultMaterial,
        ReferencedMaterial,
        CustomMaterial,
        Effect,
        Behavior,
        // node subtypes
        Layer = 100,
        Camera,
        Light,
        Model,
        Group,
        Text,
        Component,
        Alias
    };

    static constexpr Type AnyObject = Type::Asset;
    static constexpr Type FirstNodeType = Type::Layer;

    enum PropSetFlag {
        PropSetDefaults = 0x01,
        PropSetOnMaster = 0x02
    };
    Q_DECLARE_FLAGS(PropSetFlags, PropSetFlag)

    enum State {
        Enabled,
        Disabled
    };

    GraphObject(Type type);
    virtual ~GraphObject();

    // hmm where have I seen this before...
    Type type() const { return m_type; }
    QString typeName () const;

    GraphObject *parent() const { return m_parent; }
    GraphObject *firstChild() const { return m_firstChild; }
    GraphObject *lastChild() const { return m_lastChild; }
    GraphObject *nextSibling() const { return m_nextSibling; }
    GraphObject *previousSibling() const { return m_previousSibling; }
    int childCount() const;
    GraphObject *childAtIndex(int idx) const;
    void removeChildNode(GraphObject *node);
    void removeAllChildNodes();
    void prependChildNode(GraphObject *node);
    void appendChildNode(GraphObject *node);
    void insertChildNodeBefore(GraphObject *node, GraphObject *before);
    void insertChildNodeAfter(GraphObject *node, GraphObject *after);
    void reparentChildNodesTo(GraphObject *newParent);

    virtual void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags);
    virtual void applyPropertyChanges(const PropertyChangeList &changeList);

    bool isNode() const { return m_type >= FirstNodeType; }
    float startTime() { return m_startTime * 0.001f; }
    float endTime() { return m_endTime * 0.001f; }
    State state() const { return m_state; }

    QString qmlId();
    virtual void writeQmlHeader(QTextStream &output, int tabLevel) = 0;
    virtual void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) = 0;
    virtual void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) = 0;
    virtual void writeQmlFooter(QTextStream &output, int tabLevel);

    void destroyGraph();

    QByteArray m_id;
    QString m_name;
    qint32 m_startTime = 0;
    qint32 m_endTime = 10000;

private:
    Q_DISABLE_COPY(GraphObject)
    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    struct ObjectExtraMetaData
    {
        struct Data {
            QVector<QByteArray> propertyNames;
            QVector<QVariant> propertyValues;
        };
        QScopedPointer<Data> data;
    } metaData;

    ObjectExtraMetaData *extraMetaData() { return &metaData; }
    const ObjectExtraMetaData *extraMetaData() const { return &metaData; }

    GraphObject *m_parent = nullptr;
    GraphObject *m_firstChild = nullptr;
    GraphObject *m_lastChild = nullptr;
    GraphObject *m_nextSibling = nullptr;
    GraphObject *m_previousSibling = nullptr;
    Type m_type = AnyObject;
    State m_state = Enabled;

    friend class UipPresentation;
};

class Scene : public GraphObject
{
public:
    Scene();
    ~Scene() override;

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;

    bool m_useClearColor = true;
    QColor m_clearColor = Qt::black;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
    void writeQmlFooter(QTextStream &output, int tabLevel) override;
};

class AnimationTrack
{
public:
    enum AnimationType {
        NoAnimation = 0,
        Linear,
        EaseInOut,
        Bezier
    };

    struct KeyFrame {
        KeyFrame() = default;
        KeyFrame(float time_, float value_)
            : time(time_), value(value_)
        { }

        float time = 0; // seconds
        float value = 0;
        union {
            float easeIn;
            float c2time;
        };
        union {
            float easeOut;
            float c2value;
        };
        float c1time;
        float c1value;
    };
    using KeyFrameList = QVector<KeyFrame>;

    AnimationTrack() = default;
    AnimationTrack(AnimationType type_, GraphObject *target_, const QString &property_)
        : m_type(type_), m_target(target_), m_property(property_)
    { }

    AnimationType m_type = NoAnimation;
    GraphObject *m_target = nullptr;
    QString m_property;
    bool m_dynamic = false;
    KeyFrameList m_keyFrames;
};

Q_DECLARE_TYPEINFO(AnimationTrack::KeyFrame, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(AnimationTrack, Q_MOVABLE_TYPE);

inline bool operator==(const AnimationTrack::KeyFrame &a, const AnimationTrack::KeyFrame &b)
{
    return a.time == b.time;
}

inline bool operator!=(const AnimationTrack::KeyFrame &a, const AnimationTrack::KeyFrame &b)
{
    return !(a == b);
}

inline bool operator==(const AnimationTrack &a, const AnimationTrack &b)
{
    return a.m_target == b.m_target && a.m_property == b.m_property;
}

inline bool operator!=(const AnimationTrack &a, const AnimationTrack &b)
{
    return !(a == b);
}

class Slide : public GraphObject
{
public:
    enum PlayMode {
        StopAtEnd = 0,
        Looping,
        PingPong,
        Ping,
        PlayThroughTo
    };

    enum InitialPlayState {
        Play = 0,
        Pause
    };

    enum PlayThrough {
        Next,
        Previous,
        Value
    };

    using PropertyChanges = QHash<GraphObject *, PropertyChangeList *>;
    using AnimationTrackList = QVector<AnimationTrack>;

    Slide();
    ~Slide() override;

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    const QSet<GraphObject *> &objects() const { return m_objects; } // NB does not include objects from master
    void addObject(GraphObject *obj);
    void removeObject(GraphObject *obj);

    const PropertyChanges &propertyChanges() const { return m_propChanges; }
    void addPropertyChanges(GraphObject *target, PropertyChangeList *changeList); // changeList ownership transferred
    void removePropertyChanges(GraphObject *target);
    PropertyChangeList *takePropertyChanges(GraphObject *target);

    const AnimationTrackList &animations() const { return m_anims; }
    void addAnimation(const AnimationTrack &track);
    void removeAnimation(const AnimationTrack &track);

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    PlayMode m_playMode = StopAtEnd;
    InitialPlayState m_initialPlayState = Play;
    PlayThrough m_playThrough = Next;
    QVariant m_playThroughValue;
    QSet<GraphObject *> m_objects;
    PropertyChanges m_propChanges;
    AnimationTrackList m_anims;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
    void writeQmlFooter(QTextStream &output, int tabLevel) override;
};

class Image : public GraphObject
{
public:
    enum MappingMode {
        UVMapping = 0,
        EnvironmentalMapping,
        LightProbe,
        IBLOverride
    };

    enum TilingMode {
        Tiled = 0,
        Mirrored,
        NoTiling
    };

    enum ImagePropertyChanges {
        SourceChanges = 1 << 0
    };

    Image();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    bool isDefaultScaleAndRotation();

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    QString m_sourcePath;
    float m_scaleU = 1;
    float m_scaleV = 1;
    MappingMode m_mappingMode = UVMapping;
    TilingMode m_tilingHoriz = NoTiling;
    TilingMode m_tilingVert = NoTiling;
    float m_rotationUV = 0;
    float m_positionU = 0;
    float m_positionV = 0;
    float m_pivotU = 0;
    float m_pivotV = 0;
    QString m_subPresentation;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class Node : public GraphObject
{
public:
    enum NodeFlag {
        Active = 0x01, // eyeball
        IgnoresParentTransform = 0x02
    };
    Q_DECLARE_FLAGS(Flags, NodeFlag)

    enum RotationOrder {
        XYZ = 0,
        YZX,
        ZXY,
        XZY,
        YXZ,
        ZYX,
        XYZr,
        YZXr,
        ZXYr,
        XZYr,
        YXZr,
        ZYXr
    };

    enum Orientation {
        LeftHanded = 0,
        RightHanded
    };

    enum NodePropertyChanges {
        TransformChanges = 1 << 0,
        OpacityChanges = 1 << 1,
        EyeballChanges = 1 << 2
    };
    static const int FIRST_FREE_PROPERTY_CHANGE_BIT = 3;

    Node(Type type);

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    Flags m_flags = Active;
    QVector3D m_rotation;
    QVector3D m_position;
    QVector3D m_scale = QVector3D(1, 1, 1);
    QVector3D m_pivot;
    float m_localOpacity = 100.0f;
    qint32 m_skeletonId = -1;
    RotationOrder m_rotationOrder = YXZ;
    Orientation m_orientation = LeftHanded;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class LayerNode : public Node
{
public:
    enum Flag {
        DisableDepthTest = 0x01,
        DisableDepthPrePass = 0x02,
        TemporalAA = 0x04,
        FastIBL = 0x08,

        // non-uip
        ExplicitSize = 0x1000
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum ProgressiveAA {
        NoPAA = 0,
        PAA2x,
        PAA4x,
        PAA8x
    };

    enum MultisampleAA {
        NoMSAA = 0,
        MSAA2x,
        MSAA4x,
        SSAA
    };

    enum LayerBackground {
        Transparent = 0,
        SolidColor,
        Unspecified
    };

    enum BlendType {
        Normal = 0,
        Screen,
        Multiply,
        Add,
        Subtract,
        Overlay,
        ColorBurn,
        ColorDodge
    };

    enum HorizontalFields {
        LeftWidth = 0,
        LeftRight,
        WidthRight
    };

    enum VerticalFields {
        TopHeight = 0,
        TopBottom,
        HeightBottom
    };

    enum Units {
        Percent = 0,
        Pixels
    };

    enum LayerPropertyChanges {
        AoOrShadowChanges = 1 << Node::FIRST_FREE_PROPERTY_CHANGE_BIT,
        LayerContentSubTreeChanges = 1 << (Node::FIRST_FREE_PROPERTY_CHANGE_BIT + 1),
        LayerContentSubTreeLightsChange = 1 << (Node::FIRST_FREE_PROPERTY_CHANGE_BIT + 2),
        SourcePathChanges = 1 << (Node::FIRST_FREE_PROPERTY_CHANGE_BIT + 3)
    };

    LayerNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    void outputAAModeAndQuality(QTextStream &output, int tabLevel, const QString &propertyName);

    QSize m_explicitSize;

    Flags m_layerFlags = FastIBL;
    ProgressiveAA m_progressiveAA = NoPAA;
    MultisampleAA m_multisampleAA = NoMSAA;
    bool m_antialiasingSet = false;
    LayerBackground m_layerBackground = Transparent;
    QColor m_backgroundColor = Qt::black;
    BlendType m_blendType = Normal;

    HorizontalFields m_horizontalFields = LeftWidth;
    float m_left = 0;
    Units m_leftUnits = Percent;
    float m_width = 100;
    Units m_widthUnits = Percent;
    float m_right = 0;
    Units m_rightUnits = Percent;
    VerticalFields m_verticalFields = TopHeight;
    float m_top = 0;
    Units m_topUnits = Percent;
    float m_height = 100;
    Units m_heightUnits = Percent;
    float m_bottom = 0;
    Units m_bottomUnits = Percent;

    QString m_sourcePath;

    float m_aoStrength = 0;
    float m_aoDistance = 5;
    float m_aoSoftness = 50;
    float m_aoBias = 0;
    qint32 m_aoSampleRate = 2;
    bool m_aoDither = false;

    QString m_lightProbe_unresolved;
    float m_probeBright = 100;
    float m_probeHorizon = -1;
    float m_probeFov = 180;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class CameraNode : public Node
{

public:
    CameraNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    bool m_orthographic = false;
    float m_fov = 60;
    bool m_fovHorizontal = false;
    float m_clipNear = 10;
    float m_clipFar = 5000;
    bool m_frustumCulling = false;
    float m_zoom = 1.0;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class LightNode : public Node
{
public:
    enum LightType {
        Directional = 0,
        Point,
        Area
    };

    LightNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    QString m_scope_unresolved;
    GraphObject *m_scope = nullptr;
    LightType m_lightType = Directional;
    QColor m_lightDiffuse = Qt::white;
    QColor m_lightSpecular = Qt::white;
    QColor m_lightAmbient = Qt::black;
    float m_brightness = 100;
    float m_constantFade = 1;
    float m_linearFade = 0;
    float m_expFade = 0;
    float m_areaWidth = 100;
    float m_areaHeight = 100;
    bool m_castShadow = false;
    float m_shadowFactor = 10;
    float m_shadowFilter = 35;
    qint32 m_shadowMapRes = 9;
    float m_shadowBias = 0;
    float m_shadowMapFar = 5000;
    float m_shadowMapFov = 90;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class  ModelNode : public Node
{
public:
    enum Tessellation {
        None = 0,
        Linear,
        Phong,
        NPatch
    };

    enum ModelPropertyChanges {
        MeshChanges = 1 << Node::FIRST_FREE_PROPERTY_CHANGE_BIT
    };

    ModelNode();
    ~ModelNode() override;

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    QString m_mesh_unresolved;
    Tessellation m_tessellation = None;
    float m_edgeTess = 4;
    float m_innerTess = 4;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class GroupNode : public Node
{
public:
    GroupNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};



class ComponentNode : public Node
{

public:
    ComponentNode();
    ~ComponentNode() override;

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    class Slide *m_masterSlide = nullptr;
    class Slide *m_currentSlide = nullptr;


    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class TextNode : public Node
{

public:
    enum HorizontalAlignment {
        Left = 0,
        Center,
        Right
    };

    enum VerticalAlignment {
        Top = 0,
        Middle,
        Bottom
    };

    enum WordWrap {
        Clip = 0,
        WrapWord,
        WrapAnywhere
    };

    enum Elide {
        ElideNone = 0,
        ElideLeft,
        ElideMiddle,
        ElideRight
    };

    enum TextPropertyChanges {
        TextureImageDepChanges = 1 << Node::FIRST_FREE_PROPERTY_CHANGE_BIT
    };

    TextNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    QString m_text = QStringLiteral("Text");
    QColor m_color = Qt::white;
    QString m_font = QStringLiteral("TitilliumWeb-Regular");
    float m_size = 36;
    HorizontalAlignment m_horizAlign = Center;
    VerticalAlignment m_vertAlign = Middle;
    float m_leading = 0;
    float m_tracking = 0;
    bool m_shadow = false;
    float m_shadowStrength = 80;
    float m_shadowOffsetX = 0;
    float m_shadowOffsetY = 0;
    float m_shadowOffset = 10; // To be removed in 2.x (when UIP version is next updated)
    HorizontalAlignment m_shadowHorzAlign = Right; // To be removed in 2.x (when UIP version is next updated)
    VerticalAlignment m_shadowVertAlign = Bottom; // To be removed in 2.x (when UIP version is next updated)
    QVector2D m_boundingBox;
    WordWrap m_wordWrap = WrapWord;
    Elide m_elide = ElideNone;

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
};

class DefaultMaterial : public GraphObject
{
public:
    enum ShaderLighting {
        PixelShaderLighting = 0,
        NoShaderLighting
    };

    enum BlendMode {
        Normal = 0,
        Screen,
        Multiply,
        Overlay,
        ColorBurn,
        ColorDodge
    };

    enum SpecularModel {
        DefaultSpecularModel = 0,
        KGGX,
        KWard
    };

    enum DefaultMaterialPropertyChanges {
        BlendModeChanges = 1 << 0
    };

    DefaultMaterial();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;

    ShaderLighting m_shaderLighting = PixelShaderLighting;
    BlendMode m_blendMode = Normal;
    bool m_vertexColors = false;
    QColor m_diffuse = Qt::white;
    QString m_diffuseMap_unresolved;
    QString m_specularReflection_unresolved;
    QColor m_specularTint = Qt::white;
    float m_specularAmount = 0;
    QString m_specularMap_unresolved;
    SpecularModel m_specularModel = DefaultSpecularModel;
    float m_specularRoughness = 0;
    QString m_roughnessMap_unresolved;
    float m_fresnelPower = 0;
    float m_ior = 0.2f;
    QString m_bumpMap_unresolved;
    QString m_normalMap_unresolved;
    float m_bumpAmount = 0.5f;
    QString m_displacementMap_unresolved;
    float m_displaceAmount = 20;
    float m_opacity = 100;
    QString m_opacityMap_unresolved;
    QColor m_emissiveColor = Qt::white;
    float m_emissiveFactor = 0;
    QString m_emissiveMap_unresolved;
    QString m_translucencyMap_unresolved;
    float m_translucentFalloff = 1;
    float m_diffuseLightWrap = 0;
    // lightmaps
    QString m_lightmapIndirectMap_unresolved;
    QString m_lightmapRadiosityMap_unresolved;
    QString m_lightmapShadowMap_unresolved;
    // IBL override
    QString m_lightProbe_unresolved;
};

class ReferencedMaterial : public GraphObject
{
public:
    enum ReferencedMaterialPropertyChanges {
         ReferenceChanges = 1 << 0
    };

    ReferencedMaterial();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;

    QString m_referencedMaterial_unresolved;
    GraphObject *m_referencedMaterial = nullptr;
    // lightmap overrides
    QString m_lightmapIndirectMap_unresolved;
    QString m_lightmapRadiosityMap_unresolved;
    QString m_lightmapShadowMap_unresolved;
    // IBL override
    QString m_lightProbe_unresolved;
};

class CustomMaterialInstance : public GraphObject
{
public:
    enum CustomMaterialPropertyChanges {
        SourceChanges = 1 << 0
    };

    CustomMaterialInstance();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;

    QString m_material_unresolved;
    bool m_materialIsResolved = false;
    QVariantMap m_materialPropertyVals;
    PropertyChangeList m_pendingCustomProperties;
    // lightmaps
    QString m_lightmapIndirectMap_unresolved;
    QString m_lightmapRadiosityMap_unresolved;
    QString m_lightmapShadowMap_unresolved;
    // IBL override
    QString m_lightProbe_unresolved;
};

class EffectInstance : public GraphObject
{
public:
    enum EffectInstancePropertyChanges {
        EyeBallChanges = 1 << 0,
        SourceChanges = 1 << 1
    };

    EffectInstance();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
    void writeQmlFooter(QTextStream &output, int tabLevel) override;

    QString m_effect_unresolved;
    bool m_effectIsResolved = false;
    bool m_eyeballEnabled = true;
    PropertyChangeList m_pendingCustomProperties;
};

class  BehaviorInstance : public GraphObject
{
public:
    enum BehaviorInstancePropertyChanges {
        EyeBallChanges = 1 << 0
    };

    BehaviorInstance();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);

    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;
    void writeQmlFooter(QTextStream &output, int tabLevel) override;

    QString m_behavior_unresolved;
    bool m_behaviorIsResolved = false;
    bool m_eyeballEnabled = true;
    PropertyChangeList m_pendingCustomProperties;
    QVariantMap m_behaviorPropertyVals;
};

class AliasNode : public Node
{
public:
    AliasNode();

    void setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags) override;
    void applyPropertyChanges(const PropertyChangeList &changeList) override;

    template<typename V> void setProps(const V &attrs, PropSetFlags flags);
    // GraphObject interface
public:
    void writeQmlHeader(QTextStream &output, int tabLevel) override;
    void writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel = false) override;
    void writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel) override;

    QString m_referencedNode_unresolved;
    GraphObject *m_referencedNode = nullptr;

};

struct UipPresentationData;
class UipPresentation
{
public:
    UipPresentation();
    ~UipPresentation();
    void reset();

    enum Rotation {
        NoRotation = 0,
        Clockwise90,
        Clockwise180,
        Clockwise270
    };

    QString sourceFile() const;
    void setSourceFile(const QString &s);
    QString assetFileName(const QString &xmlFileNameRef, int *part) const;

    QString name() const;
    void setName(const QString &s);

    QString author() const;
    QString company() const;
    int presentationWidth() const;
    int presentationHeight() const;
    Rotation presentationRotation() const;
    bool maintainAspectRatio() const;

    void setAuthor(const QString &author);
    void setCompany(const QString &company);
    void setPresentationWidth(int w);
    void setPresentationHeight(int h);
    void setPresentationRotation(Rotation r);
    void setMaintainAspectRatio(bool maintain);

    Scene *scene() const;
    Slide *masterSlide() const;

    void setScene(Scene *p);
    void setMasterSlide(Slide *p);

    bool registerObject(const QByteArray &id, GraphObject *p); // covers both the scene and slide graphs
    void unregisterObject(const QByteArray &id);

    template <typename T = GraphObject>
    const T *object(const QByteArray &id) const { return static_cast<const T *>(getObject(id)); }
    template <typename T = GraphObject>
    T *object(const QByteArray &id) { return static_cast<T *>(getObject(id)); }

    template <typename T = GraphObject>
    const T *objectByName(const QString &name) const { return static_cast<const T *>(getObjectByName(name)); }
    template <typename T = GraphObject>
    T *objectByName(const QString &name) { return static_cast<T *>(getObjectByName(name)); }

    void registerImageBuffer(const QString &sourcePath, bool hasTransparency);

    typedef QHash<QString, bool> ImageBufferMap;
    const ImageBufferMap &imageBuffer() const;

    void applyPropertyChanges(const Slide::PropertyChanges &changeList) const;
    void applySlidePropertyChanges(Slide *slide) const;

    template<typename T> T *newObject(const QByteArray &id)
    {
        T *obj = new T;
        return registerObject(id, obj) ? obj : nullptr; // also sets obj->id
    }

    GraphObject *newObject(const char *type, const QByteArray &id);

    GraphObject *getObject(const QByteArray &id) const;
    GraphObject *getObjectByName(const QString &name) const;

    QScopedPointer<UipPresentationData> d;
    QHash<QString, bool> m_imageTransparencyHash;
    friend class UipParser;
};

struct UipPresentationData
{
    QString sourceFile;
    QString name;
    QString author;
    QString company;
    int presentationWidth = 0;
    int presentationHeight = 0;
    UipPresentation::Rotation presentationRotation = UipPresentation::NoRotation;
    bool maintainAspectRatio = false;
    qint64 loadTime = 0;
    qint64 meshesLoadTime = 0;

    Scene *scene = nullptr;
    Slide *masterSlide = nullptr;
    QHash<QByteArray, GraphObject *> objects; // node ptrs managed by scene, not owned

    // Note: the key here is the sourcePath before it's resolved!
    QHash<QString, bool /* hasTransparency */> imageBuffers;
};

QT_END_NAMESPACE

#endif // UIPPRESENTATION_H
