///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/generator.hpp>
#include <cppcoro/on_scope_exit.hpp>

#include "doctest/doctest.h"

TEST_SUITE_BEGIN("generator");

using cppcoro::generator;

TEST_CASE("default-constructed generator is empty sequence")
{
	generator<int> ints;
	CHECK(ints.begin() == ints.end());
}

TEST_CASE("generator of arithmetic type returns by copy")
{
	auto f = []() -> generator<float>
	{
		co_yield 1.0f;
		co_yield 2.0f;
	};

	auto gen = f();
	auto iter = gen.begin();
	// TODO: Should this really be required?
	//static_assert(std::is_same<decltype(*iter), float>::value, "operator* should return float by value");
	CHECK(*iter == 1.0f);
	++iter;
	CHECK(*iter == 2.0f);
	++iter;
	CHECK(iter == gen.end());
}

TEST_CASE("generator of reference returns by reference")
{
	auto f = [](float& value) -> generator<float&>
	{
		co_yield value;
	};

	float value = 1.0f;
	for (auto& x : f(value))
	{
		CHECK(&x == &value);
		x += 1.0f;
	}

	CHECK(value == 2.0f);
}

TEST_CASE("generator doesn't start until its called")
{
	bool reachedA = false;
	bool reachedB = false;
	bool reachedC = false;
	auto f = [&]() -> generator<int>
	{
		reachedA = true;
		co_yield 1;
		reachedB = true;
		co_yield 2;
		reachedC = true;
	};

	auto gen = f();
	CHECK(!reachedA);
	auto iter = gen.begin();
	CHECK(reachedA);
	CHECK(!reachedB);
	CHECK(*iter == 1);
	++iter;
	CHECK(reachedB);
	CHECK(!reachedC);
	CHECK(*iter == 2);
	++iter;
	CHECK(reachedC);
	CHECK(iter == gen.end());
}

TEST_CASE("destroying generator before completion destructs objects on stack")
{
	bool destructed = false;
	bool completed = false;
	auto f = [&]() -> generator<int>
	{
		auto onExit = cppcoro::on_scope_exit([&]
		{
			destructed = true;
		});

		co_yield 1;
		co_yield 2;
		completed = true;
	};

	for (auto x : f())
	{
		CHECK(x == 1u);
		CHECK(!destructed);
		break;
	}

	CHECK(!completed);
	CHECK(destructed);
}

TEST_CASE("generator throwing before yielding first element rethrows out of begin()")
{
	class X {};

	auto g = []() -> cppcoro::generator<int>
	{
		throw X{};
		co_return;
	}();

	try
	{
		g.begin();
		FAIL("should have thrown");
	}
	catch (const X&)
	{
	}
}

TEST_CASE("generator throwing after first element rethrows out of operator++")
{
	class X {};

	auto g = []() -> cppcoro::generator<int>
	{
		co_yield 1;
		throw X{};
	}();

	auto iter = g.begin();
	REQUIRE(iter != g.end());
	try
	{
		++iter;
		FAIL("should have thrown");
	}
	catch (const X&)
	{
	}
}

TEST_SUITE_END();
