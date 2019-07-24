#pragma once
#include <type_traits>

#define OPERATOR_ASSIGN(x) template<class RHS> Derived& operator##x##(const RHS& rhs) { return this->zip(rhs, [](auto& a, const auto& b) { a##x##b; }); }

template<class T, typename = void> class TestWidth
{ 
public:
	static const int value = 1;
	static auto ptr(const T& v) { return &v; }
};

template<class T> class TestWidth<T, std::void_t<decltype(T::Width)>>
{ 
public:
	static const int value = T::Width;
	static auto ptr(const T& v) { return v.begin(); }
};

template<class Derived, int N> class GenericAlgebra
{
	template<int M, class RHS, class F> Derived& zip_fixed(const RHS* rhs, const F& func)
	{
		auto p1 = ((Derived*)this)->begin();
		for (int i = 0; i < N; ++i)
		{
			func(p1[i], rhs[i%M]);
		}
		return *((Derived*)(this));
	}

	/*template<class RHS, class F> Derived& zip(const RHS& rhs, const F& func)
	{
		if constexpr (std::is_integral<decltype(RHS::Width)>::value)
			return zip_fixed<RHS::Width>(rhs.begin(), func);
		else
			return zip_fixed<1>(&rhs, func);
	}*/

	/*
	template<class RHS, class F, typename std::enable_if<std::is_integral<decltype(RHS::Width)>::value == true, int>::type = 0>
	Derived& zip(const RHS& rhs, const F& func)
	{
		return zip_fixed<RHS::Width>(rhs.begin(), func);
	}*/

	template<class RHS, class F>//, typename std::enable_if<std::is_integral<decltype(RHS::Width)>::value == false, int>::type = 0>
	Derived& zip(const RHS& rhs, const F& func)
	{
		return zip_fixed<TestWidth<RHS>::value>(TestWidth<RHS>::ptr(rhs), func);
	}

public:

	OPERATOR_ASSIGN(+=)
	OPERATOR_ASSIGN(-=)
	OPERATOR_ASSIGN(*=)
	OPERATOR_ASSIGN(/=)


	//__forceinline Derived& operator-=(const Derived& rhs) { return this->zip(rhs); }
	//__forceinline Derived& operator*=(const Derived& rhs) { return this->zip(rhs); }
	//__forceinline Derived& operator/=(const Derived& rhs) { return this->zip(rhs); }
};

template<class X, int N> class Vector : public GenericAlgebra<Vector<X,N>, N>
{
	X data[N];
public:
	static const int Width = N;

	const X* begin() const { return data; }
	const X* end() const { return data + N; }
	X* begin()  { return data; }
	X* end() { return data + N;	}
};