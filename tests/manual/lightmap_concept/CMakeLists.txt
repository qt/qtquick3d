qt_internal_add_manual_test(lightmap_concept
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::QuickControls2
)

set(qml_resource_files
    "main.qml"
    "lightmapgen.vert"
    "lightmapgen.frag"
    "lightmapuse.vert"
    "lightmapuse.frag"
    "hskin.png"
    "meshes/cube.mesh"
    "meshes/sphere.mesh"
    "meshes/torus.mesh"
    "meshes/suzanne.mesh"
    "meshes/animal.mesh"
)

qt_internal_add_resource(lightmap_concept "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
