#include "stbox/tsgx/log.h"
#include "stbox_t.h"

namespace stbox {
Logger::~Logger() {
  ocall_log_string(m_log_rank, m_ss.c_str());
}

Logger &Logger::start(const std::string &file, uint32_t line,
                      const std::string &function) {
  std::string::size_type pos = file.find_last_of("/");
  std::string f = file.substr(pos + 1);

  m_ss = file + std::string(":") + std::to_string(line) + std::string(":") +
         function + std::string(", ");
  return *this;
}
Logger &Logger::operator<<(const std::string &t) {
  m_ss = m_ss + t;
  return *this;
}
Logger &Logger::operator<<(stx_status t) {
  m_ss = m_ss + std::to_string(t);
  return *this;
}
Logger &Logger::operator<<(sgx_status_t t) {
  m_ss = m_ss + std::string(sgx_status_string(t));
  return *this;
}
} // namespace stbox
