#include "../include/util/result.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace base;

void test_error_code() {
    std::cout << "Test ErrorCode..." << std::endl;

    ErrorCode success(ErrorCode::SUCCESS);
    assert(success.isSuccess());
    assert(!success.isError());
    assert(success.code() == ErrorCode::SUCCESS);

    ErrorCode error(ErrorCode::FILE_NOT_FOUND, "File not found");
    assert(error.isError());
    assert(error.code() == ErrorCode::FILE_NOT_FOUND);
    assert(error.message() == "File not found");

    std::cout << "  PASSED" << std::endl;
}

void test_result_with_value() {
    std::cout << "Test Result<int>..." << std::endl;

    Result<int> success = Result<int>::success(42);
    assert(success.isSuccess());
    assert(success.hasValue());
    assert(success.value() == 42);
    assert(success.errorCode() == ErrorCode::SUCCESS);

    Result<int> failure = Result<int>::failure(ErrorCode::FILE_NOT_FOUND);
    assert(failure.isError());
    assert(!failure.hasValue());
    assert(failure.errorCode() == ErrorCode::FILE_NOT_FOUND);

    Result<int> failure2 = Result<int>::failure(ErrorCode::NETWORK_ERROR, "Connection refused");
    assert(failure2.isError());
    assert(failure2.errorMessage() == "Connection refused");

    std::cout << "  PASSED" << std::endl;
}

void test_result_void() {
    std::cout << "Test Result<void>..." << std::endl;

    Result<void> success = Result<void>::success();
    assert(success.isSuccess());
    assert(!success.isError());

    Result<void> failure = Result<void>::failure(ErrorCode::JSON_PARSE_ERROR, "Invalid JSON");
    assert(failure.isError());
    assert(failure.errorCode() == ErrorCode::JSON_PARSE_ERROR);
    assert(failure.errorMessage() == "Invalid JSON");

    std::cout << "  PASSED" << std::endl;
}

void test_result_string() {
    std::cout << "Test Result<std::string>..." << std::endl;

    Result<std::string> success = Result<std::string>::success("hello");
    assert(success.isSuccess());
    assert(success.value() == "hello");

    Result<std::string> failure = Result<std::string>::failure(ErrorCode::INVALID_ARGUMENT);
    assert(failure.isError());

    std::cout << "  PASSED" << std::endl;
}

void test_status() {
    std::cout << "Test Status..." << std::endl;

    Status ok = Status::success();
    assert(ok.isSuccess());

    Status error = Status::failure(ErrorCode::UNKNOWN_ERROR);
    assert(error.isError());

    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   BaseLib Result<T, E> Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_error_code();
    test_result_with_value();
    test_result_void();
    test_result_string();
    test_status();

    std::cout << "========================================" << std::endl;
    std::cout << "   ALL TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
