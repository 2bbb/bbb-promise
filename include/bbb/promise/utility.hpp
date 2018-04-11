#pragma once

#ifndef bbb_promise_utility_hpp
#define bbb_promise_utility_hpp

#include <bbb/promise/type_traits.hpp>
#include <bbb/promise/base_promise.hpp>
#include <bbb/promise/promise_void.hpp>
#include <bbb/promise/promise.hpp>

namespace bbb {
	template <typename result_type>
	static typename promise<result_type>::ref resolve(result_type arg) {
		return promise<result_type>::create(
			[=](typename promise<result_type>::defer &d) {
				d.resolve(arg);
			}
		);
	};
		
	template <typename result_type>
	static typename promise<result_type>::ref reject(std::exception &err) {
		return promise<result_type>::create(
			[=, &err](typename promise<result_type>::defer &d) {
				try {
					throw err;
				} catch (...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
			}
		);
	};
		
	template <typename result_type>
	static typename promise<result_type>::ref reject(std::exception_ptr err) {
		return promise<result_type>::create(
			[=, &err](typename promise<result_type>::defer &d) {
				try {
					std::rethrow_exception(err);
				} catch (...) {
					std::exception_ptr err_ptr = std::current_exception();
					d.reject(err_ptr);
				}
			}
		);
	};
		
	template <typename promise_ref>
	static auto await(promise_ref pr)
		-> enable_if_t<
			!std::is_same<
				unwrap_promise_ref_t<promise_ref>,
				void
			>::value,
			unwrap_promise_ref_t<promise_ref>
		>
	{
		using type = unwrap_promise_ref_t<promise_ref>;
		std::promise<type> p;
		std::future<type> v = p.get_future();
		auto then = pr->then(
			[&p](type result){
				p.set_value(result);
			}, [&p](std::exception_ptr err_ptr) {
				p.set_exception(err_ptr);
			}
		);
		auto &&res = v.get();
		then->manager->clear();
		then->parent.reset();
		then->self.reset();
		then->manager.reset();
		return std::move(res);
	}
		
	static void await(typename promise<void>::ref pr) {
		std::promise<std::uint8_t> p;
		std::future<std::uint8_t> v = p.get_future();
		auto then = pr->then(
			[&p] {
				p.set_value(0);
			}, [&p](std::exception_ptr err_ptr) {
				p.set_exception(err_ptr);
			}
		);
		then->manager->clear();
		then->parent.reset();
		then->self.reset();
		then->manager.reset();
	}
		
	namespace promise_detail {
		template <typename replacee_type>
		struct replace_void_to_uint8
		{ using type = replacee_type; };
		
		template <>
		struct replace_void_to_uint8<void>
		{ using type = std::uint8_t; };
		
		template <typename type>
		using replace_void_to_uint8_t = typename replace_void_to_uint8<type>::type;
		
		template <typename promise_ref>
		inline static auto await_without_void(promise_ref pr)
			-> enable_if_t<
				!std::is_same<
					unwrap_promise_ref_t<promise_ref>,
					void
				>::value,
				unwrap_promise_ref_t<promise_ref>
			>
		{ return bbb::await(pr); }
		
		inline static std::uint8_t await_without_void(typename promise<void>::ref pr) {
			bbb::await(pr);
			return 0;
		}

		template <typename ... types, std::size_t ... indices>
		static auto all(bbb::index_sequence<indices ...>,
						typename promise<types>::ref ... ps)
			-> typename promise<std::tuple<replace_void_to_uint8_t<types> ...>>::ref
		{
			using promise_t = promise<std::tuple<replace_void_to_uint8_t<types> ...>>;
			std::tuple<typename promise<types>::ref ...> promises_tuple{ps ...};
			std::tuple<replace_void_to_uint8_t<types> ...> results{
				await_without_void(std::get<indices>(promises_tuple)) ...
			};
			return bbb::resolve(results);
		};
	};
		
	template <typename ... types>
	static auto all(types ... ps)
		-> decltype(bbb::promise_detail::all<unwrap_promise_ref_t<types> ...>(bbb::make_index_sequence<sizeof...(ps)>(), ps ...))
	{
		return bbb::promise_detail::all<unwrap_promise_ref_t<types> ...>(bbb::make_index_sequence<sizeof...(ps)>(), ps ...);
	};
		
	template <typename type>
	static typename promise<type>::ref create_promise(std::function<void(typename promise<type>::defer &)> f) {
		return promise<type>::create(f);
	}
		
	template <typename type>
	static typename promise<type>::ref create_promise(std::function<type()> f) {
		return promise<type>::create([=](typename promise<type>::defer &defer) {
			try {
				defer.resolve(f());
			} catch(...) {
				defer.reject(std::current_exception());
			}
		});
	}
		
	template <typename function_type>
	static auto create_promise(function_type f)
		-> enable_if_t<
			!is_function<function_type>::value,
			decltype(create_promise(function_traits<function_type>::cast(f)))
		>
	{
		return create_promise(function_traits<function_type>::cast(f));
	}
};

#endif
