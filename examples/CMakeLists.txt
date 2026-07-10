cmake_minimum_required(VERSION 3.25)

add_executable(parser_example parser/parser_example.cpp)

target_link_libraries(
    parser_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(order_book_example order_book/order_book_example.cpp)

target_link_libraries(
    order_book_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(pcap_replay_example transport/pcap_replay_example.cpp)

target_link_libraries(
    pcap_replay_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(book_engine_example book/book_engine_example.cpp)

target_link_libraries(
    book_engine_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(vwap_example analytics/vwap_example.cpp)

target_link_libraries(
    vwap_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(soupbintcp_example transport/soupbintcp_example.cpp)

target_link_libraries(
    soupbintcp_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(moldudp64_example transport/moldudp64_example.cpp)

target_link_libraries(
    moldudp64_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(overlay_example overlay/overlay_example.cpp)

target_link_libraries(
    overlay_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(bars_example analytics/bars_example.cpp)

target_link_libraries(
    bars_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(microstructure_example analytics/microstructure_example.cpp)

target_link_libraries(
    microstructure_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(auctions_example analytics/auctions_example.cpp)

target_link_libraries(
    auctions_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(csv_sink_example io/csv_sink_example.cpp)

target_link_libraries(
    csv_sink_example PRIVATE
    itch::itch
    warnings::strict
)

if(ITCH_WITH_ARROW)
    add_executable(arrow_export_example io/arrow_export_example.cpp)

    target_link_libraries(
        arrow_export_example PRIVATE
        itch::itch
        warnings::strict
    )
endif()

add_executable(encoder_example encoder/encoder_example.cpp)

target_link_libraries(
    encoder_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(replay_example replay/replay_example.cpp)

target_link_libraries(
    replay_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(price_time_indicators_example utilities/price_time_indicators_example.cpp)

target_link_libraries(
    price_time_indicators_example PRIVATE
    itch::itch
    warnings::strict
)
