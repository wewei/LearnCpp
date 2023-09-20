#include <iostream>
#include "./demo.h"
#include "./rvo.h"
#include "./coroutine.h"
#include "./tasks.h"
#include "./tasks2.h"

int main() {
    // runDemo<RVODemo>();
    // runDemo<CoroutineDemo>();
    // runDemo<TasksDemo>();
    runDemo<Tasks2Demo>();
    return 0;
}
