#include <stdio.h>
//#include <iostream>
//#include <sstream>
#include <string>
#include <new>
#include <vector>
#include <stdint.h>
#include <fstream>
//#include <stdexcept>
#include <chrono>
#include <map>
#include <filesystem>
#include <algorithm>

typedef uint8_t word;
typedef uint16_t dblword;
#define wordones 0xff
#define wordbits 8
#define carryones 0xff00

class dynsiz{
    public:
    std::vector<word> ds; //Number array ( ds[0] least significant figure )
    dynsiz* div_remp; //Pointer to remainder of integer division
    dynsiz(int n);
    dynsiz(std::string numstr);
    ~dynsiz();
    /* AI-written functions start */
    //Copy constructor
    dynsiz(const dynsiz& x);
    // Copy assignment (strong exception safety)
    dynsiz& operator=(const dynsiz& x);
    // Move constructor
    dynsiz(dynsiz&& other) noexcept;
    // Move assignment
    dynsiz& operator=(dynsiz&& other) noexcept;
    /* AI-written end */
    bool expandSize(int n);
    void printNums();
    void printHex();
    void ensureEqLen(dynsiz* bptr);//Ensure pointed synsize have equal size as this object
    void operator<<=(word bs);//bs: amount of bits to be shifted
    private:
    inline void pm(dynsiz b,bool plus);
    public:
    void operator+=(dynsiz& b);
    void operator-=(dynsiz& b);
    void operator*=(dynsiz& b);
    dynsiz operator/(dynsiz d);//integer divison
    dynsiz legacy_divide(dynsiz d);//integer divison
    dynsiz operator%(dynsiz d);
    void digitDiv(dynsiz b,dynsiz base,int maxdigits);//Divide number by b and express it as number in given base
    bool operator<(dynsiz b);
    bool operator>(dynsiz b);
    bool eqzero();
};
inline void append_entries2f(std::fstream* file, dynsiz* entries, int ent_arr_size);
void extract_entriesOF(std::fstream* file, int entries);