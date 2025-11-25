#include <iostream>

// 1. The Parent
class Base {
public:
    Base() { std::cout << "1. Base Constructor\n"; }
    virtual ~Base() { std::cout << "4. Base Destructor\n"; }
};

// 2. The Child
class Derived : public Base {
public:
    Derived() { std::cout << "2. Derived Constructor\n"; }
    virtual ~Derived() { std::cout << "3. Derived Destructor\n"; }
};

int main() {
    std::cout << "--- Entering Scope ---\n";
    {
        Derived obj; // Created on the stack
    } 
    // 'obj' goes out of scope here. 
    // Destruction starts automatically.
    /*
           --- Entering Scope ---
            1. Base Constructor
            2. Derived Constructor
            3. Derived Destructor
            4. Base Destructor
            --- Exited Scope --- 
    */
    
    
    std::cout << "--- Exited Scope ---\n";
    return 0;
}
