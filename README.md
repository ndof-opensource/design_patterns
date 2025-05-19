
Design Principles
-----------------
1.) Never waste memory unnecessarily.
2.) Don't require the user to use the heap.
3.) All things that allocate should accept an allocator.
4.) Minimize design decisions that restrict to the greatest extent possible (reasonable?)
5.) Don't make unnecessarily nested function calls. (overruled by 1)
6.) All functions should have noexcept versions that return std::expected when possible.
7.) Verbosity is ok.
