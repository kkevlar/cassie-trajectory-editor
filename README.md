
# Cassie Trajectory Tool


This tool runs on top of the the simulation program, [MuJoCo](http://www.mujoco.org/), and is designed to generate and modify walking trajectories for the bipedal robot, [Cassie](http://www.agilityrobotics.com/robots/).
The generated walking trajectories can act as the reference motion in [reinforcement learning](https://arxiv.org/abs/1803.05580) so varied walking behaviors can be learned on Cassie.


This tool was developed [Kevin Kellar](https://github.com/kkevlar) and with the mentorship of [Patrick Clary](https://github.com/pclary) for use within the [Dynamic Robotics Laboratory](http://mime.oregonstate.edu/research/drl/) at Oregon State University.

# Getting Started 

## Getting Started: Compilation / Dependencies

This tool was developed and tested on Ubuntu 18.

1. Clone
2. Add a [MuJoCo key](https://www.roboti.us/license.html) to the repository root directory and name it mjkey.txt
3. Install `libglfw3-dev` and `wget`
4. `make`
5. `./traj [-l loopcount#] [trajectory_filename]`

## Loopcount Flag

Showing more than one cycle of a trajectory may help the user edit more efficiently.

Loopcount = 1 | Loopcount = 2
--- | ---
`./traj` | `./traj -l 2`
![lc1](https://i.imgur.com/YWtxRSI.png) | ![lc2](https://i.imgur.com/UWZL2YB.png)

In both modes, there is **only one cycle of the trajectory when exported**.
Increasing loopcount above 1 can make editing more intuitive and natural.
However, there are a few remaining bugs ([#34](https://github.com/osudrl/cassie-trajectory-editor/issues/34) and [#35](https://github.com/osudrl/cassie-trajectory-editor/issues/35)) with the editing tools when the loopcount is greater than 1.

By **default**, loopcount is **set to 1**, where the tool's behavior is **stable**.
Yet for certain types of trajectories, the user may find that a multi-cycle view is more intuitive and easy to use, and can choose to edit while displaying trajectory cycles with the `-l` flag.

## Compilation: Troubleshooting

Error | Solution
--- | ---
"MuJoCo will need a product key to run the tool. Please provide a product key for MuJoCo and name it mjkey.txt." | The makefile will terminate compilation if mjkey.txt is not in the root repository directory. Please [provide this file](https://www.roboti.us/license.html).

# Controls


## "New" Commands

Input Command | Context | Effect
--- | --- | ---
Ctrl+Z | Any | Undo a trajectory modification. Will reset the timeline to its complete previous state. An 'Undo' can be redone with 'Redo'
Ctrl+Y or Ctrl+Shift+Z | Any | Redo a trajectory modification
Ctrl+P | Any | Load a perturbation from the configuration file at last.pert; In the current implementation, any perturbation with the mouse overwrites this file
Ctrl+R | Anytime Permitted | Refines a completed modification. Uses the list of frame/xpos target pairs saved after every call to `node_perform_pert()`, and will re-run IK on each target with a smaller error cutoff. Can be undone with the Undo command
Ctrl+E | Any | 'Expands' the current pose. The resulting timeline (undo-able) will have the same length and number of poses as the previous timeline, but will be filled with the current pose (except xpos[0], which is copied over)
Space | Any | Toggles pause for trajectory playback. When paused the camera may still be moved, bodies can be selected, nodes can be perturbed, IK will still be solved, Undo/Redo/LoadPert/Refine/Expand will all still work
Right Arrow | Any | Will step the trajectory time forward by some arbitrary step size. This step is in relation to visualization time, not number of frames although this may change in the future
Left Arrow | Any | Will step back by the step size
Up Arrow | Any | Will step forward by 10x the step size
Down Arrow | Any | Will step back by 10x the step size
Ctrl+Scroll | Not dragging a node | Works the same as Left/Right Arrows but for Up/Down scroll wheel
Ctrl+Scroll | Dragging a node | Scales the standard deviation of the trajectory perturbation. Scrolling up will increase the standard deviation and will smooth the perturbation over more nearby frames, while a scroll down will do the opposite
Ctrl+Shift+Scroll | Dragging a node | Scales the 'height' of the [Gaussian distribution](https://en.wikipedia.org/wiki/Normal_distribution) that is used to smooth perturbations. Height defaults at 1 and will not go lower than 1, but an increased height will cause nearby nodes to be transformed as much as the root node of the perturbation. Often used in combination with a 'target' perturbation command to hold the body in the same place for a number of frames


# Tool Source Documentation

## Important Data Structures

Each major data structure is explained below

Sub-Heading | Information
--- | ----
Memory Location | Where in memory does this data actually live? Stack frame, globals, heap, etc.
Setup | How and where is structure is initially set up
Usages | Relevant times the structure is used in functions
Fields | List of contained fields, ordered from most to least relevant

### traj_info_t ([Definition](https://github.com/osudrl/cassie-trajectory-editor/blob/master/src/main.h))

The traj_info struct initially was defined just to encapsulate the [mjModel\*](http://www.mujoco.org/book/reference.html#mjModel) / [mjData\*](http://www.mujoco.org/book/reference.html#mjData) references such that method calls had the ability to modify the robot's qpos values. Now, however, the struct has expanded to hold all the current runtime information about the tool, including the pose timeline and other common structures used by MuJoCo method calls.

#### Memory Location

The traj_info struct is allocated as a global [towards the top of simulate.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/simulate.c#L135:L152), and is passed to all functions as a pointer to this definition.

#### Setup

The struct is initially set up by [simulate.c : reset_traj_info()](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/simulate.c#L135:L152) which is called by [simulate.c : loadmodel()](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/simulate.c#L534:L584).

#### Usages

Nearly every function of the tool takes a traj_info reference because it allows access to the the list of timelines and allows helper functions to modify the model's joint angles, external forces, etc.

#### Fields

Type / Name | Description | Initial State | Used In
--- | --- | --- | ---
**[mjModel\*](http://www.mujoco.org/book/reference.html#mjModel)** m | Contains the information about the simulated Cassie model | Initialized in `reset_traj_info()` with the value set by `load_model()` | When making calls to MuJoCo functions such as `mj_foward()` and `mj_step()` 
**[mjData\*](http://www.mujoco.org/book/reference.html#mjData)** d | Contains runtime physics data, such as joint positions, forces, and velocities | Same as above | Same as above
**bool\*** paused | A reference to the paused variable in simulate.c's globals | Same as above | Used in [main_traj.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/main-traj.c#L49:L63) to calculate the runtime of the visualization
**[mjvPerturb\*](http://www.mujoco.org/book/reference.html#mjvPerturb)** pert | A reference to struct allocated in simulate.c's globals, containing data about the user dragging and dropping nodes | Initialized in `reset_traj_info()` | `main-traj.c : allow_node_transformations()` and some helper functions in node.c
**[timeline_t\*](https://github.com/osudrl/cassie-trajectory-editor#timeline_t-definition)** timeline | A struct listing each discrete pose throughout the step duration | Initialized in timeline.c | Most of the timeline.c functions use this field for setting / overwriting poses
**pdikdata_t** ik | A struct containing the parameters for solving IK using the [MuJoCo control function callback](http://www.mujoco.org/book/reference.html#mjcb_control) | In [ik.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/ik.c#L128:L133) | Used to control the pdik solver in [pdik.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/pdik.c#L21:L40)
**selection_t** selection | A struct containing the parameters corroponding to the selections and transformations of nodes and bodies | Initialized in `reset_traj_info()` | Used by the node.c transformation functions and `main-traj.c : allow_node_transformations()` 


### pdikdata_t ([Definition](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/pdik.h#L11:L24))


A struct containing the parameters for solving IK using the [MuJoCo control function callback](http://www.mujoco.org/book/reference.html#mjcb_control)


#### Memory Location


Stored within the traj_info struct.


#### Setup


Set up mostly in [ik.c](https://github.com/osudrl/cassie-trajectory-editor/blob/5373ee8e04b0bfeaffbbbfd6c14abcb2e2690360/src/ik.c#L128:L133) and a bit in [simulate.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/simulate.c#L146:L147)


#### Usages


Used in [pdik.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/pdik.c#L21:L40) to perform PD-based IK through the [MuJoCo control function callback](http://www.mujoco.org/book/reference.html#mjcb_control).


#### Fields


Type / Name | Description | Initial State | Used In
--- | --- | --- | ---
**int** body id | The body id on Cassie which is being manipulated | Set in ik.c using the function argument from the call node.c | Used in pdik.c, defines which body the pdik controller is applied
**double[3]** target body | The xyz coordinates which are the target for the IK to be solved | Same as above | Used in `apply_pd_controller()` in pdik.c as the target for the PD controller
**double** lowscore | The smallest "error" from the body's position to the target position so far | Initially set as a large value so that it is set to the actual error after a single step of pdik | Used in ik.c to decide when to stop iterating the IK solver
**double** pd_k | The 'spring constant', which is the coefficient for the P (positional) term for the pd controller | Set in [ik.c](https://github.com/osudrl/cassie-trajectory-editor/blob/5d9722b7fdfb91c40a43e6a97ea7320624ed869f/src/ik.c#L67:L82) | Used in `apply_pd_controller()` to scale the positional term
**double** pd_b | The 'damping constant', which is the coefficient for the D (derivative) term for the pd controller | Same as above | Used in `apply_pd_controller()` to scale the derivative term
**int32_t** max_doik | Controls the maximum number of steps for the IK solver before it will be forced to give up | Set in ik.c to the value of the [count parameter](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/ik.c#L128) which is passed by the call in [node.c](https://github.com/osudrl/cassie-trajectory-editor/blob/0dbf44c7536c35cd1c7d0dfab21b6e0a6ace8941/src/node.c#L89:L95) | Used to set the initial value of doik (see below)
**int32_t** doik | Controls the number of steps of IK that the solver will do. | Initially set to the value of max_doik in ik.c during the pdikdata struct setup | Used in the `pdik_per_step_control()` function, as it will only perform IK while this value is a positive number. Decrements this number every simulation step.

### selection_t ([Definition](https://github.com/osudrl/cassie-trajectory-editor/blob/master/src/main.h))

A struct containing the parameters corroponding to the selections and transformations of nodes and bodies.
Not really used apart from the traj_info struct, but logically groups the information.


#### Memory Location

Stored within the traj_info struct.


#### Setup

Setup in `reset_traj_info()` in simulate.c

#### Usages

Used primarily in node.c selection functions and in `main : allow_node_transformations()`.
In many modules, the macro `SEL` [provides an alias](https://github.com/osudrl/cassie-trajectory-editor/blob/23891bf8d50521b568a936f3c957935b47e269fe/src/node.c#L4) for accessing this structure.


#### Fields

Almost every field is initialized in `simulate.c : reset_traj_info()`, so that column was deleted.

Type / Name | Description | Usages
 --- | --- | ---
**int** id last body select | The most recent body (on cassie or node) that was selected with the mouse | `allow node transformations()` to determine selection / transformation / drop behavior
**int** id last non node select | The most recent body **on cassie** that was selected with the mouse | Same as above
**int** id last pert activenum | The most recent value of the boolean pert->active | `allow node transformations()` to determine if a node was dropped
**enum** node type | Sets the type of selection/nodes which appears when the user clicks a body | Primarily used in node.c functions like `node position initial using cassie body()` or in `allow node transformations()` to determine what node functions to call
**enum** pert type | Sets the type of perturbation filtering-- behavior of nearby nodes when a node is being currently transformed | Primary usage is to change behavior in `node calculate arbitrary target using transformation type()`
**int** nodecount | Total number of nodes displayed along the trajectory | Used by nearly all the functions which transform nodes. May still need a bug fix ([#4](https://github.com/osudrl/cassie-trajectory-editor/issues/4)) or may be changed completely ([#10](https://github.com/osudrl/cassie-trajectory-editor/issues/10)/[#1](https://github.com/osudrl/cassie-trajectory-editor/issues/1))
**double** nodesigma | The standard deviation of the Gaussian filtering used for transformations | Used in node functions which apply transformation or scale nodes visually while a node is being dragged
**double** nodeheight | Technically the height scaling of the Gaussian filtering | Used as an argument for `node calculate filter from frame offset()` to change the shape of the nearby nodes' transformations
~**int** jointnum~ | See [#25](https://github.com/osudrl/cassie-trajectory-editor/issues/25) ~The specific joint being transformed in the jointid/jointnum selection modes~ | ~Used in all the node jointmove functions to display / apply transformations for a specific joint~
**double[3]** localpos | A copy of the localpos vector in the [mjvPerturb struct](http://www.mujoco.org/book/reference.html#mjvPerturb), copied when the user has clicked on a Cassie body | Used to display nodes going through the selection point on the body in the jointid and jointnum selection types; we care about where the user clicked on the Cassie body, even when a node is selected and this pert->localpos value is overwritten by this new selection
**double[3]** joint move ref | Saves the mjvPerturb refpos while the nodes are being dragged | Saved in `node scale visually jointmove()` so that when the node is dropped, `node calculate jointdiff()` can know where the mouse was when the node was dropped (using the node's position is unsatisfactory because the nodes do not track directly with the mouse in this mode)




### timeline_t ([Definition](https://github.com/osudrl/cassie-trajectory-editor/blob/master/src/main.h))


#### Memory Location

Dynamically allocated on the heap with links constructing a doubly-linked list in both directions.
In a way, this structure is **doubly dynamically allocated** because both the `timeline_t` structure and the list of `qpos_t` structs (representing a single set of Cassie qposes) is also malloc'd based on the desired timeline size.


#### Setup

Initially malloc'd and initialzed through calls to `timeline update mj poses from realtime()`, which tests for a NULL timeline reference and then initializes the timeline with a call to `timeiline init from input file()`.
Other timeline instances are allocated through calls to `timeline duplicate()`, normally made by node.c functions such as `node perform pert()` or `node dropped jointmove()`.

#### Usages

Timeline references are passed frequently to any helper function getting or setting the robot's pose.
Anytime a pose is needed but a timeline is not a function parameter, the `traj_info->timeline` reference is assumed.


#### Fields

Type / Name | Description | Initial State | Used In
--- | --- | --- | ---
**int** numposes | The length of the qposes list (see below) | Initially set in `timeiline init from input file()` and copied over in calls to `timeline duplicate()` | Used whenever looping over every pos in the timeline
**timeline_t\*** prev / next | The previous / next timeline instances in the undo/redo list | These references are initially set to NULL by both `timeiline init from input file()` and copied over in calls to `timeline duplicate()` | function such as `node perform pert()` and `node dropped jointmove()` to chain old timelines to new 

## Node.c Module


The functions within the node module implements a few different types and specific vector math.
These functions are core the the functionality of the tool, but are long and unwieldy.

### Node Module Specific Types

**v3_t** is defined in node.h, and adds a bit more specificity than "double\*" when dealing with 3d vectors.
Although `v3_t` and `double\*` are literally interchangeable, this type should only be used when requiring a vector of length 3.
Functions with v3_t as a parameter will expect to be able to index the array at v[0], v[1], and v[2] and that these values should be the x, y and z values of the specified vector.

**cassie_body_t** and **node_body_t** do not provide any more information than an int.
Yet wrapping these ints in a struct provides strong type checking, preventing the functions which use these types from mistakenly interchanging these two different types.
Furthermore, the function prototypes in the header are able to clearly communicate what kind of body (cassie or node) is needed for the calculations.
To revert this type checking, all uses of these types can be replaced with unsigned ints, and the functions for wrapping the ids can be deleted.

### Function Reference

Function | Description | Changes to d->qpos | Changes to any Timeline
--- | --- | --- | --- 
node get frame from node body id() | Uses the node's index to calculate it's corresponding frame in the timeline | No | No
node calc frame lowhigh() | Calculates the first and last frame that the solver should plan to solve | No | No
node calclate global target using transformation type() | Acts as a sort of wrapper for `node calculate arbitrary target using scale type()`. Given a frame, fills the global target vector for that frame | **YES** | No
node calculate arbitrary target using scale type() | Scales a target vector given the initial perturbation and other scaling information | No | No
node calculate filter from frame offset() | Uses Gaussian filtering to return a scale factor given a frames a distance from the root of the perturbation | No | No
node calculate rootframe transformation vector() | Given a node that was dropped, calculate the root perturbation that should be applied to the trajectory | **YES**  | No
node caluclate jointdiff() | Returns a difference in joint value based on the Z-translation of the node | No | No
node compare looped filters() | Decides what rootframe and frame_offset should be used for scaling a perturbation when looping is enabled | No | No
node dropped jointmove() | Applies a jointmove transformation when a node is dropped | **YES\*** | **YES**
node dropped positional() | Applies a positional transformation when a node is dropped | **YES\*** | **YES**
node get body id from node index() | Returns a node_body_id given the index (starting at 0) of a node | No | No
node get body id from real body id() | Returns a node_body_id given the body id (starting at 26) of a node | No | No
node get body xpos by frame() | Returns a vector with the 3d coordinates of a body at the specified frame | **YES** | No
node get body xpos curr() | Returns a vector with the current xpos coordinates of body | No | No
node get cassie id from index() | Returns a cassie_body_id given the body id (starting at 0) of a Cassie body | No | No
node get qpos by node id() | Returns the vector of the 3d coordinates of a specified node | No | No
node perform ik on xpos transformation() | Calls the IK solver in ik.c for the given frame and target | **YES\*** | **YES**
node perform pert() | Performs a scaled, positional transformation | **YES\*** | **YES**
node position initial positional() | Moves the nodes based on the body's position throughout the timeline | **YES\*** | No
node position initial using cassie body() | Calls `node position initial positional()`, `node position jointmove()`, or `node position jointid()` based on the current tool | **YES\*\*** | No
node position jointid() | Displays extreme movement of a single joint for a single pose to allow the user to identify which joint is selected | No | No
node position jointmove() | Displays a transformation of a single joint, scaled across a number of frames | **YES\***  | No
node refine pert() | Refines the last perturbation to improve IK target accuracy (triggered by Ctrl+R) | **YES\*** | No
node scale visually jointmove() | Calls `node position jointmove()` after calculating a transformation | **YES\*\*** | No
node scale visually positional() | Shows a preview of the positional transformation while the user drags the nodes | **YES\*** | No

**YES\*:** These functions leave the qposes in **some intermediate state** which needs to **be overwritten** with their previous, cached values (as done in `node position initial using cassie body()`) or with the poses from the timeline with a call to `timeline update mj poses from realtime()`.


**YES\*\*:** These functions do modify the `d->qpos` values, but **reset the qposes** to their initial state before returning.



### node_get_body_id_from_node_index()

Definition:
```c
node_body_id_t node_get_body_id_from_node_index(int index);
```

Returns: a `node_body_id` type corresponding to the *index* provided

Assumptions: Acceptable node indecies are in the range [0,199]: at the moment, cassie.xml defines 200 node bodies

Changes to qposes: None

Changes to any timeline: None

### node_get_body_id_from_real_id()

Definition:
```c
node_body_id_t node_get_body_id_from_real_body_id(int real);
```

Returns: a `node_body_id` type corresponding to the *body id*

Assumptions: For the cassie model, cassie bodies are within the id range [1,25] so the valid real ids are within the range [26,224]

Changes to qposes: None

Changes to any timeline: None

### node_get_cassie_id_from_index()

Definition:
```c
cassie_body_id_t node_get_cassie_id_from_index(int i);
```

Returns: a `cassie_body_id_t` type corresponding to the *body id*

Assumptions: For the current cassie xml model, valid bodies range [1,25], where 1 is the pelvis and 25 is the right foot. Run `bash ./util/index-bodies.sh` for a list of bodies and associated ids.

Changes to qposes: None

Changes to any timeline: None

### node_get_qpos_by_node_id()

Definition:
```c
v3_t node_get_qpos_by_node_id(
	traj_info_t* traj_info, 
	node_body_id_t id);
```

Returns: a 3d vector used to get/set a node's position in the scene. Because nodes have mocap joints (not part of a physics chain), their qposes are used in the same way as body xposes.

Assumptions: The `node_body_id_t` was constructed meeting the above assumptions

Changes to qposes: None

Changes to any timeline: None

### node_get_body_xpos_curr()

Definition:
```c
v3_t node_get_body_xpos_curr(
	traj_info_t* traj_info, 
	cassie_body_id_t id);
```

Returns: a 3d vector of the specified body's 3d position
s
Assumptions: The `cassie_body_id_t` was constructed meeting the above assumptions

Changes to qposes: None

Changes to any timeline: None


### node_get_body_xpos_curr()

Definition:
```c
v3_t node_get_body_xpos_by_frame(
	traj_info_t* traj_info, 
	timeline_t* timeline, 
	int frame, 
	cassie_body_id_t id);
```

Returns: the body's xpos at the specified frame on the provided timeline

Parameters:

Type/Name | Description
--- | ---
timeline_t timeline | the timeline structure from which to read in the set of qposes
int frame | The frame (within the timeline) at which to set the qposes

Assumptions: The timeline reference is non NULL

Changes to qposes: **YES**, the current qposes are overwritten with the ones stored at the specified frame in the timeline struct

Changes to any timeline: None

### node_perform_ik_on_xpos_transformation()

Definition:
```c
void node_perform_ik_on_xpos_transformation(
    traj_info_t* traj_info, 
    timeline_t* overwrite,
    ik_solver_params_t* params,
    cassie_body_id_t body_id, 
    int frame, 
    int frameoffset, 
    v3_t target,
    double* ik_iter_total)
```

Given the cassie body and the target xpos for this body, this function calls the IK solver defined in ik.c and defines how many simulation cycles the solver is allowed to take. Increases the value pointed to by `ik_iter_total` by the number of simulation cycles used in solving IK.

Assumptions: The target vector and `ik_iter_total` references should all be non NULL.
Furthermore, the timeline reference should be non NULL and initialized properly.

Changes to qposes: **YES**, the current qposes will reflect the solution of the IK solver

Changes to any timeline: **YES,** the specified frame on the provided timeline will be overwritten with the solution

### node_calculate_arbitrary_target_using_scale_type()

Definition:
```c
void node_calculate_arbitrary_target_using_scale_type(
    traj_info_t* traj_info,
    double* final_curr,
    double* root_transformation,
    double* init_curr,
    double* init_root,
    int vector_size,
    double scalefactor)
```

Calculates a target (the vector can be an arbitrary dimension) using the vectors passed as parameters. 
The target will depend on the current scaling type, set by `traj_info->selection.scale_type`.

Parameters:

Name/Type | Description
--- | ---
(vector) final_curr | The vector which the result (the target) will be stored. Must be of length `vector_size`
(vector) root_transformation | The vector which describes how the "root" (often the node that was dragged and dropped) has moved in relation from its initial position at the root frame
(vector) init_curr | The initial position of the body at the frame for which the target will be calculated
(vector) init_root | The initial position of the body at the root frame before it was transformed
int vector_size | Although this function makes the most sense in 3d, its stages are illustrated below in 2d and is used for a 1d transformation by `node_position_joint_move()`. This parameter defines the number of components for each source and result vector.
double scale_factor | Also named `filter` in parts of the module, this value scales the full transformation down. This value is probably the result of the Gaussian distribution

Returns: The target vector in `final_curr`

Assumptions: 

* The vectors are non NULL and at least the length of `vector_size`
* `vector_size` > 0
* Scale factor is \(0,1\] - other values work, but do not make sense

Changes to qpos/timeline: No

Explaination:

The [scaling problem](https://github.com/osudrl/cassie-trajectory-editor/blob/master/WRITEUP.md#the-scaling-problem) discusses the need for A-Scaling and B-Scaling methods.
This function implements both the A-Scaling and B-Scaling methods.
The implementation is kept abstract so that vectors of any length can be scaled.
For positional transformations, `node calclate global target using transformation type()` calls the function with a vector size of 3.
For joint-only transformations, `node dropped jointmove()` calls the function with a vector size of 1.

<!---https://imgur.com/gallery/iLAihrA--->

### node_calclate_global_target_using_transformation_type()

Definition:

```c
void node_calclate_global_target_using_transformation_type(
    traj_info_t* traj_info,
    timeline_t* timeline,
    v3_t global_body_init_xpos_at_rootframe,
    v3_t global_body_target_xpos, 
    v3_t rootframe_transform_vector,
    int rootframe,
    int frame_offset,
    cassie_body_id_t body_id);
```

Parameters:

Name/Type | Description
--- | ---
(vector) global body init xpos at rootframe | The vector representing the initial position of the body at the root frame
(vector) global body target xpos | The **result** of the calculations from the function. Represents the target for the body at the specified frame 
(vector) rootframe transform vector | The translation that the root node experienced. For A-Scaling, all nearby nodes will undergo scaled-down transformations in the same direction
int rootframe | At what frame was the main perturbation applied? The rootframe can be found by finding node_body_id of the node that was dragged and dropped and using `get frame from node body id()`
int frame_offset | Defines how many frames away from the rootframe that the that the target is being polled. Add `frame_offset` to `rootframe` to get the absolute frame for which the target is being caluclated by calling this function
cassie body_id | The body that was perturbed

Assumptions:
* `traj_info` / `timeline` are non-NULL
* All the vectors are of length 3 and non-NULL
* Rootframe is within the bounds \[0,timeline->numposes\)
* Frame offset can be any integer- it will just wrap around the timeline

This function sets up the call to `node calculate arbitrary target using scale type()`. 
It should only be used when dealing with positional transformations (not joint transformations).
Called by `node perform pert()` and `node position scale visually()`.

Changes to qposes: **YES: qposes are overwritten** and set to timeline-> qposes at (`rootframe` + `frame_offset`).

Changes to timeline: No. The timeline is left unchaged.


### get_frame_from_node_body_id()

Definition:

```c
int get_frame_from_node_body_id(
	traj_info_t* traj_info, 
    timeline_t* timeline, 
    node_body_id_t node_id)
```

Assumptions: 
* `node_id` meets the above assumptions for a node body id

Changes to qposes: No

Changes to timeline: No


### node_calculate_rootframe_transformation_vector()

Definition:

```c
void node_calculate_rootframe_transformation_vector(
    traj_info_t* traj_info, 
    timeline_t* timeline,
    v3_t rootframe_transform_vector,
    cassie_body_id_t body_id, 
    node_body_id_t node_id)
```

Parameters:

Name/Type | Description
--- | ---
(vector) rootframe transform vector | Is the **result** of the function. Describes the transformation that the rootnode underwent
cassie body_id | The model body that the user clicked to cause the nodes to appear. This body will be perturbed
node node_id | The node body which the user dragged to cause this perturbation. The result (`rootframe transform vector`) is how far this node was dragged

<img align="right" src="https://user-images.githubusercontent.com/10334426/44414902-cc033200-a523-11e8-87e9-78f707596dc6.png" width="300">

Calculates the transformation that the root node underwent.
It subtracts the cassie body's initial position (partially transparent node, right) from the node's ending position (displaced opaque node, right). 

Assumptions:
* cassie and node body_ids meet the above assumptions
* the result vector is a non-NULL reference to a double[3]

Changes to qposes: **YES: qposes are overwritten** with the qposes from the timeline at the calculated rootframe (calculated using the node_body)

Changes to timeline: No

### node_perform_pert()

Definition:
```c
void node_perform_pert(
    traj_info_t* traj_info,
    ik_solver_params_t* params,
    v3_t rootframe_transform_vector,
    cassie_body_id_t body_id,
    int rootframe);
```

[#33](https://github.com/osudrl/cassie-trajectory-editor/pull/33) ([#29](https://github.com/osudrl/cassie-trajectory-editor/issues/29)) will introduce some discrepancies with the following general logic flow:

1. Duplicates the current (traj_info->timeline) timeline
2. Calculates IK target at rootframe
3. Performs PDIK at rootframe using the IK target
4. Calculate, scale, and use IK targets for nearby frames
5. New timeline becomes current timeline
6. Revisualize the nodes

Can perform a perturbtion with only the following information:

* What rootframe (time)
* Direction and magnitude of perturbation
* Body that was perturbed (foot,pelvis,etc)
* Scaling information:
  * Scaling type
  * Scaling std deviation
  * Scaling height
* Parameters for solving IK

Therefore, the user can load and apply perturbations from a file ([#9](https://github.com/osudrl/cassie-trajectory-editor/issues/9)).

Assumtions:

Non null references

Notes:

May fail if the perturbation is too large.
Upon failure, the function will notify the user via stdout and revert changes to the timeline.

Changes to qposes:

Qposes are overwritten, and the resulting qposes are **not defined**.
Instead, the the node module will leave it to the main module set qposes from the new timeline

Changes to timeline:

Yes.

### node_dropped_jointmove()

Definition:
```c
void node_dropped_jointmove(
    traj_info_t* traj_info,
    cassie_body_id_t body_id,
    node_body_id_t node_id)
```

Equivalent to `node perform pert()` but for jointmove perturbations not positional

### node_dropped_positional()

Definition:
```c
void node_dropped_positional(
    traj_info_t* traj_info,
    cassie_body_id_t body_id,
    node_body_id_t node_id);
```

Called by the main module.
Calculates and simplifies the call to `node perform pert()`

### node_position_jointmove()

Definition:

```c
void node_position_jointmove(
    traj_info_t* traj_info,
    cassie_body_id_t body_id,
    int rootframe,
    double jointdiff);
```

1. Sets qposes to rootframe, finds initial joint pos given the joinnum from the selection struct
2. Foeach frame
  * Sets qposes to that frame
  * Calculates filter
  * Sets the node to that body-relative position

Changes to qposes: Yes, qposes are overwritten and left in an intermediate state

Changes to timeline: No





# Contact


This tool and documentation page was written by [Kevin Kellar](https://github.com/kkevlar) for use within the [Dynamic Robotics Laboratory](http://mime.oregonstate.edu/research/drl/) at Oregon State University.
For issues, comments, or suggestions about the tool or its documentation, feel free to [contact me](https://github.com/kkevlar) or [open a GitHub issue](https://github.com/osudrl/cassie-trajectory-editor/issues?utf8=%E2%9C%93&q=label%3Adocs+).


