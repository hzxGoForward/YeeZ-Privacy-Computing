#include "config.h"
#include "ypc/configuration.h"
#include <fstream>

namespace toolkit {
namespace shuttle {

namespace bp = ::boost::program_options;
configure::configure() : m_options("Configuration File Options") {
  // clang-format off
  m_options.add_options()
    ("general.mode", bp::value<std::string>(), "file/https")
    ("data.sealed_data_url", bp::value<std::string>(), "data url")
    ("blockchain.data_type_header", bp::value<std::string>(), "c++ header file describes data type")
    ("blockchain.data_parser_lib", bp::value<std::string>(), ".a data parser used by SGX")
    ("blockchain.data_id", bp::value<std::string>(), "data id generated by SGX")
    ("blockchain.data_desc", bp::value<std::string>(), "data description")
    ("exec.parser_path", bp::value<std::string>(), "executable parser path")
    ("exec.params", bp::value<std::string>(), "params for the parser");
  // clang-format on
}
configure::~configure() {}

void configure::parse_config_file(const std::string &file) {
  std::string fp = ::ypc::configuration::instance().find_db_config_file(file);

  std::ifstream ifs{fp};
  if (!ifs) {
    std::stringstream ss;
    ss << " cannot file or open file " << file;
    throw std::runtime_error(ss.str());
  }
  bp::variables_map vm;
  auto check = [&vm, fp](const std::string &s) -> std::string {
    if (!vm.count(s)) {
      std::stringstream ss;
      ss << " No " << s << " in config file " << fp;
      throw std::runtime_error(ss.str());
    }
    return vm[s].as<std::string>();
  };
  bp::store(bp::parse_config_file(ifs, m_options, true), vm);
  m_mode = check("general.mode");
  m_sealed_data_url = check("data.sealed_data_url");
  // m_bc_data_type_header = check("blockchain.data_type_header");
  // m_bc_data_parser = check("blockchain.data_parser_lib");
  m_bc_data_id = check("blockchain.data_id");
  m_bc_data_desc = check("blockchain.data_desc");
  m_exec_parser_path = check("exec.parser_path");
  m_exec_params = check("exec.params");
}

std::string configure::help_message() const {
  std::stringstream ss;
  ss << m_options << std::endl;
  return ss.str();
}
} // namespace shuttle
} // namespace toolkit