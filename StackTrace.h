#pragma once
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <cxxabi.h> // for __cxa_demangle
#include <unistd.h>
#include <wx/stackwalk.h>
#include <wx/utils.h> //wxEcecute
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/log.h>
#include <libdwarf.h>
#include <dwarf.h>
#include <fcntl.h>

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

  static std::string Demangle(const char *name)
  {
    int status = 0;
    char *dem = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    std::string out = (status == 0 && dem) ? dem : name;
    free(dem);
    return out;
  }

  static std::string ResolveTypeName(Dwarf_Debug dbg, Dwarf_Die type_die)
  {
    if (!type_die)
      return "";

    char *name = nullptr;
    Dwarf_Error err;

    if (dwarf_diename(type_die, &name, &err) == DW_DLV_OK && name)
      return Demangle(name);

    // If it’s a typedef or pointer, resolve its underlying type
    Dwarf_Attribute attr;
    if (dwarf_attr(type_die, DW_AT_type, &attr, &err) == DW_DLV_OK)
    {
      Dwarf_Off ref;
      if (dwarf_global_formref(attr, &ref, &err) == DW_DLV_OK)
      {
        Dwarf_Die next_die;
        if (dwarf_offdie(dbg, ref, &next_die, &err) == DW_DLV_OK)
          return ResolveTypeName(dbg, next_die);
      }
    }
    return "";
  }

  static std::string GetReturnTypeForAddress(uintptr_t address)
  {
    uintptr_t base = 0;
    {
      std::ifstream maps("/proc/self/maps");
      std::string line;
      while (std::getline(maps, line))
      {
        if (line.find('r') != std::string::npos)
        {
          std::stringstream ss(line);
          std::string addrRange;
          ss >> addrRange;
          std::string baseStr = addrRange.substr(0, addrRange.find('-'));
          base = std::stoull(baseStr, nullptr, 16);
          break;
        }
      }
    }

    uintptr_t offset = address - base;

    std::string exePath = wxStandardPaths::Get().GetExecutablePath().ToStdString();
    int fd = open(exePath.c_str(), O_RDONLY);
    if (fd < 0)
      return "";

    Dwarf_Debug dbg = nullptr;
    Dwarf_Error err;
    if (dwarf_init(fd, DW_DLC_READ, nullptr, nullptr, &dbg, &err) != DW_DLV_OK)
    {
      close(fd);
      return "";
    }

    Dwarf_Unsigned cu_header_length, abbrev_offset, next_cu_header;
    Dwarf_Half version_stamp, address_size;

    while (dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp,
                                &abbrev_offset, &address_size,
                                &next_cu_header, &err) == DW_DLV_OK)
    {
      Dwarf_Die cu_die = nullptr;
      if (dwarf_siblingof(dbg, nullptr, &cu_die, &err) != DW_DLV_OK)
        continue;

      std::vector<Dwarf_Die> stack;
      stack.push_back(cu_die);

      while (!stack.empty())
      {
        Dwarf_Die die = stack.back();
        stack.pop_back();

        Dwarf_Half tag;
        if (dwarf_tag(die, &tag, &err) != DW_DLV_OK)
          continue;

        if (tag == DW_TAG_subprogram)
        {
          Dwarf_Addr lowpc = 0, highpc = 0;
          Dwarf_Attribute attr_high;
          if (dwarf_lowpc(die, &lowpc, &err) == DW_DLV_OK &&
              dwarf_attr(die, DW_AT_high_pc, &attr_high, &err) == DW_DLV_OK)
          {
            Dwarf_Half form;
            Dwarf_Unsigned high_val = 0;
            dwarf_whatform(attr_high, &form, &err);

            if (form == DW_FORM_addr)
              dwarf_formaddr(attr_high, &highpc, &err);
            else
            {
              // Offset form
              dwarf_formudata(attr_high, &high_val, &err);
              highpc = lowpc + high_val;
            }

            if (offset >= lowpc && offset < highpc)
            {
              // Found matching function
              Dwarf_Attribute attr_type;
              if (dwarf_attr(die, DW_AT_type, &attr_type, &err) == DW_DLV_OK)
              {
                Dwarf_Off type_ref;
                if (dwarf_global_formref(attr_type, &type_ref, &err) == DW_DLV_OK)
                {
                  Dwarf_Die type_die;
                  if (dwarf_offdie(dbg, type_ref, &type_die, &err) == DW_DLV_OK)
                  {
                    Dwarf_Error * MYERR;
                    std::string result = ResolveTypeName(dbg, type_die);
                    dwarf_finish(dbg, MYERR);
                    close(fd);
                    return result;
                  }
                }
              }
            }
          }
        }

        Dwarf_Die child;
        if (dwarf_child(die, &child, &err) == DW_DLV_OK)
          stack.push_back(child);

        Dwarf_Die sibling;
        if (dwarf_siblingof(dbg, die, &sibling, &err) == DW_DLV_OK)
          stack.push_back(sibling);
      }
    }
    Dwarf_Error * MYERR;
    dwarf_finish(dbg, MYERR);
    close(fd);
    return "";
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
        "llvm-symbolizer -e \"%s\" --inlining --demangle --functions --pretty-print 0x%llx",
        exePath.mb_str(),
        static_cast<unsigned long long>(offset));

    wxExecute(cmd, output, wxEXEC_SYNC);

    if (output.empty())
      return "<unknown>";

    std::string symbolLine = std::string(output[0].mb_str());
    std::string returnType = GetReturnTypeForAddress(reinterpret_cast<uintptr_t>(address));
    // Demangle possible mangled symbol in output[0]
    // return DemangleSymbol(output[0]); // for addr2line
    // return wxString::FromUTF8(output[0].mb_str());

    if (!returnType.empty()) {
      symbolLine = returnType + " " + symbolLine;
    } else {
      symbolLine = "void " + symbolLine;
    }
    return wxString::FromUTF8(symbolLine.c_str());
  }

private:
  void OnStackFrame(const wxStackFrame &frame) override
  {
    frames.push_back(frame);
    wxLogMessage("[%lu]: %s\n", (unsigned long)frame.GetLevel(), ResolveSymbol((void *)frame.GetAddress()));
  }

  StackFrameCollection frames;
  bool needFileInfo = false;
};
