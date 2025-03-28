#include "stbox/eth/eth_hash.h"
#include "ypc/base64.h"
#include "ypc/limits.h"
#include "ypc/ntobject_file.h"
#include "ypc/privacy_data_reader.h"
#include "ypc/sealed_file.h"
#include "ypc/sgx/datahub_sgx_module.h"
#include "ypc/sha.h"
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>

using stx_status = stbox::stx_status;
using namespace ypc;
void seal_file_for_parallel(const std::string &plugin, const std::string &file,
                            const std::string &sealed_file_path,
                            const std::string &sealer_path,
                            stbox::bytes &data_hash) {
  // Read origin file
  // use sgx to seal file
  privacy_data_reader reader(plugin, file);
  datahub_sgx_module sm(sealer_path.c_str());
  // simple_sealed_file sf(sealed_file_path, false);
  // std::string k(file);
  // k = k + std::string(sealer_path);

  int concurrency = std::thread::hardware_concurrency();

  sfm_t data;
  data.set<sfm_num>(concurrency);

  std::cout << concurrency << " cores, split into " << concurrency << " files."
            << std::endl;

  // magic string here!
  stbox::bytes std_hash =
      stbox::eth::keccak256_hash(stbox::string_to_byte("Fidelius"));

  uint64_t item_number = reader.get_item_number();
  if (item_number == 0) {
    std::cout << "no data to read. " << std::endl;
    exit(-1);
  }

  auto check_item_data_size = [](const std::string &_data) {
    if (_data.size() > ypc::max_item_size) {
      std::cout << "only support item size that smaller than " << max_item_size
                << " bytes!" << std::endl;
      exit(-1);
    }
  };

  size_t items_per_file = item_number / concurrency;
  std::cout << item_number << " items in total, " << items_per_file
            << " items per file." << std::endl;

  simple_sealed_file *cur_sf = nullptr; //(sealed_file_path, false);
  stbox::bytes k = std_hash;
  std::cout << "Reading " << item_number << " items ..." << std::endl;
  boost::progress_display pd(item_number);

  std::vector<sfm_item_t> items;
  for (size_t i = 0; i < item_number; ++i) {
    if (i % items_per_file == 0 &&
        ((i / items_per_file) + 1) * items_per_file <= item_number) {
      if (cur_sf) {
        delete cur_sf;
      }
      std::string file_path =
          sealed_file_path + "." + std::to_string(i / items_per_file);
      std::cout << "file " << i / items_per_file << " path: " << file_path
                << std::endl;
      cur_sf = new simple_sealed_file(file_path, false);
      k = std_hash;
      if (i != 0) {
        items.back().set<sfm_hash>(stbox::byte_to_string(k));
      }
      sfm_item_t item;
      item.set<sfm_path>(file_path);
      item.set<sfm_index>(i / items_per_file);
      items.push_back(item);
    }

    std::string item_data = reader.read_item_data();
    check_item_data_size(item_data);
    std::string s = sm.seal_data(item_data.c_str(), item_data.size());
    cur_sf->write_item(s);
    k = k + item_data;
    k = stbox::eth::keccak256_hash(k);
    ++pd;
  }
  items.back().set<sfm_hash>(stbox::byte_to_string(k));

  k = std_hash;
  for (int i = 0; i < items.size(); i++) {
    k = k + items[i].get<sfm_hash>();
  }
  k = stbox::eth::keccak256_hash(k);
  data.set<sfm_hash>(stbox::byte_to_string(k));
  data.set<sfm_items>(items);
  data.set<sfm_num>(concurrency);

  ypc::ntobject_file<sfm_t> fs(sealed_file_path);
  fs.data() = data;
  fs.write_to();
  std::cout << "\nDone read data" << std::endl;
}

void seal_file(const std::string &plugin, const std::string &file,
               const std::string &sealed_file_path,
               const std::string &sealer_path, stbox::bytes &data_hash) {
  // Read origin file use sgx to seal file
  privacy_data_reader reader(plugin, file);
  datahub_sgx_module sm(sealer_path.c_str());
  simple_sealed_file sf(sealed_file_path, false);
  std::string k(file);
  k = k + std::string(sealer_path);

  // magic string here!
  data_hash = stbox::eth::keccak256_hash(stbox::string_to_byte("Fidelius"));

  std::string item_data = reader.read_item_data();
  if (item_data.size() > ypc::max_item_size) {
    std::cout << "only support item size that smaller than " << max_item_size
              << " bytes!" << std::endl;
    exit(-1);
  }
  uint64_t item_number = reader.get_item_number();

  std::cout << "Reading " << item_number << " items ..." << std::endl;
  boost::progress_display pd(item_number);
  while (!item_data.empty()) {
    std::string s = sm.seal_data(item_data.c_str(), item_data.size());
    sf.write_item(s);
    stbox::bytes k = data_hash + item_data;
    data_hash = stbox::eth::keccak256_hash(k);
    // data_hash = SHA256(s.c_str(), s.size());
    item_data = reader.read_item_data();
    ++pd;
  }
  std::cout << "\nDone read data" << std::endl;
}

