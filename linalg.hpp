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

	template<class RHS, class F>
	Derived& zip(const RHS& rhs, const F& func)
	{
		return zip_fixed<TestSize<RHS>::value>(TestSize<RHS>::ptr(rhs), func);
	}

public:

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

	const X* begin() const { return data; }
	const X* end() const { return data + Size; }
	X* begin() { return data; }
	X* end() { return data + Size; }

	      X& at(int j, int i)       { return data[i*W + j]; }
	const X& at(int j, int i) const { return data[i*W + j]; }

};

template<class M> class MatrixTransposedReference
{
	M* pM;
public:
	using X = typename std::decay<decltype(*(pM->begin()))>::type;

	MatrixTransposedReference(M* pMatrix) : pM(pMatrix) {}

	static const int Size = M::Size;
	static const int Width = M::Height;
	static const int Height = M::Width;

	const X* begin() const { return pM->begin(); }
	const X* end() const { return pM->end(); }
	X* begin() { return pM->begin(); }
	X* end() { return pM->end(); }

	X& at(int j, int i) { return pM->at(i,j); }
	const X& at(int j, int i) const { return pM->at(i, j); }
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

	const X* begin() const { return pM->begin(); }
	const X* end() const { return pM->end(); }

	const X& at(int j, int i) const { return pM->at(i, j); }
};

template<class M> MatrixTransposedReference<M> transpose(M& matrix)
{ 	return MatrixTransposedReference<M>(&matrix);  }

template<class M> MatrixTransposedConstReference<M> transpose(const M& matrix)
{	return MatrixTransposedConstReference<M>(&matrix); }

template<class LHS, class RHS, class R> void mult_add(R& out, const LHS& vec, const RHS& mat)
{
	const int W = RHS::Width;
	const int H = RHS::Height;

	static_assert(W == LHS::Size);
	static_assert(H == R::Size);

	for(int i = 0; i < H; ++i)
		for (int j = 0; j < W; ++j)
		{
			out.at(i) += mat.at(j, i) * vec.at(j);
		}
}

template<class LHS, class RHS, class R> void mult(R& out, const LHS& vec, const RHS& mat)
{
	out.clear();
	return mult_add(out, vec, mat);
}