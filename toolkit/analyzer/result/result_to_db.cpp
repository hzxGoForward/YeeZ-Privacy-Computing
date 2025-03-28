#include "result/result_to_db.h"

using namespace toolkit::analyzer;

result_to_db::result_to_db(const std::string &url, const std::string &usrname,
                           const std::string &passwd, const std::string &dbname,
                           const std::string &request_hash)
    : m_request_hash(request_hash) {
  m_db = std::make_unique<request_db>(url, usrname, passwd, dbname);
}

void result_to_db::write_to_target(const ypc::bref &encrypted_result,
                                   const ypc::bref &result_signature,
                                   const ypc::bref &data_hash) {
  auto *ptr = m_db->db_engine_ptr();
  auto info = request_data_table::select<::encrypted_skey, ::encrypted_input,
                                         ::provider_pkey, ::enclave_hash,
                                         ::analyzer_pkey, ::forward_sig>(ptr)
                  .where(request_hash::eq(m_request_hash))
                  .eval();
  request_data_item_t item;
  if (!info.empty()) {
    assert(info.size() == 1);
    item.set<::encrypted_skey, ::encrypted_input, ::provider_pkey,
             ::enclave_hash, ::analyzer_pkey, forward_sig>(
        info[0].get<::encrypted_skey>(), info[0].get<::encrypted_input>(),
        info[0].get<::provider_pkey>(), info[0].get<::enclave_hash>(),
        info[0].get<::analyzer_pkey>(), info[0].get<::forward_sig>());
  }
  item.set<::request_hash, ::status, ::encrypted_result, ::result_signature,
           ::data_hash>(m_request_hash, 1, ypc::to_hex(encrypted_result),
                        ypc::to_hex(result_signature), ypc::to_hex(data_hash));
  request_data_table::row_collection_type rs;
  rs.push_back(item);
  request_data_table::insert_or_replace_rows(m_db->db_engine_ptr(), rs);
}

void result_to_db::read_from_target(ypc::bytes &encrypted_result,
                                    ypc::bytes &result_signature,
                                    ypc::bytes &data_hash) {
  auto *ptr = m_db->db_engine_ptr();
  auto info = request_data_table::select<::encrypted_result, ::result_signature,
                                         ::data_hash>(ptr)
                  .where(request_hash::eq(m_request_hash))
                  .eval();
  if (!info.empty()) {
    assert(info.size() == 1);
    encrypted_result = ypc::bytes::from_hex(info[0].get<::encrypted_result>());
    result_signature = ypc::bytes::from_hex(info[0].get<::result_signature>());
    data_hash = ypc::bytes::from_hex(info[0].get<::data_hash>());
  }
}
