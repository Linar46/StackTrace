static std::string TryDemangle(const std::string &name) // for addr2line results
  {
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status),
        std::free);
    return (status == 0 && demangled) ? demangled.get() : name;
  }

  static wxString DemangleSymbol(const wxString &line)
  {
    std::string text = std::string(line.mb_str());
    auto pos = text.find(" at ");
    if (pos != std::string::npos)
    {
      std::string mangled = text.substr(0, pos);
      std::string fileInfo = text.substr(pos); // includes " at ..."

      // Demangle only if it looks like a mangled C++ symbol
      if (mangled.rfind("_Z", 0) == 0)
        mangled = TryDemangle(mangled);

      text = mangled + fileInfo;
    }
    else if (text.rfind("_Z", 0) == 0)
    {
      text = TryDemangle(text);
    }

    return wxString::FromUTF8(text.c_str());
  }