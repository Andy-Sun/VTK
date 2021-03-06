PROJECT (VTKFTGL)

set(vtkftgl_THIRD_PARTY 1)
set(vtkftgl_LIBRARIES vtkftgl)
set(vtkftgl_INCLUDE_DIRS ${VTKFTGL_SOURCE_DIR}/src)
vtk_module_export_info()
set(VTK_FREETYPE_LIBRARIES ${vtkfreetype_LIBRARIES})

#
# Dependency mask
#
INCLUDE_REGULAR_EXPRESSION(".*")

#
# Collect the required libs
#
SET (VTKFTGL_LIBS "")

#
# Do not use the STL (portability issue)
#
SET (VTKFTGL_DO_NOT_USE_STL 1)

#
# Source files
#
SET (FTGL_SRCS
  src/FTBitmapGlyph.cpp
  src/FTBitmapGlyphRenderOpenGL.cpp
  src/FTCharmap.cpp
  src/FTFace.cpp
  src/FTFont.cpp
  src/FTGLBitmapFont.cpp
  src/FTGLBitmapFontRenderOpenGL.cpp
  src/FTGLPixmapFont.cpp
  src/FTGLPixmapFontRenderOpenGL.cpp
  src/FTGlyph.cpp
  src/FTGlyphContainer.cpp
  src/FTLibrary.cpp
  src/FTPixmapGlyph.cpp
  src/FTPixmapGlyphRenderOpenGL.cpp
  src/FTSize.cpp
)

#
# Do not use texture fonts (crash on Type1 fonts)
#
SET (VTKFTGL_DO_NOT_USE_TEXTURE_FONT 1)
IF (NOT VTKFTGL_DO_NOT_USE_TEXTURE_FONT)
  SET (FTGL_SRCS ${FTGL_SRCS}
    src/FTGLTextureFont.cpp
    src/FTTextureGlyph.cpp
  )
ENDIF ()

#
# Do not use vector fonts (we only need pixmap and bitmaps and vector fonts
# needs glu). Might be an option at some point, but set it to 'true' for now.
#
SET (VTKFTGL_DO_NOT_USE_VECTORISER 1)
IF (NOT VTKFTGL_DO_NOT_USE_VECTORISER)
  SET (FTGL_SRCS ${FTGL_SRCS}
    src/FTExtrdGlyph.cpp
    src/FTGLExtrdFont.cpp
    src/FTGLOutlineFont.cpp
    src/FTGLPolygonFont.cpp
    src/FTOutlineGlyph.cpp
    src/FTPolyGlyph.cpp
  )
ENDIF ()

#
# Shared/static lib settings)
#
IF (BUILD_SHARED_LIBS)
  SET (VTKFTGL_DLL 1)
ELSE ()
  SET (VTKFTGL_STATIC 1)
ENDIF ()

#
# Win32 flags (and shared/static lib settings)
#
IF (WIN32)
  # This will add -DUSE_STD_NAMESPACE and -DWIN32
  SET (VTKFTGL_WIN32_FLAGS 1)
ENDIF ()

#
# Unix flags
#
IF (UNIX)
  IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
    SET (VTKFTGL_HPUX_SOURCE 1)
  ENDIF()
ENDIF ()

#
# We need OpenGL
#
# Use GL library provided by parent VTK if possible.
#
IF(NOT OPENGL_FOUND)
  FIND_PACKAGE(OpenGL)
ENDIF()
IF (OPENGL_INCLUDE_PATH)
  INCLUDE_DIRECTORIES (${OPENGL_INCLUDE_PATH})
ENDIF ()
IF(OPENGL_gl_LIBRARY)
  SET (VTKFTGL_LIBS "${VTKFTGL_LIBS};${OPENGL_gl_LIBRARY}")
ENDIF()

IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  SET (VTKFTGL_LIBS ${VTKFTGL_LIBS} ${OSMESA_LIBRARY} )
ENDIF ()

