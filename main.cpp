#include "vector.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <cassert>
#include <string>
#include <random>
#include <algorithm>

using namespace std;
using namespace chrono;

template<typename T>
void print_test_result(const string& test_name, T expected, T actual) {
    cout << test_name << ": ";
    if (expected == actual) {
        cout << "PASSED" << endl;
    } else {
        cout << "FAILED (Expected: " << expected << ", Got: " << actual << ")" << endl;
    }
}

class Timer {
    high_resolution_clock::time_point start;
public:
    Timer() : start(high_resolution_clock::now()) {}
    double elapsed() const {
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - start).count() / 1000.0;
    }
};

void test_functionality() {
    cout << "\n=== Functionality Tests ===\n";

    // Test constructors and push_back
    Vector<int> custom_vec;
    std::vector<int> std_vec;

    for(int i = 0; i < 10; ++i) {
        custom_vec.push_back(i);
        std_vec.push_back(i);
    }

    // Test size and capacity
    print_test_result("Size test", std_vec.size(), custom_vec.getSize());

    // Test element access
    bool access_test = true;
    for(size_t i = 0; i < custom_vec.getSize(); ++i) {
        if(custom_vec[i] != std_vec[i]) {
            access_test = false;
            break;
        }
    }
    cout << "Element access test: " << (access_test ? "PASSED" : "FAILED") << endl;

    // Test pop_back
    for(int i = 0; i < 5; ++i) {
        custom_vec.pop_back();
        std_vec.pop_back();
    }
    print_test_result("Pop back test", std_vec.size(), custom_vec.getSize());

    // Test element values after pop_back
    access_test = true;
    for(size_t i = 0; i < custom_vec.getSize(); ++i) {
        if(custom_vec[i] != std_vec[i]) {
            access_test = false;
            break;
        }
    }
    cout << "Element values after pop_back: " << (access_test ? "PASSED" : "FAILED") << endl;

    // Test clear
    custom_vec.clear();
    std_vec.clear();
    print_test_result("Clear test", std_vec.size(), custom_vec.getSize());
}

void test_performance() {
    cout << "\n=== Performance Tests ===\n";
    const int N = 1000000;

    // Test push_back performance
    {
        Vector<int> custom_vec;
        Timer custom_timer;
        for (int i = 0; i < N; ++i) {
            custom_vec.push_back(i);
        }
        double custom_time = custom_timer.elapsed();

        std::vector<int> std_vec;
        Timer std_timer;
        for (int i = 0; i < N; ++i) {
            std_vec.push_back(i);
        }
        double std_time = std_timer.elapsed();

        cout << "Push back " << N << " elements:\n";
        cout << "Custom vector: " << custom_time << "ms\n";
        cout << "STD vector: " << std_time << "ms\n";
        cout << "Ratio (custom/std): " << custom_time/std_time << "\n";
    }

    // Test pop_back performance
    {
        Vector<int> custom_vec;
        std::vector<int> std_vec;

        // Fill vectors
        for (int i = 0; i < N; ++i) {
            custom_vec.push_back(i);
            std_vec.push_back(i);
        }

        Timer custom_timer;
        for (int i = 0; i < N; ++i) {
            custom_vec.pop_back();
        }
        double custom_time = custom_timer.elapsed();

        Timer std_timer;
        for (int i = 0; i < N; ++i) {
            std_vec.pop_back();
        }
        double std_time = std_timer.elapsed();

        cout << "\nPop back " << N << " elements:\n";
        cout << "Custom vector: " << custom_time << "ms\n";
        cout << "STD vector: " << std_time << "ms\n";
        cout << "Ratio (custom/std): " << custom_time/std_time << "\n";
    }

    // Test random access performance
    {
        Vector<int> custom_vec;
        std::vector<int> std_vec;

        for (int i = 0; i < N; ++i) {
            custom_vec.push_back(i);
            std_vec.push_back(i);
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, N-1);

        Timer custom_timer;
        volatile int sum = 0;
        for (int i = 0; i < N; ++i) {
            sum += custom_vec[dis(gen)];
        }
        double custom_time = custom_timer.elapsed();

        Timer std_timer;
        sum = 0;
        for (int i = 0; i < N; ++i) {
            sum += std_vec[dis(gen)];
        }
        double std_time = std_timer.elapsed();

        cout << "\nRandom access " << N << " elements:\n";
        cout << "Custom vector: " << custom_time << "ms\n";
        cout << "STD vector: " << std_time << "ms\n";
        cout << "Ratio (custom/std): " << custom_time/std_time << "\n";
    }

    // Test mixed operations performance
    {
        Vector<int> custom_vec;
        std::vector<int> std_vec;

        Timer custom_timer;
        for (int i = 0; i < N/2; ++i) {
            custom_vec.push_back(i);
        }
        for (int i = 0; i < N/4; ++i) {
            custom_vec.pop_back();
        }
        for (int i = 0; i < N/4; ++i) {
            custom_vec.push_back(i);
        }
        double custom_time = custom_timer.elapsed();

        Timer std_timer;
        for (int i = 0; i < N/2; ++i) {
            std_vec.push_back(i);
        }
        for (int i = 0; i < N/4; ++i) {
            std_vec.pop_back();
        }
        for (int i = 0; i < N/4; ++i) {
            std_vec.push_back(i);
        }
        double std_time = std_timer.elapsed();

        cout << "\nMixed operations test:\n";
        cout << "Custom vector: " << custom_time << "ms\n";
        cout << "STD vector: " << std_time << "ms\n";
        cout << "Ratio (custom/std): " << custom_time/std_time << "\n";
    }
}

int main() {
    test_functionality();
    test_performance();

    return 0;
}
