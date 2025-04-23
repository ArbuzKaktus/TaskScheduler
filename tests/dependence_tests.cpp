#include <gtest/gtest.h>
#include "../lib/scheduler.h"
#include <string>
#include <stdexcept>

struct TestClass {
    int multiply(int x) const { return x * factor; }
    float divide(float x) const { return x / factor; }
    
    int factor;
};

TEST(DependencyTest, DiamondDependencies) {
    TTaskScheduler scheduler;
    
    auto baseTask = scheduler.add([]() { return 10; });
    auto baseFuture = scheduler.getFutureResult<int>(baseTask);
    
    auto leftPath = scheduler.add([](int x) { return x + 5; }, baseFuture);
    auto rightPath = scheduler.add([](int x) { return x * 2; }, baseFuture);
    
    auto leftFuture = scheduler.getFutureResult<int>(leftPath);
    auto rightFuture = scheduler.getFutureResult<int>(rightPath);
    
    auto finalTask = scheduler.add([](int a, int b) { return a + b; }, leftFuture, rightFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(baseTask), 10);
    EXPECT_EQ(scheduler.getResult<int>(leftPath), 15);
    EXPECT_EQ(scheduler.getResult<int>(rightPath), 20);
    EXPECT_EQ(scheduler.getResult<int>(finalTask), 35);
}

TEST(DependencyTest, MultiLevelDependencies) {
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 1; });
    auto task2 = scheduler.add([]() { return 2; });
    auto task3 = scheduler.add([]() { return 3; });
    
    auto future1 = scheduler.getFutureResult<int>(task1);
    auto future2 = scheduler.getFutureResult<int>(task2);
    auto future3 = scheduler.getFutureResult<int>(task3);
    
    auto task4 = scheduler.add([](int a, int b) { return a + b; }, future1, future2);
    auto task5 = scheduler.add([](int a, int b) { return a * b; }, future2, future3);
    
    auto future4 = scheduler.getFutureResult<int>(task4);
    auto future5 = scheduler.getFutureResult<int>(task5);
    
    auto task6 = scheduler.add([](int a, int b) { return a - b; }, future4, future5);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task4), 3);
    EXPECT_EQ(scheduler.getResult<int>(task5), 6);
    EXPECT_EQ(scheduler.getResult<int>(task6), -3);
}

TEST(DependencyTest, MethodDependencies) {
    TTaskScheduler scheduler;
    
    TestClass testObj{5};
    
    auto task1 = scheduler.add([]() { return 10; });
    auto future1 = scheduler.getFutureResult<int>(task1);
    
    auto task2 = scheduler.add(&TestClass::multiply, testObj, future1);
    auto future2 = scheduler.getFutureResult<int>(task2);
    
    auto task3 = scheduler.add([](int x) { return static_cast<float>(x) / 2.0f; }, future2);
    auto future3 = scheduler.getFutureResult<float>(task3);
    
    auto task4 = scheduler.add(&TestClass::divide, testObj, future3);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(task1), 10);
    EXPECT_EQ(scheduler.getResult<int>(task2), 50);
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(task3), 25.0f);
    EXPECT_FLOAT_EQ(scheduler.getResult<float>(task4), 5.0f);
}

TEST(DependencyTest, MixedTypeDependencies) {
    TTaskScheduler scheduler;
    
    auto stringTask = scheduler.add([]() { return std::string("value:"); });
    auto intTask = scheduler.add([]() { return 42; });
    
    auto stringFuture = scheduler.getFutureResult<std::string>(stringTask);
    auto intFuture = scheduler.getFutureResult<int>(intTask);
    
    auto combineTask = scheduler.add([](const std::string& s, int x) { 
        return s + " " + std::to_string(x); 
    }, stringFuture, intFuture);
    
    auto combineFuture = scheduler.getFutureResult<std::string>(combineTask);
    
    auto lengthTask = scheduler.add([](const std::string& s) { 
        return s.length(); 
    }, combineFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<std::string>(stringTask), "value:");
    EXPECT_EQ(scheduler.getResult<int>(intTask), 42);
    EXPECT_EQ(scheduler.getResult<std::string>(combineTask), "value: 42");
    EXPECT_EQ(scheduler.getResult<size_t>(lengthTask), 9);
}

TEST(DependencyTest, ComplexWorkflow) {
    TTaskScheduler scheduler;
    
    auto input1 = scheduler.add([]() { return 10; });
    auto input2 = scheduler.add([]() { return 20; });
    
    auto future1 = scheduler.getFutureResult<int>(input1);
    auto future2 = scheduler.getFutureResult<int>(input2);
    
    auto squareTask = scheduler.add([](int x) { return x * x; }, future1);
    auto doubleTask = scheduler.add([](int x) { return x * 2; }, future2);
    
    auto squareFuture = scheduler.getFutureResult<int>(squareTask);
    auto doubleFuture = scheduler.getFutureResult<int>(doubleTask);
    
    auto sumTask = scheduler.add([](int x, int y) { return x + y; }, 
                                 squareFuture, doubleFuture);
    
    auto sumFuture = scheduler.getFutureResult<int>(sumTask);
    
    auto finalTask = scheduler.add([](int x) { return std::to_string(x) + " is the result"; }, 
                                   sumFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(squareTask), 100);
    EXPECT_EQ(scheduler.getResult<int>(doubleTask), 40);
    EXPECT_EQ(scheduler.getResult<int>(sumTask), 140);
    EXPECT_EQ(scheduler.getResult<std::string>(finalTask), "140 is the result");
}

