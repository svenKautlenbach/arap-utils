#include "gtest/gtest.h"

#include "ArapUtils.h"
#include "ArapTimers.h"

TEST(StringOperations, Split)
{
	ASSERT_EQ(3, arap::strings::Utilities::split("Jou mees tere.", " ").size());
}

TEST(SimpleTimer, Expiration)
{
	arap::SimpleTimer timerTest(2);
	auto timeout = timerTest.nextTimeout();
	ASSERT_GE(timeout, 1);
	ASSERT_LT(timeout, 3);
	sleep(2);
	ASSERT_TRUE(timerTest.expired());
}
