#include <iostream>
using namespace std;

class Base {
public:
    void nonVirtualFunc() { cout << "Base non-virtual function\n"; }
    virtual void virtualFunc() { cout << "Base virtual function\n"; }
    void baseOnlyFunc() { cout << "Base only function\n"; }

};

class Derived : public Base {
public:
    void nonVirtualFunc() { cout << "Derived non-virtual function\n"; }
    void virtualFunc() override { cout << "Derived virtual function\n"; }
    void derivedOnlyFunc() { cout << "Derived only function\n"; }

};

int main() {
    // Upcasting ->derived* derviedptr = &Derivedobj ; baseptr = dynamic_cast<base*>(derviedptr) ;
    Derived d;
    Base* basePtr = dynamic_cast<Base*>(&d);
    basePtr->nonVirtualFunc(); // Calls Base::nonVirtualFunc()
    basePtr->virtualFunc();    // Calls Derived::virtualFunc()
    basePtr->baseOnlyFunc();  // Calls Base::nonvirtualFunc()
    // basePtr->derivedOnlyFunc(); Compilation error
    
    // Upcasting -: Base ptr is equal to dynamic_cast of Derivedptr
    // Calling non virtual functions will only call non virtual functions of Base class
    // calling virtual functions will only call virtual functions of Derived class
    std::cout<<std::endl;
    
    //Downcasting -> base* baseptr = &Derivedobj  ;  derviedPtr = dynamic_cast<dervied*>(baseptr) ;
    Base *b = &d;
    Derived* derivedPtr = dynamic_cast<Derived*>(b);
    if(derivedPtr) {
        derivedPtr->nonVirtualFunc();  // Calls Derived's non-virtual function
        derivedPtr->derivedOnlyFunc(); // Calls Derived-only function
        derivedPtr->virtualFunc();   // Calls Dervied virtualFunc
    } else {
        cout << "Downcast failed\n";
        // Base b;
        // Dervied* derviedPtr = dynamic <Dervied*>(&b); will fail the downcasting
        // as the base class is not pointing to anything which is derived
    }
    
    
    return 0;
}
