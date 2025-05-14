#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <string>

#include "json.hpp"

#ifndef CRACK_TASK
#define CRACK_TASK

struct CrackTaskRequest {
    std::string hash;
    int maxLength;
    int partNumber;
    int partCount;

    nlohmann::json toJson() const;
};
CrackTaskRequest crackTaskRequestfromJson(const nlohmann::json& j);

struct CrackTaskResult {
    std::string hash;
    std::string result;
    int partNumber;

    nlohmann::json toJson() const;;
};
CrackTaskResult crackTaskResultfromJson(const nlohmann::json& j);

#endif