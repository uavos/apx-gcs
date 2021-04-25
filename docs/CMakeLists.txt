set(docs_bin ${CMAKE_CURRENT_BINARY_DIR}/out)

set(docs_dest ${docs_bin}/_docs/gcs)
set(docs_data ${docs_bin}/_data/gcs.yml)

set(docs_files)
get_property(docs_files GLOBAL PROPERTY "APX_PLUGINS_DOCS")

add_custom_command(
    OUTPUT "${docs_data}" "${docs_data}.always" # ensure always build
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/pages/gcs.yml ${docs_data}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pages ${docs_dest}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${docs_dest}/gcs.yml
    COMMAND ${PYTHON_EXECUTABLE} ${APX_SHARED_DIR}/tools/make_docs.py --dest ${docs_dest} --data ${docs_data} --srcs ${docs_files}
    DEPENDS ${docs_files} ${APX_SHARED_DIR}/tools/make_docs.py
    VERBATIM
)
add_custom_target(docs DEPENDS ${docs_data})

# test in apx-docs local repo
# set(docs_test ${docs_bin}/apx-docs)
set(docs_test ${PROJECT_SOURCE_DIR}/../apx-docs)
set(docs_test_data ${docs_test}/_data/gcs.yml)

# if(NOT EXISTS ${docs_test})
#     message(FATAL_ERROR "Missing ${docs_test}")
# endif()

add_custom_command(
    OUTPUT "${docs_test_data}" "${docs_test_data}.always" # ensure always build
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${docs_bin} ${docs_test}
    DEPENDS ${docs} ${docs_data}
    VERBATIM
)
add_custom_target(docs-test DEPENDS ${docs_test_data})
