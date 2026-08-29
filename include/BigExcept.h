#include <stdexcept>

namespace mpal{
    void ThrowIfCharInvalid(size_t index, const std::string& str, const std::string& message){
        throw std::invalid_argument("Invalid character at index " + std::to_string(index) + " in string \"" + str + "\": " + message);
    }
}