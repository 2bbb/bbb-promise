#pragma once

#ifndef bbb_promise_hpp
#define bbb_promise_hpp

#include <iostream>
#include <vector>
#include <future>
#include <functional>
#include <memory>
#include <list>

#include "./core.hpp"
#include "./integer_sequence.hpp"
#include "./function_traits.hpp"

#define bbb_promise_debug_flag 1

#if bbb_promise_debug_flag
#	include <iostream>
#	include <typeinfo>
#endif

namespace bbb {
    template <typename result_type>
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

    template <typename promise_ref>
    inline static promise_ref init_promise(promise_ref p) {
        p->manager = std::make_shared<std::list<base_promise_ref>>();
        p->manager->push_back(p);
        return p;
    }

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
    
    template<>
    struct promise<void> : base_promise {
        using ref = std::shared_ptr<promise<void>>;
        
        struct defer {
            defer() : promise() {};
            defer(defer &&) = default;
            void resolve()
            { promise.set_value(0); }
            void reject(std::exception_ptr e)
            { promise.set_exception(e); }
            std::promise<uint8_t> promise;
        };
        
        inline static ref create(std::function<void(defer &)> callback, bool sync = false) {
            return init_promise(std::make_shared<promise<void>>(callback, false));
        }
        
        promise(std::function<void(defer &)> callback, bool sync = false)
        : callback(callback)
        , d()
        , future(new std::future<std::uint8_t>(d.promise.get_future()))
        {
            if(sync) {
                try {
                    callback(d);
                } catch(...) {
                    std::exception_ptr err_ptr = std::current_exception();
                    d.reject(err_ptr);
                }
                remove_parent(parent);
            } else {
                thread = std::move(std::thread([=](defer d) {
                    try {
                        callback(d);
                    } catch(...) {
                        std::exception_ptr err_ptr = std::current_exception();
                        d.reject(err_ptr);
                    }
                    remove_parent(parent);
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
        auto then_impl(std::function<new_result_type()> callback)
            -> enable_if_t<!std::is_same<new_result_type, void>::value, typename promise<new_result_type>::ref>
        {
            using new_promise = promise<new_result_type>;
            auto future = this->future;
            return setup_promise(std::make_shared<new_promise>(
                [callback, future](typename new_promise::defer &d) {
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
        
        typename promise<void>::ref then_impl(std::function<void()> callback) {
            using new_promise = promise<void>;
            auto future = this->future;
            return setup_promise(std::make_shared<new_promise>(
                [callback, future](typename new_promise::defer &d) {
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
            -> enable_if_t<!std::is_same<new_result_type, void>::value, typename promise<new_result_type>::ref>
        {
            using new_promise = promise<new_result_type>;
            auto future = this->future;
            return setup_promise(std::make_shared<new_promise>(
                [callback, err_callback, future](typename new_promise::defer &d) {
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
        
        typename promise<void>::ref then_impl(
            std::function<void()> callback,
            std::function<void(std::exception_ptr)> err_callback
        ) {
            using new_promise = promise<void>;
            auto future = this->future;
            return setup_promise(std::make_shared<new_promise>(
                [callback, err_callback, future](typename new_promise::defer &d) {
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
        std::shared_ptr<std::future<uint8_t>> future;
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
            return then_impl(function_traits<function_type>::cast(callback));
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
        
        void await() {
            std::promise<uint8_t> p;
            std::future<uint8_t> v = p.get_future();
            
            then([&p](){
                p.set_value(0);
            }, [&p](std::exception_ptr err_ptr) {
                p.set_exception(err_ptr);
            });
            v.get();
            return;
        }
    };

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
                remove_parent(parent);
            } else {
                thread = std::move(std::thread([=](defer d) {
                    try {
                        callback(d);
                    } catch(...) {
                        std::exception_ptr err_ptr = std::current_exception();
                        d.reject(err_ptr);
                    }
                    remove_parent(parent);
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

#endif /* bbb_promise_hpp */