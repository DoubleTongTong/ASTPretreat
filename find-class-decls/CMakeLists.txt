set(LLVM_LINK_COMPONENTS support)

add_definitions("-g")
add_clang_executable(find-class-decls
  FindClassDecls.cpp
  TConfigurationManager.cpp
  TSQLite.cpp
  THeaderFileManager.cpp
  TVisitorUtils.cpp
  )
INCLUDE_DIRECTORIES(/home/tongtong/sqlite/build/include)
INCLUDE_DIRECTORIES(/home/tongtong/boost/boost_1_80_0/build/include)
LINK_DIRECTORIES(/home/tongtong/sqlite/build/lib)
LINK_DIRECTORIES(/home/tongtong/boost/boost_1_80_0/build/lib)
target_link_libraries(find-class-decls
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  libsqlite3.so
  stdc++fs
  )
