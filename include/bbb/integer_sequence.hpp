#pragma once

#ifndef bbb_promise_integer_sequence_hpp
#define bbb_promise_integer_sequence_hpp

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "./core.hpp"

namespace bbb {
	inline namespace integer_sequences {
		template <typename type, type ... ns>
		struct integer_sequence {
			using value_type = type;
			static constexpr std::size_t size() noexcept { return sizeof...(ns); }
		};
		
		namespace detail {
			template <typename integer_type, integer_type n, integer_type ... ns>
			struct make_integer_sequence
			: embedding<resolve_t<conditional_t<
				n == 0,
				defer<integer_sequence<integer_type, ns ...>>,
				detail::make_integer_sequence<integer_type, n - 1, n - 1, ns ...>
			>>> {};
		};
		
		template <typename type, type n>
		using make_integer_sequence = get_type<detail::make_integer_sequence<type, n>>;
		
		template <std::size_t ... ns>
		using index_sequence = integer_sequence<std::size_t, ns ...>;
		
		template <std::size_t n>
		using make_index_sequence = make_integer_sequence<std::size_t, n>;
		
		template <typename... types>
		using index_sequence_for = make_index_sequence<sizeof...(types)>;
	};
};

#endif
