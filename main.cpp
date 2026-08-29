#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <vector>
#include <climits>

#include "utils.h"
#include "BigInt.h"
#include "BigUtils.h"
#include "uint128.h"
#include "int128.h"

using namespace mpal;
int main(){
    // std::string path = "../data.dat";
    // std::ifstream file(path, std::ios::in | std::ios::out);
    // if(!file){
    //     std::cerr << "Failed to open file: " << path << std::endl;
    //     return -1;
    // }    
    // std::string line;    
    // BigInt bigInts[2];
    // size_t index = 0;
    // while(std::getline(file, line)){
    //   if(!line.empty() && line.back() == '\r')
    //       line.pop_back();          // 去掉 Windows 的 \r
    //   if(line.empty())
    //       continue;

    //   try{
    //       bigInts[index] = BigInt(line);
    //       ++index;
    //       if(index >= 2)
    //           break;
    //   }catch(const std::invalid_argument& e){
    //       std::cerr << "Error parsing line: " << line << std::endl;
    //   }
    // }
    
    // uint128_t u128 = uint128_t(18446744073709551615);
    // uint128_t u128_2 = uint128_t(18446744461658);
    // unsigned int t;
    // std::cout << "u128: " << u128 << std::endl;
    // std::cout << "u128_2: " << u128_2 << std::endl;
    // std::cout << "u128 + u128_2: " << (u128 + u128_2) << std::endl;
    // std::cout << "max: " << uint128_t::max() << std::endl; 
    // std::cin >> t;std::cout << "t: " << t << std::endl;
    
    mpal::BigInt a("1234567890123456789012345678901234567890");
    mpal::BigInt b("9876543210987654321098765432109876543210");
    mpal::BigInt c = a * b;
    std::cout << "a: " << a.ToString() << std::endl;
    std::cout << "b: " << b.ToString() << std::endl;
    std::cout << "a * b: " << c.ToString() << std::endl;

    uint128_t u("161986165846341464846845");
    std::cout << "u: " << u.ToString(DEC) << std::endl;
}