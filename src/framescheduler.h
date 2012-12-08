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
#ifndef _framescheduler_h
#define _framescheduler_h

#include "datatypes.h"
#include "doublebufferedresource.h"
#include "thread.h"
#include "workunitkey.h"
#include "systemcalls.h"

#include <map>
#include <set>
#include <fstream>

/// @file
/// @brief This file has the Declarations for the main FrameScheduler class.

namespace Mezzanine
{
    namespace Threading
    {
        class MonopolyWorkUnit;
        class iWorkUnit;
        class LogAggregator;
        class LogBufferSwapper;
        class FrameScheduler;

        /// @brief This is central object in this algorithm, it is responsible for spawning threads and managing the order that work units are executed.
        /// @details For a detailed description of the @ref algorithm_sec "Algorithm" this implements see the @ref algorithm_sec "Algorithm" section on
        /// the Main page.
        class MEZZ_LIB FrameScheduler
        {
            friend class LogAggregator;
            friend class LogBufferSwapper;

            protected:
                /// @brief A collection of all the work units that are not Monopolies and do not have affinity for a given thread.
                /// @details This stores a sorted listing(currently a vector) of @ref Mezzanine::Threading::WorkUnitKey "WorkUnitKey" instances.
                /// These include just the metadata required for sorting @ref Mezzanine::Threading::WorkUnit "WorkUnit"s. Higher priority
                /// @ref Mezzanine::Threading::WorkUnit "WorkUnit"s are higher/later in the collection. This is list is sorted by calls
                /// to @ref Mezzanine::Threading::FrameScheduler::SortWorkUnitsMain "SortWorkUnitsMain" or @ref Mezzanine::Threading::FrameScheduler::SortWorkUnitsAll "SortWorkUnitsAll".
                std::vector<WorkUnitKey> WorkUnitsMain;

                /// @brief A collection of @ref Mezzanine::Threading::WorkUnit "WorkUnit"s that must be run on the main thread.
                /// @details This is very similar to @ref WorkUnitsMain except that the @ref Mezzanine::Threading::WorkUnit "WorkUnit"s
                /// are only run in the main thread and are sorted by calls to @ref SortWorkUnitsAll or @ref SortWorkUnitsAffinity .
                std::vector<WorkUnitKey> WorkUnitsAffinity;

                /// @brief A structure designed to minimalistically reprsent Dependency and Reverse Dependency Graphs in work units
                typedef std::map<
                                iWorkUnit*,
                                std::set<iWorkUnit*>
                            > DependentGraphType;

                /// @brief This structure allows reverse lookup of dependencies.
                /// @details This is is a key part of the workunit sorting algorithm. This is calculated
                /// @warning
                DependentGraphType DependentGraph;

                /// @brief This maintains ownership of all the thread specific resources.
                /// @note There should be the same amount of more these than entries in the Threads vector.
                std::vector<DefaultThreadSpecificStorage::Type*> Resources;

                /// @brief A way to track an arbitrary number of threads.
                /// @note There should never be more of these than Resources, and if there are more at the beginning of a frame the resources will be created in CreateThreads().
                std::vector<thread*> Threads;

                /// @brief A collection of all the monopolies this scheduler must run and keep ownership of.
                std::vector<MonopolyWorkUnit*> Monopolies;

                /// @brief When the logs are aggregated, this is where they are sent
                std::ostream* LogDestination;

                /// @brief What time did the current Frame Start at.
                MaxInt CurrentFrameStart;

                /// @brief How many threads with this try to execute with in the next frame.
                Whole CurrentThreadCount;

                /// @brief Used to store a count of frames from the begining of game execution.
                Whole FrameCount;

                /// @brief The Maximum frame rate this algorithm should run at.
                Whole TargetFrameLength;

                /// @brief To prevent frame time drift this many microseconds is subtracted from the wait period to allow time for calculations.
                Integer TimingCostAllowance;

                /// @brief Set based on which constructor is called, and only used during destruction.
                bool LoggingToAnOwnedFileStream;

