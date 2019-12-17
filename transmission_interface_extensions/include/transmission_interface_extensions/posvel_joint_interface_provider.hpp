#ifndef TRANSMISSION_INTERFACE_EXTENSIONS_POSVEL_JOINT_INTERFACE_PROVIDER_HPP
#define TRANSMISSION_INTERFACE_EXTENSIONS_POSVEL_JOINT_INTERFACE_PROVIDER_HPP

#include <string>

#include <hardware_interface/posvel_command_interface.h>
#include <hardware_interface/robot_hw.h>
#include <transmission_interface/joint_state_interface_provider.h>
#include <transmission_interface/position_joint_interface_provider.h>
#include <transmission_interface/transmission_info.h>
// for RequisiteProvider, JointInterfaces,RawJointDataMap,JointData
#include <transmission_interface/transmission_interface_loader.h>
#include <transmission_interface/velocity_joint_interface_provider.h>
#include <transmission_interface_extensions/common_namespaces.hpp>
#include <transmission_interface_extensions/public_adapter.hpp>

#include <boost/foreach.hpp>

namespace transmission_interface_extensions {

class PosVelJointInterfaceProvider : public ti::RequisiteProvider {
public:
  PosVelJointInterfaceProvider() {}

  virtual ~PosVelJointInterfaceProvider() {}

  virtual bool updateJointInterfaces(const ti::TransmissionInfo &transmission_info,
                                     hi::RobotHW *robot_hw, ti::JointInterfaces &joint_interfaces,
                                     ti::RawJointDataMap &raw_joint_data_map) {
    // first, setup state interfaces as a state provider
    if (!state_provider_.updateJointInterfaces(transmission_info, robot_hw, joint_interfaces,
                                               raw_joint_data_map)) {
      return false;
    }

    // If interface does not yet exist in robot hardware abstraction, add it and use internal data
    // structures
    if (!robot_hw->get< hi::PosVelJointInterface >()) {
      robot_hw->registerInterface(&getPosVelJointInterface());
    }
    hi::PosVelJointInterface *const interface(robot_hw->get< hi::PosVelJointInterface >());

    // Register joints on the hardware interface
    BOOST_FOREACH (const ti::JointInfo &joint_info, transmission_info.joints_) {
      // Do nothing if joint already exists on the hardware interface
      if (hasResource(joint_info.name_, *interface)) {
        continue;
      }

      // Update hardware interface
      ti::RawJointData &raw_joint_data(raw_joint_data_map[joint_info.name_]);
      const hi::PosVelJointHandle handle(
          joint_interfaces.joint_state_interface.getHandle(joint_info.name_),
          &raw_joint_data.position_cmd, &raw_joint_data.velocity_cmd);
      interface->registerHandle(handle);
    }

    return true;
  }

protected:
  virtual bool getJointStateData(const ti::TransmissionInfo &transmission_info,
                                 const ti::RawJointDataMap &raw_joint_data_map,
                                 ti::JointData &jnt_state_data) {
    // retrieve state data as a state provider
    return state_provider_.getJointStateData(transmission_info, raw_joint_data_map, jnt_state_data);
  }

  virtual bool getJointCommandData(const ti::TransmissionInfo &transmission_info,
                                   const ti::RawJointDataMap &raw_joint_data_map,
                                   ti::JointData &jnt_cmd_data) {
    // retrieve command data as command providers
    return pos_cmd_provider_.getJointCommandData(transmission_info, raw_joint_data_map,
                                                 jnt_cmd_data) &&
           vel_cmd_provider_.getJointCommandData(transmission_info, raw_joint_data_map,
                                                 jnt_cmd_data);
  }

  virtual bool getActuatorStateData(const ti::TransmissionInfo &transmission_info,
                                    hi::RobotHW *robot_hw, ti::ActuatorData &act_state_data) {
    // retrieve state data as a state provider
    return state_provider_.getActuatorStateData(transmission_info, robot_hw, act_state_data);
  }

  virtual bool getActuatorCommandData(const ti::TransmissionInfo &transmission_info,
                                      hi::RobotHW *robot_hw, ti::ActuatorData &act_cmd_data) {
    // retrieve command data as command providers
    return pos_cmd_provider_.getActuatorCommandData(transmission_info, robot_hw, act_cmd_data) &&
           vel_cmd_provider_.getActuatorCommandData(transmission_info, robot_hw, act_cmd_data);
  }

  virtual bool registerTransmission(ti::TransmissionLoaderData &loader_data,
                                    TransmissionHandleData &handle_data) {
    // register state, position command and velocity command transmissions (if not yet)
    return state_provider_.registerTransmission(loader_data, handle_data) &&
           pos_cmd_provider_.registerTransmission(loader_data, handle_data) &&
           vel_cmd_provider_.registerTransmission(loader_data, handle_data);
  }

private:
  // return the joint interface. it must be in the static memory space
  // to allow access from outside of this plugin.
  static hi::PosVelJointInterface &getPosVelJointInterface() {
    static hi::PosVelJointInterface interface;
    return interface;
  }

  PublicAdapter< ti::JointStateInterfaceProvider > state_provider_;
  PublicAdapter< ti::PositionJointInterfaceProvider > pos_cmd_provider_;
  PublicAdapter< ti::VelocityJointInterfaceProvider > vel_cmd_provider_;
};
} // namespace transmission_interface_extensions

#endif