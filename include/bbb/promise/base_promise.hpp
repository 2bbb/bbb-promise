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
		std::shared_ptr<std::list<ref>> manager;
		
		template <typename promise_ref>
		promise_ref setup_promise(promise_ref p) {
			p->parent = shared_from_this();
			p->self = p;
			p->manager = manager;
			p->manager->push_back(p);
			return p;
		}
		void remove_parent(ref parent) {
			if(parent) {
				auto it = std::remove_if(manager->begin(), manager->end(), [parent](base_promise::ref v) {
					return v.get() == parent.get();
				});
				manager->erase(it, manager->end());
				parent->manager.reset();
				parent->suicide();
				parent.reset();
			}
		}
		void suicide() {
			self.reset();
		}
	};
};

#endif
