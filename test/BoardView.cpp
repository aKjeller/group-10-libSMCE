/*
 *  test/BoardView.cpp
 *  Copyright 2020-2021 ItJustWorksTM
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#include <catch2/catch_test_macros.hpp>
#include "SMCE/Board.hpp"
#include "SMCE/BoardConf.hpp"
#include "SMCE/BoardView.hpp"
#include "SMCE/Toolchain.hpp"
#include "defs.hpp"

using namespace std::literals;

TEST_CASE("BoardView GPIO", "[BoardView]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "pins", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
    smce::Board br{};
    // clang-format off
    smce::BoardConfig bc{
        /* .pins = */{0, 2},
        /* .gpio_drivers = */{
            smce::BoardConfig::GpioDrivers{
                0,
                smce::BoardConfig::GpioDrivers::DigitalDriver{true, false},
                smce::BoardConfig::GpioDrivers::AnalogDriver{true, false}
            },
            smce::BoardConfig::GpioDrivers{
                2,
                smce::BoardConfig::GpioDrivers::DigitalDriver{false, true},
                smce::BoardConfig::GpioDrivers::AnalogDriver{false, true}
            },
        }
    };
    // clang-format on
    REQUIRE(br.configure(std::move(bc)));
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    auto bv = br.view();
    REQUIRE(bv.valid());
    auto pin0 = bv.pins[0].digital();
    REQUIRE(pin0.exists());
    auto pin1 = bv.pins[1].digital();
    REQUIRE_FALSE(pin1.exists());
    auto pin2 = bv.pins[2].digital();
    REQUIRE(pin2.exists());
    std::this_thread::sleep_for(1ms);

    pin0.write(false);
    test_pin_delayable(pin2, true, 16384, 1ms);
    pin0.write(true);
    test_pin_delayable(pin2, false, 16384, 1ms);
    REQUIRE(br.stop());
}

TEST_CASE("BoardView UART", "[BoardView]") {
    smce::Toolchain tc{SMCE_PATH};
    REQUIRE(!tc.check_suitable_environment());
    smce::Sketch sk{SKETCHES_PATH "uart", {.fqbn = "arduino:avr:nano"}};
    const auto ec = tc.compile(sk);
    if (ec)
        std::cerr << tc.build_log().second;
    REQUIRE_FALSE(ec);
    smce::Board br{};
    REQUIRE(br.configure({.uart_channels = {{}}}));
    REQUIRE(br.attach_sketch(sk));
    REQUIRE(br.start());
    auto bv = br.view();
    REQUIRE(bv.valid());
    auto uart0 = bv.uart_channels[0];
    REQUIRE(uart0.exists());
    REQUIRE(uart0.rx().exists());
    REQUIRE(uart0.tx().exists());
    auto uart1 = bv.uart_channels[1];
    REQUIRE_FALSE(uart1.exists());
    REQUIRE_FALSE(uart1.rx().exists());
    REQUIRE_FALSE(uart1.tx().exists());
    std::this_thread::sleep_for(1ms);

    std::array out = {'H', 'E', 'L', 'L', 'O', ' ', 'U', 'A', 'R', 'T', '\0'};
    std::array<char, out.size()> in{};
    REQUIRE(uart0.rx().write(out) == out.size());
    int ticks = 16'000;
    do {
        if (ticks-- == 0)
            FAIL("Timed out");
        std::this_thread::sleep_for(1ms);
    } while (uart0.tx().size() != in.size());
    REQUIRE(uart0.tx().read(in) == in.size());
    REQUIRE(uart0.tx().size() == 0);
    REQUIRE(in == out);

#if !MSVC_DEBUG
    std::reverse(out.begin(), out.end());
    REQUIRE(uart0.rx().write(out) == out.size());
    ticks = 16'000;
    do {
        if (ticks-- == 0)
            FAIL("Timed out");
        std::this_thread::sleep_for(1ms);
    } while (uart0.tx().size() != in.size());
    REQUIRE(uart0.tx().read(in) == in.size());
    REQUIRE(uart0.tx().size() == 0);
    REQUIRE(in == out);
#endif

    REQUIRE(br.stop());
}


