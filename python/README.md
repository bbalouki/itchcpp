# itchcpp — a fast, drop-in backend for the pure-Python `itch` package

`itchcpp` is a native (C++) NASDAQ TotalView-ITCH 5.0 parser, order-book engine,
and analytics layer exposed to Python via pybind11. It is built to be a
**drop-in, faster backend** for the pure-Python
[`itch`](https://github.com/bbalouki/itch) package (published on PyPI as
[`itchfeed`](https://pypi.org/project/itchfeed/)).

The pure-Python parser unpacks every message with `struct.unpack` and materializes a
Python object in interpreter code, which is slow on full-day feeds. `itchcpp` does the
framing and field decoding in C++ (gigabytes per second) while exposing the **same
public API**: the same message classes (same names, same raw `bytes`/`int`
attributes), the same `MessageParser` semantics, the same `MarketMessage` helpers,
the same constants, and the same `create_message` factory.

## Drop-in usage

The package layout mirrors `itch.*`, so migrating only changes the import root:

```python
# pure-Python                              # native backend
from itch.parser import MessageParser      from itchcpp.parser import MessageParser
from itch.messages import create_message   from itchcpp.messages import create_message
from itch.indicators import MARKET_CATEGORY from itchcpp.indicators import MARKET_CATEGORY
```

```python
from itchcpp.parser import MessageParser
from itchcpp.messages import AddOrderNoMPIAttributionMessage

parser = MessageParser()  # or MessageParser(message_type=b"AFE") to filter types
with open("01302020.NASDAQ_ITCH50", "rb") as itch_file:
    for message in parser.parse_file(itch_file):
        if isinstance(message, AddOrderNoMPIAttributionMessage):
            # Raw attributes are bytes/int, exactly like the pure-Python package.
            print(message.stock, message.decode_price("price"), message.shares)
            # decode() returns a human-readable dataclass (symbols trimmed,
            # prices scaled):
            print(message.decode())
```

`parse_file` takes an open **binary file object** (so `gzip.open(path, "rb")` works
too); `parse_stream(raw_bytes)` parses an in-memory buffer. Both return lazy
iterators, apply the `message_type` filter, and stop at the end-of-messages system
event (`S` with `event_code == b"C"`).

## What is exposed

- **`itchcpp.messages`** — every message class under its exact upstream name
  (`SystemEventMessage`, `StockDirectoryMessage`, `AddOrderNoMPIAttributionMessage`,
  `AddOrderMPIDAttribution`, `OrderExecutedMessage`, `NonCrossTradeMessage`,
  `CrossTradeMessage`, `NOIIMessage`, `MWCBDeclineLeveMessage`,
  `RetailPriceImprovementIndicator`, `DLCRMessage`, ...); the abstract bases
  `MarketMessage`, `AddOrderMessage`, `ModifyOrderMessage`, `TradeMessage` (the
  concrete classes are registered as virtual subclasses, so `isinstance` works); the
  `messages` registry (`dict[bytes, type]`); the `AllMessages` constant; and the
  `create_message(message_type, **kwargs)` factory.
- **`MarketMessage` helpers** on every message: `decode()`, `decode_price(name)`,
  `to_bytes()`, `set_timestamp(ts1, ts2)`, `split_timestamp()`,
  `get_attributes(call_able=False)`, plus the `message_type`, `description`,
  `message_size`, and `price_precision` metadata.
- **`itchcpp.parser.MessageParser`** — `parse_file`, `parse_stream`,
  `parse_messages(data, callback)`, `get_message_type(bytes)`, and the `message_type`
  filter.
- **`itchcpp.indicators`** — the field code lookup tables (`SYSTEM_EVENT_CODES`,
  `MARKET_CATEGORY`, `TRADING_STATES`, `ISSUE_SUB_TYPE_VALUES`, ...).
- **Native extras** (not part of the pure-Python API): `itchcpp.book`
  (`BookManager`, `L3Book`, `Bbo`) for single-pass order-book reconstruction, and
  `itchcpp.analytics` (`Vwap`).

## Install

Prebuilt wheels are published to PyPI for Linux, macOS, and Windows (CPython
3.9-3.13), so most users need no compiler:

```bash
pip install itchcpp
```

### Build from source

Building from source needs a C++20 compiler and pybind11 (pulled in automatically as
a build dependency). The `pyproject.toml` lives at the repository root, so build from
there:

```bash
pip install .
```

This invokes scikit-build-core, which configures the CMake project with
`-DITCH_BUILD_PYTHON=ON` and builds the `itchcpp._itchcpp` extension module into the
`itchcpp` package. To build just the extension with CMake directly:

```bash
cmake -S . -B build -DITCH_BUILD_PYTHON=ON
cmake --build build --target itchcpp_python
```

## Relationship to the `itch` (itchfeed) package

`itchcpp` ships as a separate package and does not replace `itchfeed`; it exposes the
same API under the `itchcpp` import root. There are two migration paths:

1. **Switch the import root** in your own code (`itch` -> `itchcpp`).
2. **Wire it as an optional backend** in `itchfeed`, so installed users get a
   transparent speedup with no code change:

   ```python
   try:
       from itchcpp.parser import MessageParser  # fast C++ backend
       from itchcpp.messages import create_message, messages
   except ImportError:  # fall back to the pure-Python implementation
       from itch.parser import MessageParser
       from itch.messages import create_message, messages
   ```

The native message objects expose **raw** field values (`bytes` for character fields,
`int` for prices and quantities) just like the pure-Python classes, so existing code
that calls `decode_price(...)` or `decode()` keeps working unchanged.
