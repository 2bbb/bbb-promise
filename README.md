# bbb::promise

```
     _/        _/        _/
    _/_/_/    _/_/_/    _/_/_/
   _/    _/  _/    _/  _/    _/
  _/    _/  _/    _/  _/    _/
 _/_/_/    _/_/_/    _/_/_/
```

[W.I.P.] easy to use promise like javascript.

## ex.

```cpp
bbb::create_promise([] { return 4; }) // == bbb::resolve(4)
    ->then([](int data) {
        std::this_thread::sleep_for(std::chrono::millisecond(5000));
        std::cout << data << std::endl;
    })
    ->then([] {
        std::cout << "finish" << std::endl;
    })
    ->catch([](std::exception_ptr err_ptr) {
        try {
            std::rethrow_error(err_ptr);
        } catch(std::exception &err) {
            std::cerr << err.what() << std::endl;
        }
    });
```

## License

MIT License.

## Author

* ISHII 2bit
* i[at]2bit.jp

## At the last

Please create a new issue if there is a problem.

And please throw a pull request if you have a cool idea!!

If you get happy with using this addon, and you're rich, please donation for support continuous development.

Bitcoin: `17AbtW73aydfYH3epP8T3UDmmDCcXSGcaf`
