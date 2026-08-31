cmake_minimum_required(VERSION 4.1)
project(graphics_coursework)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GLEW REQUIRED)

find_library(FREEIMAGE_LIB FreeImage)

include(FetchContent)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG        v0.9.8
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
        nativefiledialog-extended
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
        GIT_TAG        v1.3.0
)
FetchContent_MakeAvailable(nativefiledialog-extended)

include_directories(${GLFW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})

add_executable(graphics_coursework main.cpp
        Scene.cpp
        Scene.h
        objects/SceneObject.cpp
        objects/SceneObject.h
        objects/Cuboid.cpp
        objects/Cuboid.h
        GlmMaths.h
        Shader.h
        Shader.cpp
        objects/MeshObject.cpp
        objects/MeshObject.h
        InputManager.cpp
        InputManager.h
        3dStruct/threeDModel.h
        3dStruct/threeDModel.cpp
        TextureHandler/TextureHandler.cpp
        Structures/Vector3d.cpp
        Octree/Octree.cpp
        Box/Box.cpp
        Utilities/IntersectionTests.cpp
        Obj/OBJLoader.cpp
        Images/ImageLoading.cpp
        objects/ModelObject.cpp
        objects/ModelObject.h
        CoasterTrack.cpp
        CoasterTrack.h
        Light.h
        objects/Cart.cpp
        objects/Cart.h
)

target_link_libraries(graphics_coursework ${OPENGL_LIBRARIES} glfw glm::glm ${GLEW_LIBRARIES} freeimage magic_enum::magic_enum nfd)

add_custom_target(copy-files
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/glsl_files"
        "${CMAKE_CURRENT_BINARY_DIR}/glsl_files"
)

add_custom_target(models
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "${CMAKE_CURRENT_BINARY_DIR}/models"
)

add_custom_target(copy-config
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "${CMAKE_CURRENT_BINARY_DIR}/config"
)

add_dependencies(graphics_coursework copy-files models copy-config)