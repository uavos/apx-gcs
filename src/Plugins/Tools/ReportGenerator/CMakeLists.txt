set(target "ReportGenerator")

apx_plugin(DEPENDS lib.ApxGcs QT 
SRCS "."
"rgJS"
"rgUtils")

target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(PATH_TO_TYPESCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/rgTypeScript)
set(PATH_TO_CONFIG ${PATH_TO_TYPESCRIPT}/tsconfig.json)
set(GENERATED_FILE_NAME bundle.gen.js)
set(PATH_TO_GENERATED_FILE ${PATH_TO_TYPESCRIPT}/generated/${GENERATOR_FILE_NAME})
set(PATH_TO_REPORTGENERATOR_RESOURCE ${APX_RESOURCES_DIR}/scripts/ReportGenerator)
set(TSC_COMMAND tsc --build ${PATH_TO_CONFIG})

execute_process(
    COMMAND ${TSC_COMMAND}
    RESULT_VARIABLE TSC_RESULT
    OUTPUT_VARIABLE TSC_OUTPUT
)

if(NOT TSC_RESULT EQUAL 0)
    message(WARNING " Failed to compile TypeScript files \n${TSC_OUTPUT}")
else()
    file(COPY ${PATH_TO_GENERATED_FILE} DESTINATION ${PATH_TO_REPORTGENERATOR_RESOURCE})
endif()

