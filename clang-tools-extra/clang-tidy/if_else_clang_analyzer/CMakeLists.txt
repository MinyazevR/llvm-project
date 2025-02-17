set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyTestModule STATIC
  testModule.cpp
  test.cc
  LINK_LIBS
  clangTidy
  clangTidyUtils

  DEPENDS
  omp_gen
  ClangDriverOptions
  )

clang_target_link_libraries(clangTidyTestModule
  PRIVATE
  clangAnalysis
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangSerialization
  clangTooling
  clang
  clangFrontend
  clangParse
  clangSema
  clangEdit
  clangSupport
  clangAPINotes
  clangRewrite
)

