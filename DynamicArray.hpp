#pragma once

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class DynamicArray
{
  public:
    DynamicArray() : n{0}, cap{4}, mem{alloc(cap)} {}

    DynamicArray(const DynamicArray& other): n{other.n}, cap{other.cap}, mem{alloc(cap)}
    {
      for(size_t i = 0; i < n; ++i)
        new (&mem[i]) T( other[i] );
    }

    DynamicArray& operator=(const DynamicArray& other)
    {
      if(this == &other)
        return *this;

      //NOTE: other array is bigger than we have capacity for, so destruct
      //everything and realloc to its capacity
      if(other.n > cap)
      {
        destructAndFree(mem, n);
        n   = other.n;
        cap = other.cap;
        mem = alloc(cap);
        for(size_t i = 0; i < n; ++i)
          new (&mem[i]) T( other[i] );
      }
      else //NOTE: other array can fit within our existing capacity
      {
        while(n > 0)
          operator[](--n).~T();

        n = other.n;
        for(size_t i = 0; i < n; ++i)
          new (&mem[i]) T( other[i] );
      }

      return *this;
    }

    DynamicArray(DynamicArray&& other): n{other.n}, cap{other.cap}, mem{other.mem}
    {
      other.n   = 0;
      other.cap = 0;
      other.mem = nullptr;
    }

    DynamicArray& operator=(DynamicArray&& other)
    {
      if(this != &other)
      {
        destructAndFree(mem, n);
        n   = std::exchange(other.n, 0);
        cap = std::exchange(other.cap, 0);
        mem = std::exchange(other.mem, nullptr);
      }

      return *this;
    }

    ~DynamicArray()
    {
      destructAndFree(mem, n);
    }

    const T& operator[](size_t i) const { return mem[i]; }
    T& operator[](size_t i) 
    { 
      return const_cast<T&>(
             const_cast<const DynamicArray&>(*this).operator[](i));
    }

    const T& At(size_t i) const
    {
      if(i >= n)
        throw oobErr();

      return operator[](i);
    }

    T& At(size_t i) 
    { 
      return const_cast<T&>(
             const_cast<const DynamicArray&>(*this).At(i));
    }

    template <typename U>
    void Append(U&& elem) { Insert(n, std::forward<U>(elem)); }

    template <typename U>
    void Insert(size_t i, U&& elem)
    {
      if(i > n)
        throw oobErr();

      if constexpr (std::is_trivially_copyable<T>::value)
        makeRoomAt_trivial(i);
      else
        makeRoomAt_nontrivial(i);

      new(&mem[i]) T( std::forward<U>(elem) );
      ++n;
    }

    void Delete(size_t i)
    {
      if(i >= n)
        throw oobErr();

      if constexpr ( !std::is_trivially_copyable<T>::value )
      {
        mem[i].~T();
        for(size_t elemI = i+1; elemI < n; ++elemI)
        {
          new (&mem[elemI-1]) T( std::move(mem[elemI]) );
          mem[elemI].~T();
        }
      }
      else
      {
        std::memmove(&mem[i], &mem[i+1], (n-i-1)*sizeof(T));
      }

      --n;
    }

    size_t Size() const { return n; }
    size_t Capacity() const { return cap; }

  private:
    size_t n;
    size_t cap;
    T* mem;

    void destructAndFree(T* mem, size_t n)
    {
      while(n > 0)
        operator[](--n).~T();

      std::free(mem);
    }

    //INFO: makes room for 1 element but does not increase n
    //assumes arg is already bounds checked.
    //NOTE: since T is trivially copyable its safe to just memmove & realloc 
    //the underlying memory of the elements.
    void makeRoomAt_trivial(size_t i)
    {
      if(n == cap)
      {
        mem = static_cast<T*>( std::realloc(mem, newCap() * sizeof(T)) );

        if(!mem)
          throw std::runtime_error{std::strerror(errno)};
      }

      std::memmove(&mem[i+1], &mem[i], (n-i)*sizeof(T));
    }

    //INFO: makes room for 1 element but does not increase n
    //assumes arg is already bounds checked.
    void makeRoomAt_nontrivial(size_t i)
    {
      //NOTE: case where we have enough capacity & only need to move elements 
      //ahead of where we need to make room for.
      if(n < cap)
      {
        makeRoomAt_simple_nontrivial(i);
        return;
      }

      //NOTE: case where we dont have enough capacity and need to resize mem & 
      //move all elements anyway.
      T* newMem = alloc(newCap());

      //NOTE: init elements behind i to new memory exactly relative to where 
      //they were
      for(size_t elemI = 0; elemI < i; ++elemI)
        new (&newMem[elemI]) T( std::move(mem[elemI]) );

      //NOTE: init elements at and after i to new memory relative to where 
      //they were with 1 element offset
      for(size_t elemI = i; elemI < n; ++elemI)
        new (&newMem[elemI+1]) T( std::move(mem[elemI]) );

      destructAndFree(mem, n);
      mem = newMem;
    }

    //INFO: makes room for 1 element but does not increase n.
    //Assumes arg is already bounds checked.
    //Assumes cap is at least n + 1.
    void makeRoomAt_simple_nontrivial(size_t i)
    {
      if(n == 0)
        return;

      //NOTE: Iterate backwards, constructing a new element 1 element ahead and 
      //then destructing to make room for the next
      for(size_t elemI = n; elemI-- > i; )
      {
        new (&mem[elemI+1]) T( std::move(mem[elemI]) );
        mem[elemI].~T();
      }

    }

    size_t newCap()
    {
      return cap == 0 ? cap=1 : cap*=2;
    }

    T* alloc(size_t nElems) const
    {
      T* mem = static_cast<T*>( std::malloc(nElems * sizeof(T)) );

      if(!mem)
        throw std::runtime_error{std::strerror(errno)};

      return mem;
    }

    std::out_of_range oobErr() const
    {
      return std::out_of_range{"out of bounds"};
    }
};

