#include <gtest/gtest.h>
#include "../lib/scheduler.h"
#include <string>
#include <vector>

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