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
#ifndef _framescheduler_cpp
#define _framescheduler_cpp

#include "framescheduler.h"
#include "doublebufferedresource.h"
#include "monopoly.h"

#include <exception>
#include <iostream>
#include <algorithm>

/// @file
/// @brief This is the core object implementation for this algorithm

#ifdef _MEZZ_THREAD_WIN32_
    #ifdef _MSC_VER
        #pragma warning( disable : 4706) // Disable Legitimate assignment in a WorkUnit acquisition loops
    #endif
#endif


namespace Mezzanine
{
    namespace Threading
    {

        /// @cond 0

        /// @brief This is the function that all threads will run, except the main.
        /// @param ThreadStorage A pointer to a ThreadSpecificStorage that has the required data for a thread after it launches.
        void ThreadWork(void* ThreadStorage)
        {
            DefaultThreadSpecificStorage::Type& Storage = *((DefaultThreadSpecificStorage::Type*)ThreadStorage);
            FrameScheduler& FS = *(Storage.GetFrameScheduler());
            WorkUnit* CurrentUnit;
            while( (CurrentUnit = FS.GetNextWorkUnit()) )
            {
                if(Starting==CurrentUnit->TakeOwnerShip())
                    { CurrentUnit->operator()(Storage); }
            }
        }

        /// @brief This is the function that the main thread rungs.
        /// @param ThreadStorage A pointer to a ThreadSpecificStorage that has the required data for a thread after it launches.
        void ThreadWorkAffinity(void* ThreadStorage)
        {
            DefaultThreadSpecificStorage::Type& Storage = *((DefaultThreadSpecificStorage::Type*)ThreadStorage);
            FrameScheduler& FS = *(Storage.GetFrameScheduler());
            WorkUnit* CurrentUnit;
            while( (CurrentUnit = FS.GetNextWorkUnitAffinity()) )
            {
                if(Starting==CurrentUnit->TakeOwnerShip())
                    { CurrentUnit->operator()(Storage); }
            }
        }

        /// @endcond

        // Protected Methods
        std::ostream& FrameScheduler::GetLog()
            { return *LogDestination; }

        void FrameScheduler::CreateThreads()
        {
            for(Whole Count = 1; Count<CurrentThreadCount; ++Count)
            {
                if(Count+1>Resources.size())
                    { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }
                Threads.push_back(new thread(ThreadWork, Resources[Count]));
            }
            // Add the check for trying a different amount of frames here
        }

        void FrameScheduler::JoinAllThreads()
        {
            for(std::vector<thread*>::iterator Iter=Threads.begin(); Iter!=Threads.end(); ++Iter)
            {
                (*Iter)->join();
                delete *Iter;
            }
            Threads.clear();
            Threads.reserve(CurrentThreadCount);
        }

        void FrameScheduler::DeleteThreads()
        {
            for(std::vector<thread*>::iterator Iter = Threads.begin(); Iter!=Threads.end(); ++Iter)
                { delete *Iter; }
        }

        void FrameScheduler::WaitUntilNextThread()
        {
            if(TargetFrameLength)
            {
                /*Whole TargetFrameEnd = 1000000/TargetFrameRate + CurrentFrameStart;  // original Timing algorithm is usually about 8 milliseconds longer than 60 seconds on Sqeaky's workstation Mercury
                Whole WaitTime = TargetFrameEnd - GetTimeStamp();
                if(WaitTime>1000000)
                    { WaitTime = 0; }
                Mezzanine::Threading::this_thread::sleep_for( WaitTime );*/
                Whole TargetFrameEnd = TargetFrameLength + CurrentFrameStart;
                Whole WaitTime = Whole(TargetFrameEnd - GetTimeStamp()) - TimingCostAllowance;
                if(WaitTime>1000000)
                    { WaitTime = 0; }
                Mezzanine::Threading::this_thread::sleep_for( WaitTime );
                MaxInt AdjustmentTime = GetTimeStamp();
                if(AdjustmentTime<TargetFrameEnd-TimingCostAllowanceGap)
                    { TimingCostAllowance-=(TargetFrameEnd-AdjustmentTime)/2; }
                if(AdjustmentTime>TargetFrameEnd)
                    { TimingCostAllowance+=(AdjustmentTime-TargetFrameEnd)/2; }
            }
        }

