# add_shader(TARGET SHADER OUTPUT DEFINE...)
function(add_shader)
  list(POP_FRONT ARGV TARGET SHADER OUTPUT)
  list(TRANSFORM ARGV PREPEND "-D" OUTPUT_VARIABLE DEFINES)

  get_filename_component(SHADER ${SHADER} ABSOLUTE)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/generated)
  set(COMMAND
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    --target-env spirv1.5
    -V
    --vn ${OUTPUT}
    -o ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/${OUTPUT}.h
    ${DEFINES}
    ${SHADER}
  )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/${OUTPUT}.h
    COMMAND echo ${COMMAND}
    COMMAND ${COMMAND}
    DEPENDS ${SHADER}
    COMMENT "Compiling ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/${OUTPUT}.h"
  )

  add_custom_target(${OUTPUT} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/${OUTPUT}.h)
  add_dependencies(${TARGET} ${OUTPUT})
endfunction()
