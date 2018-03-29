#pragma once

#ifndef bbb_promise_hpp
#define bbb_promise_hpp

#include <iostream>
#include <vector>
#include <future>
#include <functional>
#include <memory>

#include "./core.hpp"
#include "./integer_sequence.hpp"
#include "./function_traits.hpp"

namespace bbb {
	template <typename result_type>
	struct promise;
	
	template<>
	struct promise<void> {
		struct defer {
			defer() : promise() {};
			defer(defer &&) = default;
			void resolve()
			{ promise.set_value(0); }
			void reject(std::exception_ptr e)
			{ promise.set_exception(e); }
			std::promise<uint8_t> promise;
		};
		
		promise(std::function<void(defer)> callback, bool sync = false)
		: callback(callback)
		, d()
		, future(new std::future<std::uint8_t>(d.promise.get_future()))
		{
			if(sync) {
				try {
					callback(std::move(d));
				} catch(...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
				std::this_thread::sleep_for(std::chrono::nanoseconds(10));
				delete this; 
			} else {
				thread = std::move(std::thread([=](defer d) {
					try {
						callback(std::move(d));
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
					std::this_thread::sleep_for(std::chrono::nanoseconds(10));
					delete this; 
				}, std::move(d)));
				thread.detach();
			}
		};
		
		promise(promise &&) = default;
		
		virtual ~promise() { std::cout << "destruct " << typeid(decltype(*this)).name() << std::endl; };
	private:
		template <typename new_result_type>
		auto then_impl(std::function<new_result_type()> callback)
			-> enable_if_t<!std::is_same<new_result_type, void>::value, promise<new_result_type> &>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
					try {
						future->get();
						d.resolve(callback());
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
				}
			));
		};
		
