#include <iostream>
#include <typeinfo>

#include <bbb/promise.hpp>

struct exception : std::exception {
	exception(std::string w) : w(w) {};
	std::string w;
	virtual const char* what() const noexcept { return w.c_str(); };
};

int main(int argc, char *argv[]) {
	auto &promise = bbb::resolve(4)
		.then([](int x) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			std::cout << "1st " << x << std::endl;
			throw 1;
			return x * 2;
		})
		.then(
			[](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				std::cout << "2nd " << x << std::endl;
				return x * 2;
			}
			, [](std::exception_ptr errp) {
				try {
					std::rethrow_exception(errp);
				} catch(std::exception &e) {
					std::cerr << "2nd catch except: " << e.what() << std::endl;
				} catch(int n) {
					std::cerr << "2nd catch except: " << n << std::endl;
					return n;
				}
				return 3;
			}
		)
		.then([](int x) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			std::cout << "3rd " << x << std::endl;
//			throw exception("C");
			return x;
		})
		.except([](std::exception_ptr ptr) {
			if(!ptr) return 0;
			std::cout << "except" << std::endl;
			try {
				std::rethrow_exception(ptr);
			} catch(std::exception &err) {
				std::cerr << err.what() << std::endl;
				return -1;
			} catch(int x) {
				std::cerr << "catch int " << x << std::endl;
				return x;
			}
		});
	auto result = bbb::await(promise);
	std::cout << "await: " << result << std::endl;
	return 0;
}
