#include "keymgr_sgx_module.h"
#include "ekeymgr_u.h"
#include "sgx_urts.h"
#include <stdexcept>

keymgr_sgx_module::keymgr_sgx_module(const char *mod_path)
    : ::stbox::sgx_module(mod_path) {}
keymgr_sgx_module::~keymgr_sgx_module() {}

uint32_t keymgr_sgx_module::get_secp256k1_sealed_private_key_size() {
  return ecall<uint32_t>(::get_secp256k1_sealed_private_key_size);
}
uint32_t
keymgr_sgx_module::generate_secp256k1_key_pair(bref &_pkey,
                                               bref &_sealed_private_key) {
  uint8_t *public_key;
  uint32_t pkey_size;
  uint8_t *sealed_private_key;
  uint32_t sealed_size;
  stbox::buffer_length_t buf_pub(&pkey_size, &public_key,
                                 get_secp256k1_public_key_size);
  stbox::buffer_length_t buf_sec(&sealed_size, &sealed_private_key,
                                 ::get_secp256k1_sealed_private_key_size);
  auto t = ecall<uint32_t>(::generate_secp256k1_key_pair, stbox::xmem(buf_pub),
                           stbox::xlen(buf_pub), stbox::xmem(buf_sec),
                           stbox::xlen(buf_sec));

  _pkey = bref(public_key, pkey_size);
  _sealed_private_key = bref(sealed_private_key, sealed_size);
  return t;
}

uint32_t keymgr_sgx_module::sign_message(const uint8_t *sealed_private_key,
                                         uint32_t sealed_size,
                                         const uint8_t *data,
                                         uint32_t data_size, bref &_sig) {
  uint8_t *sig;
  uint32_t sig_size;
  stbox::buffer_length_t buf_sig(&sig_size, &sig, get_secp256k1_signature_size);
  auto t = ecall<uint32_t>(::sign_message, (uint8_t *)sealed_private_key,
                           sealed_size, (uint8_t *)data, data_size,
                           stbox::xmem(buf_sig), stbox::xlen(buf_sig));

  _sig = bref(sig, sig_size);
  return t;
}

uint32_t keymgr_sgx_module::verify_signature(
    const uint8_t *data, uint32_t data_size, const uint8_t *sig,
    uint32_t sig_size, const uint8_t *public_key, uint32_t pkey_size) {
  return ecall<uint32_t>(::verify_signature, (uint8_t *)data, data_size,
                         (uint8_t *)sig, sig_size, (uint8_t *)public_key,
                         pkey_size);
}

uint32_t keymgr_sgx_module::encrypt_message(const uint8_t *public_key,
                                            uint32_t pkey_size,
                                            const uint8_t *data,
                                            uint32_t data_size,
                                            ypc::bref &_cipher) {

  uint8_t *cipher;
  uint32_t cipher_size;

  stbox::buffer_length_t buf_cip(&cipher_size, &cipher,
                                 ::get_rijndael128GCM_encrypt_size, data_size);

  auto t = ecall<uint32_t>(::encrypt_message, (uint8_t *)public_key, pkey_size,
                           (uint8_t *)data, data_size, stbox::xmem(buf_cip),
                           stbox::xlen(buf_cip));
  _cipher = ypc::bref(cipher, cipher_size);
  return t;
}

uint32_t keymgr_sgx_module::decrypt_message(const uint8_t *sealed_private_key,
                                            uint32_t sealed_size,
                                            const uint8_t *cipher,
                                            uint32_t cipher_size, bref &_data) {
  uint8_t *data;
  uint32_t data_size;
  stbox::buffer_length_t buf_data(&data_size, &data,
                                  get_rijndael128GCM_decrypt_size, cipher_size);
  auto t = ecall<uint32_t>(::decrypt_message, (uint8_t *)sealed_private_key,
                           sealed_size, (uint8_t *)cipher, cipher_size,
                           stbox::xmem(buf_data), stbox::xlen(buf_data));

  _data = bref(data, data_size);
  return t;
}

uint32_t keymgr_sgx_module::backup_private_key(
    const uint8_t *sealed_private_key, uint32_t sealed_size,
    const uint8_t *pub_key, uint32_t pkey_size, bref &_backup_private_key) {
  uint32_t skey_size = 32;
  uint8_t *backup_private_key;
  uint32_t bp_size;
  stbox::buffer_length_t buf_bak(&bp_size, &backup_private_key,
                                 ::get_rijndael128GCM_encrypt_size, skey_size);
  auto t = ecall<uint32_t>(::backup_private_key, (uint8_t *)sealed_private_key,
                           sealed_size, (uint8_t *)pub_key, pkey_size,
                           stbox::xmem(buf_bak), stbox::xlen(buf_bak));

  _backup_private_key = bref(backup_private_key, bp_size);
  return t;
}

uint32_t keymgr_sgx_module::restore_private_key(
    const uint8_t *backup_private_key, uint32_t bp_size,
    const uint8_t *priv_key, uint32_t skey_size, bref &_sealed_private_key) {

  uint32_t sealed_size;
  uint8_t *sealed_private_key;
  stbox::buffer_length_t buf_res(&sealed_size, &sealed_private_key,
                                 ::get_secp256k1_sealed_private_key_size);
  auto t = ecall<uint32_t>(::restore_private_key, (uint8_t *)backup_private_key,
                           bp_size, (uint8_t *)priv_key, skey_size,
                           stbox::xmem(buf_res), stbox::xlen(buf_res));
  _sealed_private_key = bref(sealed_private_key, sealed_size);
  return t;
}

uint32_t keymgr_sgx_module::session_request(sgx_dh_msg1_t *dh_msg1,
                                            uint32_t *session_id) {
  return ecall<uint32_t>(::session_request, dh_msg1, session_id);
}
uint32_t keymgr_sgx_module::exchange_report(sgx_dh_msg2_t *dh_msg2,
                                            sgx_dh_msg3_t *dh_msg3,
                                            uint32_t session_id) {
  return ecall<uint32_t>(::exchange_report, dh_msg2, dh_msg3, session_id);
}
uint32_t keymgr_sgx_module::generate_response(secure_message_t *req_message,
                                              size_t req_message_size,
                                              size_t max_payload_size,
                                              secure_message_t *resp_message,
                                              size_t resp_message_size,
                                              uint32_t session_id) {
  return ecall<uint32_t>(::generate_response, req_message, req_message_size,
                         max_payload_size, resp_message, resp_message_size,
                         session_id);
}
uint32_t keymgr_sgx_module::end_session(uint32_t session_id) {
  return ecall<uint32_t>(::end_session, session_id);
}

uint32_t keymgr_sgx_module::forward_message(
    uint32_t msg_id, const uint8_t *cipher, uint32_t cipher_size,
    const uint8_t *epublic_key, uint32_t epkey_size, const uint8_t *ehash,
    uint32_t ehash_size, const uint8_t *verify_key, uint32_t vpkey_size,
    const uint8_t *sig, uint32_t sig_size) {
  return ecall<uint32_t>(::forward_message, msg_id, (uint8_t *)cipher,
                         cipher_size, (uint8_t *)epublic_key, epkey_size,
                         (uint8_t *)ehash, ehash_size, (uint8_t *)verify_key,
                         vpkey_size, (uint8_t *)sig, sig_size);
}
