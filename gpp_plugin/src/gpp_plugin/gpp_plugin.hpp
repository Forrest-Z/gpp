#pragma once

#include <gpp_interface/post_planning_interface.hpp>
#include <gpp_interface/pre_planning_interface.hpp>
#include <costmap_2d/costmap_2d_ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mbf_costmap_core/costmap_planner.h>
#include <nav_core/base_global_planner.h>
#include <pluginlib/class_loader.h>
#include <ros/ros.h>
#include <xmlrpcpp/XmlRpcException.h>
#include <xmlrpcpp/XmlRpcValue.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gpp_plugin {

/**
 * @brief POD defining the meta information required to load a plugin
 *
 * This allows us to define the required tags (name and type) at compile time
 */
template <typename _Plugin>
struct PluginDefinition {
  using type = _Plugin;
  static const std::string package;
  static const std::string base_class;
};

// define shortcuts to the resource types
using gpp_interface::PostPlanningInterface;
using gpp_interface::PrePlanningInterface;
using mbf_costmap_core::CostmapPlanner;
using nav_core::BaseGlobalPlanner;

// clang-format off

// below the definition of the different plugins.
// we put it into the header, since this is type of the interface.
// extend here, if you want to load other types.

// Preplanning specialization
template <>
const std::string PluginDefinition<PrePlanningInterface>::package = "gpp_interface";

template <>
const std::string PluginDefinition<PrePlanningInterface>::base_class = "gpp_interface::PrePlanningInterface";

// Preplanning specialization
template <>
const std::string PluginDefinition<PostPlanningInterface>::package = "gpp_interface";

template <>
const std::string PluginDefinition<PostPlanningInterface>::base_class = "gpp_interface::PostPlanningInterface";

// nav-core specialization
template <>
const std::string PluginDefinition<BaseGlobalPlanner>::package = "nav_core";

template <>
const std::string PluginDefinition<BaseGlobalPlanner>::base_class = "nav_core::BaseGlobalPlanner";

// clang-format on

// below the plugin loading machinery. the implementation is moved into cpp,
// since we provide the (only valid) template arguments at compile time.

/**
 * @brief Base class for loading a plugin with a valid PluginDefinition
 *
 * @tparam _Plugin Plugin-type. You need to provide a specialization of
 * the PluginDefinition for the _Plugin for this to work.
 */
template <typename _Plugin>
struct PluginManager : public pluginlib::ClassLoader<_Plugin> {
  // the defintion with compile-time package and base_class tags
  using Definition = PluginDefinition<_Plugin>;

  PluginManager();
};

/**
 * @brief Common interface for a plugin-manager
 *
 * The class defines the ownership and storage of plugins.
 * Typically we own the plugin and store all of them in a vector.
 */
template <typename _Plugin>
struct ManagerInterface {
  // this class owns the plugin
  using PluginPtr = typename pluginlib::UniquePtr<_Plugin>;

  using NamedPlugin = std::pair<std::string, PluginPtr>;
  using PluginMap = std::vector<NamedPlugin>;

  inline const PluginMap&
  getPlugins() const noexcept {
    return plugins_;
  }

protected:
  PluginMap plugins_;
};

/**
 * @brief Loads an array of plugins.
 *
 * @section Usage
 *
 * The class offers two methods:
 * - load will try to read the types and names from the parameter server and
 *   load the specified plugins. This method is idempotent.
 * - getPlugins will return the loaded plugins.
 *
 * Both methods are not thread-safe. The concurrency management must be done
 * by the user.
 *
 * Code example:
 *
 * @code{cpp}
 * // you will need a note-handle
 * ros::NodeHandle nh("~");
 *
 * // create the manager
 * ArrayPluginManager<MyPlugin> manager;
 *
 * // load the plugins
 * try{
 *  // will throw if "my_resource_tag" does not specify an array
 *  manager.load("my_resource_tag", nh);
 * }
 * catch(std::invalid_argument& ex){
 *  std::cerr << "failed to load " << ex.what() << std::endl;
 *  return;
 * }
 *
 * // get the plugins and init them as you like
 * const auto& plugins = manager.getPlugins();
 * ...
 * @endcode
 *
 * @section Parameters
 *
 * The ros-parameter resource defined under the _resource argument (see
 * load-method) must be an array.
 * The array resquires two tags - 'name' and 'type'.
 * 'name' defines a unique descriptor which will be passed to a plugin.
 * 'type' defines the type of the plugin.
 * Both tags must have literal values.
 *
 * Code example:
 *
 * @code{yaml}
 * my_resource_tag:
 *  - {name: foo, type: a_valid_type}
 *  - {name: baz, type: another_type}
 * @endcode
 *
 * @section Remarks
 *
 * We don't need an explicit destructor here, since the destructors are
 * called in reverse order of the construction: so the ManagerInterface
 * is always destructed first.
 *
 * https://stackoverflow.com/questions/31518581/order-of-destruction-in-the-case-of-multiple-inheritance
 *
 * @tparam _Plugin as in PluginManager<_Plugin>
 *
 */
