requires(!watchos)
requires(qtHaveModule(quick))
requires(qtHaveModule(openglextensions))
qtHaveModule(gui):requires(qtConfig(opengl))

load(qt_parts)
