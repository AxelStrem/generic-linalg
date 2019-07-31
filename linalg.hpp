#pragma once
#include <type_traits>

#define OPERATOR_ASSIGN(x) template<class RHS> Derived& operator##x##(const RHS& rhs) { return this->zip(rhs, [](auto& a, const auto& b) { a##x##b; }); }

template<class T, typename = void> class TestSize
{ 
public:
	static const int value = 1;
	static auto ptr(const T& v) { return &v; }
};

template<class T> class TestSize<T, std::void_t<decltype(T::Size)>>
{ 
public:
	static const int value = T::Size;
	static auto ptr(const T& v) { return v.begin(); }
};

template<class T, typename = void> struct CountDimensions
{  static const int value = 1; };
template<class T> struct CountDimensions<T, std::void_t<decltype(T::Width+T::Height)>>
{  static const int value = 2; };

template<class T, typename = void> struct ScalarType
{   using type = T;   };
template<class T> struct CountDimensions<T, std::void_t<typename T::Scalar>>
{	using type = typename T::Scalar;  };

template<class Derived, int N> class GenericAlgebra
{
	template<int M, class RHS, class F> Derived& zip_fixed(RHS rhs, const F& func)
	{
		auto p1 = ((Derived*)this)->begin();
		for (int i = 0; i < N; ++i)
		{
			func(p1[i], rhs[i%M]);
		}
		return *((Derived*)(this));
	}

	template<class RHS, class F>
	Derived& zip(const RHS& rhs, const F& func)
	{
		return zip_fixed<TestSize<RHS>::value>(TestSize<RHS>::ptr(rhs), func);
	}

public:

	OPERATOR_ASSIGN(=)
	OPERATOR_ASSIGN(+=)
	OPERATOR_ASSIGN(-=)
	OPERATOR_ASSIGN(*=)
	OPERATOR_ASSIGN(/=)

	void clear()
	{
		for (auto& x : *((Derived*)this))
			x = 0;
	}

};

template<class X, int N> class Vector : public GenericAlgebra<Vector<X,N>, N>
{
	X data[N];
public:
	static const int Size = N;
	using Scalar = X;

	const X* begin() const { return data; }
	const X* end() const { return data + N; }
	X* begin()  { return data; }
	X* end() { return data + N;	}

	X&       at(int i)       { return data[i]; }
	const X& at(int i) const { return data[i]; }
};

template<class X, int W, int H> class Matrix : public GenericAlgebra<Matrix<X, W, H>, W*H>
{
	X data[W*H];
public:
	static const int Size = W*H;
	static const int Width = W;
	static const int Height = H;
	using Scalar = X;

	const X* begin() const { return data; }
	const X* end() const { return data + Size; }
	X* begin() { return data; }
	X* end() { return data + Size; }

	      X& at(int j, int i)       { return data[i*W + j]; }
	const X& at(int j, int i) const { return data[i*W + j]; }

	template<class RHS> Matrix<X,W,H>& operator=(const RHS& rhs)
	{   return GenericAlgebra<Matrix<X, W, H>, W*H>::operator=(rhs);	}
};

template<class X, int W, int H> struct TransposedIterator
{
	X* data;
	int offset = 0;

	static int transpose_index(int index)
	{
		int j = index % W;
		int i = index / W;
		return j * H + i;
	}

	X& operator[](int index) { return data[transpose_index(index)]; }
	const X& operator[](int index) const { return data[transpose_index(index)]; }
};

template<class M> class MatrixTransposedReference : public GenericAlgebra<MatrixTransposedReference<M>, M::Size>
{
	M* pM;
public:
	using X = typename std::decay<decltype(*(pM->begin()))>::type;

	MatrixTransposedReference(M* pMatrix) : pM(pMatrix) {}

	static const int Size = M::Size;
	static const int Width = M::Height;
	static const int Height = M::Width;

	const TransposedIterator<X, Width, Height> begin() const { return TransposedIterator<X, Width, Height>{pM->begin()}; }
	const TransposedIterator<X, Width, Height> end() const { return TransposedIterator<X, Width, Height>{pM->begin(), Size}; }
	TransposedIterator<X, Width, Height> begin() { return TransposedIterator<X, Width, Height>{pM->begin()}; }
	TransposedIterator<X, Width, Height> end()   { return TransposedIterator<X, Width, Height>{pM->begin(), Size}; }

	X& at(int j, int i) { return pM->at(i,j); }
	const X& at(int j, int i) const { return pM->at(i, j); }

	template<class RHS> MatrixTransposedReference<M>& operator=(const RHS& rhs)
	{   return GenericAlgebra<MatrixTransposedReference<M>, M::Size>::operator=(rhs);   }
};

template<class M> class MatrixTransposedConstReference
{
	const M* pM;
public:
	using X = typename std::decay<decltype(*(pM->begin()))>::type;

	MatrixTransposedConstReference(const M* pMatrix) : pM(pMatrix) {}

	static const int Size = M::Size;
	static const int Width = M::Height;
	static const int Height = M::Width;

	const TransposedIterator<X, Width, Height> begin() const { return TransposedIterator<X, Width, Height>{pM->begin()}; }
	const TransposedIterator<X, Width, Height> end() const { return TransposedIterator<X, Width, Height>{pM->begin(), Size}; }

	const X& at(int j, int i) const { return pM->at(i, j); }
};

template<class M> MatrixTransposedReference<M> transpose(M& matrix)
{ 	return MatrixTransposedReference<M>(&matrix);  }

template<class M> MatrixTransposedConstReference<M> transpose(const M& matrix)
{	return MatrixTransposedConstReference<M>(&matrix); }

template<class LHS, class RHS, class R> void mult_add(R& out, const LHS& lhs, const RHS& rhs)
{
	if constexpr(CountDimensions<LHS>::value==2)
	{
		const int W = LHS::Width;
		const int H = LHS::Height;

		static_assert(H == RHS::Size);
		static_assert(W == R::Size);

		for (int i = 0; i < H; ++i)
			for (int j = 0; j < W; ++j)
			{
				out.at(j) += lhs.at(j, i) * rhs.at(i);
			}
	}
	else
	{
		const int W = RHS::Width;
		const int H = RHS::Height;

		static_assert(W == LHS::Size);
		static_assert(H == R::Size);

		for (int i = 0; i < H; ++i)
			for (int j = 0; j < W; ++j)
			{
				out.at(i) += lhs.at(j) * rhs.at(j, i);
			}
	}
}

template<class LHS, class RHS, class R> void mult(R& out, const LHS& lhs, const RHS& rhs)
{
	out.clear();
	return mult_add(out, lhs, rhs);
}