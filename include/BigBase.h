#ifndef BIG_BASE_H
#define BIG_BASE_H

class BigBase{
public:
    bool IsNegative() const { return isNegative_; }
    bool IsPositive() const { return !isNegative_; }
    void Inverse() { isNegative_ = !isNegative_; }
    void SetSign(bool isPositive) { isNegative_ = !isPositive; }
protected:
    BigBase() = default;
    BigBase(bool isNegative) : isNegative_(isNegative) {}
    bool isNegative_ = false;    
};
#endif