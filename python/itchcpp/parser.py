"""Native ITCH message parser.

Mirrors ``itch.parser`` from the pure-Python ``itchfeed`` package. The
:class:`MessageParser` is implemented in C++ (the ``_itchcpp`` extension) and
exposes the same interface: an optional ``message_type`` byte filter, lazy
``parse_stream`` / ``parse_file`` iterators, ``parse_messages`` callback dispatch,
and ``get_message_type``.
"""

from ._itchcpp import AllMessages, MessageParser

__all__ = ["AllMessages", "MessageParser"]
