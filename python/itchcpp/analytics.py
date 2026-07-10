"""Native streaming analytics (an itchcpp extra, beyond the pure-Python ``itch`` API).

Currently exposes a volume-weighted average price (VWAP) accumulator fed with raw
4-decimal prices and share quantities.
"""

from ._itchcpp import Vwap

__all__ = ["Vwap"]
