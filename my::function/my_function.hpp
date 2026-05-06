#ifndef _MY_FUNCTION_H_
#define _MY_FUNCTION_H_

#include <cstddef>
#include <functional>
// #include <memory>
// #include <optional>
#include <type_traits>
#include <utility>

namespace my {

template <typename Signature> class function;

template <typename R, typename... Args> class function<R(Args...)> {
private:
  struct CallableBase {
    virtual ~CallableBase() = default;
    virtual R invoke(Args &&...args) = 0;
    virtual CallableBase *clone(void *buffer) const = 0;
    virtual CallableBase *move_clone(void *buffer) = 0;
    virtual void destroy() = 0;
  };

  template <typename F> struct CallableModel : CallableBase {
    F functor_;

    template <typename U> CallableModel(U &&f) : functor_(std::forward<U>(f)) {}

    R invoke(Args &&...args) override {
      return std::invoke(functor_, std::forward<Args>(args)...);
    }

    CallableBase *clone(void *buffer) const override {
      if (buffer)
        return new (buffer) CallableModel(functor_);
      return new CallableModel(functor_);
    }

    CallableBase *move_clone(void *buffer) override {
      if (buffer)
        return new (buffer) CallableModel(std::move(functor_));
      return new CallableModel(std::move(functor_));
    }

    void destroy() override { this->~CallableModel(); }
  };

  static constexpr std::size_t SBO_SIZE = 32;
  alignas(std::max_align_t) std::byte buffer_[SBO_SIZE];
  CallableBase *callable_ = nullptr;

  bool is_sbo() const noexcept { return (void *)callable_ == (void *)buffer_; }

  template <typename F> static constexpr bool fits_in_sbo() noexcept {
    using ModelType = CallableModel<std::decay_t<F>>;
    return sizeof(ModelType) <= SBO_SIZE &&
           alignof(std::max_align_t) % alignof(ModelType) == 0;
  }

  void clear() {
    if (callable_) {
      if (is_sbo())
        callable_->destroy();
      else
        delete callable_;
      callable_ = nullptr;
    }
  }

public:
  function() noexcept = default;

  function(std::nullptr_t) noexcept {}

  template <typename F, typename = std::enable_if_t<
                            !std::is_same_v<std::decay<F>, function>>>
  function(F &&f) {
    using DecayedF = std::decay_t<F>;
    using ModelType = CallableModel<DecayedF>;

    if constexpr (fits_in_sbo<DecayedF>()) {
      callable_ = new (buffer_) ModelType(std::forward<F>(f));
    } else {
      callable_ = new ModelType(std::forward<F>(f));
    }
  }

  ~function() { clear(); }

  function(const function &other) {
    if (other.callable_) {
      void *buf_ptr = other.is_sbo() ? buffer_ : nullptr;
      callable_ = other.callable_->clone(buf_ptr);
    }
  }

  function(function &&other) noexcept {
    if (other.callable_) {
      if (other.is_sbo()) {
        callable_ = other.callable_->move_clone(buffer_);
        other.clear();
      } else {
        callable_ = other.callable_;
        other.callable_ = nullptr;
      }
    }
  }

  function &operator=(const function &other) {
    if (this != &other) {
      clear();
      if (other.callable_) {
        void *buff_ptr = other.is_sbo() ? buffer_ : nullptr;
        callable_ = other.callable_->clone(buff_ptr);
      }
    }
    return *this;
  }

  function &operator=(function &&other) noexcept {
    if (this != &other) {
      clear();
      if (other.callable_) {
        if (other.is_sbo()) {
          callable_ = other.callable_->move_clone(buffer_);
          other.clear();
        } else {
          callable_ = other.callable_;
          other.callable_ = nullptr;
        }
      }
    }
    return *this;
  }

  function &operator=(std::nullptr_t) noexcept {
    clear();
    return *this;
  }

  explicit operator bool() const noexcept { return callable_ != nullptr; }

  R operator()(Args... args) const {
    if (!callable_) {
      throw std::bad_function_call();
    }
    return callable_->invoke(std::forward<Args>(args)...);
  }
};

} // namespace my

#endif
