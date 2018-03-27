#pragma once

#ifndef bbb_promise_core_hpp
#define bbb_promise_core_hpp

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bbb {
	namespace {
		template <bool b, typename type>
		using enable_if_t = typename std::enable_if<b, type>::type;
		template <bool b, typename t, typename f>
		using conditional_t = typename std::conditional<b, t, f>::type;

		template <typename T>
		using get_type = typename T::type;
		
		template <typename embedding_type>
		struct embedding { using type = embedding_type; };
		
		template <typename t>
		struct defer { using type = t; };
		
		template <bool b, typename t, typename f>
		using defered_conditional = conditional_t<b, defer<t>, defer<f>>;
		
		template <typename t>
		using resolve_t = get_type<t>;
	};
};

#endif
