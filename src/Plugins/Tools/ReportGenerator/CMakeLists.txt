set(target "ReportGenerator")

apx_plugin(DEPENDS lib.ApxGcs QT 
SRCS "."
"rgJS"
"rgUtils")

target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(PATH_TO_CONFIG ${APX_RESOURCES_DIR}/scripts/ReportGenerator/tsconfig.json)
set(TSC_COMMAND tsc --build ${PATH_TO_CONFIG})

execute_process(
    COMMAND ${TSC_COMMAND}
    RESULT_VARIABLE TSC_RESULT
    OUTPUT_VARIABLE TSC_OUTPUT
)

if(NOT TSC_RESULT EQUAL 0)
    message(WARNING " Failed to compile TypeScript files \n${TSC_OUTPUT}")
endif()

