#include <stdexcept>

#include "gtest/gtest.h"

#include "ArapUtils.h"

TEST(StringOperations, FilesystemReadAndWrite)
{
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-write.txt", "tere"));
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-write.txt", "tere\n", false));
	ASSERT_THROW(arap::strings::Utilities::writeToFile("/hullumaja/tere.txt", "tere"), std::runtime_error);
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-nothing.txt", ""));

	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-read.txt", "t"));
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-read1.txt", "t\n"));
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-read2.txt", "t\n\n"));
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-read3.txt", "a\na\nc"));
	ASSERT_NO_THROW(arap::strings::Utilities::writeToFile("./test-read4.txt", "1\n2\n3\n"));

	ASSERT_EQ(arap::strings::Utilities::getLines("./test-nothing.txt").size(), 0);
	ASSERT_EQ(arap::strings::Utilities::getLines("./test-read.txt").size(), 1);
	ASSERT_EQ(arap::strings::Utilities::getLines("./test-read1.txt").size(), 1);
	ASSERT_EQ(arap::strings::Utilities::getLines("./test-read2.txt").size(), 2);
	ASSERT_EQ(arap::strings::Utilities::getLines("./test-read3.txt").size(), 3);
	ASSERT_EQ(arap::strings::Utilities::getLines("./test-read4.txt").size(), 3);
	
	ASSERT_TRUE(arap::strings::Utilities::getLines("./test-read2.txt").back().empty());
	auto testRows = arap::strings::Utilities::getLines("./test-read4.txt");
	ASSERT_TRUE(testRows.front() == "1");
	ASSERT_TRUE(testRows.at(1) == "2");
	ASSERT_TRUE(testRows.back() == "3");
}

TEST(StringOperations, Split)
{
	ASSERT_EQ(3, arap::strings::Utilities::split("Jou mees tere.", " ").size());	
}

