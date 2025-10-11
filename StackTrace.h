#pragma once
#include <wx/stackwalk.h>
#include <vector>
#include <wx/utils.h> //wxEcecute
#include <sstream>
#include <fstream>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <cstdint>
#include <wx/log.h>
#include <cxxabi.h> // for __cxa_demangle
#include <memory>
#include <string>

class wxStackTrace final : public wxStackWalker
{
public:
  using StackFrameCollection = std::vector<wxStackFrame>;
  using SizeType = typename StackFrameCollection::size_type;

  explicit wxStackTrace(bool needFileInfo = false) noexcept
      : wxStackWalker{}, needFileInfo{needFileInfo}
  {
    Capture(1);
  }

  void Capture(SizeType skipFrames)
  {
    try
    {
      Walk(skipFrames);
    }
    catch (...)
    {
      frames.clear();
    }
  }

  static uintptr_t GetModuleBase()
  {
    std::ifstream maps("/proc/self/maps");
    std::string line;
    while (std::getline(maps, line))
    {
      if (line.find("r") != std::string::npos)
      {
        std::stringstream ss(line);
        std::string addrRange;
        ss >> addrRange;
        std::string baseStr = addrRange.substr(0, addrRange.find('-'));
        return std::stoull(baseStr, nullptr, 16);
      }
    }
    return 0;
  }

  wxString ResolveSymbol(void *address) const
  {
    uintptr_t base = GetModuleBase();
    uintptr_t offset = reinterpret_cast<uintptr_t>(address) - base;

    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxArrayString output;
    wxString cmd = wxString::Format(
        // "addr2line -e \"%s\" -f -p 0x%llx",
        "llvm-symbolizer -e \"%s\" --inlining --demangle --functions --pretty-print 0x%llx", // prints clearly added for return-type
        exePath.mb_str(),
        static_cast<unsigned long long>(offset));

    wxExecute(cmd, output, wxEXEC_SYNC);

    if (output.empty())
      return "<unknown>";

    // Demangle possible mangled symbol in output[0]
    // return DemangleSymbol(output[0]); // for addr2line
    return wxString::FromUTF8(output[0].mb_str());
  }

private:
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
  void OnStackFrame(const wxStackFrame &frame) override
  {
    frames.push_back(frame);
    wxLogMessage("[%lu]: %s\n", (unsigned long)frame.GetLevel(), ResolveSymbol((void *)frame.GetAddress()));
  }

  StackFrameCollection frames;
  bool needFileInfo = false;
};
