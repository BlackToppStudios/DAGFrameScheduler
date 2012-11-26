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
#ifndef _workunit_cpp
#define _workunit_cpp

/// @file
/// @brief This file has the implementation of the workunit.

#include "workunit.h"
#include "systemcalls.h"
#include "atomiccas.h"
#include "doublebufferedresource.h"

#ifdef MEZZ_DEBUG
#include <cassert>
#endif

namespace Mezzanine
{
    namespace Threading
    {
        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the dependents as in what must not start until this finishes.

        Whole WorkUnit::GetDependentCount(WorkUnit* Caller) const
        {
            #ifdef MEZZ_DEBUG
            assert(this != Caller); // This will throw if the function is called a second time from the first calling Workunit. Since this should get called on every WorkUnit if the least dependent WorkUnits are called first this can finds all errors.
            if (0==Caller)
                { Caller=(WorkUnit*)this; }
            #endif

            Whole Results = Dependents.size();
            for(std::vector<WorkUnit*>::const_iterator Iter=Dependents.begin(); Iter!=Dependents.end(); ++Iter)
                { Results += (*Iter)->GetDependentCount(Caller); }
            return Results;
        }

        Whole WorkUnit::GetDependentCount() const
        {
            #ifdef MEZZ_DEBUG
                return GetDependentCount(0);
            #else
                return GetDependentCount((WorkUnit*)this);
            #endif
        }

        void WorkUnit::AddDependent(WorkUnit* NewDependent)
            { Dependents.push_back(NewDependent); }

        void WorkUnit::RemoveDependent(WorkUnit* RemoveDependent)
        {
            Dependents.erase(
                        std::remove(Dependents.begin(),Dependents.end(),RemoveDependent),
                        Dependents.end()
                    );
        }

        void WorkUnit::ClearDependents()
            { Dependents.clear(); }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the dependencies as in what must finish before we can run this work unit.

        Whole WorkUnit::GetDependencyCount(WorkUnit* Caller) const
        {
            #ifdef MEZZ_DEBUG
            assert(this != Caller); // see GetDependentCount(WorkUnit*)
            if (0==Caller)
                { Caller=(WorkUnit*)this; }
            #endif

            Whole Results = Dependencies.size();
            for(std::vector<WorkUnit*>::const_iterator Iter=Dependencies.begin(); Iter!=Dependencies.end(); ++Iter)
                { Results += (*Iter)->GetDependencyCount(Caller); }
            return Results;
        }

        Whole WorkUnit::GetDependencyCount() const
        {
            #ifdef MEZZ_DEBUG
                return GetDependencyCount(0);
            #else
                return GetDependencyCount((WorkUnit*)this);
            #endif
        }
        void WorkUnit::AddDependency(WorkUnit* NewDependency)
        {
            Dependencies.push_back(NewDependency);
            NewDependency->AddDependent(this);
        }

        void WorkUnit::RemoveDependency(WorkUnit* RemoveDependency)
        {
            Dependencies.erase(
                        std::remove(Dependencies.begin(),Dependencies.end(),RemoveDependency),
                        Dependencies.end()
                    );
        }

        void WorkUnit::ClearDependencies()
            { Dependencies.clear(); }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the ownership and RunningState
        RunningState WorkUnit::TakeOwnerShip()
        {
            // We check that all the dependencies are met before trying to search the framescheduler
            for (std::vector<WorkUnit*>::iterator Iter = Dependencies.begin(); Iter!=Dependencies.end(); ++Iter)
            {
                if( Complete != (*Iter)->GetRunningState() )
                    { return NotStarted; }
            }

            if(NotStarted ==  AtomicCompareAndSwap(&CurrentRunningState, NotStarted, Running) )
                { return Starting;} // This is the only place a starting should be generated, and it is never placed in CurrentRunningState

            return NotStarted;
        }

        RunningState WorkUnit::GetRunningState() const
            { return (RunningState)CurrentRunningState; } // This only works because we set all of in RunningState to be unsigned.

        void WorkUnit::PrepareForNextFrame()
        {
            CurrentRunningState=NotStarted;

        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the preformance log

        RollingAverage<Whole>& WorkUnit::GetPerformanceLog()
            { return PerformanceLog; }

        void WorkUnit::operator() (ThreadSpecificStorage& CurrentThreadStorage, FrameScheduler& CurrentFrameScheduler)
        {
            MaxInt Begin = Mezzanine::GetTimeStamp();
            #ifdef MEZZ_DEBUG
            CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable() << "<WorkunitStart BeginTimeStamp=\"" << Begin << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << std::endl;
            #endif

            this->DoWork(CurrentThreadStorage, CurrentFrameScheduler);
            MaxInt End = Mezzanine::GetTimeStamp();
            this->GetPerformanceLog().Insert( Whole(End-Begin)); // A whole is usually a 32 bit type, which is fine unless a single workunit runs for 35 minutes.
            CurrentRunningState = Complete;

            #ifdef MEZZ_DEBUG
            CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable() << "<WorkunitEnd EndTimeStamp=\"" << End << "\" Duration=\"" << (End-Begin) << "\" DurationStored=\"" << Whole(End-Begin) << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << std::endl;
            #endif
        }

        WorkUnitKey WorkUnit::GetSortingKey()
            { return WorkUnitKey( this->GetDependentCount(), GetPerformanceLog().GetAverage(), this); }

    }

}

#endif
