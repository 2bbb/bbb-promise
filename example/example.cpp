#include <iostream>
#include <typeinfo>

#include <bbb/promise.hpp>

struct exception : std::exception {
	exception(std::string w) : w(w) {};
	std::string w;
	virtual const char* what() const noexcept { return w.c_str(); };
};

int main(int argc, char *argv[]) {
	std::promise<int> promise;
	std::future<int> future = promise.get_future();
	
	try {
		bbb::resolve(4)
			.then([](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				std::cout << "1st " << x << std::endl;
				throw 1;
				throw exception("A");
				return x * 2;
			})
			.then(
				[](int x) {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					std::cout << "2nd " << x << std::endl;
					throw exception("B");
					return x * 2;
				}
				, [](std::exception_ptr errp) {
					try {
						std::rethrow_exception(errp);
					} catch(std::exception &e) {
						std::cerr << "2nd catch except " << e.what() << std::endl;
					}
					return 3;
				}
			)
			.then([&](int x) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				std::cout << "3rd " << x << std::endl;
				throw exception("C");
				promise.set_value(x * 2);
			})
			.except([&](std::exception_ptr ptr) -> void {
				if(!ptr) return;
				std::cout << "except" << std::endl;
				try {
					std::rethrow_exception(ptr);
				} catch(std::exception &err) {
					std::cerr << err.what() << std::endl;
					promise.set_value(-1);
				} catch(int x) {
					std::cerr << "catch int " << x << std::endl;
					promise.set_value(x);
				}
			});
	} catch(std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	std::cout << future.get() << std::endl;
	return 0;
}
