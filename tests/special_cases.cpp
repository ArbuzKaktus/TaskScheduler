#include <gtest/gtest.h>
#include "../lib/scheduler.h"
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <memory>
#include <algorithm>

TEST(SpecialCasesTest, EmptyScheduler) {
    TTaskScheduler scheduler;
    EXPECT_NO_THROW(scheduler.executeAll());
}

TEST(SpecialCasesTest, GetResultOfUnexecutedTask) {
    TTaskScheduler scheduler;
    auto task = scheduler.add([]() { return 42; });
    EXPECT_EQ(scheduler.getResult<int>(task), 42);
    EXPECT_TRUE(task->IsExecuted());
}

TEST(SpecialCasesTest, TaskWithException) {
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 10; });
    auto future1 = scheduler.getFutureResult<int>(task1);
    
    auto task2 = scheduler.add([](int x) { 
        throw std::runtime_error("Task failure");
        return x * 2; 
    }, future1);
    
    auto task3 = scheduler.add([]() { return 30; });
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 10);
    EXPECT_EQ(scheduler.getResult<int>(task3), 30);
    EXPECT_THROW(scheduler.getResult<int>(task2), std::runtime_error);
}

TEST(SpecialCasesTest, VoidReturnType) {
    TTaskScheduler scheduler;
    
    int value = 0;
    auto task = scheduler.add([&value]() { value = 42; });
    
    scheduler.executeAll();
    
    EXPECT_TRUE(task->IsExecuted());
    EXPECT_EQ(value, 42);
}

TEST(SpecialCasesTest, LongDependencyChain) {
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 1; });
    std::shared_ptr<TaskBase> currentTask = task1;
    
    const int chainLength = 100;
    for (int i = 2; i <= chainLength; ++i) {
        auto future = scheduler.getFutureResult<int>(std::static_pointer_cast<Task<int>>(currentTask));
        currentTask = scheduler.add([i](int x) { return x + i; }, future);
    }
    
    auto finalTask = std::static_pointer_cast<Task<int>>(currentTask);
    
    int result = scheduler.getResult<int>(finalTask);
    int expected = (1 + chainLength) * chainLength / 2;
    
    EXPECT_EQ(result, expected);
}

TEST(SpecialCasesTest, ExecutionOrderIndependence) {
    TTaskScheduler scheduler;
    
    std::vector<int> order;
    
    auto task4 = scheduler.add([&order]() { 
        order.push_back(4);
        return 4; 
    });
    
    auto task1 = scheduler.add([&order]() { 
        order.push_back(1);
        return 1; 
    });
    
    auto task3 = scheduler.add([&order]() { 
        order.push_back(3);
        return 3; 
    });
    
    auto task2 = scheduler.add([&order]() { 
        order.push_back(2);
        return 2; 
    });
    
    scheduler.executeAll();
    
    std::vector<int> sorted_order = order;
    std::sort(sorted_order.begin(), sorted_order.end());
    
    EXPECT_EQ(order.size(), 4);
    EXPECT_EQ(sorted_order, std::vector<int>({1, 2, 3, 4}));
}

TEST(SpecialCasesTest, LargeData) {
    TTaskScheduler scheduler;
    
    const size_t dataSize = 1000000;
    auto task1 = scheduler.add([dataSize]() {
        std::vector<int> largeVector(dataSize, 1);
        return largeVector;
    });
    
    auto future1 = scheduler.getFutureResult<std::vector<int>>(task1);
    
    auto task2 = scheduler.add([](const std::vector<int>& data) {
        int sum = 0;
        for (int value : data) {
            sum += value;
        }
        return sum;
    }, future1);
    
    int result = scheduler.getResult<int>(task2);
    EXPECT_EQ(result, dataSize);
}

TEST(SpecialCasesTest, TasksWithDelays) {
    TTaskScheduler scheduler;
    
    auto start = std::chrono::steady_clock::now();
    
    auto task1 = scheduler.add([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 1;
    });
    
    auto task2 = scheduler.add([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 2;
    });
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto future2 = scheduler.getFutureResult<int>(task2);
    
    auto task3 = scheduler.add([](int a, int b) {
        return a + b;
    }, future1, future2);
    
    int result = scheduler.getResult<int>(task3);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(result, 3);
    EXPECT_GE(duration, 100);
}

struct NonDefaultConstructible {
    explicit NonDefaultConstructible(int val) : value(val) {}
    int value;
};

TEST(SpecialCasesTest, OnlyNecessaryTasksExecuted) {
    TTaskScheduler scheduler;
    
    bool executed1 = false, executed2 = false, executed3 = false;
    
    auto task1 = scheduler.add([&executed1]() {
        executed1 = true;
        return 1;
    });
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto task2 = scheduler.add([&executed2](int x) {
        executed2 = true;
        return x + 2;
    }, future1);
    
    auto task3 = scheduler.add([&executed3]() {
        executed3 = true;
        return 3;
    });
    
    int result = scheduler.getResult<int>(task2);
    
    EXPECT_EQ(result, 3);
    EXPECT_TRUE(executed1);
    EXPECT_TRUE(executed2);
    EXPECT_FALSE(executed3);
}