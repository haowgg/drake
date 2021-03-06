#include "drake/systems/framework/basic_vector.h"

#include <cmath>
#include <sstream>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include "drake/common/autodiff.h"
#include "drake/common/eigen_types.h"
#include "drake/common/symbolic.h"
#include "drake/common/test_utilities/eigen_matrix_compare.h"
#include "drake/systems/framework/test_utilities/my_vector.h"

namespace drake {
namespace systems {
namespace {

// Tests the initializer_list functionality.
GTEST_TEST(BasicVectorTest, InitializerList) {
  const BasicVector<double> empty1;  // Default constructor.
  const BasicVector<double> empty2{};  // Initializer list.
  EXPECT_EQ(0, empty1.size());
  EXPECT_EQ(0, empty2.size());

  const BasicVector<double> pair{1.0, 2.0};
  ASSERT_EQ(2, pair.size());
  EXPECT_EQ(1.0, pair[0]);
  EXPECT_EQ(2.0, pair[1]);

  const BasicVector<double> from_floats{3.0f, 4.0f, 5.0f};
  ASSERT_EQ(3, from_floats.size());
  EXPECT_EQ(3.0, from_floats[0]);
  EXPECT_EQ(4.0, from_floats[1]);
  EXPECT_EQ(5.0, from_floats[2]);

  const BasicVector<AutoDiffXd> autodiff{22.0};
  ASSERT_EQ(1, autodiff.size());
  EXPECT_EQ(22.0, autodiff[0].value());
}

// Tests SetZero functionality.
GTEST_TEST(BasicVectorTest, SetZero) {
  BasicVector<double> vec{1.0, 2.0, 3.0};
  EXPECT_EQ(Eigen::Vector3d(1.0, 2.0, 3.0), vec.get_value());
  vec.SetZero();
  EXPECT_EQ(Eigen::Vector3d(0, 0, 0), vec.get_value());
}

// Tests that the BasicVector<double> is initialized to NaN.
GTEST_TEST(BasicVectorTest, DoubleInitiallyNaN) {
  BasicVector<double> vec(3);
  Eigen::Vector3d expected(NAN, NAN, NAN);
  EXPECT_TRUE(CompareMatrices(expected, vec.get_value(),
                              Eigen::NumTraits<double>::epsilon(),
                              MatrixCompareType::absolute));
}

// Tests that the BasicVector<AutoDiffXd> is initialized to NaN.
GTEST_TEST(BasicVectorTest, AutodiffInitiallyNaN) {
  BasicVector<AutoDiffXd> vec(3);
  EXPECT_TRUE(std::isnan(vec[0].value()));
  EXPECT_TRUE(std::isnan(vec[1].value()));
  EXPECT_TRUE(std::isnan(vec[2].value()));
}

// Tests that the BasicVector<symbolic::Expression> is initialized to NaN.
GTEST_TEST(BasicVectorTest, SymbolicInitiallyNaN) {
  BasicVector<symbolic::Expression> vec(1);
  EXPECT_TRUE(symbolic::is_nan(vec.get_value()[0]));
}

// Tests BasicVector<T>::Make.
GTEST_TEST(BasicVectorTest, Make) {
  auto dut1 = BasicVector<double>::Make(1.0, 2.0);
  auto dut2 = BasicVector<double>::Make({3.0, 4.0});
  EXPECT_TRUE(CompareMatrices(dut1->get_value(), Eigen::Vector2d(1.0, 2.0)));
  EXPECT_TRUE(CompareMatrices(dut2->get_value(), Eigen::Vector2d(3.0, 4.0)));
}

// Tests that BasicVector<symbolic::Expression>::Make does what it says on
// the tin.
GTEST_TEST(BasicVectorTest, MakeSymbolic) {
  auto vec = BasicVector<symbolic::Expression>::Make(
      symbolic::Variable("x"),
      2.0,
      symbolic::Variable("y") + 2.0);
  EXPECT_EQ("x", vec->GetAtIndex(0).to_string());
  EXPECT_EQ("2", vec->GetAtIndex(1).to_string());
  EXPECT_EQ("(2 + y)", vec->GetAtIndex(2).to_string());
}

// Tests that the BasicVector has a size as soon as it is constructed.
GTEST_TEST(BasicVectorTest, Size) {
  BasicVector<double> vec(5);
  EXPECT_EQ(5, vec.size());
}

// Tests that the BasicVector can be mutated in-place.
GTEST_TEST(BasicVectorTest, Mutate) {
  BasicVector<double> vec(2);
  vec.get_mutable_value() << 1, 2;
  Eigen::Vector2d expected;
  expected << 1, 2;
  EXPECT_EQ(expected, vec.get_value());
}

// Tests that the BasicVector can be addressed as an array.
GTEST_TEST(BasicVectorTest, ArrayOperator) {
  BasicVector<double> vec(2);
  vec[0] = 76;
  vec[1] = 42;

  Eigen::Vector2d expected;
  expected << 76, 42;
  EXPECT_EQ(expected, vec.get_value());
  EXPECT_EQ(76, vec[0]);
  EXPECT_EQ(42, vec[1]);
}

// Tests that the BasicVector can be set from another vector.
GTEST_TEST(BasicVectorTest, SetWholeVector) {
  BasicVector<double> vec(2);
  vec.get_mutable_value() << 1, 2;
  Eigen::Vector2d next_value;
  next_value << 3, 4;
  vec.set_value(next_value);
  EXPECT_EQ(next_value, vec.get_value());
}

// Tests that when BasicVector is cloned, its data is preserved.
GTEST_TEST(BasicVectorTest, Clone) {
  BasicVector<double> vec(2);
  vec.get_mutable_value() << 1, 2;

  std::unique_ptr<BasicVector<double>> clone = vec.Clone();

  Eigen::Vector2d expected;
  expected << 1, 2;
  EXPECT_EQ(expected, clone->get_value());
}

// Tests that an error is thrown when the BasicVector is set from a vector
// of a different size.
GTEST_TEST(BasicVectorTest, ReinitializeInvalid) {
  BasicVector<double> vec(2);
  Eigen::Vector3d next_value;
  next_value << 3, 4, 5;
  EXPECT_THROW(vec.set_value(next_value), std::out_of_range);
}

// Tests the infinity norm computation
GTEST_TEST(BasicVectorTest, NormInf) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  BasicVector<double> vec(2);
  vec.get_mutable_value() << 3, -4;
  EXPECT_EQ(vec.NormInf(), 4);
#pragma GCC diagnostic pop
}

// Tests the infinity norm for an autodiff type.
GTEST_TEST(BasicVectorTest, NormInfAutodiff) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  // Set up the device under test ("dut").
  // The DUT is a vector with two values [-11.5, 22.5].
  // The ∂/∂t of DUT is [1.5, 3.5] (where t is some arbitrary variable).
  AutoDiffXd element0;
  element0.value() = -11.5;
  element0.derivatives() = Vector1d(1.5);
  AutoDiffXd element1;
  element1.value() = 22.5;
  element1.derivatives() = Vector1d(3.5);
  BasicVector<AutoDiffXd> dut{element0, element1};