boost::program_options::variables_map parse_command_line(int argc,
                                                         char *argv[]) {
  namespace bp = boost::program_options;
  bp::options_description all("Yeez Privacy Data Hub options");

  // clang-format off
  all.add_options()
    ("help", "help message")
    ("data-url", bp::value<std::string>(), "Data URL")
    ("plugin-path", bp::value<std::string>(), "shared library for read data")
    ("sealed-data-url", bp::value<std::string>(), "Sealed Data URL")
    ("sealer-path", bp::value<std::string>(), "sealer path")
    ("output", bp::value<std::string>(), "output file path")
    ("enable-parallel", "enable partationing sealed file into N blocks, where N is the number for CPU cores");
  // clang-format on

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, all), vm);

  if (vm.count("help")) {
    std::cout << all << std::endl;
    exit(-1);
  }
  return vm;
}

int main(int argc, char *argv[]) {
  boost::program_options::variables_map vm;
  try {
    vm = parse_command_line(argc, argv);
  } catch (...) {
    std::cout << "invalid cmd line parameters!" << std::endl;
    return -1;
  }
  if (!vm.count("data-url")) {
    std::cout << "data not specified!" << std::endl;
    return -1;
  }
  if (!vm.count("sealed-data-url")) {
    std::cout << "sealed data url not specified" << std::endl;
    return -1;
  }
  if (!vm.count("sealer-path")) {
    std::cout << "sealer not specified" << std::endl;
    return -1;
  }
  if (!vm.count("output")) {
    std::cout << "output not specified" << std::endl;
    return -1;
  }
  if (!vm.count("plugin-path")) {
    std::cout << "library not specified" << std::endl;
  }

  std::string plugin = vm["plugin-path"].as<std::string>();
  std::string iris_file = vm["data-url"].as<std::string>();
  std::string sealed_iris_file = vm["sealed-data-url"].as<std::string>();
  std::string enclave_file = vm["sealer-path"].as<std::string>();
  std::string output = vm["output"].as<std::string>();

  stbox::bytes data_hash;
  std::ofstream ofs;
  ofs.open(output);
  if (!ofs.is_open()) {
    std::cout << "Cannot open file " << output << "\n";
    return -1;
  }
  ofs.close();

  if (vm.count("enable-parallel")) {
    seal_file_for_parallel(plugin, iris_file, sealed_iris_file, enclave_file,
                           data_hash);
  } else {
    seal_file(plugin, iris_file, sealed_iris_file, enclave_file, data_hash);
  }

  ofs.open(output);
  if (!ofs.is_open()) {
    std::cout << "Cannot open file " << output << "\n";
    return -1;
  }
  ofs << "data_url"
      << " = " << iris_file << "\n";
  ofs << "sealed_data_url"
      << " = " << sealed_iris_file << "\n";
  ofs << "sealer_enclave"
      << " = " << enclave_file << "\n";
  ofs << "data_id"
      << " = " << ::ypc::to_hex(data_hash) << "\n";

  privacy_data_reader reader(plugin, iris_file);
  ofs << "item_num"
      << " = " << reader.get_item_number() << "\n";

  // sample and format are optional
  bytes sample = reader.get_sample_data();
  if (sample.size() > 0) {
    ofs << "sample_data"
        << " = " << ypc::to_hex(sample) << "\n";
  }
  std::string format = reader.get_data_format();
  if (format.size() > 0) {
    ofs << " data_fromat"
        << " = " << format << "\n";
  }
  ofs.close();

  std::cout << "done sealing" << std::endl;
  return 0;
}

extern "C" {
uint32_t next_sealed_item_data(uint8_t **data, uint32_t *len);
void free_sealed_item_data(uint8_t *data);
}

uint32_t next_sealed_item_data(uint8_t **data, uint32_t *len) { return 0; }
void free_sealed_item_data(uint8_t *data) {}
