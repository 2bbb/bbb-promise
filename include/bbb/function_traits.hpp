#pragma once

#ifndef bbb_promise_function_traits_hpp
#define bbb_promise_function_traits_hpp

#include <type_traits>
#include <functional>

#include "./core.hpp"

namespace bbb {
	inline namespace function_traits_utils {
		template <typename>
		struct is_function
		: std::false_type {};
		template <typename res, typename ... arguments>
		struct is_function<std::function<res(arguments ...)>>
		: std::true_type {};
		
		template <std::size_t index, typename ... arguments>
		using type_at_t = typename std::tuple_element<index, std::tuple<arguments ...>>::type;
		
		template <typename patient>
		struct has_call_operator {
			template <typename inner_patient, decltype(&inner_patient::operator())> struct checker {};
			template <typename inner_patient> static std::true_type  check(checker<inner_patient, &inner_patient::operator()> *);
			template <typename>               static std::false_type check(...);
			using type = decltype(check<patient>(nullptr));
			static constexpr bool value = type::value;
		};
		template <typename res, typename ... args>
		struct has_call_operator<res(args ...)> {
			using type = std::true_type;
			static constexpr bool value = type::value;
		};
		template <typename res, typename ... args>
		struct has_call_operator<res(*)(args ...)> {
			using type = std::true_type;
			static constexpr bool value = type::value;
		};
		template <typename res, typename ... args>
		struct has_call_operator<res(&)(args ...)> {
			using type = std::true_type;
			static constexpr bool value = type::value;
		};
		
		namespace detail {
			template <typename ret, typename ... arguments>
			struct function_traits {
				static constexpr std::size_t arity = sizeof...(arguments);
				using result_type = ret;
				using arguments_types_tuple = std::tuple<arguments ...>;
				template <std::size_t index>
				using argument_type = type_at_t<index, arguments ...>;
				using raw_function_type = ret(arguments ...);
				using function_type = std::function<ret(arguments ...)>;
				template <typename function_t>
				static constexpr function_type cast(function_t f) {
					return static_cast<function_type>(f);
				}
			};
		};
		
		template <typename T>
		struct function_traits : public function_traits<decltype(&T::operator())> {};
		
		template <typename class_type, typename ret, typename ... arguments>
		struct function_traits<ret(class_type::*)(arguments ...) const>
		: detail::function_traits<ret, arguments ...> {};
		
		template <typename class_type, typename ret, typename ... arguments>
		struct function_traits<ret(class_type::*)(arguments ...)>
		: detail::function_traits<ret, arguments ...> {};
		
		template <typename ret, typename ... arguments>
		struct function_traits<ret(*)(arguments ...)>
		: detail::function_traits<ret, arguments ...> {};
		
		template <typename ret, typename ... arguments>
		struct function_traits<ret(arguments ...)>
		: detail::function_traits<ret, arguments ...> {};
		template <typename ret, typename ... arguments>
		struct function_traits<ret(&)(arguments ...)>
		: detail::function_traits<ret, arguments ...> {};
		
		template <typename ret, typename ... arguments>
		struct function_traits<std::function<ret(arguments ...)>>
		: detail::function_traits<ret, arguments ...> {};
	};
};

#endif
