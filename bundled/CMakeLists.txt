# Copy the content in bundled/content next to the binary, so a fresh build starts with OpenGFX and the
# bundled NewGRFs available. The .tar files come from bundled/fetch_content.py; tradetycoon_products.grf is
# generated outside this repository from the player's own Capitalism Lab files (tools/products_grf/build.py).

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/newgrf)

file(GLOB BUNDLED_BASESET_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/content/baseset/*.tar)
file(GLOB BUNDLED_NEWGRF_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/content/newgrf/*.tar ${CMAKE_CURRENT_SOURCE_DIR}/content/newgrf/*.grf)

if(NOT BUNDLED_NEWGRF_FILES)
    message(WARNING "No bundled content found; run 'python3 bundled/fetch_content.py' to download OpenGFX and the NewGRFs")
endif()

foreach(BUNDLED_SOURCE_FILE IN LISTS BUNDLED_BASESET_FILES BUNDLED_NEWGRF_FILES)
    get_filename_component(BUNDLED_FILE_NAME "${BUNDLED_SOURCE_FILE}" NAME)
    get_filename_component(BUNDLED_SUBDIR "${BUNDLED_SOURCE_FILE}" DIRECTORY)
    get_filename_component(BUNDLED_SUBDIR "${BUNDLED_SUBDIR}" NAME)
    set(BUNDLED_BINARY_FILE "${CMAKE_BINARY_DIR}/${BUNDLED_SUBDIR}/${BUNDLED_FILE_NAME}")

    add_custom_command(OUTPUT ${BUNDLED_BINARY_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${BUNDLED_SOURCE_FILE}
                    ${BUNDLED_BINARY_FILE}
            MAIN_DEPENDENCY ${BUNDLED_SOURCE_FILE}
            COMMENT "Copying bundled ${BUNDLED_FILE_NAME}"
    )

    list(APPEND BUNDLED_BINARY_FILES ${BUNDLED_BINARY_FILE})
endforeach()

add_custom_target(bundled_content_files
        DEPENDS
        ${BUNDLED_BINARY_FILES}
)

add_dependencies(openttd
    bundled_content_files
)
