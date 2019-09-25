#include "common.hpp"

#ifdef TEST_MAPREDUCE_VECTOR

namespace test_map_vector
{
		size_t seq_init_impl(skepu::Index1D index) {
				return index.i;
		}
		auto seq_init = skepu::Map<0>(seq_init_impl);

		size_t add_impl(size_t x, size_t y) {
				return x + y;
		}

		size_t mult_impl(size_t x, size_t y) {
				return x * y;
		}
		auto dotprod = skepu::MapReduce<2>(mult_impl, add_impl);

		TEST_CASE( "Map on skepu::Vector" ) {
				FOR_N {
				skepu::Vector<size_t> a(n);
				skepu::Vector<size_t> b(n);
				seq_init(a);
				seq_init(b);

				SECTION ( "dotproduct")  {
						auto res = dotprod(a.begin(), a.end(),b.begin());
						size_t correct = 0;
						for (size_t i = 0; i < n; ++i){
								correct += i*i;
						}
						assert (correct == res);
				}

		}
}

#endif