        void FrameScheduler::UpdateDependentGraph(const std::vector<WorkUnitKey>& Units)
        {
            // for each WorkUnit
            for(std::vector<WorkUnitKey>::const_iterator Iter=Units.begin(); Iter!=Units.end(); ++Iter)
            {
                // For each Dependent of that Workunit
                for(std::vector<WorkUnit*>::const_iterator DepIter=Iter->Unit->Dependencies.begin(); DepIter!=Iter->Unit->Dependencies.end(); ++DepIter)
                {
                    // Make a record of the reverse association
                    DependentGraph[*DepIter].insert(Iter->Unit);
                }
            }
        }

        void FrameScheduler::UpdateWorkUnitKeys(std::vector<WorkUnitKey> &Units)
        {
            for(std::vector<WorkUnitKey>::iterator Iter=Units.begin(); Iter!=Units.end(); ++Iter)
            {
                *Iter = Iter->Unit->GetSortingKey(*this);
            }
        }

        FrameScheduler::FrameScheduler(std::fstream *_LogDestination, Whole StartingThreadCount) :
			LogDestination(_LogDestination),
            CurrentFrameStart(0),
			CurrentThreadCount(StartingThreadCount),
            FrameCount(0), TargetFrameLength(16666),
            TimingCostAllowance(125),
			LoggingToAnOwnedFileStream(true)
        { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }

        FrameScheduler::FrameScheduler(std::ostream *_LogDestination, Whole StartingThreadCount) :
			LogDestination(_LogDestination),
            CurrentFrameStart(0),
            CurrentThreadCount(StartingThreadCount),
            FrameCount(0), TargetFrameLength(16666),
			TimingCostAllowance(125),
			LoggingToAnOwnedFileStream(false)
        { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }

        FrameScheduler::~FrameScheduler()
        {
            //StopScheduler();
            if(LoggingToAnOwnedFileStream)
            {
                ((std::fstream*)LogDestination)->close();
                delete LogDestination;
            }
            for(std::vector<WorkUnitKey>::iterator Iter=WorkUnitsMain.begin(); Iter!=WorkUnitsMain.end(); ++Iter)
                { delete Iter->Unit; }
            for(std::vector<MonopolyWorkUnit*>::iterator Iter = Monopolies.begin(); Iter!=Monopolies.end(); ++Iter)
                { delete *Iter; }
            for(std::vector<DefaultThreadSpecificStorage::Type*>::iterator Iter = Resources.begin(); Iter!=Resources.end(); ++Iter)
                { delete *Iter; }
            DeleteThreads();
        }

        void FrameScheduler::AddWorkUnit(WorkUnit* MoreWork)
        { this->WorkUnitsMain.push_back(MoreWork->GetSortingKey(*this)); }

        Whole FrameScheduler::GetDependentCountOf(WorkUnit* Work, bool UsedCached)
        {
            if(UsedCached)
                { UpdateDependentGraph(); }
            Whole Results = DependentGraph[Work].size();
            for(std::set<WorkUnit*>::iterator Iter=DependentGraph[Work].begin(); Iter!=DependentGraph[Work].end(); ++Iter)
            {
                Results+=GetDependentCountOf(*Iter);
            }
            return Results;
        }

        //void FrameScheduler::RemoveWorkUnit(WorkUnit* LessWork)
        //    { WorkUnitsMain.erase(LessWork->GetSortingKey()); }

