/**
 * @file cartesian_position_constraint.h
 * @brief The cartesian position constraint
 *
 * @author Matthew Powelson
 * @date May 18, 2020
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2020, Southwest Research Institute
 *
 * @par License
 * Software License Agreement (Apache License)
 * @par
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * @par
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef TRAJOPT_IFOPT_CARTESIAN_POSITION_CONSTRAINT_H
#define TRAJOPT_IFOPT_CARTESIAN_POSITION_CONSTRAINT_H

#include <trajopt_utils/macros.h>
TRAJOPT_IGNORE_WARNINGS_PUSH
#include <Eigen/Eigen>
#include <ifopt/constraint_set.h>

#include <tesseract_kinematics/core/forward_kinematics.h>
#include <tesseract_environment/core/environment.h>
#include <tesseract_environment/core/utils.h>
TRAJOPT_IGNORE_WARNINGS_POP

#include <trajopt/utils.hpp>
#include <trajopt_ifopt/variable_sets/joint_position_variable.h>

namespace trajopt
{
/**
 * @brief Contains kinematic information for the cartesian position cost
 */
struct CartPosKinematicInfo
{
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using Ptr = std::shared_ptr<CartPosKinematicInfo>;
  using ConstPtr = std::shared_ptr<const CartPosKinematicInfo>;

  CartPosKinematicInfo() = default;
  CartPosKinematicInfo(tesseract_kinematics::ForwardKinematics::ConstPtr manip,
                       tesseract_environment::AdjacencyMap::ConstPtr adjacency_map,
                       const Eigen::Isometry3d& world_to_base,
                       std::string link,
                       const Eigen::Isometry3d& tcp = Eigen::Isometry3d::Identity())
    : manip_(std::move(manip))
    , adjacency_map_(std::move(adjacency_map))
    , world_to_base_(world_to_base)
    , link_(std::move(link))
    , tcp_(tcp)
  {
    kin_link_ = adjacency_map_->getLinkMapping(link_);
    if (kin_link_ == nullptr)
    {
      CONSOLE_BRIDGE_logError("Link name '%s' provided does not exist.", link_.c_str());
      assert(false);
    }
  }

  tesseract_kinematics::ForwardKinematics::ConstPtr manip_;
  tesseract_environment::AdjacencyMap::ConstPtr adjacency_map_;
  Eigen::Isometry3d world_to_base_;
  std::string link_;
  tesseract_environment::AdjacencyMapPair::ConstPtr kin_link_;
  Eigen::Isometry3d tcp_;
};

class CartPosConstraint : public ifopt::ConstraintSet
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using Ptr = std::shared_ptr<CartPosConstraint>;
  using ConstPtr = std::shared_ptr<const CartPosConstraint>;

  CartPosConstraint(const Eigen::Isometry3d& target_pose,
                    CartPosKinematicInfo::ConstPtr kinematic_info,
                    JointPosition::Ptr position_var,
                    const std::string& name = "CartPos");

  /**
   * @brief CalcValues Calculates the values associated with the constraint
   * @param joint_vals Input joint values for which FK is solved
   * @return Error of FK solution from target, size 6. The first 3 terms are associated with position and the last 3 are
   * associated with orientation.
   */
  Eigen::VectorXd CalcValues(const Eigen::Ref<Eigen::VectorXd>& joint_vals) const;
  /**
   * @brief Returns the values associated with the constraint. In this case that is the concatenated joint values
   * associated with each of the joint positions should be n_dof_ * n_vars_ long
   * @return
   */
  Eigen::VectorXd GetValues() const override;

  /**
   * @brief  Returns the "bounds" of this constraint. How these are enforced is up to the solver
   * @return Returns the "bounds" of this constraint
   */
  std::vector<ifopt::Bounds> GetBounds() const override;

  void SetBounds(const std::vector<ifopt::Bounds>& bounds);

  /**
   * @brief Fills the jacobian block associated with the constraint
   * @param jac_block Block of the overall jacobian associated with these constraints
   */
  void CalcJacobianBlock(const Eigen::Ref<Eigen::VectorXd>& joint_vals, Jacobian& jac_block) const;
  /**
   * @brief Fills the jacobian block associated with the given var_set.
   * @param var_set Name of the var_set to which the jac_block is associated
   * @param jac_block Block of the overal jacobian associated with these constraints and the var_set variable
   */
  void FillJacobianBlock(std::string var_set, Jacobian& jac_block) const override;

  void SetTargetPose(const Eigen::Isometry3d& target_pose);

  const CartPosKinematicInfo::ConstPtr& getKinematicInfo() { return kinematic_info_; }

  Eigen::Isometry3d GetTargetPose() const;

  Eigen::Isometry3d GetCurrentPose() const;

  bool use_numeric_differentiation_ = true;

private:
  /** @brief The number of joints in a single JointPosition */
  long n_dof_;

  /** @brief Bounds on the positions of each joint */
  std::vector<ifopt::Bounds> bounds_;

  /** @brief Pointers to the vars used by this constraint.
   *
   * Do not access them directly. Instead use this->GetVariables()->GetComponent(position_var->GetName())->GetValues()*/
  JointPosition::Ptr position_var_;

  Eigen::Isometry3d target_pose_;
  Eigen::Isometry3d target_pose_inv_;
  CartPosKinematicInfo::ConstPtr kinematic_info_;
};
};  // namespace trajopt
#endif