                /// @brief Get the endpoint for the logs.
                /// @return An std:ostream reference which can be streame to commit log entries.
                std::ostream& GetLog();

                /// @brief Create more threads if they are required.
                void CreateThreads();

                /// @brief Used when completing the work of a frame, to cleaning end the execution of the threads.
                void JoinAllThreads();

                /// @brief Simply iterates over and deletes everything in Threads.
                void DeleteThreads();

                /// @brief Wait until enough time has elapsed to allow each frame an equal amount of time in one second if as many frames as the target framerate are run.
                /// @details Wait 1/TargetFrame Seconds, minus time already run.
                void WaitUntilNextThread();

                /// @brief
                void UpdateDependentGraph(const std::vector<WorkUnitKey> &Units);
                void UpdateWorkUnitKeys(std::vector<WorkUnitKey> &Units);

                //std::vector<WorkUnit*> WorkUnitsWithAffinity;
                //virtual void StartEachThread();
                //virtual void JoinWithEachThread();
                //virtual bool CheckWorkIsComplete();
                // some way to expiriment with thread size.

           public:
                /// @brief Create a Framescheduler, that owns a filestream for logging.
                /// @param _LogDestination An fstream that will be closed and deleted when this framescheduler is destroyed. Defaults to a new Filestream Logging to 'Log.txt'.
                /// @param StartingThreadCount How many threadsDefaults to 1.
                /// @warning This must be constructed from the Main(only) thread for any features with thread affinity to work correctly.
                FrameScheduler(
                        std::fstream *_LogDestination = new std::fstream("Log.txt", std::ios::out | std::ios::app),
                        Whole StartingThreadCount = GetCPUCount()
                    );


                /// @brief Create a Framescheduler, that logs to an unowned stream.
                /// @param _LogDestination Any stream, other than an fstream, and it will be closed (not deleted) when this frame scheduler is destroyed.
                /// @param StartingThreadCount How many threadsDefaults to 1.
                /// @warning This must be constructed from the Main(only) thread for any features with thread affinity to work correctly.
                FrameScheduler(
                        std::ostream* _LogDestination,
                        Whole StartingThreadCount = GetCPUCount()
                    );

                /// @brief Destructor
                /// @details Deletes all std::fstream, WorkUnit, MonopolyWorkUnit and ThreadSpecificStorage objects that this was passed or created during its lifetime.
                virtual ~FrameScheduler();

                //ThreadSpecificStorage& GetThreadSpecificStorage(thread::id ThreadID);
                //ThreadSpecificStorage& GetThisThreadsSpecificStorage();

                /// @brief Add a normal WorkUnit to this For scheduling.
                /// @param MoreWork A pointer the the WorkUnit, that the FrameScheduler will take ownership of, and schedule for work.
                virtual void AddWorkUnit(iWorkUnit* MoreWork);

                /// @brief Add a normal WorkUnit to this For scheduling.
                /// @param MoreWork A pointer the the WorkUnit, that the FrameScheduler will take ownership of, and schedule for work.
                virtual void AddWorkUnitAffinity(iWorkUnit* MoreWork);

                /// @brief Remove a WorkUnit, and regain ownership of it
                /// @param LessWork a pointer to a WorkUnit that should no longer be scheduled.
                /// @details This is relative slow compared to adding or finding a working unit, this works in linear time relative to the number of WorkUnits in the scheduler.
                /// @warning This does not cleanup dependencies and can get you in trouble if other work units depend on the one removed
                virtual void RemoveWorkUnit(iWorkUnit* LessWork);

                /// @brief How many other WorkUnit instances must wait on this one.
                /// @param Work The WorkUnit to get the Updated count of.
                /// @param UsedCached If the cache is already up to date leaving this false, and not updating it can save significant time.
                /// @return A Whole Number representing the amount of WorkUnit instances that cannot start until this finishes.
                virtual Whole GetDependentCountOf(iWorkUnit *Work, bool UsedCached=false);

