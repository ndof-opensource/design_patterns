
Design Principles
-----------------
1.) Never waste memory unnecessarily.<br>
2.) Avoid backing into a design corner, i.e., making decisions on behalf of the users unnecessarily.<br>
3.) All constructs should be allocator-aware.  <br>
4.) Ensure thread-safe options always exist.<br>
5.) Avoid performance overhead, but not at the expense of memory.<br>
6.) All functions should have noexcept versions that return std::expected when warranted.<br>
7.) Verbosity is ok.<br>
8.) Mark methods [[nodiscard]] where applicable.<br>
