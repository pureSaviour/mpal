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
    // BigInt a;
    // std::cin >> a;
    // BigInt b;
    // std::cin >> b;
    // std::cout << a << std::endl;
    // std::cout << b << std::endl;
    // std::cout << a + b << std::endl;
    std::vector<uint32_t> digits = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};    
    for(auto d : digits){
        std::cout << std::hex << d << std::endl;
    }    
}