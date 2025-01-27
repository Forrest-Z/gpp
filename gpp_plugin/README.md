# GppPlugin

## API

### Parameters

The `gpp_plugin` follows the well known ros-syntax for defining and loading pluginlib-based plugins.
The user can define three plugin-groups under the tags `pre_planning`, `planning` and `post_planning`.

Every group must be defined as a list.
The execution order with a plugin-group is defined by the order in the list.

Every list element must be a dictionary with the tags `name` and `type`.
Both tags must have string values.
The `name` can be chosen freely and will be passed to the plugin (allowing the user to define parameters).
The `type` must be resolvable to a valid plugin.

Additionally the user may define `on_failure_break` and `on_success_break` tags.
Those tags must have boolean values.
`on_failure_break` defaults to true, `on_success_break` defaults to false.

Finally, every group has a default value.
This value is used if no break condition (`on_success_break` or `on_failure_break`) is activated.

Below the detailed documentation.

#### ~\<name>\/pre_planning (list)

List, as defined above.
The `type` must be resolvable to a plugin implementing the `gpp_interface::PrePlanningInterface`.

This parameter is optional.

#### ~\<name>\/planning (list)

List, as defined above.
The `type` must be resolvable to a plugin implementing either `nav_core::BaseGlobalPlanner` or `mbf_costmap_core::CostmapPlanner`.

This parameter is required and must define at least one valid global planner.

#### ~\<name>\/post_planning (list)

List, as defined above.
The `type` must be resolvable to a plugin implementing the `gpp_interface::PostPlanningInterface`.

This parameter is optional.

#### ~\<name>\/pre_planning_default_value (bool, true)

Default outcome of the pre_planning group.

#### ~\<name>\/planning_default_value (bool, true)

Default outcome of the planning group.

#### ~\<name>\/post_planning_default_value (bool, true)

Default outcome of the post_planning group.

#### ~\<name>\/tolerance (double, default: 0.1)

Metric tolerance.
This value will be used as tolerance for the pre-planning plugins, if the `gpp_plugin` is loaded as `nav_core::BaseGlobalPlanner`.
If the `gpp_plugin` is loaded as `mbf_costmap_core::CostmapPlanner`, the tolerance passed to `makePlan` will have precedence.

## Example

Below two example configs for the `move_base` and `move_base_flex` frameworks.

In the example we mark all pre-planning steps as "optional": we will ignore the failure and return success but default.
The planning group is a selector-node from behavior-trees: we will pick the first successful plan.
If no planner is successful, we return false (failure).
The post-planing group is a sequence-node from behavior-trees: we want that all steps succeed and will abort the execution as soon as one child-plugin fails.
Note, that the sequence-node is the default configuration.

### MoveBase

```yaml
base_global_planner: "gpp_plugin::GlobalPlannerPipeline"

# move-base will pass the type to the loaded plugin.
GlobalPlannerPipeline:
    # metric tolerance for the pre-planning step.
    tolerance: 0.1

    # define the pre-planning plugins
    pre_planning:
    -  {name: first_pre_planning_name, type: first_pre_planning_type, on_failure_break: false}
    -  {name: second_pre_planning_name, type: second_pre_planning_type, on_failure_break: false}

    # define the planning plugins
    planning_default_value: false
    planning:
    -  {name: first_planning_name, type: first_planning_type, on_failure_break: false, on_success_break: true}
    -  {name: second_planning_name, type: second_planning_type, on_failure_break: false, on_success_break: true}

    # define the post-planning plugins
    post_planning:
    -  {name: first_post_planning_name, type: first_post_planning_type}
    -  {name: second_post_planning_name, type: second_post_planning_type}

```

### MoveBaseFlex

```yaml
# your planner definition
# we name our planner gpp
planner:
    - {name: gpp, type: gpp_plugin::GlobalPlannerPipeline}

# now the specification for the gpp_plugin
gpp:
    # metric tolerance for the pre-planning step.
    # it will be ignored if the plugin is loaded as mbf_costmap_core::CostmapPlanner.
    tolerance: 0.1

    # define the pre-planning plugins
    pre_planning:
    -  {name: first_pre_planning_name, type: first_pre_planning_type, on_failure_break: false}
    -  {name: second_pre_planning_name, type: second_pre_planning_type, on_failure_break: false}

    # define the planning plugins
    planning_default_value: false
    planning:
    -  {name: first_planning_name, type: first_planning_type, on_failure_break: false, on_success_break: true}
    -  {name: second_planning_name, type: second_planning_type, on_failure_break: false, on_success_break: true}

    # define the post-planning plugins
    post_planning:
    -  {name: first_post_planning_name, type: first_post_planning_type}
    -  {name: second_post_planning_name, type: second_post_planning_type}

```
