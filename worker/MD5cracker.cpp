#include"MD5cracker.hpp"


MD5Cracker::MD5Cracker() : alphabet("abcdefghijklmnopqrstuvwxyz0123456789") {}

std::string MD5Cracker::Crack(const std::string& targetHash, int maxLength, int partNumber, int partCount) {
    for (int length = 1; length <= maxLength; ++length) {
        int total = static_cast<int>(std::pow(alphabet.size(), length));
        for (int i = partNumber - 1; i < total; i += partCount) {

            std::string candidate = IntToCandidate(i, length);
            std::string hashCandidate = CalculateMD5(candidate);
            if (hashCandidate == targetHash) {
                return candidate;
            }
        }
    }
    return "";
}

std::string alphabet;

std::string MD5Cracker::IntToCandidate(int num, int length) {
    int base = alphabet.size();
    std::string candidate(length, ' ');
    for (int j = length - 1; j >= 0; --j) {
        candidate[j] = alphabet[num % base];
        num /= base;
    }
    return candidate;
}

std::string MD5Cracker::CalculateMD5(const std::string& input) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    unsigned char digest[MD5_DIGEST_LENGTH];
    unsigned int digest_len;

    if (!context) {
        throw std::runtime_error("Failed to create MD5 context");
    }

    if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1 ||
        EVP_DigestUpdate(context, input.c_str(), input.size()) != 1 ||
        EVP_DigestFinal_ex(context, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("MD5 calculation failed");
    }

    EVP_MD_CTX_free(context);

    std::ostringstream hexStream;
    for (unsigned int i = 0; i < digest_len; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }

    return hexStream.str();
}