#include <string>
#include "json.hpp"

#include"main.hpp"

nlohmann::json CrackTaskRequest::toJson() const {
    return {
        {"hash", hash},
        {"maxLength", maxLength},
        {"partNumber", partNumber},
        {"partCount", partCount}
    };
}

CrackTaskRequest crackTaskRequestfromJson(const nlohmann::json& j) {
    return {
        .hash = j.at("hash").get<std::string>(),
        .maxLength = j.at("maxLength").get<int>(),
        .partNumber = j.at("partNumber").get<int>(),
        .partCount = j.at("partCount").get<int>()
    };
}

nlohmann::json CrackTaskResult::toJson() const {
    return {
        {"hash", hash},
        {"result", result},
        {"partNumber", partNumber}
    };
}

CrackTaskResult crackTaskResultfromJson(const nlohmann::json& j) {
    return {
        .hash = j.at("hash").get<std::string>(),
        .result = j.at("result").get<std::string>(),
        .partNumber = j.at("partNumber").get<int>()
    };
}

