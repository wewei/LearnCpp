#include <iostream>

#include "./demo.h"
#include "./rvo.h"
#include "./coroutine.h"
#include "./tasks.h"
#include "./tasks2.h"
#include "./tasks3.h"

int f(int &&x) {
    return 0;
}

int main() {
    // runDemo<RVODemo>();
    // runDemo<CoroutineDemo>();
    // runDemo<TasksDemo>();
    runDemo<Tasks3Demo>();
    return 0;
}
