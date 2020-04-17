#ifndef TRAJOPT_IFOPT_JOINT_POSITION_CONSTRAINT_H
#define TRAJOPT_IFOPT_JOINT_POSITION_CONSTRAINT_H

#include <ifopt/constraint_set.h>
#include <trajopt_ifopt/variable_sets/joint_position_variable.h>

#include <Eigen/Eigen>

namespace trajopt
{
/**
 * @brief This creates a joint position constraint. Allows bounds to be set on a joint position
 */
class JointPosConstraint : public ifopt::ConstraintSet
{
public:
  using Ptr = std::shared_ptr<JointPosConstraint>;
  using ConstPtr = std::shared_ptr<const JointPosConstraint>;

  /**
   * @brief JointPosConstraint
   * @param targets Target joint position (length should be n_dof). Upper and lower bounds are set to this value
   * @param position_vars Variables to which this constraint is applied. Note that all variables should have the same
   * number of components (joint DOF)
   * @param name Name of the constraint
   */
  JointPosConstraint(const Eigen::VectorXd& targets,
                     const std::vector<JointPosition::Ptr>& position_vars,
                     const std::string& name = "JointPos");

  /**
   * @brief JointPosConstraint
   * @param bounds Bounds on target joint position (length should be n_dof)
   * @param position_vars Variables to which this constraint is applied
   * @param name Name of the constraint
   */
  JointPosConstraint(const std::vector<ifopt::Bounds>& bounds,
                     const std::vector<JointPosition::Ptr>& position_vars,
                     const std::string& name = "JointPos");

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

  /**
   * @brief Fills the jacobian block associated with the given var_set.
   * @param var_set Name of the var_set to which the jac_block is associated
   * @param jac_block Block of the overal jacobian associated with these constraints and the var_set variable
   */
  void FillJacobianBlock(std::string var_set, Jacobian& jac_block) const override;

private:
  /** @brief The number of joints in a single JointPosition */
  long n_dof_;
  /** @brief The number of JointPositions passed in */
  long n_vars_;

  /** @brief Bounds on the positions of each joint */
  std::vector<ifopt::Bounds> bounds_;

  /** @brief Pointers to the vars used by this constraint.
   *
   * Do not access them directly. Instead use this->GetVariables()->GetComponent(position_var->GetName())->GetValues()*/
  std::vector<JointPosition::Ptr> position_vars_;
};
};  // namespace trajopt
#endif
