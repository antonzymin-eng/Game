# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/Game/_deps/lz4-src"
  "/workspaces/Game/_deps/lz4-build"
  "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix"
  "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/tmp"
  "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/src/lz4-populate-stamp"
  "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/src"
  "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/src/lz4-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/src/lz4-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/Game/_deps/lz4-subbuild/lz4-populate-prefix/src/lz4-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
