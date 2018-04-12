#pragma once

#ifndef bbb_promise_base_promise_hpp
#define bbb_promise_base_promise_hpp

#include <list>
#include <memory>

#include <bbb/promise/type_traits.hpp>

namespace bbb {
	struct base_promise : std::enable_shared_from_this<base_promise> {
		using ref = std::shared_ptr<base_promise>;
		base_promise::ref parent;
		base_promise::ref self;
		
		template <typename promise_ref>
		promise_ref setup_promise(promise_ref p) {
			p->parent = shared_from_this();
			p->self = p;
			return p;
		}
		void finish_process() {
			if(parent) {
				parent->suicide();
				parent.reset();
			}
			self.reset();
		}
		void suicide() {
			self.reset();
		}
	};
	
	template <typename promise_ref>
	inline static promise_ref init_promise(promise_ref p) {
		return p;
	}
};

#endif
