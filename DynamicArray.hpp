#pragma once

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <stdexcept>

template <typename T>
class DynamicArray
{
  public:
    DynamicArray(size_t _n=0) : 
      n{_n}, 
      cap{n == 0 ? 32 : n*2}, 
      mem{ static_cast<T*>(std::malloc(cap*sizeof(T))) }
    {
      checkAllocErr();
    }

    T& operator[](size_t i)
    {
      if(i > n-1)
        throw oobErr();

      return mem[i];
    }

    const T& operator[](size_t i) const
    { return const_cast<DynamicArray&>(*this).operator[](i); }

    template <typename U>
    void Append(U&& elem) 
    { 
      Insert(n, std::forward<U>(elem));
    }

    template <typename U>
    void Insert(size_t i, U&& elem)
    {
      if(i > n)
        throw oobErr();

      if(n == cap)
        resize(cap*2);

      std::memmove(&mem[i+1], &mem[i], (n-i)*sizeof(T));

      ++n;
      new(&mem[i]) T{ std::forward<U>(elem) };
    }

    void Delete(size_t i)
    {
      if(i >= n)
        throw oobErr();

      std::memmove(&mem[i], &mem[i+1], (--n-i)*sizeof(T));
    }

    size_t Size() const { return n; }
    size_t Capacity() const { return cap; }

    ~DynamicArray()
    {
      std::free(mem);
    }

  private:
    size_t n;
    size_t cap;
    T* mem;

    void resize(size_t newCap)
    {
      cap = newCap;
      mem = static_cast<T*>(std::realloc(mem, cap*sizeof(T)));
      checkAllocErr();
    }

    void checkAllocErr()
    {
      if(!mem)
        throw std::runtime_error{std::strerror(errno)};
    }

    std::runtime_error oobErr()
    {
      return std::runtime_error{"out of bounds"};
    }
};

