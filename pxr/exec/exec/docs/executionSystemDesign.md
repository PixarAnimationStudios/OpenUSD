# OpenExec System Design {#page_Execution_System_Design}

OpenExec is a general-purpose computation engine primarily used in rigging, 
animation, layout, crowds, shading, lighting, and some simulation workflows. 
In the common case, given a set of authored animation controls as input, the 
execution system compiles an **execution network** derived from a rig composed 
of scene objects to evaluate computations. The process of 
[compilation](#Mf_Execution_System_Design_Compilation) of the execution 
network involves interpreting the authored scene description in combination with 
registered definitions that define 
[computations](#Mf_Execution_System_Design_Computations). Common computations 
include: computing posed point positions, object visibility, and bounding boxes. 
At its most basic representation, the execution network is a vectorized data 
flow network, where many uniquely identifiable elements may flow along a single 
connection. This architecture lends itself to opportunities for parallelism. 

Guiding principles in the design of the execution system include:
- Fast evaluation for interactive use cases (e.g. munging an avar), enabling
  users to iterate quickly.
- Reliable cached computation, which facilitates efficient multithreading.
- Abstraction of execution representation away from authoring intent. This is
  an important separation of concerns that allows targeted optimization of the
  execution system to be developed without obfuscating the simplicity and
  editing ease of the user interface.

## Prelude: Object Model
It is useful to contextualize this discussion of the design of the execution 
system relative to the primitive objects that make up an interactive scene.  
Any object that can hold a value is an **attribute**, which can be assigned a 
value by the user, modified by computations, and/or be used to extract computed 
values. Attributes are the inputs and outputs of execution. Attributes are 
owned by high-level containers called **prims**. Users express relationships 
and data flow between the prims in the scene and their associated attributes, 
which can be consumed by **computations** to ascribe executional behaviors to 
objects in the scene. Extending prims and computations is the primary mechanism 
for clients to evolve the capabilities of the interactive environment. 

## Computations {#Mf_Execution_System_Design_Computations}
OpenExec provides some 
[built-in computations](#group_Exec_Builtin_Computations), such as the default 
'computeValue' computation to request the computed value of an attribute. 
[Plugin computations](#group_Exec_ComputationDefinitionLanguage) can also be 
published by schemas to define the set of supported behaviors.

> **Note:** Computations are implemented via stateless callbacks, and a
> fundamental requirement is that all inputs to the callback must be
> explicitly supplied as a part of the computation's definition. The stateless 
> nature of the callbacks captured by the execution network enables important 
> optimizations, discussed in the section on the 
> [execution network](#Mf_Execution_System_Design_Network).

## Phases of Execution
Execution is broken up into three separate phases, where each phase amortizes
costs for the following phase. In order of execution, as well as increasing
frequency and decreasing runtime cost, the phases are:
1. [Compilation](#Mf_Execution_System_Design_Compilation)
2. [Scheduling](#Mf_Execution_System_Design_Scheduling)
3. [Evaluation](#Mf_Execution_System_Design_Evaluation)

### Compilation {#Mf_Execution_System_Design_Compilation}
Compilation is responsible for converting scene objects into optimized data
structures that can be quickly and repeatedly evaluated and incrementally
updated in response to user interaction. This phase of execution introduces a
layer of abstraction that allows us to separate a lightweight network 
representation, which is optimized for fast repeated evaluation, from the 
expressive scene description required for authoring workflows. A primary 
benefit of compiling an execution network is that data flow in the 
network can be vectorized, which has significant payoffs in performance -- in 
part due to improved memory access patterns. The product of compilation is a 
vectorized data flow network composed of nodes and connections between them to 
describe the structure of computations. 

Note that compilation is typically the most expensive phase of execution, but 
full compilation happens relatively infrequently in most workflows. After 
initial compilation, topological changes are incrementally applied to the 
network. **Recompilation** ensures that only pertinent subsections of the 
network are updated in response to authoring while maintaining the fidelity and 
responsiveness of the scene. Changes that do not result in structural changes 
to the compiled network, e.g. attribute value changes, do not trigger 
recompilation; these non-structural changes only require value initialization 
prior to evaluation. 

Effectively, the language expressing execution behaviors via plugin and 
builtin computations and scene description comprise a complex instruction set 
that gets mapped to a much reduced instruction set, the data flow network, by 
the process of compilation.  

### Scheduling {#Mf_Execution_System_Design_Scheduling}
Scheduling is the next phase of execution, which uses the compiled network and 
the client's **execution request** consisting of a set of desired outputs (e.g. 
the posed point positions of a character). A common client is a viewport, which 
generates a request for all the visible objects in the scene. Execution uses 
the client's request to build a schedule that encodes the nodes in the network 
that need to run in order to compute the requested values. This schedule is an 
acceleration structure that serves to mitigate the costs of traversing the  
network. It is especially beneficial for performance because it amortizes the 
cost of dependency analysis that would otherwise have to be incurred during 
each network evaluation. Schedules also impose guidelines on memory access, by 
guaranteeing that data-flow nodes are visited no more than once during a round 
of evaluation and that data is accessed contiguously. Scheduling is performed 
more frequently than compilation but not as often as evaluation in many common 
workflows. A schedule must be generated for each new set of requested outputs, 
and it is invalidated and re-generated in response to changes in network 
topology.

### Evaluation {#Mf_Execution_System_Design_Evaluation}
The final and most frequently performed phase of execution is evaluation, 
which is responsible for running the nodes in the network according to the 
schedule to produce values for requested outputs. This is the stage that 
invokes the callback associated with each pertinent node. Evaluation must occur 
any time an input value changes or a new output is requested. Note that 
evaluation only occurs for nodes appearing in the schedule generated for the 
set of requested outputs. This pull-based architecture is well-suited to 
caching and invalidation optimizations. Evaluation begins at the leaves of the 
network (representing the final computed values of requested outputs) and 
traverses data dependencies to the root of the network, truncating once any 
node returns a cache hit. Then, it unwinds the stack of nodes discovered along 
the traversal and invokes their callbacks, beginning with the root and any 
nodes with fully cached input dependencies. The parallel executor, enabled by 
default in OpenExec, evaluates nodes in parallel, only requiring 
synchronization on their input dependencies. Multithreaded evaluation can be 
disabled via the env setting: 

    os.environ['PRESTO_MULTITHREADED_MUNGING'] = '0'

## Engine Architecture
The execution engine is composed of a network, scheduler, data managers, and 
executors, which facilitate efficient multithreading and separation of concerns.  

![Execution Engine](images/executionSystemDesignEngineArchitecture.drawio.svg 
"Execution Engine Architecture")

### Network {#Mf_Execution_System_Design_Network}
The execution network is generated from authored scene objects to statically 
represent computations and the dependencies among them. The network consists of 
nodes and connections between them. For example, an EfLeafNode is a simple type 
of node with no outputs that functions purely as a recipient of **invalidation**,
which is the mechanism for tracking and broadcasting changes to attribute values. 
Clients request values of computations via an execution request consisting of 
desired leaf node outputs.

### Schedulers
The scheduler is responsible for the scheduling phase of evaluation, producing 
schedules that are used by executors for repeated evaluations of the network. 
Clients are not able to inspect the schedule; rather it is returned as an 
opaque object when a client makes a request (e.g. by opening a viewport) and 
consumed by the execution system in subsequent rounds of evaluation. 
Scheduling-level optimizations analyze the list of nodes that need to run 
(often an expensive operation) to provide information that executors can take 
advantage of, e.g. task dependencies that inform multithreaded evaluation.

### Data Managers
Data managers are responsible for storing the computed data that corresponds 
to nodes in the network. Each executor has a single data manager that 
encapsulates the computed values for all the nodes in the network. In its 
simplest representation, the data manager is a map from network output to 
computed value. Data managers also store validity masks that track the dirty 
state of outputs, which drives invalidation in response to network changes. 
Data managers provide a location to store any per-node or per-output 
information that is required by the executor. 

### Executors
Given a compiled network and a schedule representing a set of requested 
outputs, executors perform the final stage of evaluation, querying and storing 
computed results in their associated data manager. This is the inner loop of 
execution, which needs to run as efficiently as possible. A useful 
characteristic of executors is that they can be arranged in a hierarchy, where 
the child is able to read from (and in some cases, even write to) its parent's 
computed results in order to avoid redundant computation. Specialization 
of executors enables experimentation with different ways to evaluate the 
network. For example, VdfParallelExecutorEngineBase implements multithreaded 
evaluation of the network.