                /// @brief Gets the next available workunit for execution.
                /// @details This finds the next available WorkUnit which has not started execution, has no dependencies that have
                /// not complete, has the most WorkUnits Units that depend on it (has the highest runtime in the case of a tie).
                /// @return A pointer to the WorkUnit that could be executed or a null pointer if that could not be acquired. This does not give ownership of that WorkUnit.
                virtual iWorkUnit* GetNextWorkUnit();

                /// @brief Just like @ref GetNextWorkUnit except that it searchs through and prioritizes work units with affinity.
                /// @return A pointer to the WorkUnit that could be executed *in the main thread* or a null pointer if that could not be acquired. This does not give ownership of that WorkUnit.
                virtual iWorkUnit* GetNextWorkUnitAffinity();

                /// @brief Is the work of the Frame Done?
                /// @return This returns true if all the WorkUnit instances are complete, and false otherwise.
                virtual bool AreAllWorkUnitsComplete();

                /// @brief Get the amount of threads that will be used to execute WorkUnits a the start of the next frame.
                /// @return A Whole with the current desired thread count.
                virtual Whole GetThreadCount();

                /// @brief Set the amount of thread to use.
                /// @param NewThreadCount The amount of threads to use starting at the begining of the next frame.
                virtual void SetThreadCount(Whole NewThreadCount);

                /// @brief Do one frame worth of work.
                /// @details Every Monopoly will be executed once and each work unit will be executed once.
                /// @warning Do not call this on an unsorted set of WorkUnits. Use @ref FrameScheduler::SortWorkUnitsAll() and @ref DependentGraph to sort WorkUnits after
                /// They are inserted into the frame scheduler for the first time. This doesn't need to happen each frame, just the frames
                /// new WorkUnits are added.
                virtual void DoOneFrame();

                /// @brief Take any steps required to prepare all owned WorkUnits for execution next frame.
                virtual void ResetAllWorkUnits();

                /// @brief Sort the the main pool of WorkUnits to allow them to be used more efficiently in the next frame executed.
                /// @param UpdateDependentGraph_ Should the internal cache of reverse dependents be updated.
                /// @details See @ref Mezzanine::Threading::FrameScheduler::DependentGraph "DependentGraph"
                /// for the appropriate times to use this.
                void SortWorkUnitsMain(bool UpdateDependentGraph_ = true);

                /// @brief Sort the WorkUnits that must run on the main thread to allow them to be used more efficiently in the next frame executed.
                /// @param UpdateDependentGraph_ Should the internal cache of reverse dependents be updated.
                /// @details See @ref Mezzanine::Threading::FrameScheduler::DependentGraph "DependentGraph"
                /// for the appropriate times to use this.
                void SortWorkUnitsAffinity(bool UpdateDependentGraph_ = true);

                /// @brief Sort all the WorkUnits that must run on the main thread to allow them to be used more efficiently in the next frame executed.
                /// @param UpdateDependentGraph_ Should the internal cache of reverse dependents be updated.
                /// @details See @ref Mezzanine::Threading::FrameScheduler::DependentGraph "DependentGraph"
                /// for the appropriate times to use this.
                void SortWorkUnitsAll(bool UpdateDependentGraph_ = true);

                // advanced is done by default when sorting the WorkUnits
                void UpdateDependentGraph();

                /// @brief Get the current number of frames that have elapsed
                /// @return A Whole containing the Frame Count.
                Whole GetFrameCount() const;

                /// @brief Get the desired length of a a frame.
                Whole GetFrameLength() const;

                /// @brief Set the desired Frate rate.
                /// @param FrameRate in frames per second
                /// @details Defaults to 60, to maximize smoothmess of execution (no human can see that fast),
                /// while not killing battery life. This is a maximum framerate, if WorkUnits take too long
                /// to execute this will not make them finish faster. This controls a delay to prevent the
                /// machine's resources from being completely tapped. @n @n Set this to 0 to never pause
                /// and run as fast as possible.
                /// @return A Whole containing the Target frame rate.
                void SetFrameRate(Whole FrameRate);

                /// @brief Set the Desired length of a frame in microseconds.
                /// @param FrameLength The desired minimum length of the frame. Use 0 for no pause.
                void SetFrameLength(Whole FrameLength);


        };
    }

}

#endif
