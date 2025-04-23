#include "../lib/scheduler.h"
#include <gtest/gtest.h>
#include <string>


class Calculator {
public:
  int add(int a) const { return a + stored_value; }

  int subtract(int a) const { return stored_value - a; }

  int multiply(int a) const { return a * multiplier; }

  float divide(float a) const { return a / multiplier; }

  int getValue() const { return multiplier; }

  void setValue(int value) { multiplier = value; }

  int multiplier = 2;
  int stored_value = 30;
};

class StringProcessor {
public:
  std::string append(const std::string &text) const { return prefix + text; }

  int countChars(const std::string &text) const { return text.length(); }

  std::string prefix = "Processed: ";
};

TEST(ClassMethodTest, SimpleMethodNoArgs) {
  TTaskScheduler scheduler;

  Calculator calc;
  calc.multiplier = 5;

  auto task = scheduler.add(&Calculator::getValue, calc);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(task), 5);
}

TEST(ClassMethodTest, MethodWithArgs) {
  TTaskScheduler scheduler;

  Calculator calc;
  calc.stored_value = 30;

  auto task1 = scheduler.add(&Calculator::add, calc, 10);
  auto task2 = scheduler.add(&Calculator::subtract, calc, 10);
  auto task3 = scheduler.add(&Calculator::multiply, calc, 15);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(task1), 40);
  EXPECT_EQ(scheduler.getResult<int>(task2), 20);
  EXPECT_EQ(scheduler.getResult<int>(task3), 30);
}

TEST(ClassMethodTest, MethodWithFutureArgs) {
  TTaskScheduler scheduler;

  Calculator calc;

  auto baseTask = scheduler.add([]() { return 10; });
  auto baseFuture = scheduler.getFutureResult<int>(baseTask);

  auto task1 = scheduler.add(&Calculator::multiply, calc, baseFuture);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(baseTask), 10);
  EXPECT_EQ(scheduler.getResult<int>(task1), 20);
}

TEST(ClassMethodTest, ChainedMethods) {
  TTaskScheduler scheduler;

  Calculator calc;
  calc.stored_value = 7;
  StringProcessor stringProc;

  auto numTask = scheduler.add(&Calculator::add, calc, 5);
  auto numFuture = scheduler.getFutureResult<int>(numTask);

  auto stringTask = scheduler.add(
      [](int num) { return "Number: " + std::to_string(num); }, numFuture);
  auto stringFuture = scheduler.getFutureResult<std::string>(stringTask);

  auto processedTask =
      scheduler.add(&StringProcessor::append, stringProc, stringFuture);
  auto lengthTask =
      scheduler.add(&StringProcessor::countChars, stringProc, stringFuture);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(numTask), 12);
  EXPECT_EQ(scheduler.getResult<std::string>(stringTask), "Number: 12");
  EXPECT_EQ(scheduler.getResult<std::string>(processedTask),
            "Processed: Number: 12");
  EXPECT_EQ(scheduler.getResult<int>(lengthTask), 10);
}

TEST(ClassMethodTest, MixedLambdaAndMethods) {
  TTaskScheduler scheduler;

  Calculator calc;

  auto task1 = scheduler.add([]() { return 100; });
  auto future1 = scheduler.getFutureResult<int>(task1);

  auto task2 = scheduler.add(&Calculator::multiply, calc, future1);
  auto future2 = scheduler.getFutureResult<int>(task2);

  auto task3 = scheduler.add(
      [](int x) { return static_cast<float>(x) / 10.0f; }, future2);
  auto future3 = scheduler.getFutureResult<float>(task3);

  auto task4 = scheduler.add(&Calculator::divide, calc, future3);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(task1), 100);
  EXPECT_EQ(scheduler.getResult<int>(task2), 200);
  EXPECT_FLOAT_EQ(scheduler.getResult<float>(task3), 20.0f);
  EXPECT_FLOAT_EQ(scheduler.getResult<float>(task4), 10.0f);
}

