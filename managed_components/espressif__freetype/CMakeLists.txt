# idf_component_register()

# # Override options defined in freetype/CMakeLists.txt.
# # We could have used normal set(...) here if freetype enabled CMake policy CMP0077.
# option(FT_DISABLE_HARFBUZZ "" ON)
# option(FT_DISABLE_BZIP2 "" ON)
# option(FT_DISABLE_BROTLI "" ON)
# option(FT_DISABLE_PNG "" ON)
# option(FT_DISABLE_ZLIB "" ON)

# # These are regular CMake variables, so we can set them directly.
# set(SKIP_INSTALL_ALL TRUE)
# set(BUILD_SHARED_LIBS OFF)

# add_subdirectory(freetype output)

# # https://gitlab.freedesktop.org/freetype/freetype/-/issues/1299
# target_compile_options(freetype PRIVATE "-Wno-dangling-pointer")

# target_link_libraries(${COMPONENT_LIB} INTERFACE freetype)

set(srcs
    lv_ftsystem.c
    freetype/src/base/ftmm.c
    freetype/src/base/ftbase.c
    freetype/src/base/ftbitmap.c
    freetype/src/base/ftdebug.c
    freetype/src/base/ftglyph.c
    freetype/src/base/ftinit.c
    freetype/src/base/ftstroke.c
    freetype/src/cache/ftcache.c
    freetype/src/gzip/ftgzip.c
    freetype/src/sfnt/sfnt.c
    freetype/src/smooth/smooth.c
    freetype/src/truetype/truetype.c
)
set(includes "." "freetype/include")

set(public_requires "lvgl__lvgl")

idf_component_register(SRCS ${srcs}
                    REQUIRES ${public_requires}
                    INCLUDE_DIRS ${includes})

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-dangling-pointer")


