file(GLOB scripts
  "*.sh"
)
file(COPY ${scripts} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})