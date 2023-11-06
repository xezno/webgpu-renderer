-- Project info
set_project("HackweekWebGPU")
set_version("1.0.0")

target("App")
  set_kind("binary")
  add_files("main.cpp")
  set_languages("cxx17")

  -- Treat warnings as errors
  add_cxflags("/WX")
