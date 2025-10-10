#pragma once
#include <wx/stackwalk.h>
#include <vector>
#include <wx/utils.h> // for wxExecute
#include <sstream>
#include <wx/filename.h>
#include <wx/stdpaths.h>

class wxStackTrace final : public wxStackWalker
{
public:
  using StackFrameCollection = std::vector<wxStackFrame>;
  using SizeType = typename StackFrameCollection::size_type;

  // wxStackTrace() noexcept : wxStackTrace(1, false) {}
  explicit wxStackTrace(bool needFileInfo) noexcept : wxStackTrace(1, needFileInfo) {}

  wxStackTrace(SizeType skipFrames, bool needFileInfo) noexcept : wxStackWalker{}, needFileInfo{needFileInfo}
  {
    try
    {
      Walk(skipFrames + 0);
    }
    catch (...)
    {
      frames.clear();
    }
  }

  wxString ResolveSymbol(void *address)
  {
    // wxString cmd = wxString::Format("addr2line -e %s -f -p %p", wxGetFullModuleName(), address);
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxString cmd = wxString::Format("addr2line -e %s -f -p %p", exePath, address);
    wxArrayString output;
    wxExecute(cmd, output, wxEXEC_SYNC);
    if (!output.empty())
      return output[0];
    return "<unknown>";
  }
  wxString ToString() const noexcept
  {
    wxString result;
    for (const auto &frame : frames)
    {
      result += wxString::Format("Frame %lu: %s\n",
                                 (unsigned long)frame.GetLevel(),
                                 frame.GetName());
      if (needFileInfo)
      {
        if (!frame.GetFileName().empty())
          result += wxString::Format("  at %s:%lu\n",
                                     frame.GetFileName(),
                                     (unsigned long)frame.GetLine());
      }
    }
    return result;
  }

private:
  // void OnStackFrame(const wxStackFrame &frame) override
  // {
  //   frames.push_back(frame);
  // }
  void OnStackFrame(const wxStackFrame &frame) override
  {
    frames.push_back(frame);
    // Optionally print right away for debugging
    wxPrintf("Captured frame %lu: %s\n",
             (unsigned long)frame.GetLevel(),
             ResolveSymbol((void *)frame.GetAddress()));
  }

  StackFrameCollection frames;
  bool needFileInfo = false;
};