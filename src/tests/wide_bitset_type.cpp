/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file wide_bitset_type.cpp Test functionality of WideEnumBitSet, including the string parameter round trip. */

#include "../stdafx.h"

#include "../core/enum_type.hpp"
#include "../core/wide_bitset_type.hpp"
#include "../newgrf_type.h"
#include "../strings_internal.h"

#include "../3rdparty/catch2/catch.hpp"

#include "../safeguards.h"

enum TestValue : uint8_t {};
static constexpr TestValue TEST_END{200};
using TestSet = WideEnumBitSet<TestValue, TEST_END>;

TEST_CASE("WideEnumBitSet - Set, Test and word boundaries")
{
	TestSet set;
	CHECK(set.None());

	for (uint8_t v : {0, 63, 64, 127, 128, 199}) set.Set(TestValue{v});

	CHECK(set.Count() == 6);
	CHECK(set.Test(TestValue{63}));
	CHECK(set.Test(TestValue{64}));
	CHECK(set.Test(TestValue{199}));
	CHECK_FALSE(set.Test(TestValue{65}));
	CHECK(set.Word(0) == ((1ULL << 63) | 1ULL));
	CHECK(set.Word(1) == ((1ULL << 63) | 1ULL));

	set.Reset(TestValue{64});
	CHECK_FALSE(set.Test(TestValue{64}));
	CHECK(set.Count() == 5);
}

TEST_CASE("WideEnumBitSet - Iteration in order")
{
	TestSet set{TestValue{150}, TestValue{3}, TestValue{64}, TestValue{199}};

	std::vector<uint8_t> values;
	for (TestValue v : set) values.push_back(v);

	CHECK(values == std::vector<uint8_t>{3, 64, 150, 199});
	CHECK(set.GetNthSetBit(2) == TestValue{150});
	CHECK_FALSE(set.GetNthSetBit(4).has_value());
}

TEST_CASE("WideEnumBitSet - Flip, All and validity are masked to the valid values")
{
	TestSet set;
	set.Flip();
	CHECK(set.All());
	CHECK(set.IsValid());
	CHECK(set.Count() == 200);
	CHECK(set.Word(3) == (1ULL << (200 - 192)) - 1);

	TestSet some{TestValue{1}, TestValue{100}};
	CHECK(set.All(some));
	CHECK((set & some) == some);
	CHECK(TestSet{}.Set().Reset(some).Count() == 198);
}

TEST_CASE("WideEnumBitSet - Construction from a 64 bit integer fills the lowest word")
{
	TestSet set{0x8000000000000001ULL};
	CHECK(set.Test(TestValue{0}));
	CHECK(set.Test(TestValue{63}));
	CHECK(set.Count() == 2);
}

TEST_CASE("WideEnumBitSet - String parameter round trip")
{
	TestSet set{TestValue{2}, TestValue{70}, TestValue{199}};

	std::array<StringParameter, 2> params{StringParameter{set}, StringParameter{uint64_t{0x5}}};
	StringParameters args(params);

	CHECK(args.GetNextParameterWideBitSet<TestSet>() == set);
	CHECK(args.GetNextParameterWideBitSet<TestSet>() == TestSet{TestValue{0}, TestValue{2}});
}
