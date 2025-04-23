#include <gtest/gtest.h>
#include "../lib/scheduler.h"
#include <string>
#include <algorithm>
#include <functional>

TEST(SchedulerTest, SimpleTasksNoDependendencies) {
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 42; });
    
    int value = 10;
    auto task2 = scheduler.add([](int x) { return x * 2; }, value);
    
    float a = 3.0f, b = 4.0f;
    auto task3 = scheduler.add([](float x, float y) { return x + y; }, a, b);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 42);
    EXPECT_EQ(scheduler.getResult<int>(task2), 20);
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(task3), 7.0f);
}

TEST(SchedulerTest, SimpleDependencies) {
    TTaskScheduler scheduler;
    
    int initial = 5;
    auto task1 = scheduler.add([](int x) { return x * x; }, initial);
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto task2 = scheduler.add([](int x) { return x + 10; }, future1);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 25);
    EXPECT_EQ(scheduler.getResult<int>(task2), 35);
}

TEST(SchedulerTest, MultipleDependencies) {
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 10; });
    auto task2 = scheduler.add([]() { return 20; });
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto future2 = scheduler.getFutureResult<int>(task2);
    auto task3 = scheduler.add([](int x, int y) { return x + y; }, future1, future2);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 10);
    EXPECT_EQ(scheduler.getResult<int>(task2), 20);
    EXPECT_EQ(scheduler.getResult<int>(task3), 30);
}

TEST(SchedulerTest, DependencyChain) {
    TTaskScheduler scheduler;
    
    std::vector<int> executionOrder;
    
    auto task1 = scheduler.add([&executionOrder]() { 
        executionOrder.push_back(1);
        return 100; 
    });
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto task2 = scheduler.add([&executionOrder](int x) { 
        executionOrder.push_back(2);
        return x / 2; 
    }, future1);
    
    auto future2 = scheduler.getFutureResult<int>(task2);
    auto task3 = scheduler.add([&executionOrder](int x) { 
        executionOrder.push_back(3);
        return x * 3; 
    }, future2);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 100);
    EXPECT_EQ(scheduler.getResult<int>(task2), 50);
    EXPECT_EQ(scheduler.getResult<int>(task3), 150);
    
    std::vector<int> expectedOrder = {1, 2, 3};
    EXPECT_EQ(executionOrder, expectedOrder);
}

TEST(SchedulerTest, DifferentDataTypes) {
    TTaskScheduler scheduler;
    
    auto stringTask = scheduler.add([]() { return std::string("Hello"); });
    auto stringFuture = scheduler.getFutureResult<std::string>(stringTask);
    auto combineTask = scheduler.add([](const std::string& s) { 
        return s + ", World!"; 
    }, stringFuture);
    
    bool flag = true;
    auto boolTask = scheduler.add([](bool f) { return !f; }, flag);
    
    auto doubleTask = scheduler.add([]() { return 3.14159; });
    auto doubleFuture = scheduler.getFutureResult<double>(doubleTask);
    auto mathTask = scheduler.add([](double d) { 
        return d * 2.0; 
    }, doubleFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<std::string>(stringTask), "Hello");
    EXPECT_EQ(scheduler.getResult<std::string>(combineTask), "Hello, World!");
    EXPECT_EQ(scheduler.getResult<bool>(boolTask), false);
    EXPECT_DOUBLE_EQ(scheduler.getResult<double>(doubleTask), 3.14159);
    EXPECT_DOUBLE_EQ(scheduler.getResult<double>(mathTask), 6.28318);
}

TEST(SchedulerTest, ComplexDependencyTree) {
    TTaskScheduler scheduler;
    
    auto a = scheduler.add([]() { return 5; });
    auto b = scheduler.add([]() { return 10; });
    
    auto aFuture = scheduler.getFutureResult<int>(a);
    auto bFuture = scheduler.getFutureResult<int>(b);
    
    auto c = scheduler.add([](int x) { return x * 2; }, aFuture);
    auto d = scheduler.add([](int x) { return x - 3; }, bFuture);
    
    auto cFuture = scheduler.getFutureResult<int>(c);
    auto dFuture = scheduler.getFutureResult<int>(d);
    
    auto e = scheduler.add([](int x, int y) { return x + y; }, cFuture, dFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(a), 5);
    EXPECT_EQ(scheduler.getResult<int>(b), 10);
    EXPECT_EQ(scheduler.getResult<int>(c), 10);
    EXPECT_EQ(scheduler.getResult<int>(d), 7);
    EXPECT_EQ(scheduler.getResult<int>(e), 17);
}

