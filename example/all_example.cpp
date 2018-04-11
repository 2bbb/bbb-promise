#include <bbb/promise.hpp>

struct exception : std::exception {
	exception(std::string w) : w(w) {};
	std::string w;
	virtual const char* what() const noexcept { return w.c_str(); };
};

int mul_2(int x) {
	return 2 * x;
}

int main(int argc, char *argv[]) {
	auto all_result = bbb::all(
		bbb::create_promise([] { return 1; })
			->then([](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				std::cout << "1st" << std::endl;
				return x;
			}),
		bbb::create_promise([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			return 2;
		})
			->then([](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				std::cout << "2nd" << std::endl;
				return x;
			}),
		bbb::create_promise([] { return 3; })
			->then([](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				std::cout << "3rd" << std::endl;
			}),
		bbb::create_promise([] { return 4; })
			->then(mul_2)
	)->then([](std::tuple<int, int, int, int> x) {
		std::cout << std::get<0>(x) << ", " << std::get<1>(x) << ", " << std::get<2>(x) << ", " << std::get<3>(x) << std::endl;
		return 6;
	});
	
	bbb::await(all_result);
	
//	all_result->clear();
	return 0;
}
