#pragma once

#ifndef bbb_promise_hpp
#define bbb_promise_hpp

#include <iostream>
#include <vector>
#include <future>
#include <functional>
#include <memory>
#include <list>

#include <bbb/core.hpp>
#include <bbb/integer_sequence.hpp>
#include <bbb/function_traits.hpp>

#define bbb_promise_debug_flag 1

#include <bbb/promise/type_traits.hpp>
#include <bbb/promise/base_promise.hpp>
#include <bbb/promise/promise_void.hpp>
#include <bbb/promise/promise.hpp>
#include <bbb/promise/utility.hpp>

#if bbb_promise_debug_flag
#	include <iostream>
#	include <typeinfo>
#endif

#endif /* bbb_promise_hpp */