TEST(ClassMethodTest, MultipleInstancesOfSameClass) {
  TTaskScheduler scheduler;

  Calculator calc1;
  calc1.multiplier = 2;

  Calculator calc2;
  calc2.multiplier = 3;

  auto task1 = scheduler.add(&Calculator::multiply, calc1, 10);
  auto future1 = scheduler.getFutureResult<int>(task1);

  auto task2 = scheduler.add(&Calculator::multiply, calc2, future1);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(task1), 20);
  EXPECT_EQ(scheduler.getResult<int>(task2), 60);
}

TEST(ClassMethodTest, DifferentDataTypes) {
  TTaskScheduler scheduler;

  class DataProcessor {
  public:
    double processDouble(double d) const { return d * 1.5; }
    bool processBool(bool b) const { return !b; }
    char processChar(char c) const { return static_cast<char>(c + 1); }

    std::string numberToString(int n) const {
      return prefix + std::to_string(n);
    }

    std::string prefix = "Value: ";
  };

  DataProcessor processor;

  auto doubleTask =
      scheduler.add(&DataProcessor::processDouble, processor, 3.14);
  auto boolTask = scheduler.add(&DataProcessor::processBool, processor, true);
  auto charTask = scheduler.add(&DataProcessor::processChar, processor, 'A');

  auto numTask = scheduler.add([]() { return 42; });
  auto numFuture = scheduler.getFutureResult<int>(numTask);
  auto stringTask =
      scheduler.add(&DataProcessor::numberToString, processor, numFuture);

  scheduler.executeAll();

  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(doubleTask), 4.71);
  EXPECT_FALSE(scheduler.getResult<bool>(boolTask));
  EXPECT_EQ(scheduler.getResult<char>(charTask), 'B');
  EXPECT_EQ(scheduler.getResult<std::string>(stringTask), "Value: 42");
}

TEST(ClassMethodTest, ComplexComputation) {
  TTaskScheduler scheduler;

  class MathProcessor {
  public:
    int square(int x) const { return x * x; }
    int cube(int x) const { return x * x * x; }
    double sqrt(int x) const { return std::sqrt(static_cast<double>(x)); }
    int add(int x) const { return x + base; }

    int base = 10;
  };

  MathProcessor math;

  auto initial = scheduler.add([]() { return 5; });
  auto initialFuture = scheduler.getFutureResult<int>(initial);

  auto squared = scheduler.add(&MathProcessor::square, math, initialFuture);
  auto squaredFuture = scheduler.getFutureResult<int>(squared);

  auto added = scheduler.add(&MathProcessor::add, math, squaredFuture);
  auto addedFuture = scheduler.getFutureResult<int>(added);

  auto multiplied = scheduler.add([](int x) { return x * 3; }, addedFuture);
  auto multipliedFuture = scheduler.getFutureResult<int>(multiplied);

  auto result = scheduler.add(&MathProcessor::square, math, multipliedFuture);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(result), 11025);
}

TEST(ClassMethodTest, TaskState) {
  TTaskScheduler scheduler;

  std::vector<int> executionOrder;

  auto task1 = scheduler.add([&executionOrder]() {
    executionOrder.push_back(1);
    return 5;
  });

  auto future1 = scheduler.getFutureResult<int>(task1);
  auto task2 = scheduler.add(
      [&executionOrder](int x) {
        executionOrder.push_back(2);
        return x * 2;
      },
      future1);

  auto future2 = scheduler.getFutureResult<int>(task2);
  auto task3 = scheduler.add(
      [&executionOrder](int x) {
        executionOrder.push_back(3);
        return x + 1;
      },
      future2);

  EXPECT_FALSE(task1->IsExecuted());
  EXPECT_FALSE(task2->IsExecuted());
  EXPECT_FALSE(task3->IsExecuted());

  task1->Execute();

  EXPECT_TRUE(task1->IsExecuted());
  EXPECT_FALSE(task2->IsExecuted());
  EXPECT_FALSE(task3->IsExecuted());

  scheduler.executeAll();

  EXPECT_TRUE(task1->IsExecuted());
  EXPECT_TRUE(task2->IsExecuted());
  EXPECT_TRUE(task3->IsExecuted());

  ASSERT_EQ(executionOrder.size(), 3);
  EXPECT_EQ(executionOrder[0], 1);
  EXPECT_EQ(executionOrder[1], 2);
  EXPECT_EQ(executionOrder[2], 3);

  EXPECT_EQ(scheduler.getResult<int>(task1), 5);
  EXPECT_EQ(scheduler.getResult<int>(task2), 10);
  EXPECT_EQ(scheduler.getResult<int>(task3), 11);
}

