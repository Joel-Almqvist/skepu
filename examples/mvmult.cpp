#include <iostream>
#include <skepu>


template<typename T>
T mvmult_f(skepu::Index1D row, const skepu::Mat<T> m, const skepu::Vec<T> v)
{
	T res = 0;
	for (size_t i = 0; i < v.size; ++i)
		res += m.data[row.i * m.cols + i] * v.data[i];
	return res;
}

// A helper function to calculate dense matrix-vector product. Used to verify that the SkePU output is correct.
template<typename T>
void directMV(skepu::Vector<T> &v, skepu::Matrix<T> &m, skepu::Vector<T> &res)
{
	int rows = m.size_i();
	int cols = m.size_j();
	
	for (int r = 0; r < rows; ++r)
	{
		T sum = T();
		for (int i = 0; i < cols; ++i)
		{
			sum += m(r,i) * v(i);
		}
		res(r) = sum;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		if(!skepu::cluster::mpi_rank())
			std::cout << "Usage: " << argv[0] << " size backend\n";
		exit(1);
	}
	
	size_t size = atoi(argv[1]);
	auto spec = skepu::BackendSpec{skepu::Backend::typeFromString(argv[2])};
	skepu::setGlobalBackendSpec(spec);
	
	skepu::Matrix<float> m(size, size);
	skepu::Vector<float> v(size), r(size), r2(size);
	m.randomize(3, 9);
	v.randomize(0, 9);
	
	m.flush();
	v.flush();
	if(!skepu::cluster::mpi_rank())
	{
	std::cout << "v: " << v << "\n";
	std::cout << "m: " << m << "\n";
	}

	r.flush();
	directMV(v, m, r);
	auto mvprod = skepu::Map(mvmult_f<float>);
	mvprod(r2, m, v);
	
	r.flush();
	r2.flush();

	if(!skepu::cluster::mpi_rank())
	{
		std::cout << "r: " << r << "\n";
		std::cout << "r2: " << r2 << "\n";
		
		for (size_t i = 0; i < size; i++)
			if (r(i) != r2(i))
				std::cout << "Output error at index " << i << ": " << r2(i) << " vs " << r(i) << "\n";
	}

	return 0;
}
