#include "gtest/gtest.h"

#include "ArapTimers.h"

TEST(SimpleTimer, Expiration)
{
	arap::SimpleTimer timerTest(1);
	ASSERT_FALSE(timerTest.expired());
	sleep(1);
	ASSERT_TRUE(timerTest.expired());
}

TEST(SimpleTimer, ExpirationWithPause)
{
	arap::SimpleTimer timerTest(1);
	ASSERT_FALSE(timerTest.expired());
	timerTest.pause();
	ASSERT_FALSE(timerTest.expired());
	timerTest.run();
	ASSERT_FALSE(timerTest.expired());
	sleep(1);
	ASSERT_TRUE(timerTest.expired());
}

TEST(SimpleTimer, ExpirationWithStop)
{
	arap::SimpleTimer timerTest(1);
	ASSERT_FALSE(timerTest.expired());
	timerTest.stop();
	ASSERT_FALSE(timerTest.expired());
	timerTest.run();
	ASSERT_FALSE(timerTest.expired());
	sleep(1);
	ASSERT_TRUE(timerTest.expired());
}

TEST(SimpleTimer, ElapsedMeasuring)
{
	arap::SimpleTimer timerTest(3);
	sleep(1);
	ASSERT_EQ(1, timerTest.elapsed());
	sleep(1);
	ASSERT_EQ(2, timerTest.elapsed());
	sleep(1);
	ASSERT_EQ(3, timerTest.elapsed());
	sleep(1);
	ASSERT_EQ(3, timerTest.elapsed());

	arap::SimpleTimer timerTest2(1);
	ASSERT_EQ(0, timerTest2.elapsed());
	timerTest2.pause();
	ASSERT_EQ(0, timerTest2.elapsed());
	sleep(1);
	ASSERT_EQ(1, timerTest2.elapsed());
	sleep(1);
	ASSERT_EQ(1, timerTest2.elapsed());
	
	arap::SimpleTimer timerTest3(1);
	timerTest3.stop();
	ASSERT_EQ(0, timerTest3.elapsed());
	sleep(1);
	ASSERT_EQ(0, timerTest3.elapsed());
}
