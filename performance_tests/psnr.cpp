/*!
* PSNR (Peak Signal to Noise Ratio): The PSNR represents a measure of the peak error between the compressed and the original image.
* It is clsely related to MSE which represents the cumulative squared error between the compressed and the original image.
*/

#include <iostream>
#include <cmath>

#include <skepu>
#include "performance_tests_common.hpp"


const size_t rows = 250;
const size_t cols = 250;
std::string application = "PSNR";

[[skepu::userconstant]] constexpr int
	MAX = 255,
	NOISE = 10;

float diff_squared(int a, int b)
{
	return (a - b) * (a - b);
}

template<typename T>
T sum(T a, T b)
{
	return a + b;
}

template<typename T>
T clamp_sum(T a, T b)
{
	T temp = a + b;
	return temp < 0 ? 0 : (temp > MAX ? MAX : temp);
}


auto clamped_sum = skepu::Map<2>(clamp_sum<int>);
auto squared_diff_sum = skepu::MapReduce<2>(diff_squared, sum<float>);

double psnr() {
	skepu::Timer timer;
	for(size_t test = 0; test < NUM_REPEATS; ++test) {
		skepu::Matrix<int> img(rows, cols), noise(rows, cols);
		skepu::Matrix<int> comp_img(rows, cols);
		
		// Generate random image and random noise
		img.randomize(0, MAX);
		noise.randomize(-NOISE, NOISE);
		

		timer.start();
		// Add noise
		clamped_sum(comp_img, img, noise);
		
		float mse = squared_diff_sum(img, comp_img) / (rows * cols);
		float result = 10 * log10((MAX * MAX) / mse);
		timer.stop();
	}
	return timer.getMedianTime();
}


constexpr auto benchmarkFunc = psnr;

void setBackend(const skepu::BackendSpec& spec) {
	clamped_sum.setBackend(spec);
	squared_diff_sum.setBackend(spec);
}

void tune() {
	skepu::backend::tuner::hybridTune(clamped_sum, 16, 1, 32, 4096);
	skepu::backend::tuner::hybridTune(squared_diff_sum, 16, 1, 32, 4096);
	clamped_sum.resetBackend();
	squared_diff_sum.resetBackend();
}

int main(int argc, char* argv[]) {
	std::vector<double> times;
	
	std::cout << application << ": Running CPU backend" << std::endl;
	skepu::BackendSpec specCPU(skepu::Backend::Type::CPU);
	setBackend(specCPU);
	double cpuTime = benchmarkFunc();
	
	std::cout << application << ": Running OpenMP backend" << std::endl;
	skepu::BackendSpec specOpenMP(skepu::Backend::Type::OpenMP);
	specOpenMP.setCPUThreads(16);
	setBackend(specOpenMP);
	times.push_back(benchmarkFunc());
	
	std::cout << application << ": Running CUDA GPU backend" << std::endl;
	skepu::BackendSpec specGPU(skepu::Backend::Type::CUDA);
	specGPU.setDevices(1);
	setBackend(specGPU);
	times.push_back(benchmarkFunc());
	
	
	std::cout << application << ": Running Oracle" << std::endl;
	double bestOracleTime = 100000000.0;
	size_t bestOracleRatio = 9999999;
	std::vector<double> oracleTimes;
	
	for(size_t ratio = 0; ratio <= 100; ratio += 5) {
		for(size_t ratio2 = 0; ratio2 <= 100; ratio2 += 5) {
			double percentage = (double)ratio / 100.0;
			skepu::BackendSpec spec(skepu::Backend::Type::Hybrid);
			spec.setDevices(1);
			spec.setCPUThreads(16);
			spec.setCPUPartitionRatio(percentage);
			
			double percentage2 = (double)ratio2 / 100.0;
			skepu::BackendSpec spec2(skepu::Backend::Type::Hybrid);
			spec2.setDevices(1);
			spec2.setCPUThreads(16);
			spec2.setCPUPartitionRatio(percentage2);
			
			clamped_sum.setBackend(spec);
			squared_diff_sum.setBackend(spec2);
// 			setBackend(spec);
			double time = benchmarkFunc();
			
			oracleTimes.push_back(time);
			if(time < bestOracleTime) {
				bestOracleTime = time;
				bestOracleRatio = ratio;
			}
			std::cout << "Ratio: " << percentage << " gave time: " << time << std::endl;
		}
	}
	times.push_back(bestOracleTime);
	std::cout << "Optimal ratio was: " << bestOracleRatio << std::endl;
	
	std::cout << application << ": Running Hybrid backend" << std::endl;
	tune();
	times.push_back(benchmarkFunc());
	
	appendPerformanceResult("speedup_openmp.dat", application, cpuTime/times[0]);
	appendPerformanceResult("speedup_cuda.dat", application, cpuTime/times[1]);
	appendPerformanceResult("speedup_oracle.dat", application, cpuTime/times[2]);
	appendPerformanceResult("speedup_hybrid.dat", application, cpuTime/times[3]);
	
	return 0;
}
