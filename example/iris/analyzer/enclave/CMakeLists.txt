set(T_SRCS eparser.cpp iris_parser.cpp sgx_plugin.cpp)

add_enclave_library(iris_parser SRCS ${T_SRCS}
  EDL ${PROJECT_SOURCE_DIR}/core/edl/eparser.edl
  EDL_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/core/edl/:${PROJECT_SOURCE_DIR}/stbox/src/:${PROJECT_SOURCE_DIR}/stbox/src/tsgx/crypto/
  )

target_link_libraries(iris_parser stbox_common_t stbox_channel_t ypc_analyzer_t)


enclave_sign(iris_parser KEY enclave_private.pem
  CONFIG enclave.config.xml)
