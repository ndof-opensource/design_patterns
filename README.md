
Design Principles
-----------------
1.) Never waste memory unnecessarily.
2.) Avoid backing into a design corner, i.e., making decisions on behalf of the users unnecessarily.
3.) All constructs should be allocator-aware.  
4.) Ensure thread-safe options awaysexist.
5.) Avoid performance overhead, but not at the expense of memory.
6.) All functions should have noexcept versions that return std::expected when warranted.
7.) Verbosity is ok.
8.) Mark methods [[nodiscard]] where applicable.
