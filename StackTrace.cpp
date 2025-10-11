#include <wx/crt.h>
#include <wx/init.h>
#include "StackTrace.h"


void method3()
{
    wxStackTrace trace(true);
    // wxPuts("Stack Trace:");
    // wxPuts(trace.ToString());
}

void method2() {
  method3();
}

char method1() {
  method2();
  return '5';
}

auto main()->int {
  auto initializer = wxInitializer {};
  method1();
}

// This example can show the following output :
// Stack Trace :
//    at method3() in StackTrace.cpp:line 7
//    at method2() in StackTrace.cpp:line 12
//    at method1() in StackTrace.cpp:line 16
//    at main in StackTrace.cpp:line 21
//    at start in <filename unknown>:line 0
