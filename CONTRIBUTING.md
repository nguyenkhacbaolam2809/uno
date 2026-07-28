# Contributing

## How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -m "Add feature"`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request

## Coding Standards

- C++17 with strict compiler flags (`-Wall -Wextra -Wpedantic -Wshadow -Werror`)
- No raw `new`/`delete` — use `std::unique_ptr`, `std::vector`
- No `using namespace std` — use explicit `std::` prefixes
- Use `enum class` for new enumerations
- Mark functions `noexcept` where possible
- Use `constexpr` for compile-time constants
- Include guards (not `#pragma once`)

## Code Style

- 4-space indentation
- `snake_case` for functions and variables
- `PascalCase` for classes
- `m_` prefix for member variables
- Opening brace on same line for functions

## Testing

All changes must pass the test suite:
```bash
mkdir build && cd build && cmake .. && make && ./test_all
```

## Pull Request Checklist

- [ ] Code compiles with `-Wall -Werror`
- [ ] All tests pass
- [ ] No new warnings
- [ ] Code follows existing style
- [ ] Commit messages are clear
