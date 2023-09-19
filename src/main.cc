#include <iostream>
#include "./demo.h"
#include "./rvo.h"
#include "./coroutine.h"
#include "./tasks.h"

int main() {
    // runDemo<RVODemo>();
    // runDemo<CoroutineDemo>();
    runDemo<TasksDemo>();
    return 0;
}
