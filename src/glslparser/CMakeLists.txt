qt_internal_add_module(Quick3DGlslParserPrivate
    INTERNAL_MODULE
    DEFINES
        QT_BUILD_QUICK3DGLSLPARSER_PRIVATE_LIB
    SOURCES
        glsl.h
        glslast.cpp glslast.h
        glslastdump.cpp glslastdump.h
        glslastvisitor.cpp glslastvisitor.h
        glslengine.cpp glslengine.h
        glslkeywords.cpp
        glsllexer.cpp glsllexer.h
        glslmemorypool.cpp glslmemorypool.h
        glslparser.cpp glslparser.h
        glslparsertable.cpp glslparsertable_p.h
        glslsemantic.cpp glslsemantic.h
        glslsymbol.cpp glslsymbol.h
        glslsymbols.cpp glslsymbols.h
        glsltype.cpp glsltype.h
        glsltypes.cpp glsltypes.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::Core
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
