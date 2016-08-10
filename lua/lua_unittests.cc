// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "base/bind.h"
#include "lua/pcall.h"
#include "lua/stack.h"
#include "testing/gtest/include/gtest/gtest.h"

class LuaTest : public testing::Test {
 protected:
  void SetUp() override {
    lua::SetTop(state_, 0);
  }

  lua::ManagedState state_;
};

TEST_F(LuaTest, PushesToStack) {
  ASSERT_TRUE(lua::Push(state_, 1));
  ASSERT_TRUE(lua::Push(state_, "str1"));
  ASSERT_TRUE(lua::Push(state_, "str2"));
  std::string str2;
  ASSERT_TRUE(lua::To(state_, -1, &str2));
  ASSERT_EQ(str2, "str2");
  base::StringPiece str1;
  ASSERT_TRUE(lua::To(state_, -2, &str1));
  ASSERT_EQ(str1, "str1");
  int number;
  ASSERT_TRUE(lua::To(state_, -3, &number));
  ASSERT_FALSE(lua::To(state_, -1, &number));
  ASSERT_EQ(number, 1);
  ASSERT_EQ(lua::GetTop(state_), 3);
}

TEST_F(LuaTest, PushesAndGetsMultipleValues) {
  ASSERT_TRUE(lua::Push(state_, 1, 2, 3, 4, 5));
  int i1, i2, i3, i4, i5;
  ASSERT_TRUE(lua::To(state_, -5, &i1, &i2, &i3, &i4, &i5));
  EXPECT_EQ(i1, 1);
  EXPECT_EQ(i2, 2);
  EXPECT_EQ(i3, 3);
  EXPECT_EQ(i4, 4);
  EXPECT_EQ(i5, 5);
  ASSERT_EQ(lua::GetTop(state_), 5);
}

TEST_F(LuaTest, PopsValues) {
  ASSERT_TRUE(lua::Push(state_, 1, 2, 3, 4, 5));
  int i1, i2, i3, i4, i5;
  ASSERT_TRUE(lua::Pop(state_, &i1, &i2, &i3, &i4, &i5));
  EXPECT_EQ(i1, 1);
  EXPECT_EQ(i2, 2);
  EXPECT_EQ(i3, 3);
  EXPECT_EQ(i4, 4);
  EXPECT_EQ(i5, 5);
  ASSERT_EQ(lua::GetTop(state_), 0);
}

TEST_F(LuaTest, TupleRepresentsMultipleValues) {
  std::tuple<int, int ,int> rets;
  ASSERT_TRUE(lua::Push(state_, 1, 2, 3, 4));
  ASSERT_TRUE(lua::Pop(state_, &rets));
  EXPECT_EQ(std::get<0>(rets), 2);
  EXPECT_EQ(std::get<1>(rets), 3);
  EXPECT_EQ(std::get<2>(rets), 4);
  int fourth;
  ASSERT_TRUE(lua::Pop(state_, &fourth));
  ASSERT_EQ(fourth, 1);
  ASSERT_EQ(lua::GetTop(state_), 0);
}

TEST_F(LuaTest, PCallWithInvalidValue) {
  std::string str;
  ASSERT_TRUE(lua::Push(state_, nullptr));
  EXPECT_FALSE(lua::PCall(state_, nullptr));
  ASSERT_EQ(lua::GetTop(state_), 1);
  ASSERT_TRUE(lua::Pop(state_, &str));
  ASSERT_EQ(str, "attempt to call a nil value");
}

void FunctionWithArgs(int, const std::string&) {
}

TEST_F(LuaTest, PCallWithInsufficientArgs) {
  std::string str;
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionWithArgs)));
  EXPECT_FALSE(lua::PCall(state_, nullptr, 123));
  ASSERT_TRUE(lua::Pop(state_, &str));
  ASSERT_EQ(str, "insufficient args, expecting 2 but got 1");
  ASSERT_EQ(lua::GetTop(state_), 0);

  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionWithArgs)));
  EXPECT_FALSE(lua::PCall(state_, nullptr));
  ASSERT_TRUE(lua::Pop(state_, &str));
  ASSERT_EQ(str, "insufficient args, expecting 2 but got 0");
  ASSERT_EQ(lua::GetTop(state_), 0);
}

TEST_F(LuaTest, PCallWithWrongArgs) {
  std::string str;
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionWithArgs)));
  EXPECT_FALSE(lua::PCall(state_, nullptr, "test", 123));
  ASSERT_TRUE(lua::Pop(state_, &str));
  ASSERT_EQ(str, "error converting arg at index 1 from string to integer");
  ASSERT_EQ(lua::GetTop(state_), 0);
}

void FunctionWithoutReturnValue() {
}

TEST_F(LuaTest, PCallWithoutReturnValue) {
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionWithoutReturnValue)));
  EXPECT_TRUE(lua::PCall(state_, nullptr));
  ASSERT_EQ(lua::GetTop(state_), 0);
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionWithArgs)));
  EXPECT_TRUE(lua::PCall(state_, nullptr, 123, "test"));
  ASSERT_EQ(lua::GetTop(state_), 0);
}

int FunctionReturnsInt(int num) {
  return num;
}

std::string FunctionReturnsString(base::StringPiece str) {
  return std::string(str.data(), str.size());
}

TEST_F(LuaTest, PCallWithReturnValue) {
  int num = 42;
  int out = 0;
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionReturnsInt)));
  ASSERT_TRUE(lua::PCall(state_, &out, num));
  EXPECT_EQ(num, out);
  ASSERT_EQ(lua::GetTop(state_), 0);

  base::StringPiece str = "valar morghulis";
  std::string str_out;
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionReturnsString)));
  ASSERT_TRUE(lua::PCall(state_, &str_out, str));
  EXPECT_EQ(str_out, str);
  ASSERT_EQ(lua::GetTop(state_), 0);
}

std::tuple<std::string, int> FunctionReturnsTuple(
    const std::string& str, int number) {
  return std::make_tuple(str, number);
}

TEST_F(LuaTest, PCallWithMultipleReturnValues) {
  ASSERT_TRUE(lua::Push(state_, base::Bind(&FunctionReturnsTuple)));
  std::tuple<std::string, int> out;
  ASSERT_TRUE(lua::PCall(state_, &out, "str", 123));
  EXPECT_EQ(std::get<0>(out), "str");
  EXPECT_EQ(std::get<1>(out), 123);
  ASSERT_EQ(lua::GetTop(state_), 0);
}
