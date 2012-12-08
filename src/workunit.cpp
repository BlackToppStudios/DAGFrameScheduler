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
#include "atomicoperations.h"
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

        /*Whole WorkUnit::GetDependentCount(WorkUnit* Caller) const
        {
            #ifdef MEZZ_DEBUG
            assert(this != Caller); // This will throw if the function is called a second time from the first calling Workunit. Since this should get called on every WorkUnit if the least dependent WorkUnits are called first this can finds all errors.
            if (0==Caller)
                { Caller=(WorkUnit*)this; }
            #endif

            Whole Results = Dependents.size();

                { Results += (*Iter)->GetDependentCount(Caller); }
            return Results;
        }*/

        Whole iWorkUnit::GetDependentCount(FrameScheduler &SchedulerToCount)
            { return SchedulerToCount.GetDependentCountOf(this); }

        Whole iWorkUnit::GetDependencyCount() const
        {
            #ifdef MEZZ_DEBUG
                return GetDependencyCount(0);
            #else
                return GetDependencyCount((WorkUnit*)this);
            #endif
        }

        iWorkUnit* iWorkUnit::GetDependency(Whole Index) const
            { return Dependencies.at(Index); }

        Whole iWorkUnit::GetImmediateDependencyCount() const
            { return Dependencies.size(); }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the dependencies as in what must finish before we can run this work unit.

        Whole iWorkUnit::GetDependencyCount(iWorkUnit* Caller) const
        {
            #ifdef MEZZ_DEBUG
            assert(this != Caller); // see GetDependentCount(WorkUnit*)
            if (0==Caller)
                { Caller=(iWorkUnit*)this; }
            #endif

            Whole Results = Dependencies.size();
            for(std::vector<iWorkUnit*>::const_iterator Iter=Dependencies.begin(); Iter!=Dependencies.end(); ++Iter)
                { Results += (*Iter)->GetDependencyCount(Caller); }
            return Results;
        }



        void iWorkUnit::AddDependency(iWorkUnit* NewDependency)
            { Dependencies.push_back(NewDependency); }

        void iWorkUnit::RemoveDependency(iWorkUnit* RemoveDependency)
        {
            Dependencies.erase(
                        std::remove(Dependencies.begin(),Dependencies.end(),RemoveDependency),
                        Dependencies.end()
                    );
        }

        void iWorkUnit::ClearDependencies()
        { Dependencies.clear(); }

        bool iWorkUnit::IsEveryDependencyComplete()
        {
            for (std::vector<iWorkUnit*>::iterator Iter = Dependencies.begin(); Iter!=Dependencies.end(); ++Iter)
            {
                if( Complete != (*Iter)->GetRunningState() )
                    { return false; }
            }
            return true;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the ownership and RunningState
        RunningState iWorkUnit::TakeOwnerShip()
        {
            if(!IsEveryDependencyComplete())
                    { return NotStarted; }

            if(NotStarted ==  AtomicCompareAndSwap(&CurrentRunningState, NotStarted, Running) )
                { return Starting;} // This is the only place a starting should be generated, and it is never placed in CurrentRunningState

            return NotStarted;
        }

        RunningState iWorkUnit::GetRunningState() const
            { return (RunningState)CurrentRunningState; } // This only works because we set all of in RunningState to be unsigned.

        void iWorkUnit::PrepareForNextFrame()
        {
            CurrentRunningState=NotStarted;

        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Work with the preformance log

        RollingAverage<Whole>& iWorkUnit::GetPerformanceLog()
            { return PerformanceLog; }

        void iWorkUnit::operator() (DefaultThreadSpecificStorage::Type& CurrentThreadStorage)
        {
            MaxInt Begin = Mezzanine::GetTimeStamp();
            #ifdef MEZZ_DEBUG
            CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable() << "<WorkunitStart BeginTimeStamp=\"" << Begin << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << std::endl;
            #endif

            this->DoWork(CurrentThreadStorage);
            MaxInt End = Mezzanine::GetTimeStamp();
            this->GetPerformanceLog().Insert( Whole(End-Begin)); // A whole is usually a 32 bit type, which is fine unless a single workunit runs for 35 minutes.
            CurrentRunningState = Complete;

            #ifdef MEZZ_DEBUG
            CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable() << "<WorkunitEnd EndTimeStamp=\"" << End << "\" Duration=\"" << (End-Begin) << "\" DurationStored=\"" << Whole(End-Begin) << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << std::endl;
            #endif
        }

        WorkUnitKey iWorkUnit::GetSortingKey(FrameScheduler& SchedulerToCount)
            { return WorkUnitKey( this->GetDependentCount(SchedulerToCount), GetPerformanceLog().GetAverage(), this); }


    }

}

#endif
