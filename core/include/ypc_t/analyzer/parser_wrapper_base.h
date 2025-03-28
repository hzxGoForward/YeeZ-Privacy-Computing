#pragma once
#include "sgx_trts.h"
#include "stbox/stx_status.h"
#include "stbox/tsgx/channel/dh_session_initiator.h"
#include "stbox/tsgx/ocall.h"
#include "ypc_t/ecommon/package.h"
#include <string.h>

#include "stbox/ebyte.h"
#include "stdlib.h"
#include <stdio.h> //vsnprintf

namespace ypc {

class parser_wrapper_base {
public:
  parser_wrapper_base();
  parser_wrapper_base(const parser_wrapper_base &) = delete;
  parser_wrapper_base &operator=(const parser_wrapper_base &) = delete;

  virtual ~parser_wrapper_base();
  virtual uint32_t begin_parse_data_item();
  virtual uint32_t parse_data_item(const uint8_t *sealed_data, uint32_t len);
  virtual uint32_t end_parse_data_item();

  virtual inline uint32_t get_encrypted_result_size() const {
    return m_encrypted_result_str.size();
  }
  virtual uint32_t get_encrypted_result_and_signature(uint8_t *encrypted_res,
                                                      uint32_t res_size,
                                                      uint8_t *result_sig,
                                                      uint32_t sig_size);

  virtual uint32_t add_block_parse_result(uint16_t block_index,
                                          uint8_t *block_result,
                                          uint32_t res_size, uint8_t *data_hash,
                                          uint32_t hash_size, uint8_t *sig,
                                          uint32_t sig_size);

  virtual uint32_t merge_parse_result(const uint8_t *encrypted_param,
                                      uint32_t len);

  virtual bool user_def_block_result_merge(
      const std::vector<std::string> &block_results) = 0;

  inline bool need_continue() { return m_continue; }

protected:
  stbox::stx_status request_private_key();
  stbox::stx_status decrypt_param(const uint8_t *encrypted_param, uint32_t len);

protected:
  std::unique_ptr<stbox::dh_session_initiator> m_datahub_session;
  std::unique_ptr<stbox::dh_session_initiator> m_keymgr_session;

  std::string m_response_str;
  std::string m_result_str;
  std::string m_private_key;
  std::string m_param;
  std::string m_encrypted_param;
  uint64_t m_cost_gas;
  std::string m_encrypted_result_str;
  std::string m_result_signature_str;

  //! for merge block result

  struct block_meta_t {
    std::string encrypted_result;
    stbox::bytes data_hash;
    stbox::bytes sig;
  };
  std::unordered_map<uint16_t, block_meta_t> m_block_results;
  bool m_continue;
};

} // namespace ypc
