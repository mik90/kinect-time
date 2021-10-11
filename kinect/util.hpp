#pragma once
#include <iostream>
namespace mik {

// I'd use std::source_location but clang doesn't have it yet so clangd will complain
#define HERE __FILE__, __LINE__
void exit_with_error(const char* file, std::uint32_t line, std::string_view err_msg);

} // namespace mik