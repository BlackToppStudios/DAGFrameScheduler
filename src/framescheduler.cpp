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
#include "frameschedulerworkunits.h"
#ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
    #include "atomicoperations.h"
#endif


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
            iWorkUnit* CurrentUnit;

            #ifdef MEZZ_USEBARRIERSEACHFRAME
            while(!FS.LastFrame)
            {
                FS.StartFrameSync.Wait(); // Syncs with Main thread in CreateThreads()
                if(FS.LastFrame)
                    { break; }
            #endif

                do
                {
                    while( (CurrentUnit = FS.GetNextWorkUnit()) ) /// @todo needs to skip ahead a unit instead of spinning
                    {
                        if(Starting==CurrentUnit->TakeOwnerShip())
                            { CurrentUnit->operator()(Storage); }
                    }
                } while(!FS.AreAllWorkUnitsComplete());

            #ifdef MEZZ_USEBARRIERSEACHFRAME
                FS.EndFrameSync.Wait(); // Syncs with Main thread in JoinAllThreads()
            }
            #endif
        }

        /// @brief This is the function that the main thread rungs.
        /// @param ThreadStorage A pointer to a ThreadSpecificStorage that has the required data for a thread after it launches.
        void ThreadWorkAffinity(void* ThreadStorage)
        {
            DefaultThreadSpecificStorage::Type& Storage = *((DefaultThreadSpecificStorage::Type*)ThreadStorage);
            FrameScheduler& FS = *(Storage.GetFrameScheduler());
            iWorkUnit* CurrentUnit;
            do
            {
                while( (CurrentUnit = FS.GetNextWorkUnitAffinity()) ) /// @todo needs to skip ahead a unit instead of spinning
                {
                    if(Starting==CurrentUnit->TakeOwnerShip())
                        { CurrentUnit->operator()(Storage); }
                }
            }
            while(!FS.AreAllWorkUnitsComplete());
        }
        /// @endcond

        ////////////////////////////////////////////////////////////////////////////////
        // Protected Methods
        std::ostream& FrameScheduler::GetLog()
            { return *LogDestination; }

        void FrameScheduler::CleanUpThreads()
        {
            #ifdef MEZZ_USEBARRIERSEACHFRAME
            while(1!=AtomicCompareAndSwap32(&LastFrame,LastFrame,1));
            StartFrameSync.SetThreadSyncCount(0);
            EndFrameSync.SetThreadSyncCount(0); // Handle situations where Threads have not been created yet
            #else
            JoinAllThreads();
            #endif
        }

        void FrameScheduler::DeleteThreads()
        {
            for(std::vector<Thread*>::iterator Iter = Threads.begin(); Iter!=Threads.end(); ++Iter)
                { delete *Iter; }
        }

        void FrameScheduler::UpdateDependentGraph(const std::vector<WorkUnitKey>& Units)
        {
            // for each WorkUnit
            for(std::vector<WorkUnitKey>::const_iterator Iter=Units.begin(); Iter!=Units.end(); ++Iter)
            {
                // For each Dependent of that Workunit
                size_t Max = Iter->Unit->GetImmediateDependencyCount();
                for(size_t Counter=0; Counter<Max; ++Counter)
                    { DependentGraph[Iter->Unit->GetDependency(Counter)].insert(Iter->Unit); } // Make a record of the reverse association
            }
        }

        void FrameScheduler::UpdateWorkUnitKeys(std::vector<WorkUnitKey> &Units)
        {
            for(std::vector<WorkUnitKey>::iterator Iter=Units.begin(); Iter!=Units.end(); ++Iter)
                { *Iter = Iter->Unit->GetSortingKey(*this); }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Construction and Destruction
        FrameScheduler::FrameScheduler(std::fstream *_LogDestination, Whole StartingThreadCount) :
			LogDestination(_LogDestination),
            CurrentFrameStart(GetTimeStamp()),
            Sorter(0),
            #ifdef MEZZ_USEBARRIERSEACHFRAME
            StartFrameSync(StartingThreadCount),
            EndFrameSync(StartingThreadCount),
            LastFrame(0),
            #endif
            #ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
            DecacheMain(0),
            DecacheAffinity(0),
            #endif
			CurrentThreadCount(StartingThreadCount),
            FrameCount(0), TargetFrameLength(16666),
            TimingCostAllowance(0),
			LoggingToAnOwnedFileStream(true)
        { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }

        FrameScheduler::FrameScheduler(std::ostream *_LogDestination, Whole StartingThreadCount) :
			LogDestination(_LogDestination),
            CurrentFrameStart(GetTimeStamp()),
            Sorter(0),
            #ifdef MEZZ_USEBARRIERSEACHFRAME
            StartFrameSync(StartingThreadCount),
            EndFrameSync(StartingThreadCount),
            LastFrame(0),
            #endif
            #ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
            DecacheMain(0),
            DecacheAffinity(0),
            #endif
            CurrentThreadCount(StartingThreadCount),
            FrameCount(0), TargetFrameLength(16666),
            TimingCostAllowance(0),
			LoggingToAnOwnedFileStream(false)
        { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }

        FrameScheduler::~FrameScheduler()
        {            
            CleanUpThreads();

            if(LoggingToAnOwnedFileStream)
            {
                ((std::fstream*)LogDestination)->close();
                delete LogDestination;
            }
            for(std::vector<WorkUnitKey>::iterator Iter=WorkUnitsMain.begin(); Iter!=WorkUnitsMain.end(); ++Iter)
                { delete Iter->Unit; }
            for(std::vector<MonopolyWorkUnit*>::iterator Iter = WorkUnitMonopolies.begin(); Iter!=WorkUnitMonopolies.end(); ++Iter)
                { delete *Iter; }
            for(std::vector<DefaultThreadSpecificStorage::Type*>::iterator Iter = Resources.begin(); Iter!=Resources.end(); ++Iter)
                { delete *Iter; }
            DeleteThreads();
        }

        ////////////////////////////////////////////////////////////////////////////////
        // WorkUnit management
        void FrameScheduler::AddWorkUnit(iWorkUnit* MoreWork)
            { this->WorkUnitsMain.push_back(MoreWork->GetSortingKey(*this)); }

        void FrameScheduler::AddWorkUnitAffinity(iWorkUnit* MoreWork)
            { this->WorkUnitsAffinity.push_back(MoreWork->GetSortingKey(*this)); }

        void FrameScheduler::AddWorkUnitMonopoly(MonopolyWorkUnit* MoreWork)
            { this->WorkUnitMonopolies.push_back(MoreWork); }

        void FrameScheduler::SortWorkUnitsMain(bool UpdateDependentGraph_)
        {
            if(UpdateDependentGraph_)
                { UpdateDependentGraph(); }
            if(WorkUnitsMain.size())
            {
                UpdateWorkUnitKeys(WorkUnitsMain);
                std::sort(WorkUnitsMain.begin(),WorkUnitsMain.end(),std::less<WorkUnitKey>() );
            }
        }

        void FrameScheduler::SortWorkUnitsAffinity(bool UpdateDependentGraph_)
        {
            if(UpdateDependentGraph_)
                { UpdateDependentGraph(); }
            if(WorkUnitsAffinity.size())
            {
                UpdateWorkUnitKeys(WorkUnitsAffinity);
                std::sort(WorkUnitsAffinity.begin(),WorkUnitsMain.end(),std::less<WorkUnitKey>() );
            }
        }

        void FrameScheduler::SortWorkUnitsAll(bool UpdateDependentGraph_)
        {
            SortWorkUnitsAffinity(UpdateDependentGraph_);
            SortWorkUnitsMain(false);
        }

        void FrameScheduler::RemoveWorkUnit(iWorkUnit *LessWork)
        {
            WorkUnitsAffinity.erase
                        (
                            std::remove(
                                    WorkUnitsAffinity.begin(),
                                    WorkUnitsAffinity.end(),
                                    LessWork->GetSortingKey(*this)
                                    )
                        );
           WorkUnitsMain.erase
                        (
                            std::remove(
                                    WorkUnitsMain.begin(),
                                    WorkUnitsMain.end(),
                                    LessWork->GetSortingKey(*this)
                                    )
                        );
           WorkUnitMonopolies.erase
                        (
                            std::remove(
                                    WorkUnitMonopolies.begin(),
                                    WorkUnitMonopolies.end(),
                                    LessWork
                                    )
                        );
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Algorithm essentials

        Whole FrameScheduler::GetDependentCountOf(iWorkUnit* Work, bool UsedCachedDepedentGraph)
        {
            if(UsedCachedDepedentGraph)
                { UpdateDependentGraph(); }
            Whole Results = DependentGraph[Work].size();
            for(std::set<iWorkUnit*>::iterator Iter=DependentGraph[Work].begin(); Iter!=DependentGraph[Work].end(); ++Iter)
            {
                Results+=GetDependentCountOf(*Iter);
            }
            return Results;
        }
        //DecacheMain(0),
        //DecacheAffinity(0),
        iWorkUnit* FrameScheduler::GetNextWorkUnit()
        {
            #ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
            bool CompleteSoFar = true;
            Int32 CurrentRun = DecacheMain;
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsMain.rbegin()+CurrentRun; Iter!=WorkUnitsMain.rend(); ++Iter)
            {
                if (NotStarted==Iter->Unit->GetRunningState())
                {
                    if(Iter->Unit->IsEveryDependencyComplete())
                        { return Iter->Unit; }
                    CompleteSoFar=false;
                }

                if(CompleteSoFar)
                {
                    CurrentRun++;
                    if(Complete==Iter->Unit->GetRunningState())
                        { AtomicCompareAndSwap32(&DecacheMain,DecacheMain,CurrentRun); }
                }
            }
            #else
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsMain.rbegin(); Iter!=WorkUnitsMain.rend(); ++Iter)
            {
                if (NotStarted==Iter->Unit->GetRunningState())
                {
                    if(Iter->Unit->IsEveryDependencyComplete())
                        { return Iter->Unit; }
                }
            }
            #endif

            return 0;
        }

        iWorkUnit* FrameScheduler::GetNextWorkUnitAffinity()
        {
            #ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
            bool CompleteSoFar = true;
            Int32 CurrentRun = DecacheAffinity;
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsAffinity.rbegin()+CurrentRun; Iter!=WorkUnitsAffinity.rend(); ++Iter)
            {
                if (NotStarted==Iter->Unit->GetRunningState())
                {
                    if(Iter->Unit->IsEveryDependencyComplete())
                        { return Iter->Unit; }
                    CompleteSoFar=false;
                }

                if(CompleteSoFar)
                {
                    CurrentRun++;
                    if(Complete==Iter->Unit->GetRunningState())
                        { AtomicCompareAndSwap32(&DecacheAffinity,DecacheAffinity,CurrentRun); }
                }
            }
            #else
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsAffinity.rbegin(); Iter!=WorkUnitsAffinity.rend(); ++Iter)
            {
                if (NotStarted==Iter->Unit->GetRunningState())
                {
                    if(Iter->Unit->IsEveryDependencyComplete())
                        { return Iter->Unit; }
                }
            }
            #endif

            return GetNextWorkUnit();
        }

        bool FrameScheduler::AreAllWorkUnitsComplete()
        {
            // start reading from units likely to be executed last.
            for(std::vector<WorkUnitKey>::iterator Iter = WorkUnitsMain.begin(); Iter!=WorkUnitsMain.end(); ++Iter)
            {
                //if(!Iter->Unit->IsEveryDependencyComplete())
                    //{ return false; }
                if(Complete!=Iter->Unit->GetRunningState())
                    { return false; }
            }

            for(std::vector<WorkUnitKey>::iterator Iter = WorkUnitsAffinity.begin(); Iter!=WorkUnitsAffinity.end(); ++Iter)
            {
                //if(!Iter->Unit->IsEveryDependencyComplete())
                //    { return false; }
                if(Complete!=Iter->Unit->GetRunningState())
                    { return false; }
            }

            return true;
        }

        void FrameScheduler::UpdateDependentGraph()
        {
            DependentGraph.clear();
            UpdateDependentGraph(WorkUnitsMain);
            UpdateDependentGraph(WorkUnitsAffinity);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Algorithm Configuration and Introspection

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

        Whole FrameScheduler::GetThreadCount()
            { return CurrentThreadCount; }

        void FrameScheduler::SetThreadCount(Whole NewThreadCount)
        { CurrentThreadCount=NewThreadCount; }

        MaxInt FrameScheduler::GetCurrentFrameStart() const
            { return CurrentFrameStart; }

        ////////////////////////////////////////////////////////////////////////////////
        // Executing a Frame

        void FrameScheduler::DoOneFrame()
        {
            RunAllMonopolies();
            CreateThreads();
            RunMainThreadWork();
            JoinAllThreads();
            ResetAllWorkUnits();
            WaitUntilNextFrame();
        }

        void FrameScheduler::RunAllMonopolies()
        {
            for(std::vector<MonopolyWorkUnit*>::iterator Iter = WorkUnitMonopolies.begin(); Iter!=WorkUnitMonopolies.end(); ++Iter)
                { (*Iter)->operator()(*(Resources.at(0))); }
        }

        void FrameScheduler::CreateThreads()
        {
            #ifdef MEZZ_USEBARRIERSEACHFRAME
                StartFrameSync.SetThreadSyncCount(CurrentThreadCount);
                EndFrameSync.SetThreadSyncCount(CurrentThreadCount);
                for(Whole Count = 1; Count<CurrentThreadCount; ++Count)
                {
                    if(Count+1>Resources.size())
                    {
                        Resources.push_back(new DefaultThreadSpecificStorage::Type(this));
                        Threads.push_back(new Thread(ThreadWork, Resources[Count]));
                    }
                }
                StartFrameSync.Wait();
            #else
                for(Whole Count = 1; Count<CurrentThreadCount; ++Count)
                {
                    if(Count+1>Resources.size())
                        { Resources.push_back(new DefaultThreadSpecificStorage::Type(this)); }
                    Threads.push_back(new Thread(ThreadWork, Resources[Count]));
                }
            #endif
        }

        void FrameScheduler::RunMainThreadWork()
        {
            ThreadWorkAffinity(Resources[0]); // Do work in this thread and get the units with affinity
        }

        void FrameScheduler::JoinAllThreads()
        {
            #ifdef MEZZ_USEBARRIERSEACHFRAME
            EndFrameSync.Wait();
            #else
            for(std::vector<Thread*>::iterator Iter=Threads.begin(); Iter!=Threads.end(); ++Iter)
            {
                (*Iter)->join();
                delete *Iter;
            }
            Threads.clear();
            Threads.reserve(CurrentThreadCount);
            #endif

            if(Sorter)
            {
                WorkUnitsAffinity = Sorter->WorkUnitsAffinity;
                WorkUnitsMain = Sorter->WorkUnitsMain;
                Sorter=0;
            }
        }

        void FrameScheduler::ResetAllWorkUnits()
        {
            /// @todo could be replace with a parallel for, or a monopoly
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsMain.rbegin(); Iter!=WorkUnitsMain.rend(); ++Iter)
                { Iter->Unit->PrepareForNextFrame(); }
            for(std::vector<WorkUnitKey>::reverse_iterator Iter = WorkUnitsAffinity.rbegin(); Iter!=WorkUnitsAffinity.rend(); ++Iter)
                { Iter->Unit->PrepareForNextFrame(); }
            #ifdef MEZZ_USEATOMICSTODECACHECOMPLETEWORK
            DecacheMain=0;
            DecacheAffinity=0;
            #endif
        }

        void FrameScheduler::WaitUntilNextFrame()
        {
            FrameCount++;
            if(TargetFrameLength)
            {
                /*Whole TargetFrameEnd = 1000000/TargetFrameRate + CurrentFrameStart;  // original Timing algorithm is usually about 8 milliseconds longer than 60 seconds on Sqeaky's workstation Mercury
                Whole WaitTime = TargetFrameEnd - GetTimeStamp();
                if(WaitTime>1000000)
                    { WaitTime = 0; }
                Mezzanine::Threading::this_thread::sleep_for( WaitTime );*/
                /*Whole TargetFrameEnd = TargetFrameLength + CurrentFrameStart;         //Second algorithm works great in debug but it is not
                Whole WaitTime = Whole(TargetFrameEnd - GetTimeStamp()) - TimingCostAllowance;
                if(WaitTime>1000000)
                    { WaitTime = 0; }
                Mezzanine::Threading::this_thread::sleep_for( WaitTime );
                MaxInt AdjustmentTime = GetTimeStamp();
                if(AdjustmentTime<TargetFrameEnd-TimingCostAllowanceGap)
                    { TimingCostAllowance-=(TargetFrameEnd-AdjustmentTime)/2; }
                if(AdjustmentTime>TargetFrameEnd)
                    { TimingCostAllowance+=(AdjustmentTime-TargetFrameEnd)/2; }*/
                Whole TargetFrameEnd = TargetFrameLength + CurrentFrameStart;
                Whole WaitTime = Whole(TargetFrameEnd - GetTimeStamp()) + TimingCostAllowance;
                if(WaitTime>1000000)
                    { WaitTime = 0; }
                Mezzanine::Threading::this_thread::sleep_for( WaitTime );
                CurrentFrameStart=GetTimeStamp();
                TimingCostAllowance -= (CurrentFrameStart-TargetFrameEnd);

            }
        }
    } // \Threading
}// \Mezanine


#endif
