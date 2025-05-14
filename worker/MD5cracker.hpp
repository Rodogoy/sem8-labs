#include <string>
#include <openssl/md5.h> 
#include <openssl/evp.h> 
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>

#ifndef MD5CRACKER
#define MD5CRACKER

class MD5Cracker {
private:
    std::string alphabet;
    std::string IntToCandidate(int num, int length);
    std::string CalculateMD5(const std::string& input);
public:
    MD5Cracker();
    std::string Crack(const std::string& targetHash, int maxLength, int partNumber, int partCount);

};

extern MD5Cracker md5Cracker;

#endif