  // The norminf(DUT) is 22.5 and the ∂/∂t of norminf(DUT) is 3.5.
  // The element1 has the max absolute value of the AutoDiffScalar's scalar.
  // It is positive, so the sign of its derivatives remains unchanged.
  AutoDiffXd expected_norminf;
  expected_norminf.value() = 22.5;
  expected_norminf.derivatives() = Vector1d(3.5);
  EXPECT_EQ(dut.NormInf().value(), expected_norminf.value());
  EXPECT_EQ(dut.NormInf().derivatives(), expected_norminf.derivatives());

  // We change the DUT to two values [-11.5, -33.5] with ∂/∂t of [1.5, 3.5].
  // The norminf(DUT) is now 33.5 and the ∂/∂t of norminf(DUT) is -3.5.
  // The element0 has the max absolute value of the AutoDiffScalar's scalar.
  // It is negative, so the sign of its derivatives gets flipped.
  dut[0].value() = -33.5;
  expected_norminf.value() = 33.5;
  expected_norminf.derivatives() = Vector1d(-1.5);
  EXPECT_EQ(dut.NormInf().value(), expected_norminf.value());
  EXPECT_EQ(dut.NormInf().derivatives(), expected_norminf.derivatives());
#pragma GCC diagnostic pop
}

// Tests all += * operations for BasicVector.
GTEST_TEST(BasicVectorTest, PlusEqScaled) {
  BasicVector<double> ogvec(2), vec1(2), vec2(2), vec3(2), vec4(2), vec5(2);
  Eigen::Vector2d ans1, ans2, ans3, ans4, ans5;
  ogvec.SetZero();
  vec1.get_mutable_value() << 1, 2;
  vec2.get_mutable_value() << 3, 5;
  vec3.get_mutable_value() << 7, 11;
  vec4.get_mutable_value() << 13, 17;
  vec5.get_mutable_value() << 19, 23;
  VectorBase<double>& v1 = vec1;
  VectorBase<double>& v2 = vec2;
  VectorBase<double>& v3 = vec3;
  VectorBase<double>& v4 = vec4;
  VectorBase<double>& v5 = vec5;
  ogvec.PlusEqScaled(2, v1);
  ans1 << 2, 4;
  EXPECT_EQ(ans1, ogvec.get_value());

  ogvec.SetZero();
  ogvec.PlusEqScaled({{2, v1}, {3, v2}});
  ans2 << 11, 19;
  EXPECT_EQ(ans2, ogvec.get_value());

  ogvec.SetZero();
  ogvec.PlusEqScaled({{2, v1}, {3, v2}, {5, v3}});
  ans3 << 46, 74;
  EXPECT_EQ(ans3, ogvec.get_value());

  ogvec.SetZero();
  ogvec.PlusEqScaled({{2, v1}, {3, v2}, {5, v3}, {7, v4}});
  ans4 << 137, 193;
  EXPECT_EQ(ans4, ogvec.get_value());

  ogvec.SetZero();
  ogvec.PlusEqScaled({{2, v1}, {3, v2}, {5, v3}, {7, v4}, {11, v5}});
  ans5 << 346, 446;
  EXPECT_EQ(ans5, ogvec.get_value());
}

// Tests ability to stream a BasicVector into a string.
GTEST_TEST(BasicVectorTest, StringStream) {
  BasicVector<double> vec(3);
  vec.get_mutable_value() << 1.0, 2.2, 3.3;
  std::stringstream s;
  s << "hello " << vec << " world";
  EXPECT_EQ(s.str(), "hello [1, 2.2, 3.3] world");
}

// Tests ability to stream a BasicVector of size zero into a string.
GTEST_TEST(BasicVectorTest, ZeroLengthStringStream) {
  BasicVector<double> vec(0);
  std::stringstream s;
  s << "foo " << vec << " bar";
  EXPECT_EQ(s.str(), "foo [] bar");
}

// Tests the default set of bounds (empty).
GTEST_TEST(BasicVectorTest, DefaultCalcInequalityConstraint) {
  VectorX<double> value = VectorX<double>::Ones(22);
  BasicVector<double> vec(1);
  Eigen::VectorXd lower, upper;
  // Deliberately set lower/upper to size 2, to check if GetElementBounds will
  // resize the bounds to empty size.
  lower.resize(2);
  upper.resize(2);
  vec.GetElementBounds(&lower, &upper);
  EXPECT_EQ(lower.size(), 0);
  EXPECT_EQ(upper.size(), 0);
}

// Tests the protected `::values()` methods.
GTEST_TEST(BasicVectorTest, ValuesAccess) {
  MyVector2d dut;
  dut[0] = 11.0;
  dut[1] = 22.0;

  // Values are as expected.
  ASSERT_EQ(dut.values().size(), 2);
  EXPECT_EQ(dut.values()[0], 11.0);
  EXPECT_EQ(dut.values()[1], 22.0);
  dut.values()[0] = 33.0;

  // The const overload is the same.
  const auto& const_dut = dut;
  EXPECT_EQ(&dut.values(), &const_dut.values());
  EXPECT_EQ(const_dut.values()[0], 33.0);
}

}  // namespace
}  // namespace systems
}  // namespace drake
