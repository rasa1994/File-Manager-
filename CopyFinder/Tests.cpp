
#include "gtest/gtest.h"
import CopyFinderModule;

TEST(GetFileExtensionTest, Test1)
{
	const std::string file = R"(C:\Users\User\Desktop\file.txt)";
	const std::string expected = ".txt";
	const std::string result = GetFileExtension(file);
	EXPECT_EQ(expected, result);
}

TEST(GetFileExtensionTest, Test2)
{
	const std::string file = R"(C:\Users\User\Desktop\file.obj)";
	const std::string expected = ".obj";
	const std::string result = GetFileExtension(file);
	EXPECT_EQ(expected, result);
}

TEST(GetFileExtensionTest, Test3)
{
	const std::string file = R"(C:\Users\User\Desktop\file.image.jpeg)";
	const std::string expected = ".jpeg";
	const std::string result = GetFileExtension(file);
	EXPECT_EQ(expected, result);
}

TEST(GetFileExtensionTest, Test4)
{
	const std::string file = R"(C:\Users\User\Desktop\file)";
	const std::string expected{""};
	const std::string result = GetFileExtension(file);
	EXPECT_EQ(expected, result);
}

TEST(GetRFile, Test5)
{
	const std::string file = R"(.\RandomFileForTestingNotExisting.txt)";
	bool result = DeleteFile(file);
	EXPECT_FALSE(result);
}

TEST(GetRFile, Test6)
{
	const std::string file = "./RandomFileForTesting.txt";
	std::ofstream out(file);
	out.close();
	bool result = DeleteFile(file);

	EXPECT_TRUE(result);
}

TEST(Compare, Test7)
{
	const std::string file = "./RandomFileForCompare.txt";
	std::ofstream out(file);
	if (out)
	{
		out.close();
		bool result = CompareFile(file, file);
		remove(file.c_str());
		EXPECT_TRUE(result);
	}
	else
	{
		FAIL();
	}
}

TEST(Compare, Test8)
{
	const std::string file1 = "RandomFileForCompare1.txt";
	const std::string file2 = "RandomFileForCompare2.txt";
	std::ofstream out(file1, std::ios::out);
	std::ofstream out2(file2, std::ios::out);
	if (out.is_open() && out2.is_open())
	{
		out << "Hello";
		out2 << "Hello123";
		out.close();
		out2.close();
		bool result = CompareFile(file1, file2);
		if (remove(file1.c_str()) != 0) FAIL();
		if (remove(file2.c_str()) != 0) FAIL();
		EXPECT_FALSE(result);
	}
	else
	{
		FAIL();
	}
}

TEST(Compare, Test9)
{
	const std::string file1 = "RandomFileForCompare1.txt";
	const std::string file2 = "RandomFileForCompare2.txt";
	std::ofstream out(file1, std::ios::out);
	std::ofstream out2(file2, std::ios::out);
	if (out.is_open() && out2.is_open())
	{
		out << "Hello123444";
		out2 << "Hello123444";
		out.close();
		out2.close();
		bool result = CompareFile(file1, file2);
		if (remove(file1.c_str()) != 0) FAIL();
		if (remove(file2.c_str()) != 0) FAIL();
		EXPECT_TRUE(result);
	}
	else
	{
		FAIL();
	}
}

int ResultRunTests()
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}