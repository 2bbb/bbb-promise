#pragma once

#ifndef bbb_promise_promise_hpp
#define bbb_promise_promise_hpp

#include <bbb/promise/type_traits.hpp>
#include <bbb/promise/base_promise.hpp>

namespace bbb {
	template <typename result_type>
	struct promise : base_promise {
		using ref = std::shared_ptr<promise>;
		
		struct defer {
			defer() : promise() {};
			defer(defer &&) = default;
			void resolve(result_type data)
			{ promise.set_value(data); }
			void reject(std::exception_ptr e)
			{ promise.set_exception(e); }
			std::promise<result_type> promise;
		};
		
		inline static ref create(std::function<void(defer &)> callback, bool sync = false) {
			return init_promise(std::make_shared<promise>(callback, false));
		}
		
		promise(std::function<void(defer &)> callback, bool sync = false)
		: callback(callback)
		, d()
		, future(new std::future<result_type>(d.promise.get_future()))
		{
			if(sync) {
				try {
					callback(d);
				} catch(...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
				finish_process();
			} else {
				thread = std::move(std::thread([=](defer d) {
					try {
						callback(d);
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
					finish_process();
				}, std::move(d)));
				thread.detach();
			}
		};
		
		promise(promise &&) = default;
		
		virtual ~promise() {
#if bbb_promise_debug_flag
			std::cout << "destruct " << typeid(decltype(*this)).name() << std::endl;
#endif
		};

	private:
		template <typename new_result_type>
		auto then_impl(std::function<new_result_type(result_type)> callback)
			-> enable_if_t<
				!std::is_same<new_result_type, void>::value,
				typename promise<new_result_type>::ref
			>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, future](typename new_promise::defer &d) {
					try {
						d.resolve(callback(future->get()));
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
				}
			));
		}
		
		typename promise<void>::ref then_impl(std::function<void(result_type)> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, future](typename new_promise::defer &d) {
					try {
						callback(future->get());
						d.resolve();
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
				}
			));
		}
		
		template <typename new_result_type>
		auto then_impl(
			std::function<new_result_type(result_type)> callback,
			std::function<new_result_type(std::exception_ptr)> err_callback
		)
			-> enable_if_t<
				!std::is_same<new_result_type, void>::value,
				typename promise<new_result_type>::ref
			>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, err_callback, future](typename new_promise::defer &d) {
					try {
						d.resolve(callback(future->get()));
					} catch(...) {
						try {
							std::exception_ptr err_ptr = std::current_exception();
							d.resolve(err_callback(err_ptr));
						} catch(...) {
							std::exception_ptr err_ptr = std::current_exception();
							d.reject(err_ptr);
						}
					}
				}
			));
		}
		
		typename promise<void>::ref then_impl(
			std::function<void(result_type)> callback,
			std::function<void(std::exception_ptr)> err_callback
		) {
			using new_promise = promise<void>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, err_callback, future](typename new_promise::defer &d) {
					try {
						callback(future->get());
						d.resolve();
					} catch(...) {
						try {
							std::exception_ptr err_ptr = std::current_exception();
							err_callback(err_ptr);
							d.resolve();
						} catch(...) {
							std::exception_ptr err_ptr = std::current_exception();
							d.reject(err_ptr);
						}
					}
				}
			));
		}
		
		auto except_impl(std::function<result_type(std::exception_ptr)> callback)
			-> enable_if_t<
				!std::is_same<result_type, void>::value,
				typename promise<result_type>::ref
			>
		{
			using new_promise = promise<result_type>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, future](typename new_promise::defer &d) {
					try {
						d.resolve(future->get());
					} catch(...) {
						try {
							std::exception_ptr err_ptr = std::current_exception();
							d.resolve(callback(err_ptr));
						} catch(...) {
							std::exception_ptr err_ptr = std::current_exception();
							d.reject(err_ptr);
						}
					}
				}
			));
		}
		
		typename promise<void>::ref except_impl(std::function<void(std::exception_ptr)> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return setup_promise(std::make_shared<new_promise>(
				[callback, future](typename new_promise::defer &d) {
					try {
						future->get();
						d.resolve();
					} catch(...) {
						try {
							std::exception_ptr err_ptr = std::current_exception();
							callback(err_ptr);
							d.resolve();
						} catch(...) {
							std::exception_ptr err_ptr = std::current_exception();
							d.reject(err_ptr);
						}
					}
				}
			));
		}

		std::function<void(defer &)> callback;
		defer d;
		std::shared_ptr<std::future<result_type>> future;
		std::thread thread;
		
	public:
		template <typename function_type>
		auto then(function_type callback)
			-> enable_if_t<
				has_call_operator<function_type>::value,
				decltype(then_impl(function_traits<function_type>::cast(callback)))
			>
		{
			return then_impl(function_traits<function_type>::cast(callback));
		};
		
		template <typename function_type, typename error_callback_type>
		auto then(function_type callback, error_callback_type error_callback)
			-> enable_if_t<
				has_call_operator<function_type>::value
				&& has_call_operator<error_callback_type>::value,
				decltype(then_impl(
					function_traits<function_type>::cast(callback),
					function_traits<error_callback_type>::cast(error_callback)
				))
			>
		{
			return then_impl(
				function_traits<function_type>::cast(callback), 
				function_traits<error_callback_type>::cast(error_callback)
			);
		};
		
		template <typename function_type>
		auto except(function_type callback)
			-> enable_if_t<
				has_call_operator<function_type>::value,
				decltype(except_impl(function_traits<function_type>::cast(callback)))
			>
		{
			return except_impl(function_traits<function_type>::cast(callback));
		};
		
		result_type await() {
			std::promise<result_type> p;
			std::future<result_type> v = p.get_future();
			
			then([&p](result_type result){
				p.set_value(result);
			}, [&p](std::exception_ptr err_ptr) {
				p.set_exception(err_ptr);
			});
			return v.get();
		}
	};
};

#endif
