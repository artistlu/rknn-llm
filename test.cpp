#include <iostream>
#include <functional>

// 通用函数,接受一个 void* 类型的参数和一个 void* 类型的操作函数
typedef void (*OperationFunc)(void*);

void processData(void* data, OperationFunc op) {
    op(data);
}

int main() {
    int myData = 42;

    // 使用 std::function 包装匿名函数
    std::function<void(void*)> lambda = [](void* data) {
        int* p = static_cast<int*>(data);
        std::cout << "Processing data: " << *p << std::endl;
        *p = *p * 2;
    };

    processData(&myData, lambda);

    std::cout << "Modified data: " << myData << std::endl;

    return 0;
}