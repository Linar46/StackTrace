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
        "addr2line -e \"%s\" -f -p 0x%llx",
        exePath.mb_str(),
        static_cast<unsigned long long>(offset));

    wxExecute(cmd, output, wxEXEC_SYNC);
    return output.empty() ? "<unknown>" : output[0];
  }

private:
  void OnStackFrame(const wxStackFrame &frame) override
  {
    frames.push_back(frame);
    // wxPrintf("Captured frame %lu: %s\n", (unsigned long)frame.GetLevel(), ResolveSymbol((void *)frame.GetAddress()));
    // wxLogMessage("Function: %s", frame.GetName());
    // wxLogMessage("Location: %s:%d", frame.GetFileName(), frame.GetLine());
    wxLogMessage("[%lu]: %s\n", (unsigned long)frame.GetLevel(), ResolveSymbol((void *)frame.GetAddress()));
  }

  StackFrameCollection frames;
  bool needFileInfo = false;
};
