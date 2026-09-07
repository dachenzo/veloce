#include <veloce/thread_pool.hpp>
#include <iostream>
#include <vector>
#include <veloce/transform.hpp>


int main() {
    std::vector<int> in(20);
    for (auto i = 0; i < 20; i++) in[i] = i;

    veloce::ThreadPool pool{};
    veloce::Transform transform{pool};

    transform([](int c) {
        return c+1;
    }, in, in);

    for (auto i = 0; i < 20; i++) {
        std::cout << in[i] << ' ';
    }

}