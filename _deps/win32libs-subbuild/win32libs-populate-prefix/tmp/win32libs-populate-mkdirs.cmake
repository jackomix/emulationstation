# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/win32-libs"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-build"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/tmp"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/src/win32libs-populate-stamp"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/src"
  "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/src/win32libs-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/src/win32libs-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/myEmulationStation/amber-es/.worktrees/lahee-integration/_deps/win32libs-subbuild/win32libs-populate-prefix/src/win32libs-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
