set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/fragment.glsl
    Shaders/vertex.glsl
    Textures/voronoi.png

    resources.qrc
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(demo-app ${SRCS})

target_link_libraries(demo-app
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)