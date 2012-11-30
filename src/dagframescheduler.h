// The DAGFrameScheduler is a Multi-Threaded lock free and wait free scheduling library.
// © Copyright 2010 - 2012 BlackTopp Studios Inc.
/* This file is part of The DAGFrameScheduler.

    The DAGFrameScheduler is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The DAGFrameScheduler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The DAGFrameScheduler.  If not, see <http://www.gnu.org/licenses/>.
*/
/* The original authors have included a copy of the license specified above in the
   'doc' folder. See 'gpl.txt'
*/
/* We welcome the use of the DAGFrameScheduler to anyone, including companies who wish to
   Build professional software and charge for their product.

   However there are some practical restrictions, so if your project involves
   any of the following you should contact us and we will try to work something
   out:
    - DRM or Copy Protection of any kind(except Copyrights)
    - Software Patents You Do Not Wish to Freely License
    - Any Kind of Linking to Non-GPL licensed Works
    - Are Currently In Violation of Another Copyright Holder's GPL License
    - If You want to change our code and not add a few hundred MB of stuff to
        your distribution

   These and other limitations could cause serious legal problems if you ignore
   them, so it is best to simply contact us or the Free Software Foundation, if
   you have any questions.

   Joseph Toppi - toppij@gmail.com
   John Blackwood - makoenergy02@gmail.com
*/
#ifndef _DAGFrameScheduler_h
#define _DAGFrameScheduler_h

/// @file
/// @brief This is the file that code using this library should include. It includes all the required components.

#include "asynchronousworkunit.h"
#include "compilerthreadcompat.h"
#include "datatypes.h"
#include "doublebufferedresource.h"
#include "framescheduler.h"
#include "frameschedulerworkunits.h"
#include "monopoly.h"
#include "mutex.h"
#include "rollingaverage.h"
#include "systemcalls.h"
#include "thread.h"
#include "threadingenumerations.h"
#include "workunit.h"
#include "workunitkey.h"

