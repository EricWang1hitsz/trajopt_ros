#include <trajopt_ifopt/constraints/cartesian_position_constraint.h>

TRAJOPT_IGNORE_WARNINGS_PUSH
#include <tesseract_kinematics/core/utils.h>
#include <console_bridge/console.h>
TRAJOPT_IGNORE_WARNINGS_POP

namespace trajopt
{
CartPosConstraint::CartPosConstraint(const Eigen::Isometry3d& target_pose,
                                     CartPosKinematicInfo::ConstPtr kinematic_info,
                                     JointPosition::Ptr position_var,
                                     const std::string& name)
  : ifopt::ConstraintSet(6, name)
  , position_var_(std::move(position_var))
  , target_pose_(target_pose)
  , target_pose_inv_(target_pose.inverse())
  , kinematic_info_(std::move(kinematic_info))
{
  // Set the n_dof and n_vars for convenience
  n_dof_ = kinematic_info_->manip_->numJoints();
  assert(n_dof_ > 0);

  bounds_ = std::vector<ifopt::Bounds>(6, ifopt::BoundZero);
}

Eigen::VectorXd CartPosConstraint::GetValues() const
{
  Eigen::Isometry3d new_pose;
  VectorXd joint_vals = this->GetVariables()->GetComponent(position_var_->GetName())->GetValues();
  kinematic_info_->manip_->calcFwdKin(new_pose, joint_vals, kinematic_info_->kin_link_->link_name);

  new_pose = kinematic_info_->world_to_base_ * new_pose * kinematic_info_->kin_link_->transform * kinematic_info_->tcp_;

  Eigen::Isometry3d pose_err = target_pose_inv_ * new_pose;
  Eigen::VectorXd err = concat(pose_err.translation(), calcRotationalError(pose_err.rotation()));

  return err;
}

// Set the limits on the constraint values
std::vector<ifopt::Bounds> CartPosConstraint::GetBounds() const { return bounds_; }

void CartPosConstraint::SetBounds(const std::vector<ifopt::Bounds>& bounds)
{
  assert(bounds.size() == 6);
  bounds_ = bounds;
}

void CartPosConstraint::FillJacobianBlock(std::string var_set, Jacobian& jac_block) const
{
  // Only modify the jacobian if this constraint uses var_set
  if (var_set == position_var_->GetName())
  {
    // Reserve enough room in the sparse matrix
    jac_block.reserve(Eigen::VectorXd::Constant(n_dof_ * 6, 1));

    // Get current joint values
    VectorXd joint_vals = this->GetVariables()->GetComponent(position_var_->GetName())->GetValues();

    Eigen::MatrixXd jac0(6, n_dof_);
    Eigen::Isometry3d tf0;

    // Calculate the jacobian
    kinematic_info_->manip_->calcFwdKin(tf0, joint_vals, kinematic_info_->kin_link_->link_name);
    kinematic_info_->manip_->calcJacobian(jac0, joint_vals, kinematic_info_->kin_link_->link_name);
    tesseract_kinematics::jacobianChangeBase(jac0, kinematic_info_->world_to_base_);
    tesseract_kinematics::jacobianChangeRefPoint(
        jac0,
        (kinematic_info_->world_to_base_ * tf0).linear() *
            (kinematic_info_->kin_link_->transform * kinematic_info_->tcp_).translation());
    tesseract_kinematics::jacobianChangeBase(jac0, target_pose_inv_);

    // Paper:
    // https://ethz.ch/content/dam/ethz/special-interest/mavt/robotics-n-intelligent-systems/rsl-dam/documents/RobotDynamics2016/RD2016script.pdf
    // The jacobian of the robot is the geometric jacobian (Je) which maps generalized velocities in
    // joint space to time derivatives of the end-effector configuration representation. It does not
    // represent the analytic jacobian (Ja) given by a partial differentiation of position and rotation
    // to generalized coordinates. Since the geometric jacobian is unique there exists a linear mapping
    // between velocities and the derivatives of the representation.
    //
    // The approach in the paper was tried but it was having issues with getting correct jacobian.
    // Must of had an error in the implementation so should revisit at another time but the approach
    // below should be sufficient and faster than numerical calculations using the err function.

    // The approach below leverages the geometric jacobian and a small step in time to approximate
    // the partial derivative of the error function. Note that the rotational portion is the only part
    // that is required to be modified per the paper.
    Eigen::Isometry3d pose_err = target_pose_inv_ * tf0;
    Eigen::Vector3d rot_err = calcRotationalError(pose_err.rotation());
    for (int c = 0; c < jac0.cols(); ++c)
    {
      auto new_pose_err = addTwist(pose_err, jac0.col(c), 1e-5);
      Eigen::VectorXd new_rot_err = calcRotationalError(new_pose_err.rotation());
      jac0.col(c).tail(3) = ((new_rot_err - rot_err) / 1e-5);
    }

    // Convert to a sparse matrix and set the jacobian
    // TODO: Make this more efficient. This does not work.
    //    Jacobian jac_block = jac0.sparseView();

    // This does work but could be faster
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < n_dof_; j++)
      {
        // Each jac_block will be for a single variable but for all timesteps. Therefore we must index down to the
        // correct timestep for this variable
        jac_block.coeffRef(i, j) = jac0(i, j);
      }
    }
  }
}

void CartPosConstraint::setTargetPose(const Eigen::Isometry3d& target_pose)
{
  target_pose_ = target_pose;
  target_pose_inv_ = target_pose.inverse();
}
}  // namespace trajopt
