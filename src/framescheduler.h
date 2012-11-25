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
#include "thread.h"
#include "doublebufferedresource.h"
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
        /// @def TimingCostAllowanceGap
        /// @brief This is a configuration settings to tune the precision of timing at compile time.
        /// @details Each frame that is executed has a desired length of time as determined by the Desired
        /// Frame Rate that can set using @ref Mezzanine::Threading::FrameScheduler::SetFrameRate "FrameScheduler::SetFrameRate()"
        /// or @ref Mezzanine::Threading::FrameScheduler::SetFrameLength "FrameScheduler::SetFrameLength()".
        /// If execution of a frame finishes before it has consumed its portion of a second, then work is paused
        /// the desired length has elapsed. This means that if there is too much work each frame that the frame rate
        /// will suffer.
        /// @n @n
        /// The timing of frame lengths with this algorithm is not perfect. Each frame ends a certain
        /// amount of microseconds early or late. This inaccuracy can be introduced by the system's timer
        /// resolution, the time it takes to lookup the time, or by the work that must be done outside of
        /// the of the timed work or pause periods to aggregate data. On Ubuntu 12.04 x64 has a simple
        /// timer with a single microsecond resolution, this algorithm tends to drift less than 2 microseconds per
        /// frame. Some other algorithms are more precise, and others are simpler, this is a balance between
        /// those. If this is not acceptable @ref Mezzanine::GetTimeStamp() "GetTimeStamp()" and
        /// @ref Mezzanine::Threading::FrameScheduler::WaitUntilNextThread() "FrameScheduler::WaitUntilNextThread()" could be
        /// modified to adjust this.
        /// @n @n
        /// The pause that is inserted each frame is not simply calculated by subtracting the current time
        /// from the target time once the work is done each frame. A small
        /// @ref Mezzanine::Threading::FrameScheduler::TimingCostAllowance "TimingCostAllowance" is
        /// subtracted from the pause before the next frame begins. This allowance allows for the compensation of
        /// variances in scheduling from the underlying system, rounding errors of the system clock and
        /// the execution of the time tracking itself. As part of this time tracking the timing cost allowance
        /// is adjusted. If the allowance was too large, therefor making the frame too short, then it is made
        /// longer for the next frame. If the allowance is too small it is made larger for the next frame. It does
        /// seem possible to track this length within a single frame without increasing the complexity a
        /// significant amount.
        /// @n @n
        /// Rather than increase the complexity of the algorithm when the default level of accuracy is
        /// suitable for most purposes, a manual compile time adjustment has been added. When comparing the length
        /// of the frame after execution and frame pause have occurred, the timing allowance is made smaller or
        /// larger based on how close the result is to the desired time. If the frame was not exactly as fast as
        /// it should have been, then the allowance is adjusted. Frames tend to take slightly longer than the
        /// target length. To compensate for this if the frame executed faster by a predefined amount or less
        /// then the timing allowance is not adjusted. This gap in the adjustment of the timing allowance could
        /// allow adjustment on different platforms in the precision of this algorithm without incurring extra
        /// complexity in timing. By default this is set to 10 microseconds.
        /// @ref Mezzanine::Threading::FrameScheduler::GetFrameLength "FrameScheduler::GetFrameLength()"
        #define  TimingCostAllowanceGap 10

        class MonopolyWorkUnit;
        class WorkUnit;
        class LogAggregator;
        class LogBufferSwapper;
        class FrameScheduler;

        /// @brief This is central object in this algorithm, it is responsible for spawning threads and managing the order that work units are executed.
        class MEZZ_LIB FrameScheduler
        {
            friend class LogAggregator;
            friend class LogBufferSwapper;

            protected:
                /// @brief A collection of all the work units that are not Monopolies and do not have affinity for a given thread.
                /// @details This stores and sorts the WorkUnit instances so that the ones that should be run first will be
                /// higher/later in the set.
                std::vector<WorkUnitKey> WorkUnitsMain;

                /// @brief This maintains ownership of all the thread specific resources.
                /// @note There should be the same amount of more these than entries in the Threads vector.
                std::vector<ThreadSpecificStorage*> Resources;

                /// @brief A way to track an arbitrary number of threads.
                /// @note There should never be more of these than Resources, and if there are more at the beginning of a frame the resources will be created in CreateThreads().
                std::vector<thread*> Threads;

                /// @brief A collection of all the monopolies this scheduler must run and keep ownership of.
                std::vector<MonopolyWorkUnit*> Monopolies;

                /// @brief When the logs are aggregated, this is where they are sent
                std::ostream* LogDestination;

                /// @brief How many threads with this try to execute with in the next frame.
                Whole CurrentThreadCount;

                /// @brief Used to store a count of frames from the begining of game execution.
                Whole FrameCount;

                /// @brief The Maximum frame rate this algorithm should run at.
                Whole TargetFrameLength;

                /// @brief What time did the current Frame Start at.
                Whole CurrentFrameStart;

                /// @brief To prevent frame time drift this many microseconds is subtracted from the wait period to allow time for calculations.
                Whole TimingCostAllowance;

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
                virtual void AddWorkUnit(WorkUnit* MoreWork);

                // @brief Remove a WorkUnit, and regain ownership of it
                // @param LessWork a pointer to a WorkUnit that should no longer be scheduled.
                // @warning This does not cleanup dependencies and can get you in trouble if other work units depend on the one removed
                // virtual void RemoveWorkUnit(WorkUnit* LessWork);

                /// @brief Gets the next available workunit for execution.
                /// @details This finds the next available WorkUnit which has not started execution, has no dependencies that have
                /// not complete, has the most WorkUnits Units that depend on it (has the highest runtime in the case of a tie).
                /// @return A pointer to the WorkUnit that could be executed. This does not give ownership of that WorkUnit.
                virtual WorkUnit* GetNextWorkUnit();

                /// @brief Get the amount of threads that will be used to execute WorkUnits a the start of the next frame.
                /// @return A Whole with the current desired thread count.
                virtual Whole GetThreadCount();

                /// @brief Set the amount of thread to use.
                /// @param NewThreadCount The amount of threads to use starting at the begining of the next frame.
                virtual void SetThreadCount(Whole NewThreadCount);

                /// @brief Do one frame worth of work.
                /// @details Every Monopoly will be executed once and each work unit will be executed once.
                /// @todo Cause Threads that stop because work appear exhausted from dependency issues to spin/wait until work is done.
                /// @todo implement thread affinity.
                virtual void DoOneFrame();

                /// @brief Take any steps required to prepare all owned WorkUnits for execution next frame.
                virtual void ResetAllWorkUnits();

                /// @brief Sort the workUnits
                void SortWorkUnits();

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