        WorkUnit* FrameScheduler::GetNextWorkUnit()
        {
            /// @todo Try adding a shortcut of keeping an iterator to the most forward unit on the first contiguous set is located.
            /*
            for (std::vector<WorkUnit*>::iterator Iter = Dependencies.begin(); Iter!=Dependencies.end(); ++Iter)
            {
                if( Complete != (*Iter)->GetRunningState() )
                    { return NotStarted; }
            }*/

            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsMain.rbegin(); Iter!=WorkUnitsMain.rend(); ++Iter)
            {
                if(NotStarted==Iter->Unit->GetRunningState())
                    { return Iter->Unit; }
            }
            return 0;
        }

        WorkUnit* FrameScheduler::GetNextWorkUnitAffinity()
        {
            /// @todo Try adding a shortcut of keeping an iterator to the most forward unit on the first contiguous set is located.

            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitAffinity.rbegin(); Iter!=WorkUnitAffinity.rend(); ++Iter)
            {
                if(NotStarted==Iter->Unit->GetRunningState())
                    { return Iter->Unit; }
            }
            return GetNextWorkUnit();
        }

        Whole FrameScheduler::GetThreadCount()
            { return CurrentThreadCount; }

        void FrameScheduler::SetThreadCount(Whole NewThreadCount)
            { CurrentThreadCount=NewThreadCount; }

        void FrameScheduler::DoOneFrame()
        {
            CurrentFrameStart=GetTimeStamp();
            for(std::vector<MonopolyWorkUnit*>::iterator Iter = Monopolies.begin(); Iter!=Monopolies.end(); ++Iter)
                { (*Iter)->operator()(*(Resources.at(0))); }
            CreateThreads();
            ThreadWorkAffinity(Resources[0]); // Do work in this thread and get the units with affinity
            // run the thing to fixup workunit sorting
            JoinAllThreads();
            FrameCount++;
            ResetAllWorkUnits();
            WaitUntilNextThread();
        }

        void FrameScheduler::ResetAllWorkUnits()
        {
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsMain.rbegin(); Iter!=WorkUnitsMain.rend(); ++Iter)
                { Iter->Unit->PrepareForNextFrame(); }
        }

        void FrameScheduler::SortWorkUnits(bool UpdateDependentGraph_)
        {
            /// @todo make the contents of this a function to reduce code duplication
            if(WorkUnitsMain.size())
            {
                if(UpdateDependentGraph_)
                    { UpdateDependentGraph(); }
                UpdateWorkUnitKeys(WorkUnitsMain);
                std::sort(WorkUnitsMain.begin(),WorkUnitsMain.end(),std::less<WorkUnitKey>() );
            }
        }

        void FrameScheduler::SortWorkUnitsAffinity(bool UpdateDependentGraph_)
        {
            if(WorkUnitAffinity.size())
            {
                if(UpdateDependentGraph_)
                    { UpdateDependentGraph(); }
                UpdateWorkUnitKeys(WorkUnitAffinity);
                std::sort(WorkUnitAffinity.begin(),WorkUnitsMain.end(),std::less<WorkUnitKey>() );
            }
        }

        void FrameScheduler::SortWorkUnitsAll(bool UpdateDependentGraph_)
        {
            if(UpdateDependentGraph_)
                { UpdateDependentGraph(); }
            SortWorkUnitsAffinity(false);
            SortWorkUnits(false);
        }

        void FrameScheduler::UpdateDependentGraph()
        {
            DependentGraph.clear();
            UpdateDependentGraph(WorkUnitsMain);
            UpdateDependentGraph(WorkUnitAffinity);
        }

        Whole FrameScheduler::GetFrameCount() const
            { return FrameCount; }

        Whole FrameScheduler::GetFrameLength() const
            { return TargetFrameLength; }

        void FrameScheduler::SetFrameRate(Whole FrameRate)
        {
            if(FrameRate)
                { TargetFrameLength = 1000000/FrameRate; }
            else
                { TargetFrameLength = 0; }
        }

        void FrameScheduler::SetFrameLength(Whole FrameLength)
            { TargetFrameLength = FrameLength; }

    }
}


#endif