		promise<void> &then_impl(std::function<void()> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
					try {
						future->get();
						callback();
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
			std::function<new_result_type()> callback,
			std::function<new_result_type(std::exception_ptr)> err_callback
		)
			-> enable_if_t<!std::is_same<new_result_type, void>::value, promise<new_result_type> &>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return *(new new_promise(
				[callback, err_callback, future](typename new_promise::defer d) {
					try {
						future->get();
						d.resolve(callback());
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
		};
		
		promise<void> &then_impl(
			std::function<void()> callback,
			std::function<void(std::exception_ptr)> err_callback
		) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, err_callback, future](typename new_promise::defer d) {
					try {
						future->get();
						callback();
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
		
		promise<void> &except_impl(std::function<void(std::exception_ptr)> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
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
		
		std::function<void(defer)> callback;
		defer d;
		std::shared_ptr<std::future<uint8_t>> future;
		std::thread thread;
		
	public:
		template <typename function_type>
		auto then(function_type callback)
			-> enable_if_t<
				has_call_operator<function_type>::value,
				decltype(then_impl(function_traits<function_type>::cast(callback))) &
			>
		{
			return then_impl(function_traits<function_type>::cast(callback));
		};
		
		template <typename function_type, typename error_callback_type>
		auto then(function_type &&callback, error_callback_type &&error_callback)
			-> enable_if_t<
				has_call_operator<function_type>::value
				&& has_call_operator<error_callback_type>::value,
				decltype(then_impl(
					function_traits<function_type>::cast(callback),
					function_traits<error_callback_type>::cast(error_callback)
				)) &
			>
		{
			return then_impl(function_traits<function_type>::cast(callback));
		};

		template <typename function_type>
		auto except(function_type callback)
			-> enable_if_t<
				has_call_operator<function_type>::value,
				decltype(except_impl(function_traits<function_type>::cast(callback))) &
			>
		{
			return except_impl(function_traits<function_type>::cast(callback));
		};
	};

	template <typename result_type>
	struct promise {
		struct defer {
			defer() : promise() {};
			defer(defer &&) = default;
			void resolve(result_type data)
			{ promise.set_value(data); }
			void reject(std::exception_ptr e)
			{ promise.set_exception(e); }
			std::promise<result_type> promise;
		};
		
		promise(std::function<void(defer)> callback, bool sync = false)
		: callback(callback)
		, d()
		, future(new std::future<result_type>(d.promise.get_future()))
		{
			if(sync) {
				try {
					callback(std::move(d));
				} catch(...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
				delete this; 
			} else {
				thread = std::move(std::thread([=](defer d) {
					try {
						callback(std::move(d));
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
					delete this;
				}, std::move(d)));
				thread.detach();
			}
		};
		
		promise(promise &&) = default;
		
		virtual ~promise() { std::cout << "destruct " << typeid(decltype(*this)).name() << std::endl; };

	private:
		template <typename new_result_type>
		auto then_impl(std::function<new_result_type(result_type)> callback)
			-> enable_if_t<!std::is_same<new_result_type, void>::value, promise<new_result_type> &>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
					try {
						d.resolve(callback(future->get()));
					} catch(...) {
						std::exception_ptr err_ptr = std::current_exception();
						d.reject(err_ptr);
					}
				}
			));
		}
		
		promise<void> &then_impl(std::function<void(result_type)> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
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
			-> enable_if_t<!std::is_same<new_result_type, void>::value, promise<new_result_type> &>
		{
			using new_promise = promise<new_result_type>;
			auto future = this->future;
			return *(new new_promise(
				[callback, err_callback, future](typename new_promise::defer d) {
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
		
		promise<void> &then_impl(
			std::function<void(result_type)> callback,
			std::function<void(std::exception_ptr)> err_callback
		) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, err_callback, future](typename new_promise::defer d) {
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
			-> enable_if_t<!std::is_same<result_type, void>::value, promise<result_type> &>
		{
			using new_promise = promise<result_type>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
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
		
		promise<void> &except_impl(std::function<void(std::exception_ptr)> callback) {
			using new_promise = promise<void>;
			auto future = this->future;
			return *(new new_promise(
				[callback, future](typename new_promise::defer d) {
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

		std::function<void(defer)> callback;
		defer d;
		std::shared_ptr<std::future<result_type>> future;
		std::thread thread;
		
	public:
		template <typename function_type>
		auto then(function_type &&callback)
			-> enable_if_t<
				has_call_operator<function_type>::value,
				decltype(then_impl(function_traits<function_type>::cast(callback))) &
			>
		{
			return then_impl(function_traits<function_type>::cast(callback));
		};
		
		template <typename function_type, typename error_callback_type>
		auto then(function_type &&callback, error_callback_type &&error_callback)
			-> enable_if_t<
				has_call_operator<function_type>::value
				&& has_call_operator<error_callback_type>::value,
				decltype(then_impl(
					function_traits<function_type>::cast(callback),
					function_traits<error_callback_type>::cast(error_callback)
				)) &
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
				decltype(except_impl(function_traits<function_type>::cast(callback))) &
			>
		{
			return except_impl(function_traits<function_type>::cast(callback));
		};
	};

	template <typename result_type>
	static promise<result_type> &resolve(result_type arg) {
		return *(new promise<result_type>(
			[=](typename promise<result_type>::defer d) {
				d.resolve(arg);
			}
		));
	};
	
	template <typename result_type>
	static promise<result_type> &reject(std::exception &err) {
		return *(new promise<result_type>(
			[=, &err](typename promise<result_type>::defer d) {
				try {
					throw err;
				} catch (...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
			}
		));
	};
	
	template <typename result_type>
	static promise<result_type> &reject(std::exception_ptr err) {
		return *(new promise<result_type>(
			[=, &err](typename promise<result_type>::defer d) {
				try {
					std::rethrow_exception(err);
				} catch (...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
			}
		));
	};
	
	template <typename type>
	static type await(promise<type> &pr) {
		std::promise<type> p;
		std::future<type> v = p.get_future();
		pr.then(
			[&p](type result){
				p.set_value(result);
			}, [&p](std::exception_ptr err_ptr) {
				p.set_exception(err_ptr);
			}
		);
		return v.get();
	}
};

#endif /* bbb_promise_hpp */