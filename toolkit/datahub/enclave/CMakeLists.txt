
include_directories(${PROJECT_SOURCE_DIR}/example)
include_directories(${PROJECT_SOURCE_DIR}/toolkit/datahub)
set(E_SRCS enclave.cpp
  ${FF_SRC_PATH}/net/common/archive.cpp
  ${PROJECT_SOURCE_DIR}/core/src/ypc_t/ecommon/signer_verify.cpp
  )

add_enclave_library(edatahub SRCS ${E_SRCS}
  EDL enclave.edl
  EDL_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/core/edl/:${PROJECT_SOURCE_DIR}/stbox/src/)
target_link_libraries(edatahub stbox_common_t stbox_channel_t ypc_analyzer_t)

enclave_sign(edatahub KEY enclave_private.pem
  CONFIG enclave.config.xml)