TEST(SchedulerTest, QuadraticEquation) {
    TTaskScheduler scheduler;
    
    float a = 1.0f;
    float b = -3.0f;
    float c = 2.0f;
    
    auto discriminantTask = scheduler.add([](float a, float c) {
        return -4 * a * c;
    }, a, c);
    
    auto b_squared = scheduler.add([](float b) {
        return b * b;
    }, b);
    
    auto discriminantFuture = scheduler.getFutureResult<float>(discriminantTask);
    auto b_squaredFuture = scheduler.getFutureResult<float>(b_squared);
    
    auto fullDiscriminant = scheduler.add([](float d1, float d2) {
        return d1 + d2;
    }, b_squaredFuture, discriminantFuture);
    
    auto fullDiscriminantFuture = scheduler.getFutureResult<float>(fullDiscriminant);
    
    auto root1 = scheduler.add([](float b, float d) {
        return (-b + std::sqrt(d)) / 2;
    }, b, fullDiscriminantFuture);
    
    auto root2 = scheduler.add([](float b, float d) {
        return (-b - std::sqrt(d)) / 2;
    }, b, fullDiscriminantFuture);
    
    scheduler.executeAll();
    
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(fullDiscriminant), 9.0f - 8.0f);
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(root1), 2.0f);
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(root2), 1.0f);
}

TEST(LambdaFunctionTest, RecursiveFibonacci) {
    TTaskScheduler scheduler;
    
    std::function<int(int)> fibonacci = [&scheduler, &fibonacci](int n) {
        if (n <= 1) return n;
        
        auto task1 = scheduler.add(fibonacci, n-1);
        auto task2 = scheduler.add(fibonacci, n-2);
        
        auto future1 = scheduler.getFutureResult<int>(task1);
        auto future2 = scheduler.getFutureResult<int>(task2);
        
        auto sumTask = scheduler.add([](int a, int b) { return a + b; }, future1, future2);
        return scheduler.getResult<int>(sumTask);
    };
    
    auto task = scheduler.add(fibonacci, 7);
    EXPECT_EQ(scheduler.getResult<int>(task), 13);
}

TEST(LambdaFunctionTest, FunctionComposition) {
    TTaskScheduler scheduler;
    
    auto double_it = [](int x) { return x * 2; };
    auto add_ten = [](int x) { return x + 10; };
    auto square_it = [](int x) { return x * x; };
    
    auto compose = [&scheduler, double_it, add_ten, square_it](int x) {
        auto step1 = scheduler.add(double_it, x);
        auto future1 = scheduler.getFutureResult<int>(step1);
        
        auto step2 = scheduler.add(add_ten, future1);
        auto future2 = scheduler.getFutureResult<int>(step2);
        
        auto step3 = scheduler.add(square_it, future2);
        return scheduler.getResult<int>(step3);
    };
    
    auto task = scheduler.add(compose, 3);
    
    EXPECT_EQ(scheduler.getResult<int>(task), 256);
}

TEST(LambdaFunctionTest, ConditionalTaskGeneration) {
    TTaskScheduler scheduler;
    
    auto generatorTask = scheduler.add([]() {
        return std::rand() % 100;
    });
    
    auto future = scheduler.getFutureResult<int>(generatorTask);
    
    auto conditionalTask = scheduler.add([&scheduler](int value) {
        if (value < 50) {
            auto smallTask = scheduler.add([](int x) { 
                return "Small value: " + std::to_string(x); 
            }, value);
            return scheduler.getResult<std::string>(smallTask);
        } else {
            auto largeTask = scheduler.add([](int x) { 
                return "Large value: " + std::to_string(x); 
            }, value);
            return scheduler.getResult<std::string>(largeTask);
        }
    }, future);
    
    auto result = scheduler.getResult<std::string>(conditionalTask);
    
    int generatedValue = scheduler.getResult<int>(generatorTask);
    if (generatedValue < 50) {
        EXPECT_EQ(result, "Small value: " + std::to_string(generatedValue));
    } else {
        EXPECT_EQ(result, "Large value: " + std::to_string(generatedValue));
    }
}

TEST(LambdaFunctionTest, BubbleSortAlgorithm) {
    TTaskScheduler scheduler;
    
    std::vector<int> data = {9, 1, 8, 2, 7, 3, 6, 4, 5};
    
    auto sortTask = scheduler.add([&scheduler](std::vector<int> arr) {
        int n = arr.size();
        bool swapped;
        
        for (int i = 0; i < n-1; i++) {
            swapped = false;
            
            auto innerLoopTask = scheduler.add([&scheduler, &arr, i, n]() {
                bool anySwap = false;
                
                for (int j = 0; j < n-i-1; j++) {
                    if (arr[j] > arr[j+1]) {
                        std::swap(arr[j], arr[j+1]);
                        anySwap = true;
                    }
                }
                
                return anySwap;
            });
            
            swapped = scheduler.getResult<bool>(innerLoopTask);
            
            if (!swapped) break;
        }
        
        return arr;
    }, data);
    
    std::vector<int> sortedData = scheduler.getResult<std::vector<int>>(sortTask);
    
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_EQ(sortedData, expected);
}