
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h> // getpid()

namespace fs = std::filesystem;

static std::string shellEscape(const std::string &s) {
  // Teste rápido: escape básico para espaços e aspas.
  // Em produção, prefira fork/exec com argv (sem shell).
  std::string out = "\"";
  for (char c : s) {
    if (c == '"')
      out += "\\\"";
    else
      out += c;
  }
  out += "\"";
  return out;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Uso: " << argv[0] << " <input.(docx|txt|odt...)> [outdir]\n";
    return 2;
  }

  fs::path input = argv[1];
  fs::path outdir = (argc >= 3) ? fs::path(argv[2]) : fs::path("./out");
  fs::create_directories(outdir);

  if (!fs::exists(input) || !fs::is_regular_file(input)) {
    std::cerr << "Arquivo inválido: " << input << "\n";
    return 3;
  }

  // Perfil isolado (evita conflito em execuções paralelas)
  fs::path profileDir =
      fs::temp_directory_path() / ("lo_profile_" + std::to_string(::getpid()));
  fs::create_directories(profileDir);
  std::string profileUri = "file://" + profileDir.string();

  std::string cmd =
      "soffice --headless --nologo --nodefault --nofirststartwizard "
      "--nolockcheck --norestore --invisible " +
      shellEscape("-env:UserInstallation=" + profileUri) + " " +
      "--convert-to " + shellEscape("pdf:writer_pdf_Export") + " " +
      "--outdir " + shellEscape(outdir.string()) + " " +
      shellEscape(input.string()) + " >/dev/null 2>&1";

  int rc = std::system(cmd.c_str());

  // limpa profile
  std::error_code ec;
  fs::remove_all(profileDir, ec);

  if (rc != 0) {
    std::cerr << "Falha na conversão (rc=" << rc << ")\n";
    return 4;
  }

  fs::path output = outdir / (input.stem().string() + ".pdf");
  if (fs::exists(output)) {
    std::cout << "OK ✅ Gerou: " << output << "\n";
    return 0;
  }

  std::cerr << "Conversão rodou, mas não achei o PDF esperado: " << output
            << "\n";
  return 5;
}
