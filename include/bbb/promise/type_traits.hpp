#pragma once

#ifndef bbb_promise_type_traits_hpp
#define bbb_promise_type_traits_hpp

#include <memory>

namespace bbb {
	template <typename type>
	struct promise;
	
	using base_promise_ref = std::shared_ptr<struct base_promise>;
		
	template <typename type>
	struct unwrap_promise_ref;
	template <typename type>
	using unwrap_promise_ref_t = typename unwrap_promise_ref<type>::type;
		
	template <typename value_type>
	struct unwrap_promise_ref<std::shared_ptr<bbb::promise<value_type>>> {
		using type = value_type;
	};
};

#endif