/// @mainpage Directed Acyclic Graph Frame Scheduler.
/// @section goal_sec Goals
/// This library tries to make writing multithreaded software easier by changing the kinds of primitives that
/// multithreaded software is built upon. Several libraries before this have attempted this already.
/// This library is different becuse it focuses on a specific kind of workload and provides the kinds of
/// guarantees that workload needs while sacrificing other guarantees that workload does need.
/// @n @n
/// This attempts to provide a multithreading solution for workloads that must be run in many iterations in
/// a given amount of realtime. Games are an ideal example. Every frame a video game, must update physics
/// simulations, make AI decisions, accept/interpret user input, enforce game rules, perform dynamic I/O
/// and render it to the screen all while maintaining a smooth FrameRate and do that while minimizing drain
/// batteries on portable devices (sometimes without even knowing if the device is portable).
/// @n @n
/// This library accomplishes those goals by removing the conventional mutlithreading primitives that so many
/// developers have come to fear, loathe or misunderstand. Mutexes, threads, memory fences, thread_local
/// storage, atomic variables, and all the pitfalls that come with them are replaced by a small set of
/// of primitives that provide all the required sophistication a typical multi-threaded application
/// requires. It does this using a new kind of @ref Mezzanine::Threading::WorkUnit "WorkUnit",
/// @ref Mezzanine::Threading::DoubleBufferedResource "Double Buffering", A strong concept of
/// Dependencies and a @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" that uses heuristics
/// to decide how to run it all without exposing needless complexity to the application developer.
///
/// @section overview_sec Overview
/// The DAGFrameScheduler is a variation on a common multithreaded work queue. It seeks to avoid its pitfalls,
/// such as non-determinism, thread contention and lackluster scalability, while keeping its advantages
/// including simplicity, understandiblity and low overhead.
/// @n @n
/// With this algorithm very few if any
/// calls will need to be made to the underlying system for synchronization in the actual work to be performed.
/// Instead, this library will provide limited
/// deterministic ordering of @ref Mezzanine::Threading::WorkUnit "WorkUnit" execution through a dependency
/// feature. Having the knowledge that one @ref Mezzanine::Threading::WorkUnit "WorkUnit" will complete after
/// another allows for resources to be used without using expensive and complex synchronization mechansisms
/// like @ref Mezzanine::Threading::mutex "mutexes", semaphores, or even an
/// @ref Mezzanine::Threading::AtomicCompareAndSwap "Atomic Compare And Swap". These primitives are provided
/// to allow use of this library in advanced ways for developers who are already familiar with
/// multithreaded systems.
/// @n @n
/// The internal work queue is not changed while a frame is executing. Because it is only read, each
/// thread can pick its own work. Synchronization still needs to occur, but it has been moved onto each
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit" it is manages this with atomic CPU operations. Like this,
/// contention is less frequent, occurring only when threads simultaneously attempt to start the same
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit", and it consumes far less time because atomic operations
/// are CPU instructions instead of Operating System calls. This is managed by the library, so individual
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit"s do not need to worry synchronization beyond telling
/// each @ref Mezzanine::Threading::WorkUnit "WorkUnit" about its data dependencies and making sure
/// all the @ref Mezzanine::Threading::WorkUnit "WorkUnit"s added to a
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler".
///
/// @section broken_sec Broken Algorithms
/// To understand why a new multithreading system is needed, it is helpful to look at other methods
/// of threading that have been used in the past, and understand what they lack or how they aren't ideal
/// for the kinds of work this algorithm is is intended for.
/// @n @n
/// I will use charts that plot possible resource use of a computer across time. Generally time will
/// run accross the top a resources, usually CPUs will run down one side.
/// @n @n
/// These threading models aren't necessarily broken, some of these clearly have a place in software
/// development. None of these are ideal for video games or other tasks that have realtime recurring execution
/// requirements. Many of these require complex algorithms, require subtle knowledge or simply aren't
/// performant enough for realtime environments.
/// @subsection broken_Single Single Threaded
/// An application using this threading model is not actually multithreaded at all. However, It has been shown
/// that software can run in a single and get good perfomance. This is benchmark all other threading models
/// get compared too.
/// @n @n
/// There is a term, Speedup ( http://en.wikipedia.org/wiki/Speedup ), which is simply a
/// comparison of the single threaded performance of an algorithm to the mutlithreaded performance. You simply
/// determine how many times more work the multithreaded algorithm does in the same time, or how many times
/// longer the single threaded algorithm takes to the same work. Ideally two threads will be twice as fast
/// (speedup of 2x), and three thread would be three times as fast (3x speedup), and so; this is called linear
/// speedup. In practice there is always some overhead in creating and synchronizing threads, so achieving
/// linear speedup is diffucult.
/// @image html Single.png "Single Threaded Execution - Fig 1."
/// @image latex Single.png "Single Threaded Execution - Fig 1."
/// @image rtf Single.png "Single Threaded Execution - Fig 1."
/// @n @n The DAGFrameScheduler library tries to tailor the threading model to the problem to minimize that
/// overhead. With a single threaded application one thread does all the work and always wastes every other
/// thread, but there is no overhead
/// @n @n
/// @subsection broken_Unplanned Unplanned Thread
/// Sometimes someone means well and tries to increase the performance of a single threaded program and tries
/// to add extra threads to increase performance. Sometimes this works, really well, sometimes there is a
/// marginal increase in performance or a significant increase in bugs. If that someone has a good plan
/// then they can usually achieve close to the best speedup possible in the given situation. This is not easy
/// and many cannot do this or do not want to invest the time it would take. If not carefully planned
/// bugs like deadlock ( http://en.wikipedia.org/wiki/Deadlock ) and race conditions
/// ( http://stackoverflow.com/questions/34510/what-is-a-race-condition )
/// can be introduced. Unfortunately no amount of testing can replace this careful planning. Without a
/// complete understanding of how multithreaded software is assembled (a plan) it is not possible to prove
/// that multithreaded software will not hang/freeze or that it will produce the correct results.
/// @n @n
/// Software with no multithreading plan could have just about any kind of execution behavior. Usually
/// unplanned software performs at least slightly better than single threaded versions of the software, but
/// frequently does not utilize all the available resources. Generally performance does not scale well as
/// unplanned software is run on more processors. Frequently, there is contention for a specific resource and
/// a thread will wait for that resource longer than is actually need.
/// @image html Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @image latex Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @image rtf Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @n @n
/// The DAGFrameScheduler is carefully planned and completely avoids costly synchronization
/// mechanisms in favor of less costly minimalistic ones. Marking one @ref Mezzanine::Threading::WorkUnit "WorkUnit"
/// as dependent on another allows the reordering of @ref Mezzanine::Threading::WorkUnit "WorkUnits" so that
/// some @ref Mezzanine::Threading::WorkUnit "WorkUnit" can be executed with no thread waiting or blocking.
/// @n @n
/// @subsubsection broken_TaskPerThread One Task Per Thread
/// A common example of poor planning is the creation of one thread for each task in a game. Despite
/// being conceptually simple, performance of systems designed this was is poor due to synchronization
/// and complexities that synchronization requires.
/// @subsection broken_ConventionWorkQueue Convention Work Queue/Thread Pools
/// Conventional work queues and thread pools are well known and robust way to increase the throughput of
/// of an application. These are ideal solutions for many systems, but not games.
/// @n @n
/// In conventional workqueues all of the work is broken into a number of small thread-safe
/// units. As these units are created they are stuffed into a queue and threads pull out units of work
/// as it completes other units it has started. This simple plan has many advantages. If there is work
/// to do, then at least one thread will be doing some, and usually more threads will be working; this is
/// good for games and the DAGFrameScheduler mimics it. If the kind of work is unknown when the software is
/// written heuristics and runtime decisions can create the kind of units of work that are required. This
/// is not the case with games and the others kinds of software this library caters to, so changes can
/// be made that remove the problems this causes. One such drawback is that a given unit of work never
/// knows if another is running or has completed, and must therefor make some pessimistic assumptions.
/// @image html Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @image latex Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @image rtf Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @n @n
/// Common synchronization mechanisms like mutexes or semaphores block the thread for an unknown
/// amount of time, and are required by the design of workqueues. There are two times this is required.
/// The first time is whenever a work unit is acquired by a thread, a mutex (or similar) must be used
/// to prevent other threads from modifying the queue as well. This impacts scalability, but can be
/// circumvented to a point. Common ways to work around this try to split up the work queue
/// pre-emptively, or feed the threads work units from varying points in the queue. The
/// DAGFrameScheduler moves the synchronizantion onto each work to greatly reduce the contention as
/// more workunits are added.
/// @n @n
/// The other, and less obvious, point of contention that has not be circumvented in a
/// satisfactory way for games is the large of amount of synchronization required between units of
/// work that must communicate. For example, there may be hundreds of thousands of pieces of data
/// that must be passed from a system into a 3d rendering system. Apply mutexes to each would slow
/// execution an impossibly long time (if it didn't introduce deadlock), while more coarse grained
/// lock would prevent large portions of physics and rendering from occurring at the time causing
/// one or both of them to wait/block. A simple solution would be to run physics before graphics,
/// but common work queues do not provide good guarantees in this regard.
/// @n @n
/// The DAGFrameScheduler was explicitly designed to provide exactly this guarantee. If the
/// physics @ref Mezzanine::Threading::WorkUnit "WorkUnit" is added to the graphics
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit" with
/// @ref Mezzanine::Threading::WorkUnit::AddDependency() "AddDependency(WorkUnit*)" then it will
/// always be run before the graphics workunit in a given frame. The drawback of this is that it
/// is more difficult to make runtime creation of workunits (It is possible but it cannot be done
/// during any frame execution), but completely removes the locking
/// mechanisms a conventional work queues. The DAGFrameScheduler has traded one useless feature
/// for a useful guarantee.
///
/// @section algorithm_sec The Algorithm
/// When first creating the DAGFrameScheduler it was called it "Dagma-CP" because when describing it
/// the phrase "Directed Acyclic Graph Minimal Assembly of Critical Path" if you are in the lucky 1%
/// who knows what all those terms mean they are very descriptive. For rest of us the algorithm tries
/// to determine what is the shortest way to execute the work that must be executed each frame. It
/// does this by assembling a logical graph of work that must done each frame and executing it.
/// Because all the entries in this will have a definite location somewhere between the beginning
/// and end, and will never circle around back to an earlier point this is called an acyclic graph.
/// @n @n
/// For scheduling concerns, there are 3 kinds of @ref Mezzanine::Threading::WorkUnit "WorkUnit"s.
/// All @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit"s are expected to monopolize cpu
/// resources at the beginning of each frame. This is ideal when working with other systems, for
/// example a phsyics system like Bullet3D. If the calls to a physics system are wrapped in a
/// @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit" then it will be given full
/// opportunity to run before the @ref Mezzanine::Threading::WorkUnit "WorkUnit"s and
/// @ref Mezzanine::Threading::AsynchronousWorkUnit "AsynchronousWorkUnit"s are run.
/// @warning AsynchronousWorkUnit and Work Unit affinity are not completely implemented at this point
/// in time. The automatic thread adjusting hueristic is also not complete.
///
/// Once all the @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit"s are done then the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" class instance spawns or activates
/// a number of threads based on a simple heuristic. Each thread queries the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" for the next piece of work that has
/// the most @ref Mezzanine::Threading::WorkUnit "WorkUnit"s that depend on it, and in the case of
/// a tie the @ref Mezzanine::Threading::WorkUnit "WorkUnit" that takes the longest to execute.
/// Execution length rather than brevity is preferred because it helps keep each thread's execution
/// time consistently short (I will add a few more pictures to describe this clearly).
/// @n @n
/// Some work must be run on specific threads, such as calls to underlying devices (for example,
/// graphics cards using Directx or OpenGL). These @ref Mezzanine::Threading::WorkUnit "WorkUnit"s
/// are put into a different listing where only the main thread will attempt to execute them. Other
/// than running these, and running these first, the behavior of the main thread is very similar to
/// other threads. Once a @ref Mezzanine::Threading::WorkUnit "WorkUnit" has been completed the
/// thread will query the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" for more work.
/// Because the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" is never modified during
/// a frame there is no need for synchronization with it specifically, this avoids a key point of
/// contention that reduces scaling. Instead the synchronization is performed with each
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit" and is an
/// @ref Mezzanine::Threading::AtomicCompareAndSwap "Atomic Compare And Swap" operation to maximize
/// performance.
/// @n @n
/// Even much of the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler"'s work is performed
/// in @ref Mezzanine::Threading::WorkUnit "WorkUnit"s, such as log aggregation and certain functions
/// that must be performed each frame.
/// @ref Mezzanine::Threading::AsynchronousWorkUnit "AsynchronousWorkUnit"s continue to run in a thread
/// beyond normal scheduling and are intended to will consume fewer CPU resources and more IO resources.
/// For example loading a large file or listening for network traffic. These will be normal
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit"s in most regards and will check on the asynchronous
/// tasks they manage each frame when they run as a normally scheduled.
/// @n @n
/// If a thread should run out of work because all the work is completed the frame will pause until it
/// should start the next frame. This pause length is calulated using a runtime configurable value on
/// the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler". If a thread has checked every
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit" and some are still not executing, but could not
/// be started because of incomplete dependencies the thread will simply iterate over every
/// @ref Mezzanine::Threading::WorkUnit "WorkUnit" in the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" until the dependencies of one are
/// met and allows one to be executed. This implicitly guarantees that at least one thread will
/// always do work, and if dependencies chains are kept short then it is more likely that several
/// threads will advance.
/// @n @n
/// The @ref Mezzanine::Threading::WorkUnit "WorkUnit" classes are designed to be inherited from
/// and inserted into a @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" which will
/// manage their lifetime and execute them when requested via
/// @ref Mezzanine::Threading::FrameScheduler::DoOneFrame() "FrameScheduler::DoOneFrame()".
/// @n @n
/// Insert DAGFramescheduler picture here.
/// @n @n
/// This documentation should not be considered complete nor should the algorithm
/// both are still under development.




/// @brief All of the Mezzanine game library components reside in this namespace.
/// @details The DAG Frame Scheduler is just one part of many in the Mezzanine. The Mezzanine as a
/// whole is intended to tie a complex collection of libraries into one cohesive library.
namespace Mezzanine
{
    /// @brief This is where game specific threading algorithms and a minimalistic subset of the std threading library a held
    /// @details this implements All of the Multithreaded Algorithms from BlackTopp Studios, parts of std::thread,
    /// std::this_thread, std:: mutex, and maybe a few others. In general only the specialized gaming algorithms store here are
    /// intended to be used in game code.
    namespace Threading
        {}
}

#endif
