#include <iostream>
#include "./rvo.h"

using std::cout, std::endl;

struct TypeA {

    TypeA() {
        cout << "Constructor<TypeA>" << endl;
    }

    TypeA(const TypeA &a) {
        cout << "Copy Constructor<TypeA>" << endl;
    }

    TypeA(TypeA &&a) {
        cout << "Move Constructor<TypeA>" << endl;
    }

    TypeA & operator =(const TypeA &a) {
        cout << "Copy Assignment<TypeA>" << endl;
        return *this;
    }

    TypeA & operator =(TypeA &&a) {
        cout << "Move Assignment<TypeA>" << endl;
        return *this;
    }

    ~TypeA() {
        cout << "Destructor<TypeA>" << endl;
    }
};

TypeA getTypeA() {
    TypeA a;
    cout << "After declaring 'a'" << endl;
    return a;
}

void RVODemo::run() {
    TypeA a0 = getTypeA();
    cout << "After getTypeA()" << endl;
    
}