template <typename _Plugin>
struct ArrayPluginManager : public PluginManager<_Plugin>,
                            public ManagerInterface<_Plugin> {
  void
  load(const std::string& _resource, ros::NodeHandle& _nh);
};

// compile time specification of the ArrayPluginManager
using PrePlanningManager = ArrayPluginManager<PrePlanningInterface>;
using PostPlanningManager = ArrayPluginManager<PostPlanningInterface>;
using GlobalPlannerManager = ArrayPluginManager<BaseGlobalPlanner>;

/**
 * @brief Combine pre-planning, planning and post-planning to customize the
 * your path.
 *
 * The planner implements BaseGlobalPlanner and CostmapPlanner interfaces.
 *
 * @section Parameters
 *
 * Define the pre_planning plugins under the tag `pre_planning`. Those plugins
 * must adhere to the `gpp_interface::PrePlanning` interface. Define planning
 * plugins under the tag `planning` and the post-planning plugins under the tag
 * `post_planning`. Those plugins must adhere to `nav_core::BaseGlobalPlanner`
 * and `gpp_interface::PostPlanning`, respectively.
 *
 * The plugins under every tag (`pre_planning`, `planning` or `post_planning`)
 * must be defined as an array. Every element withing the array must have
 * the tags `name` and `type` - following the standard ros syntax for
 * pluginlib-loaded plugins.
 *
 * You need to provide at least one plugin under the tag `planning`.
 *
 * If you are not using `move_base_flex`, you can also define a custom tolerance
 * under the parameter `tolerance` - defining the metric goal tolerance.
 *
 * Below is a code example
 *
 * @code{yaml}
 *
 * # goal tolerance in meters
 * tolerance: 0.1
 *
 * # define the pre-planning plugins
 * pre_planning:
 * -  {name: first_pre_planning_name, type: first_pre_planning_type}
 * -  {name: second_pre_planning_name, type: second_pre_planning_type}
 *
 * # define the planning plugins
 * planning:
 * -  {name: first_planning_name, type: first_planning_type}
 * -  {name: second_planning_name, type: second_planning_type}
 *
 * # define the post-planning plugins
 * post_planning:
 * -  {name: first_post_planning_name, type: first_post_planning_type}
 * -  {name: second_post_planning_name, type: second_post_planning_type}
 *
 * @endcode
 *
 */
struct GlobalPlannerPipeline : public BaseGlobalPlanner, public CostmapPlanner {
  // define our interface types
  using Pose = geometry_msgs::PoseStamped;
  using Path = std::vector<Pose>;
  using Map = costmap_2d::Costmap2DROS;

  bool
  makePlan(const Pose& _start, const Pose& _goal, Path& _plan) override;

  bool
  makePlan(const Pose& _start, const Pose& _goal, Path& _plan,
           double& _cost) override;

  uint32_t
  makePlan(const Pose& start, const Pose& goal, double tolerance, Path& plan,
           double& cost, std::string& message) override;
  void
  initialize(std::string name, Map* costmap_ros) override;

  bool
  cancel() override;

private:
  bool
  prePlanning(Pose& _start, Pose& _goal, double _tolerance);

  bool
  postPlanning(Path& _path, double& _cost);

  bool
  globalPlanning(const Pose& _start, const Pose& _goal, Path& _plan,
                 double& _cost);

  double tolerance_;
  std::atomic_bool cancel_;

  // nav_core conforming members
  std::string name_;
  Map* costmap_ = nullptr;

  PrePlanningManager pre_planning_;
  PostPlanningManager post_planning_;
  GlobalPlannerManager global_planning_;
};

}  // namespace gpp_plugin