#
# We obviously need FreeType
#
SET (VTKFTGL_LIBS "${VTKFTGL_LIBS};${VTK_FREETYPE_LIBRARIES}")

#
# Define the library (and install it)
#
VTK_ADD_LIBRARY (vtkftgl ${FTGL_SRCS})
TARGET_LINK_LIBRARIES(vtkftgl ${VTKFTGL_LIBS})

#
# Create the configuration file
#
ADD_DEFINITIONS (-DVTKFTGL)
CONFIGURE_FILE(${VTKFTGL_SOURCE_DIR}/vtkftglConfig.h.in
               ${VTKFTGL_BINARY_DIR}/vtkftglConfig.h)

#
# Build examples (testing purposes)
# (Warning, they use Glut)
#
#OPTION (FTGL_BUILD_EXAMPLES "Build FTGL examples (warning: you need Glut)" OFF)
#MARK_AS_ADVANCED (FTGL_BUILD_EXAMPLES)
SET(FTGL_BUILD_EXAMPLES 0)

IF (FTGL_BUILD_EXAMPLES)

  #
  # Need GLU and GLUT
  #
  FIND_PACKAGE(GLU)
  FIND_PACKAGE(GLUT)

  IF (GLU_LIBRARY AND GLUT_LIBRARY)

    SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_LIBS}")

    IF (GLUT_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLUT_INCLUDE_PATH})
    ENDIF ()
    SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};${GLUT_LIBRARY}")

    IF (GLU_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLU_INCLUDE_PATH})
    ENDIF ()
    SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};${GLU_LIBRARY}")

    #
    # Do we need X ?
    #
    IF (VTK_USE_X)
      SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};-lXt")
      FIND_LIBRARY (XMU_LIBRARY Xmu
        /usr/lib 
        /usr/local/lib 
        /opt/graphics/OpenGL/lib 
        /usr/openwin/lib 
        /usr/X11R6/lib
        /usr/contrib/X11R6/lib
      )
      MARK_AS_ADVANCED (XMU_LIBRARY)
      IF (XMU_LIBRARY)
        SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};${XMU_LIBRARY}")
      ENDIF ()
      SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};${X11_LIBRARIES}")
    ENDIF ()

    #
    # Win32 flags
    #
    IF (WIN32)
      # - Avoid the glutCreateMenu_ATEXIT_HACK pb.
      # - Sort-of a hack to avoid the bug in glut.h to avoid
      #   ambiguity between 'std::exit(int)' and 'std::exit(int)' in function
      ADD_DEFINITIONS (-DGLUT_DISABLE_ATEXIT_HACK)
      ADD_DEFINITIONS (-DGLUT_BUILDING_LIB)
    ENDIF ()

    #
    # Apple libraries.
    #
    IF (APPLE)
      SET (VTKFTGL_EXAMPLES_LIBS "-framework OpenGL" "-framework GLUT" ${VTKFTGL_EXAMPLES_LIBS})
    ENDIF ()

    #
    # Link examples to FTGL and Freetype
    #
    SET (VTKFTGL_EXAMPLES_LIBS "${VTKFTGL_EXAMPLES_LIBS};vtkftgl;${VTK_FREETYPE_LIBRARIES}")

    #
    # Example 1
    #
    SET(FTGL_DEMO_SRCS
      demo/tb.c
      demo/trackball.c
      demo/FTGLDemo.cpp
    )
    ADD_EXECUTABLE (ftgl_demo ${FTGL_DEMO_SRCS})
    TARGET_LINK_LIBRARIES(ftgl_demo ${VTKFTGL_EXAMPLES_LIBS})

    #
    # Example 2
    #
    ADD_EXECUTABLE (ftgl_demo2 demo/demo.cpp)
    TARGET_LINK_LIBRARIES(ftgl_demo2 ${VTKFTGL_EXAMPLES_LIBS})

  ENDIF ()

ENDIF ()