TEST(DependencyTest, ReusingSameTask) {
    TTaskScheduler scheduler;
    
    auto baseTask = scheduler.add([]() { return 5; });
    auto baseFuture = scheduler.getFutureResult<int>(baseTask);
    
    auto task1 = scheduler.add([](int x) { return x + 1; }, baseFuture);
    auto task2 = scheduler.add([](int x) { return x + 2; }, baseFuture);
    auto task3 = scheduler.add([](int x) { return x + 3; }, baseFuture);
    
    scheduler.executeAll();
    
    EXPECT_EQ(scheduler.getResult<int>(baseTask), 5);
    EXPECT_EQ(scheduler.getResult<int>(task1), 6);
    EXPECT_EQ(scheduler.getResult<int>(task2), 7);
    EXPECT_EQ(scheduler.getResult<int>(task3), 8);
}

TEST(DependencyTest, CyclicDependency) {
    
    TTaskScheduler scheduler;
    
    auto task1 = scheduler.add([]() { return 1; });
    auto future1 = scheduler.getFutureResult<int>(task1);
    
    auto task2 = scheduler.add([](int x) { return x + 1; }, future1);
    auto future2 = scheduler.getFutureResult<int>(task2);
    
    task1->AddDependendTask(task2);
    
    EXPECT_THROW(scheduler.executeAll(), std::runtime_error);
}

TEST(DependencyTest, GetResultExecutesOnlyNecessaryTasks) {
  TTaskScheduler scheduler;
  
  bool executed1 = false, executed2 = false, executed3 = false;
  bool executed4 = false, executed5 = false, executed6 = false;
  
  auto task1 = scheduler.add([&executed1]() {
      executed1 = true;
      return 10;
  });
  
  auto task2 = scheduler.add([&executed2]() {
      executed2 = true;
      return 20;
  });
  
  auto task3 = scheduler.add([&executed3]() {
      executed3 = true;
      return 30;
  });
  
  auto future1 = scheduler.getFutureResult<int>(task1);
  auto future2 = scheduler.getFutureResult<int>(task2);
  auto future3 = scheduler.getFutureResult<int>(task3);
  
  auto task4 = scheduler.add([&executed4](int x) {
      executed4 = true;
      return x * 2;
  }, future1);
  
  auto task5 = scheduler.add([&executed5](int x) {
      executed5 = true;
      return x + 5;
  }, future2);
  
  auto future4 = scheduler.getFutureResult<int>(task4);
  auto future5 = scheduler.getFutureResult<int>(task5);
  auto task6 = scheduler.add([&executed6](int x, int y) {
      executed6 = true;
      return x + y;
  }, future4, future5);

  int result4 = scheduler.getResult<int>(task4);
  
  EXPECT_TRUE(executed1);
  EXPECT_FALSE(executed2);
  EXPECT_FALSE(executed3);
  EXPECT_TRUE(executed4);
  EXPECT_FALSE(executed5);
  EXPECT_FALSE(executed6);
  
  EXPECT_EQ(result4, 20);
  
  int result6 = scheduler.getResult<int>(task6);
  
  EXPECT_TRUE(executed1);
  EXPECT_TRUE(executed2);
  EXPECT_FALSE(executed3);
  EXPECT_TRUE(executed4);
  EXPECT_TRUE(executed5);
  EXPECT_TRUE(executed6);
  
  EXPECT_EQ(result6, 45);
  
  scheduler.executeAll();
  
  EXPECT_TRUE(executed3);
  EXPECT_EQ(scheduler.getResult<int>(task3), 30);
}

TEST(DependencyTest, GetResultCacheBehavior) {
  TTaskScheduler scheduler;
  
  int count1 = 0, count2 = 0, count3 = 0;
  
  auto task1 = scheduler.add([&count1]() {
      count1++;
      return 10;
  });
  
  auto future1 = scheduler.getFutureResult<int>(task1);
  auto task2 = scheduler.add([&count2](int x) {
      count2++;
      return x * 2;
  }, future1);
  
  auto future2 = scheduler.getFutureResult<int>(task2);
  auto task3 = scheduler.add([&count3](int x) {
      count3++;
      return x + 5;
  }, future2);
  
  int result3 = scheduler.getResult<int>(task3);
  EXPECT_EQ(result3, 25);
  EXPECT_EQ(count1, 1);
  EXPECT_EQ(count2, 1);
  EXPECT_EQ(count3, 1);
  
  result3 = scheduler.getResult<int>(task3);
  EXPECT_EQ(result3, 25);
  EXPECT_EQ(count1, 1);
  EXPECT_EQ(count2, 1);
  EXPECT_EQ(count3, 1);
  
  int result2 = scheduler.getResult<int>(task2);
  EXPECT_EQ(result2, 20);
  EXPECT_EQ(count1, 1);
  EXPECT_EQ(count2, 1);
  EXPECT_EQ(count3, 1);
}