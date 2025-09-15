#include <ostream>
#include <iostream>
#include <cstring>

class Allocator {
public:

    virtual void init () {
        
    }

    virtual uint8_t* allocate () {
        return nullptr;
    }

    virtual void deallocate () {
    }
};