class ResourceOwner {
public:
  ResourceOwner() { constructCount++; }

  ResourceOwner(const ResourceOwner &) { copyCount++; }

  ResourceOwner(ResourceOwner &&) noexcept { moveCount++; }

  ~ResourceOwner() { destructCount++; }

  int process(int x) const { return x * x; }

  static void resetCounters() {
    constructCount = 0;
    copyCount = 0;
    moveCount = 0;
    destructCount = 0;
  }

  static int constructCount;
  static int copyCount;
  static int moveCount;
  static int destructCount;
};

int ResourceOwner::constructCount = 0;
int ResourceOwner::copyCount = 0;
int ResourceOwner::moveCount = 0;
int ResourceOwner::destructCount = 0;

TEST(ClassMethodTest, ComplexObjectLifecycle) {
  TTaskScheduler scheduler;

  ResourceOwner::resetCounters();

  {
    ResourceOwner owner;

    auto task1 = scheduler.add(&ResourceOwner::process, owner, 5);
    auto future1 = scheduler.getFutureResult<int>(task1);

    auto task2 = scheduler.add([](int x) { return x + 1; }, future1);

    scheduler.executeAll();

    EXPECT_EQ(scheduler.getResult<int>(task1), 25);
    EXPECT_EQ(scheduler.getResult<int>(task2), 26);
  }

  EXPECT_GE(ResourceOwner::constructCount, 1);
  EXPECT_GE(ResourceOwner::destructCount, 1);
}

TEST(ClassMethodTest, ContainerReturnTypes) {
  TTaskScheduler scheduler;

  class ContainerProcessor {
  public:
    std::vector<int> generateVector(int size) const {
      std::vector<int> result(size);
      for (int i = 0; i < size; ++i) {
        result[i] = i * i;
      }
      return result;
    }

    int sumVector(const std::vector<int> &vec) const {
      int sum = 0;
      for (int val : vec) {
        sum += val;
      }
      return sum;
    }
  };

  ContainerProcessor processor;

  auto task1 = scheduler.add(&ContainerProcessor::generateVector, processor, 5);
  auto future1 = scheduler.getFutureResult<std::vector<int>>(task1);

  auto task2 =
      scheduler.add(&ContainerProcessor::sumVector, processor, future1);

  scheduler.executeAll();

  auto result1 = scheduler.getResult<std::vector<int>>(task1);
  EXPECT_EQ(result1.size(), 5);
  EXPECT_EQ(result1[0], 0);
  EXPECT_EQ(result1[1], 1);
  EXPECT_EQ(result1[2], 4);
  EXPECT_EQ(result1[3], 9);
  EXPECT_EQ(result1[4], 16);

  EXPECT_EQ(scheduler.getResult<int>(task2), 30);
}

TEST(ClassMethodTest, ConstRefParameter) {
  TTaskScheduler scheduler;

  class RefProcessor {
  public:
    double process(const double &value) const { return value * 2.5; }
  };

  RefProcessor processor;

  double inputValue = 10.0;
  auto task = scheduler.add(&RefProcessor::process, processor, inputValue);

  scheduler.executeAll();

  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(task), 25.0);
}

TEST(ClassMethodTest, StateModification) {
  TTaskScheduler scheduler;

  class Counter {
  public:
    int increment(int amount) {
      count += amount;
      return count;
    }

    int getCount() const { return count; }

    int count = 0;
  };

  Counter counter;

  auto task1 = scheduler.add(&Counter::increment, counter, 5);
  auto future1 = scheduler.getFutureResult<int>(task1);

  auto task2 = scheduler.add(&Counter::increment, counter, 7);
  auto future2 = scheduler.getFutureResult<int>(task2);

  auto task3 = scheduler.add(&Counter::getCount, counter);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<int>(task1), 5);
  EXPECT_EQ(scheduler.getResult<int>(task2), 12);
  EXPECT_EQ(scheduler.getResult<int>(task3), 12);
  EXPECT_EQ(counter.count, 12);
}

