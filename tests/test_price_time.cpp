#include <gtest/gtest.h>

#include <chrono>
#include <format>
#include <string>

#include "itch/price.hpp"
#include "itch/time.hpp"

TEST(PriceTest, StandardScaleUsesFourDecimals) {
    const auto price = itch::make_price(1500000);  // 150.0000
    EXPECT_EQ(price.raw(), 1500000u);
    EXPECT_EQ(itch::StandardPrice::divisor(), 10000u);
    EXPECT_DOUBLE_EQ(price.to_double(), 150.0);
    EXPECT_EQ(price.to_string(), "150.0000");
}

TEST(PriceTest, MwcbScaleUsesEightDecimals) {
    const auto price = itch::make_mwcb_price(15000000000ull);  // 150.00000000
    EXPECT_EQ(itch::MwcbPrice::divisor(), 100000000ull);
    EXPECT_DOUBLE_EQ(price.to_double(), 150.0);
    EXPECT_EQ(price.to_string(), "150.00000000");
}

TEST(PriceTest, TheTwoScalesAreDistinctTypes) {
    // The standard and MWCB prices are different types, so the 4-vs-8 decimal
    // mix-up cannot compile. This is a documentation-as-test of that intent.
    static_assert(!std::is_same_v<itch::StandardPrice, itch::MwcbPrice>);
    EXPECT_EQ(itch::StandardPrice::decimals, 4u);
    EXPECT_EQ(itch::MwcbPrice::decimals, 8u);
}

TEST(PriceTest, ComparesByRawValue) {
    EXPECT_TRUE(itch::make_price(100) < itch::make_price(200));
    EXPECT_TRUE(itch::make_price(200) == itch::make_price(200));
}

TEST(PriceTest, FormatterRendersThroughStdFormat) {
    EXPECT_EQ(std::format("{:.4f}", itch::make_price(1500000)), "150.0000");
}

TEST(TimeTest, CombinesSessionDateAndTimestamp) {
    using namespace std::chrono;
    // 09:30:00.000000000 = 34200 seconds past midnight.
    constexpr std::uint64_t nanos = 34200ull * 1'000'000'000ull;
    const auto              point = itch::to_time_point(2020y / January / 30, nanos);

    const auto expected = sys_days {2020y / January / 30} + hours {9} + minutes {30};
    EXPECT_EQ(point, expected);
}

TEST(TimeTest, FormatsTimestampAsTimeOfDay) {
    constexpr std::uint64_t nanos = 34200ull * 1'000'000'000ull;
    EXPECT_EQ(itch::format_timestamp(nanos), "09:30:00.000000000");
}

TEST(TimeTest, FormatsFullTimePoint) {
    using namespace std::chrono;
    constexpr std::uint64_t nanos = 34200ull * 1'000'000'000ull;
    const auto              text  = itch::format_time_point(2020y / January / 30, nanos);
    EXPECT_NE(text.find("2020-01-30"), std::string::npos);
    EXPECT_NE(text.find("09:30:00"), std::string::npos);
}
