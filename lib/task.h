#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct is_member_function_pointer : std::is_member_function_pointer<T> {};

template <typename ReturnType>
class CallableBase {
public:
    virtual ~CallableBase() = default;
    virtual ReturnType invoke() = 0;
    virtual CallableBase* clone() const = 0;
};

template <typename ReturnType, typename CallableType>
class CallableHolder : public CallableBase<ReturnType> {
public:
    explicit CallableHolder(CallableType callable) : callable_(std::move(callable)) {}
    
    ReturnType invoke() override {
        return callable_();
    }
    
    CallableBase<ReturnType>* clone() const override {
        return new CallableHolder(callable_);
    }
    
private:
    CallableType callable_;
};

template <typename ReturnType>
class Function {
public:
    Function() : impl_(nullptr) {}
    
    template <typename CallableType>
    Function(CallableType callable) 
        : impl_(new CallableHolder<ReturnType, CallableType>(std::move(callable))) {}
    
    Function(const Function& other) 
        : impl_(other.impl_ ? other.impl_->clone() : nullptr) {}
    
    Function(Function&& other) noexcept : impl_(other.impl_) {
        other.impl_ = nullptr;
    }
    
    Function& operator=(const Function& other) {
        if (this != &other) {
            delete impl_;
            impl_ = other.impl_ ? other.impl_->clone() : nullptr;
        }
        return *this;
    }
    
    Function& operator=(Function&& other) noexcept {
        if (this != &other) {
            delete impl_;
            impl_ = other.impl_;
            other.impl_ = nullptr;
        }
        return *this;
    }
    
    ~Function() {
        delete impl_;
    }
    
    ReturnType operator()() {
        if (!impl_) {
            throw std::bad_function_call();
        }
        return impl_->invoke();
    }
    
    explicit operator bool() const {
        return impl_ != nullptr;
    }
    
private:
    CallableBase<ReturnType>* impl_;
};

template <typename T> class FutureResult;

class TaskBase {
public:
  TaskBase() : executed_(false) {}

  virtual ~TaskBase() {}

  virtual void Execute() = 0;

  bool IsExecuted() const { return executed_; }

  void AddDependendTask(std::shared_ptr<TaskBase> task) {
    dependencies_.push_back(task);
  }

  const std::vector<std::shared_ptr<TaskBase>> GetDependecies() {
    return dependencies_;
  }

protected:
  bool executed_;
  std::vector<std::shared_ptr<TaskBase>> dependencies_;
};

template <typename ReturnType> 
class Task : public TaskBase {
public:
  template <typename Callable,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable)
      : callable_([callable = std::forward<Callable>(callable)]() {
          return callable();
        }) {}

  template <typename Callable, typename Arg1,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1)]() {
          return callable(getValue(arg1));
        }) {}

  template <typename Callable, typename Arg1, typename Arg2,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1, Arg2&& arg2)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1),
                   arg2 = std::forward<Arg2>(arg2)]() {
          return callable(getValue(arg1), getValue(arg2));
        }) {}

  template <typename ClassType>
  Task(ReturnType (ClassType::*method)() const, ClassType& instance)
      : callable_([method, &instance]() { return (instance.*method)(); }) {}
        
  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(T) const, ClassType &instance,
       const FutureResult<T> &future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(const T&) const, ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(ReturnType (ClassType::*method)(const T&), ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          return (instance.*method)(getValue(future));
        }) {}
  
  template <typename ClassType, typename MethodArg, typename Arg1>
  Task(ReturnType (ClassType::*method)(MethodArg), ClassType& instance, Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          return (instance.*method)(getValue(arg1));
        }) {}

  template <typename ClassType, typename MethodArg, typename Arg1>
  Task(ReturnType (ClassType::*method)(MethodArg) const, ClassType& instance,
       Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          return (instance.*method)(getValue(arg1));
        }) {}

  void Execute() override {
    if (!IsExecuted()) {
      try {
        if (!dependencies_.empty()) {
          for (auto& elem: dependencies_) {
            elem->Execute();
          }
        }
        result_ = callable_();
        executed_ = true;
      }
      catch(...) {
        throw;
      }
    }
  }

  const ReturnType& GetResult() {
    if (!IsExecuted()) {
      Execute();
    }
    return result_;
  }

  
private:
  Function<ReturnType> callable_;
  ReturnType result_;
  template <typename T> friend class FutureResult;

  template <typename T>
  static const T& getValue(const FutureResult<T> &future) {
    return future.get();
  }

  template <typename Arg> static Arg getValue(const Arg& arg) { return arg; }
};

template <> 
class Task<void> : public TaskBase {
public:
  template <typename Callable,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable)
      : callable_([callable = std::forward<Callable>(callable)]() {
          callable();
        }) {}

  template <typename Callable, typename Arg1,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1)]() {
          callable(getValue(arg1));
        }) {}

  template <typename Callable, typename Arg1, typename Arg2,
  typename = std::enable_if_t<!is_member_function_pointer<std::decay_t<Callable>>::value>>
  Task(Callable&& callable, Arg1&& arg1, Arg2&& arg2)
      : callable_([callable = std::forward<Callable>(callable),
                   arg1 = std::forward<Arg1>(arg1),
                   arg2 = std::forward<Arg2>(arg2)]() {
          callable(getValue(arg1), getValue(arg2));
        }) {}

  template <typename ClassType>
  Task(void (ClassType::*method)() const, ClassType& instance)
      : callable_([method, &instance]() { (instance.*method)(); }) {}
        
  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(T) const, ClassType &instance,
       const FutureResult<T> &future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(const T&) const, ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}

  template <typename ClassType, typename T>
  Task(void (ClassType::*method)(const T&), ClassType &instance,
        const FutureResult<T>& future)
      : callable_([method, &instance, future]() {
          (instance.*method)(getValue(future));
        }) {}
  
  template <typename ClassType, typename MethodArg, typename Arg1>
  Task(void (ClassType::*method)(MethodArg), ClassType& instance, Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          (instance.*method)(getValue(arg1));
        }) {}

  template <typename ClassType, typename Arg1, typename MethodArg>
  Task(void (ClassType::*method)(MethodArg) const, ClassType& instance,
       Arg1&& arg1)
      : callable_([method, &instance, arg1 = std::forward<Arg1>(arg1)]() {
          (instance.*method)(getValue(arg1));
        }) {}

  void Execute() override {
    if (!IsExecuted()) {
      try {
        if (!dependencies_.empty()) {
          for (auto& elem: dependencies_) {
            elem->Execute();
          }
        }
        callable_();
        executed_ = true;
      }
      catch(...) {
        throw;
      }
    }
  }

  void GetResult() {
    if (!IsExecuted()) {
      Execute();
    }
  }

private:
  Function<void> callable_;

  template <typename T>
  static const T& getValue(const FutureResult<T> &future) {
    return future.get();
  }

  template <typename Arg> static Arg getValue(const Arg& arg) { return arg; }
};


template <typename T> class FutureResult {
public:
  using value_type = T;
  explicit FutureResult(std::shared_ptr<Task<T>> task) : task_(task) {}

  const T& get() const { return task_->result_; }

  std::shared_ptr<Task<T>> getTask() const { return task_; }

private:
  std::shared_ptr<Task<T>> task_;
};