TEST(ClassMethodTest, MethodChains) {
  TTaskScheduler scheduler;

  class Transformer {
  public:
    double multiplyByFactor(double value) const { return value * factor; }

    double addOffset(double value) const { return value + offset; }

    double factor = 2.0;
    double offset = 5.0;
  };

  Transformer transformer;

  auto initialValue = 10.0;

  auto task1 =
      scheduler.add(&Transformer::multiplyByFactor, transformer, initialValue);
  auto future1 = scheduler.getFutureResult<double>(task1);

  auto task2 = scheduler.add(&Transformer::addOffset, transformer, future1);
  auto future2 = scheduler.getFutureResult<double>(task2);

  auto task3 =
      scheduler.add(&Transformer::multiplyByFactor, transformer, future2);
  auto future3 = scheduler.getFutureResult<double>(task3);

  auto task4 = scheduler.add(&Transformer::addOffset, transformer, future3);

  scheduler.executeAll();

  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(task1), 20.0);
  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(task2), 25.0);
  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(task3), 50.0);
  EXPECT_DOUBLE_EQ(scheduler.getResult<double>(task4), 55.0);
}

TEST(ClassMethodTest, MultipleObjectDependencies) {
  TTaskScheduler scheduler;

  class StringManipulator {
  public:
    explicit StringManipulator(std::string prefix) : prefix_(prefix) {}

    std::string addPrefix(const std::string &text) const {
      return prefix_ + text;
    }

  private:
    std::string prefix_;
  };

  class NumberConverter {
  public:
    std::string toString(int number) const { return std::to_string(number); }
  };

  NumberConverter converter;
  StringManipulator prefixer1("ID: ");
  StringManipulator prefixer2("Result: ");

  auto task1 = scheduler.add(&NumberConverter::toString, converter, 42);
  auto future1 = scheduler.getFutureResult<std::string>(task1);

  auto task2 = scheduler.add(&StringManipulator::addPrefix, prefixer1, future1);
  auto future2 = scheduler.getFutureResult<std::string>(task2);

  auto task3 = scheduler.add(&StringManipulator::addPrefix, prefixer2, future2);

  scheduler.executeAll();

  EXPECT_EQ(scheduler.getResult<std::string>(task1), "42");
  EXPECT_EQ(scheduler.getResult<std::string>(task2), "ID: 42");
  EXPECT_EQ(scheduler.getResult<std::string>(task3), "Result: ID: 42");
}

TEST(ClassMethodTest, ConditionalExecution) {
  TTaskScheduler scheduler;

  class Validator {
  public:
    bool isValid(int value) const { return value > threshold; }

    std::string processValidValue(int value) const {
      return "Valid: " + std::to_string(value);
    }

    std::string processInvalidValue(int value) const {
      return "Invalid: " + std::to_string(value);
    }

    int threshold = 10;
  };

  Validator validator;

  int value1 = 15;
  auto validationTask1 = scheduler.add(&Validator::isValid, validator, value1);
  auto validResult1 = scheduler.getFutureResult<bool>(validationTask1);

  auto processTask1 = scheduler.add(
      [&validator](bool isValid, int value) {
        if (isValid) {
          return validator.processValidValue(value);
        } else {
          return validator.processInvalidValue(value);
        }
      },
      validResult1, value1);

  int value2 = 5;
  auto validationTask2 = scheduler.add(&Validator::isValid, validator, value2);
  auto validResult2 = scheduler.getFutureResult<bool>(validationTask2);

  auto processTask2 = scheduler.add(
      [&validator](bool isValid, int value) {
        if (isValid) {
          return validator.processValidValue(value);
        } else {
          return validator.processInvalidValue(value);
        }
      },
      validResult2, value2);

  scheduler.executeAll();

  EXPECT_TRUE(scheduler.getResult<bool>(validationTask1));
  EXPECT_FALSE(scheduler.getResult<bool>(validationTask2));
  EXPECT_EQ(scheduler.getResult<std::string>(processTask1), "Valid: 15");
  EXPECT_EQ(scheduler.getResult<std::string>(processTask2), "Invalid: 5");
}