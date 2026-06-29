# itchcpp — Python bindings

`itchcpp` is a native (C++) NASDAQ TotalView-ITCH 5.0 parser, order-book engine,
and analytics layer exposed to Python via pybind11. It is designed as a faster,
drop-in **backend for the pure-Python [`itch`](https://github.com/bbalouki/itch)
package**: it mirrors that package's `MessageParser` API and message-class field
and `decode_price` semantics, so existing code can switch to the native parser
with minimal changes.

## Build / install

The bindings require a C++20 compiler and pybind11. From the repository root:

```bash
pip install ./python
```

This invokes scikit-build-core, which configures the parent CMake project with
`-DITCH_BUILD_PYTHON=ON` and builds the `itchcpp` extension module.

To build just the extension with CMake directly:

```bash
cmake -S . -B build -DITCH_BUILD_PYTHON=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build --target itchcpp_python
```

## Usage

```python
import itchcpp

parser = itchcpp.MessageParser()
for message in parser.parse_file("01302020.NASDAQ_ITCH50"):
    if isinstance(message, itchcpp.AddOrderMessage):
        print(message.stock, message.decode_price("price"), message.shares)

# Full-market book reconstruction in one pass:
manager = itchcpp.BookManager()
with open("01302020.NASDAQ_ITCH50", "rb") as handle:
    manager.process_stream(handle.read())
book = manager.book_for_symbol("AAPL")
print(book.bbo().bid_price, book.bbo().ask_price)
```

## Relationship to the `itch` package

The pure-Python `itch` package is already published on PyPI. Rather than rename or
break it, `itchcpp` ships as a separate, native package exposing the same API
surface (`MessageParser.parse_file`/`parse_stream`, typed message classes,
`decode_price`). The recommended migration is to have `itch` optionally import
`itchcpp` as its parsing backend when installed, falling back to the pure-Python
implementation otherwise — giving existing users a transparent speedup without a
breaking change.
