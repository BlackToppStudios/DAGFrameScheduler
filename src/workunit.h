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
#ifndef _workunit_h
#define _workunit_h

#include "datatypes.h"
#include "framescheduler.h"
#include "mutex.h"
#include "rollingaverage.h"
#include "threadingenumerations.h"
#include "thread.h"
#include "workunitkey.h"

#include <vector>

/// @file
/// @brief This file has the defintion of the workunit.

namespace Mezzanine
{
    namespace Threading
    {
        /// @brief Default implementation of WorkUnit. This represents on piece of work through time.
        class MEZZ_LIB WorkUnit
        {
            friend class FrameScheduler;

            private:
                //WorkUnit(const WorkUnit&) = delete;
                //WorkUnit& operator=(const WorkUnit&) = delete;
                //WorkUnit& operator=(const WorkUnit&) volatile = delete;

                /// @brief Remove Copy constructor, Copying a work unit does not make sense, it breaks scheduling, made private.
                WorkUnit(const WorkUnit&) {}
                /// @brief Assignment operator, Assignment on a WorkUnit Does Not make sense, it breaks scheduling, made private.
                WorkUnit& operator=(WorkUnit& Unused) { return Unused; }

            protected:
                /////////////////////////////////////////////////////////////////////////////////////////////
                // Data Members

                /// @brief A collection of of workunits that must be complete before this one can start.
                std::vector<WorkUnit*> Dependencies;

                /// @brief A rolling average of execution times.
                DefaultRollingAverage<Whole>::Type PerformanceLog;

                /// @brief This controls do work with this after it has .
                //volatile int32_t CurrentRunningState;
                volatile Int32 CurrentRunningState;

                /////////////////////////////////////////////////////////////////////////////////////////////
                // Work with the dependents as in what WorkUnits must not start until this finishes.

            public:
                /// @brief This returns the count workunits that depend on this work unit.
                /// @param SchedulerToCount The @ref FrameScheduler has a cache of this data, it is impossible to calculate without the complete list of WorkUnits on it either.
                /// @details Because Dependents are not tracked this iterates over entry in the FrameScheduler it is passed
                /// @return A Whole is returned containing the count.
                virtual Whole GetDependentCount(FrameScheduler &SchedulerToCount);

                // @brief Perform whatever tracking is required to have another workunit depend on this one.
                // @param NewDependent The WorkUnit That Depends on this one.
                // @details Increment the DependentCount. NewDependent Ignored in this niave implemention, but could be useful in derived versions.
                //virtual void AddDependent(WorkUnit* NewDependent);

                // @brief Perform whatever tracking is required to have another workunit no longer depend on this one.
                // @details In this simple implementation decrement the DependentCount.
                // @param RemoveDependent Removed from the list of Dependents.
                // @todo Cleanup WorkUnit Dependency removal.
                //virtual void RemoveDependent(WorkUnit* RemoveDependent);

                // @brief Drop any information about what work Units depend on this one.
                // @details Set dependent count to zero, but a derived implementation could do anything.
                //virtual void ClearDependents();

                /////////////////////////////////////////////////////////////////////////////////////////////
                // Work with the dependencies as in what must finish before we can run this work unit.

            protected:
                /// @brief Does any required work for GetDependencyCount() and calls GetDependencyCount(WorkUnit*) on all dependency WorkUnits.
                /// @param Caller The WorkUnit that initiated the query to allow breaking of cycles.
                /// @return A partial count of dependencies unless this == Caller, then it returns a complete count.
                Whole GetDependencyCount(WorkUnit* Caller) const;

            public:
                /// @brief How many other WorkUnits does this one depend on?
                /// @return A Whole containing the answer.
                virtual Whole GetDependencyCount() const;

                /// @brief Force this WorkUnit to Start after another has completed.
                /// @param NewDependency The WorkUnit that must start after this one has completed.
                /// @warning Changing this outside the schedule, once the scheduler has started can cause undefined behavior.
                virtual void AddDependency(WorkUnit* NewDependency);

                /// @brief Remove a dependency
                /// @param RemoveDependency A pointer to the WorkUnit to remove as a dependency
                virtual void RemoveDependency(WorkUnit* RemoveDependency);

                /// @brief Drop any information about what work Units this one depends on.
                virtual void ClearDependencies();

                virtual bool IsEveryDependencyComplete();

                /////////////////////////////////////////////////////////////////////////////////////////////
                // Work with the ownership and RunningState

                /// @brief Attempts to atomically start the work unit in the current thread.
                /// @return Returns RunningState::Starting if this thread was able to gain ownership and start the workunit, returns RunningState::NotStarted otherwise
                virtual RunningState TakeOwnerShip();

                /// @brief Retrieves the current RunningState of the thread.
                /// @return A RunningState that indicates if the thread is running, started, etc... This information can be changed at any time and should be considered stale immediately after retrieval.
                virtual RunningState GetRunningState() const;

                /// @brief This resets the running state and takes any further action required to use the WorkUnit again.
                virtual void PrepareForNextFrame();

                /////////////////////////////////////////////////////////////////////////////////////////////
                // Work with the preformance log

                /// @brief Get the internal rolling average for querying.
                /// @return A const reference to the internal Rolling Average.
                virtual RollingAverage<Whole> &GetPerformanceLog();

                /////////////////////////////////////////////////////////////////////////////////////////////
                // Deciding when to and doing the work

                /// @brief This tracks work unit metadata, then calls DoWork
                /// @param CurrentThreadStorage The @ref ThreadSpecificStorage that this WorkUnit will operate with when executing.
                virtual void operator() (DefaultThreadSpecificStorage::Type& CurrentThreadStorage);

                /// @brief Get the sorting metadata.
                /// @param SchedulerToCount This uses the metadata on the @ref FrameScheduler to generate the Dependency listing required.
                /// @return A WorkUnitKey suitable for sorting this workunit
                virtual WorkUnitKey GetSortingKey(FrameScheduler &SchedulerToCount);

                /// @brief WorkUnits Must implement these to do the work
                virtual void DoWork(DefaultThreadSpecificStorage::Type& CurrentThreadStorage) = 0;

                /// @brief Simple constructor
                WorkUnit() : CurrentRunningState(NotStarted)
                {}

                /// @brief Virtual destructor
                virtual ~WorkUnit(){}
        };

    } // \Threading
}

#endif
