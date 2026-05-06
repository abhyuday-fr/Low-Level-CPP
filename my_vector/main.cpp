#include <iostream>
#include <stdexcept>

namespace my{
  template <typename T>
  class vector{
    private:
      T* data;
      size_t _size;
      size_t _capacity;

      void resize(size_t new_capacity){
        T* new_data = new T[new_capacity];
        for(size_t i = 0; i < _size; i++){
          new_data[i] = data[i];
        }

        delete[] data;
        data = new_data;
        _capacity = new_capacity;
      }
    
    public:
      vector() : data(nullptr), _size(0), _capacity(0) {}

      ~vector(){
        delete[] data;
      }

      void push_back(const T& value){
        if(_size == _capacity){
          size_t new_capacity = (_capacity == 0) ? 1 : _capacity * 2;
          resize(new_capacity);
        }
        data[_size++] = value;
      }

      size_t size() const { return _size; }

      size_t capacity() const { return _capacity; }

      T& operator[](size_t index){
        if(index >= _size){
          throw std::out_of_range("Index out of range");
        }
        return data[index];
      }

      const T& operator[](size_t index) const {
        if(index >= _size){
          throw std::out_of_range("Index out of range");
        }
        return data[index];
      }

      void reserve(size_t new_capacity) {
        if (new_capacity <= _capacity) return; 

        T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));

        for (size_t i = 0; i < _size; i++) {
            new (&new_data[i]) T(std::move(data[i]));
            data[i].~T(); 
        }
        
        ::operator delete(data);

        data = new_data;
        _capacity = new_capacity;
      }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
      if (_size == _capacity) {
          reserve(_capacity == 0 ? 1 : _capacity * 2);
      }
      new (&data[_size]) T(std::forward<Args>(args)...);
      return data[_size++];
    }

    template <typename... Args>
    T* emplace(T* pos, Args&&... args) {
        size_t index = pos - data;
        
        if (_size == _capacity) {
            reserve(_capacity == 0 ? 1 : _capacity * 2);
            pos = data + index;
        }

        for (size_t i = _size; i > index; --i) {
            new (&data[i]) T(std::move(data[i - 1]));
            data[i - 1].~T();
        }

        new (&data[index]) T(std::forward<Args>(args)...);
        ++_size;

        return data + index;
    }

    T* begin() { return data; }
    T* end() { return data + _size; }
    const T* begin() const { return data; }
    const T* end() const { return data + _size; }

    void pop_back() {
        if (_size > 0) {
            --_size;
            data[_size].~T();
        }
    }

    void clear() {
        for (size_t i = 0; i < _size; i++) {
            data[i].~T();
        }
        _size = 0;
    }

    void shrink_to_fit() {
      if (_capacity == _size) {
          return; 
      }

      if (_size == 0) {
          ::operator delete(data);
          data = nullptr;
          _capacity = 0;
          return;
      }

      T* new_data = static_cast<T*>(::operator new(_size * sizeof(T)));
\
      for (size_t i = 0; i < _size; i++) {
          new (&new_data[i]) T(std::move(data[i]));
          data[i].~T();
      }

      ::operator delete(data);

      data = new_data;
      _capacity = _size;
    }
  };
}

int main(){
  my::vector<int> a;
  a.push_back(1);
  a.push_back(2);
  for(const auto& num : a){
    std::cout << num << "\n";
  }
}