#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gar/gar.hpp>
#include <iostream>
#include <string>
#include <vector>

static void print_version(const std::string &arg0);
static void print_help(const std::string &arg0, const std::string &cmd);

static void archive(const std::vector<std::string> &args);
static void extract(const std::vector<std::string> &args);

static void get_key_from_code(const std::string &code, uint8_t *key);

int main(int argc, char **argv) {
  std::vector<std::string> args(argv, argv + argc);

  if (args.size() < 2) {
    print_help(args[0], "");
  } else if (args[1] == "version") {
    print_version(args[0]);
  } else if (args[1] == "help") {
    if (args.size() < 3) {
      print_help(args[0], "");
    } else {
      print_help(args[0], args[2]);
    }
  } else if (args[1] == "archive") {
    archive(args);
  } else if (args[1] == "extract") {
    extract(args);
  } else {
    print_help(args[0], "");
  }

  return 0;
}

void print_version(const std::string &arg0) {
  std::cout << arg0 << ": " << gar::version.String() << "\n";
}

void print_help(const std::string &arg0, const std::string &cmd) {
  if (cmd.empty()) {
    std::cout << "Usage: " << arg0 << " [COMMAND] [...ARGUMENTS]\n"
              << "\n"
              << "Commands:\n"
              << "  version   Show version information.\n"
              << "  help      Show this message.\n"
              << "  archive   Create a GAR file.\n"
              << "  extract   Extract a file from a GAR file.\n"
              << "\n"
              << "Details about these commands can be found by running:\n"
              << " " << arg0 << " help [COMMAND]\n"
              << "\n";
  } else if (cmd == "version") {
    std::cout << "Usage: " << arg0 << " version\n"
              << "\n"
              << "Like, do you want the version in different formats?\n"
              << "I don't even know how to put git hash into executables.\n"
              << "\n";
  } else if (cmd == "help") {
    std::cout << "Usage: " << arg0 << " help [COMMAND]\n"
              << "\n"
              << "You know what? This is actually comical.\n"
              << "You needed help about getting help.\n"
              << "\n";
  } else if (cmd == "archive") {
    std::cout << "Usage: " << arg0 << " archive [GAR_FILE] [...FILES|DIRS]\n"
              << "\n"
              << "Create an archive file that contains the following files and "
                 "directories.\n"
              << "Search is done recursively.\n"
              << "\n"
              << "Defining `GAR_ENC_KEY` environment variable with 256-bit hex "
                 "code enables encryption of the archive.\n"
              << "\n";
  } else if (cmd == "extract") {
    std::cout
        << "Usage: " << arg0 << " extract [GAR_FILE] [...ITEMS]\n"
        << "\n"
        << "Extract a file from the archive file.\n"
        << "All files in GAR are stored by a hash, and therefore doesn't "
           "know the name of each of the files.\n"
        << "You have to specify every items to extract the whole contents of "
           "a GAR file.\n"
        << "\n"
        << "Defining `GAR_ENC_KEY` environment variable with 256-bit hex code "
           "enables decryption of the archive.\n"
        << "\n";
  } else {
    print_help(arg0, "");
  }
}

void archive(const std::vector<std::string> &args) {
  if (args.size() < 3) {
    print_help(args[0], "archive");
    return;
  }

  gar::Writer writer;

  const char *enc_key = std::getenv("GAR_ENC_KEY");
  if (enc_key != nullptr) {
    if (!gar::InitEnc()) {
      std::cout << "Error: gar::InitEnc()\n";
      return;
    }

    uint8_t key[32];
    get_key_from_code(enc_key, key);
    writer.SetKey(key);
  }

  if (!writer.SetFile(args[2])) {
    std::cout << "Error: gar::Writer::SetFile(\"" << args[2] << "\")\n";
    return;
  }

  for (std::size_t i = 3; i < args.size(); i++) {
    std::filesystem::path path = args[i];
    if (!std::filesystem::exists(path)) {
      std::cout << "Error: \"" << path.generic_string() << "\" not found.\n";
      continue;
    }

    if (std::filesystem::is_directory(path)) {
      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_directory()) {
          continue;
        }

        std::filesystem::path path = entry.path();

        if (!writer.Add(path.generic_string(), path.string())) {
          std::cout << "Error: gar::Writer::Add(\"" << path.generic_string()
                    << "\", \"" << path.string() << "\")\n";
        }
      }
    } else {
      if (!writer.Add(path.generic_string(), path.string())) {
        std::cout << "Error: gar::Writer::Add(\"" << path.generic_string()
                  << "\", \"" << path.string() << "\")\n";
      }
    }
  }

  if (!writer.Finish()) {
    printf("Error: gar::Writer::Finish()\n");
    return;
  }
}

void extract(const std::vector<std::string> &args) {
  if (args.size() < 3) {
    print_help(args[0], "extract");
    return;
  }

  gar::Reader reader;
  const uint8_t *key = nullptr;
  uint8_t k[32];

  const char *enc_key = std::getenv("GAR_ENC_KEY");
  if (enc_key != nullptr) {
    if (!gar::InitEnc()) {
      std::cout << "Error: gar::InitEnc()\n";
      return;
    }

    get_key_from_code(enc_key, k);
    key = k;
  }

  if (!reader.Open(args[2], key)) {
    std::cout << "Error: gar::Reader::Open(\"" << args[2] << "\")\n";
    return;
  }

  for (std::size_t i = 3; i < args.size(); i++) {
    std::vector<std::uint8_t> buffer = reader.Read(args[i]);

    if (buffer.empty()) {
      std::cout << "Error: gar::Reader::Read(\"" << args[i] << "\")\n";
      continue;
    }

    std::filesystem::path path = std::filesystem::path(args[i]).filename();

    std::ofstream os(path, std::ios::binary);
    os.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
  }
}

void get_key_from_code(const std::string &code, uint8_t *key) {
  for (int i = 0; i < 32; i++) {
    try {
      key[i] = std::stoi(code.substr(i * 2, 2), nullptr, 16);
    } catch (...) {
      key[i] = 0;
    }
  }
}
