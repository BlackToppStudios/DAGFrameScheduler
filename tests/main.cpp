// The DAGFrameScheduler is a Multi-Threaded lock free and wait free scheduling library.
// © Copyright 2010 - 2013 BlackTopp Studios Inc.
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
#ifndef _main_cpp
#define _main_cpp

/// @file
/// @brief This file defines a number of tests for the rest of the library, and is not included in binary form with the library when compiled.

#include "dagframescheduler.h"

#include <iostream>
//#include <cassert>
#include <typeinfo>
#include <cstdlib>
#include <sstream>
#include <set>
#include <map>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <numeric>

#include "pugixml.h" // Not needed for regular operation of the library, just needed for tests.

#ifdef _MEZZ_THREAD_WIN32_
    #ifdef _MSC_VER
        #pragma warning( disable : 4800) // Disable Int to bool conversion warning in pugi xml node checks
    #endif
#endif

using namespace std;
using namespace Mezzanine;
using namespace Mezzanine::Threading;

////////////////////////////////////////////////////////////////////////////////////////////////
/// Required testing functionality

/// @brief A pointer to the kinds of tests. no return type is needed, they will throw on error.
typedef void (*Test)();
/// @brief A collection of tests and their names
typedef map<String,Test> TestGroup;

/// @brief The error and help message.
void Usage(String Executable, TestGroup& Tests)
{
    cout << "Usage:" << endl << endl;
    cout << "\t" << Executable << " [testname1] [testname2] [testname3] ..." << endl << endl;
    cout << "If no tests are provided then every test will be run. The test names are not case sensitive. Here is a listing of test names: " << endl;

    Whole ColumnWidth=25;
    Whole ColumnCount=3;
    Whole WhichColumn=0;
    String CurrentOutput;

    cout << "  ";
    for(TestGroup::iterator Iter=Tests.begin(); Iter!=Tests.end(); ++Iter)
    {
        if(!(WhichColumn % ColumnCount))
        {
            WhichColumn=0;
            cout << endl << "  ";
        }
        WhichColumn++;

        CurrentOutput = Iter->first;
        for(CurrentOutput = Iter->first; CurrentOutput.size()<ColumnWidth; CurrentOutput += " ") {}

        cout << CurrentOutput;
    }
    cout << endl;
}

#ifdef _MSC_VER
    /// @brief Used to Detail exactly where a test failure occurred.
    #define ThrowOnFalse(Cond,Msg)  ThrowOnFalse_((Cond), (Msg), __FILE__, __LINE__, __FUNCTION__)
#else
    /// @brief Used to Detail exactly where a test failure occurred.
    #define ThrowOnFalse(Cond,Msg)  ThrowOnFalse_((Cond), (Msg), __FILE__, __LINE__, __func__)
#endif

/// @brief Output error data and throw an exception that won't be caught.
/// @param Condition If false, this throws an error.
/// @param Message A message about the test that failed.
/// @param File The file name the error occurred in.
/// @param LineNumber The line of the file the error occurred at.
/// @param Function The function the error occurred in.
void ThrowOnFalse_(bool Condition, const String Message, String File, Whole LineNumber, String Function)
{
    if(!Condition)
    {
        cerr << "\"" << Message << "\"" << endl
             << "File:        " << File << endl
             << "Line Number: " << LineNumber << endl
             << "Function:    " << Function << endl;
        throw(exception() );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing Random Number facilities

/// @brief Simulate a 20 sided die
/// @details Used in @ref RandomTests
/// @return A Mezzanine::Whole containing a a random number between 1 and 20 inclusive with equal chance.
Mezzanine::Whole DiceD20()
    { return rand()%20+1; }

/// @brief Simulate 2x 3 sided die being added
/// @details Used in @ref RandomTests
/// @return A Mezzanine::Whole containing a a random number between 2 and 6 inclusive with a bell curve probability. This actually generates 2 numbers between 1 and 3 inclusive adds them, then returns that.
Mezzanine::Whole Dice2D3()
    { return ((rand()%3) + (rand()%3) + 2); }

/// @brief The 'random' Test.
/// @details This tests nothing in the library itself, but good quality random numbers are required for stressing the Scheduler in other tests
void RandomTests()
{
    cout << endl << "Starting random number generation tests. Not part of the library, but required for testing." << endl;
    Mezzanine::MaxInt Timestamp1 = Mezzanine::GetTimeStamp();
    srand((int)Timestamp1);
    Whole TestRuns = 10000000;
    vector<Whole> D20Rolls;
    vector<Whole> D2d3Rolls;
    vector<Whole> D20Histogram;
    vector<Whole> D2d3Histogram;

    cout <<  "Doing " << TestRuns << " iterations of generatining numbers with each function, and initializing random seed with " << Timestamp1 << "." << endl;
    for (Whole Counter=0; Counter<TestRuns; ++Counter)
    {
        D20Rolls.push_back(DiceD20());
        D2d3Rolls.push_back(Dice2D3());
    }

    cout << "Preparing counts of the rolls." << endl;
    // init some 0s
    for (Whole Counter=0; Counter<=21; ++Counter) //providing a small amount of buffer space on either side in case dice algorithm has off by one issue.
        { D20Histogram.push_back(0); }
    for (Whole Counter=0; Counter<=7; ++Counter)
        { D2d3Histogram.push_back(0); }

    // count the actual rolls
    for(vector<Whole>::iterator Iter=D20Rolls.begin(); Iter!=D20Rolls.end(); ++Iter)
        { D20Histogram[*Iter]++; }
    for(vector<Whole>::iterator Iter=D2d3Rolls.begin(); Iter!=D2d3Rolls.end(); ++Iter)
        { D2d3Histogram[*Iter]++; }

    // display chart
    cout << "D20 rolls should be distributed evenly between 1 and 20 inclusive:" << endl;
    for (Whole Counter=0; Counter<=21; ++Counter) //providing a small amount of buffer space on either side in case dice algorithm has off by one issue.
    { cout << Counter << ": " << D20Histogram[Counter] << " \t" << (((double)D20Histogram[Counter])/((double)TestRuns))*100 << "% " << endl; }

    cout << "2d3 rolls should be distributed on a bell curve 2 and 6 inclusive:" << endl;
    for (Whole Counter=0; Counter<=7; ++Counter)
        { cout << Counter << ": " << D2d3Histogram[Counter] << " \t" << (((double)D2d3Histogram[Counter])/((double)TestRuns))*100 << "% " <<  endl; }
}
////////////////////////////////////////////////////////////////////////////////////////////////
/// Basic information acquisition

/// @brief The 'sizes' Test.
void Sizes()
{
    cout << "Determining sizeof() important types that are used throughout:" << endl
         << "iWorkUnit: " << sizeof(iWorkUnit) << endl
         << "DefaultWorkUnit: " << sizeof(DefaultWorkUnit) << endl
         << "WorkUnitKey: " << sizeof(WorkUnitKey) << endl
         << "DefaultRollingAverage<Whole>::Type: " << sizeof(DefaultRollingAverage<Whole>::Type) << endl
         << "WeightedRollingAverage<Whole,Whole>: " << sizeof(WeightedRollingAverage<Whole,Whole>) << endl
         << "BufferedRollingAverage<Whole>: " << sizeof(BufferedRollingAverage<Whole>) << endl
         << "WorkUnitMonpoly: " << sizeof(MonopolyWorkUnit) << endl
         << "DefaultThreadSpecificStorage::Type: " << sizeof(DefaultThreadSpecificStorage::Type) << endl
         << "FrameScheduler: " << sizeof(FrameScheduler) << endl
         << "WorkSorter: " << sizeof(WorkSorter) << endl
         << "thread: " << sizeof(Thread) << endl
         << "mutex: " << sizeof(Mutex) << endl
         << "Barrier: " << sizeof(Barrier) << endl
         << "vector<Whole>: " << sizeof(vector<Whole>) << endl
         << "vector<WorkUnit*>: " << sizeof(vector<Whole*>) << endl
         << "set<WorkUnit*>: " << sizeof(set<iWorkUnit*>) << endl
         << "std::vector<WorkUnitKey>::reverse_iterator: " << sizeof(std::vector<WorkUnitKey>::reverse_iterator) << endl
         << "ostream*: " << sizeof(ostream*) << endl
         << "MaxInt: " << sizeof(MaxInt) << endl
         << "Whole: " << sizeof(Whole) << endl << endl;
}

/// @brief A test to try to infer cache of CPU
void InferCacheSize()
{
    Whole TestCount = 8000;
    MaxInt VecCreateTime = 0;
    MaxInt VecWorstCaseCreateTime = 0;
    MaxInt VecLookTime = 0;
    MaxInt MapCreateTime = 0;
    MaxInt MapLookTime = 0;
    Whole DefeatCompilerOptimizer=0;

    MaxInt StartTime = GetTimeStamp();
    vector<Whole> RandomVec;
    for(Whole Counter=0; Counter<TestCount; Counter++)
        { RandomVec.push_back(Dice2D3()); }
    VecCreateTime = GetTimeStamp()-StartTime;
    DefeatCompilerOptimizer=RandomVec[rand()%TestCount];
    cout << "Output to defeat compiler optimization: " << DefeatCompilerOptimizer << endl;
    cout << "Vector creation time: " << VecCreateTime << endl;
    RandomVec.clear();

    StartTime = GetTimeStamp();
    for(Whole Counter=0; Counter<TestCount; Counter++)
        { RandomVec.insert(RandomVec.begin(), Dice2D3()); }
    VecWorstCaseCreateTime = GetTimeStamp()-StartTime;
    cout << "Vector worst create time: " << VecWorstCaseCreateTime << endl;

    StartTime = GetTimeStamp();
    for(Whole Counter=0; Counter<TestCount; Counter++)
        { DefeatCompilerOptimizer=RandomVec[rand()%TestCount]; }
    VecLookTime = GetTimeStamp()-StartTime;
    cout << "Vector rand lookup time: " << VecLookTime << endl;
    cout << "Output to defeat compiler optimization: " << DefeatCompilerOptimizer << endl;

    StartTime = GetTimeStamp();
    map<Whole,Whole> RandomMap;
    for(Whole Counter=0; Counter<TestCount; Counter++)
        { RandomMap[Counter] = Dice2D3(); }
    MapCreateTime = GetTimeStamp()-StartTime;
    DefeatCompilerOptimizer=RandomVec[rand()%TestCount];
    cout << "Map creation time: " << MapCreateTime << endl;
    cout << "Output to defeat compiler optimization: " << DefeatCompilerOptimizer << endl;

    StartTime = GetTimeStamp();
    for(Whole Counter=0; Counter<TestCount; Counter++)
        { DefeatCompilerOptimizer=RandomMap[rand()%TestCount]; }
    MapLookTime = GetTimeStamp()-StartTime;
    cout << "Map rand lookup time: " << MapLookTime << endl;
    cout << "Output to defeat compiler optimization: " << DefeatCompilerOptimizer << endl;

    cout << "Total Vector time: " << VecCreateTime+VecWorstCaseCreateTime+VecLookTime << " - Total Map Time " << MapCreateTime+MapLookTime << endl;
    cout << endl << endl;
}


/// @brief The 'untestable' Test.
/// @note This test never fails but can be verified manually if it seems something is wrong.
void Untestable()
{
    cout << "Displaying Output of untestable functions. There is no way to have known when this was written, what the results of these would be:" << endl
         << "The current time in microseconds GetTimeStamp(): " << GetTimeStamp() << endl
         << "What is the smallest amount of time the clock can measure in microseconds GetTimeStampResolution(): " << GetTimeStampResolution() << endl
         << "Current Logical Processor Count GetCPUCount(): " << GetCPUCount() << endl
         << "|Currently Compiled Threading model: "
        #ifdef MEZZ_USEBARRIERSEACHFRAME
            << "Barriers used to absolutely minimize thread creation." << endl
        #else
            << "Threads created and joined each frame." << endl
        #endif
            << "|Compilation Type: "
        #ifdef MEZZ_DEBUG
            << "Debug." << endl
        #else
            << "Release." << endl
        #endif
         << "Default length to track frames: " << MEZZ_FRAMESTOTRACK << endl



//         << "Cache size available before using RAM GetCacheSize(): " << GetCacheSize() << endl;
//         << "Size of one entry in the fastest cache GetCachelineSize(): " << GetCachelineSize() << endl;
            ;
}



////////////////////////////////////////////////////////////////////////////////////////////////
// Basic functionality test

/// @brief Used in @ref BasicThreading
void PrintHello(void*)
    { cout << "Hello from thread T1 with id: " << Mezzanine::Threading::this_thread::get_id() << endl; }

/// @brief The 'basicthreading' Test.
/// @details One of a group of tests that check the compatibility wrapper around OS functionality.
/// This checks that the Thread wrapper works
void BasicThreading()
{
    cout << "Testing Basic Thread functionality." << endl;
    cout << "This Threads id: " <<  Mezzanine::Threading::this_thread::get_id() << endl;

    cout << "Creating a thread with identifier T1 and unkown id." << endl;
    Mezzanine::Threading::Thread T1(PrintHello);
    cout << "T1 should have an id of: " << T1.get_id() << endl;

    cout << "Is T1 joinable: " << T1.joinable() << endl;
    cout << "Joining T1" << endl;
    T1.join();
    cout << "Is T1 joinable: " << T1.joinable() << endl;

    cout << "Sleeping main thread for 300ms." << endl;
    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Yielding thread to OS scheduler." << endl;
    Mezzanine::Threading::this_thread::yield();
}

/// @brief Used in @ref BasicMutex
static Mezzanine::Threading::Thread::id ThreadIDTest=0;
/// @brief Used in @ref BasicMutex
static Mezzanine::Threading::Mutex ThreadIDLock;
/// @brief Used in @ref BasicMutex
void PutIdInGlobal(void*)
{
    cout << "Thread T2 trying to lock mutex ThreadIDLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadIDLock.Lock();
    cout << "Thread T2 locked mutex: " << endl;
    ThreadIDTest = Mezzanine::Threading::this_thread::get_id();
    cout << "Thread T2 work complete unlocking mutex: " << endl;
    ThreadIDLock.Unlock();
}

/// @brief The 'basicmutex' Test.
/// @details One of a group of tests that check the compatibility wrapper around OS functionality. This
/// checks that the mutex wrapper works
void BasicMutex()
{
    cout << "Testing basic mutex functionality" << endl;
    cout << "Locking ThreadIDLock in thread: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadIDLock.Lock();

    cout << "Creating a thread with identifier T2 and unkown id." << endl;
    Mezzanine::Threading::Thread T2(PutIdInGlobal);

    cout << "Storing T2's id: " << T2.get_id() << endl;
    cout << "Unlocking ThreadIDLock from main and sleeping for 300 ms." << endl;
    Mezzanine::Threading::Thread::id T2id = T2.get_id();
    ThreadIDLock.Unlock();
    Mezzanine::Threading::this_thread::sleep_for(300000);

    ThreadIDLock.Lock();
    cout << "Does the thread report the same ID as we gathered: " << (ThreadIDTest == T2id) << endl;
    ThrowOnFalse(ThreadIDTest == T2id,"Does the thread report the same ID as we gathered");
    ThreadIDLock.Unlock();

    cout << "Joining T2" << endl;
    T2.join();
}

/// @brief Used in @ref BasicThreadingPassing
static Mezzanine::Integer ThreadPassTest=0;
/// @brief Used in @ref BasicThreadingPassing
static Mezzanine::Threading::Mutex ThreadPassLock;
/// @brief Used in @ref BasicThreadingPassing
/// @param Value The functionality being tested is that is passed correctly
void SquareInThread(void* Value)
{
    cout << "Thread T3 waiting for lock on mutex ThreadPassLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadPassLock.Lock();
    cout << "Thread T3 locked mutex: " << endl;
    ThreadPassTest = *(Mezzanine::Integer*)Value * *(Mezzanine::Integer*)Value;
    cout << "Thread T3 work complete unlocking mutex: " << endl;
    ThreadPassLock.Unlock();
}

/// @brief The 'basicthreadingpassing' Test.
/// @details One of a group of tests that check the compatibility wrapper around OS functionality. This
/// checks that data passed into a thread makes alright
void BasicThreadingPassing()
{
    cout << "Testing passing to thread functionality" << endl;
    cout << "Locking ThreadPassLock in thread: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadPassLock.Lock();

    cout << "Creating a thread with identifier T3 and unkown id." << endl;
    Mezzanine::Integer Value = 9;
    cout << "Passing " << Value << " into thread T3." << endl;
    Mezzanine::Threading::Thread T3(SquareInThread, &Value);

    cout << "Unlocking ThreadPassLock from main and sleeping for 300 ms." << endl;
    ThreadPassLock.Unlock();
    Mezzanine::Threading::this_thread::sleep_for(300000);

    ThreadPassLock.Lock();
    cout << "Thread gives us: " << ThreadPassTest << endl;
    cout << "Does the thread give us the square of what we passed it: " << (Value*Value == ThreadPassTest) << endl;
    ThrowOnFalse(Value*Value == ThreadPassTest,"Does the thread give us the square of what we passed it");
    ThreadPassLock.Unlock();

    cout << "Joining T3" << endl;
    T3.join();
}

/// @brief Used in @ref BasicMutexTry
static Mezzanine::Integer TryLockTest=0;
/// @brief Used in @ref BasicMutexTry
static Mezzanine::Threading::Mutex TryLock;
/// @brief Used in @ref BasicMutexTry
/// @param Value This is the a value passed into the thread to confirm that it works
void TryToSquareInThread(void* Value)
{
    cout << "Thread T4 trying to lock mutex ThreadPassLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    if (TryLock.TryLock())
    {
        cout << "Thread T4 locked mutex, Squaring the value " << endl;
        TryLockTest = *(Mezzanine::Integer*)Value * *(Mezzanine::Integer*)Value;
        TryLock.Unlock();
    }else{
        cout << "Thread T4 could not acquire lock, no work done" << endl;
    }
}

/// @brief The 'basicthreadingpassing' Test.
/// @details One of a group of tests that check the compatibility wrapper around OS functionality. This
/// checks that data passed into a thread makes alright
void BasicMutexTry()
{
   cout << "Testing Mutex try_lock()" << endl;

    cout << "Locking TryLock in main thread with id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThrowOnFalse(TryLock.TryLock(),"Locking TryLock in main thread");

    Mezzanine::Integer Value = 9;
    cout << "Creating a thread with identifier T4 and unkown id." << endl;
    cout << "Passing " << Value << " into thread T4, and assigning to output and waiting 200ms." << endl;
    TryLockTest = Value;
    Mezzanine::Threading::Thread T4(TryToSquareInThread, &Value);

    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Joining T4" << endl;
    T4.join();

    cout << "Unlocking TryLock." << endl;
    TryLock.Unlock();
    cout << "Value from thread's return point is " << TryLockTest << " it should be " << Value << " if it wasn't able to get mutex" << endl;
    cout << "Did T4 not get the mutex and proceed past mutex as expected: " << (TryLockTest == Value) << endl;
    ThrowOnFalse(TryLockTest == Value,"Did T4 not get the mutex and proceed past mutex as expected");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// rolling average tests

/// @brief The 'rollingaverage' Test.
void RollingAverageTests()
{
    cout << "Starting Rolling Average Tests" << endl;
    cout << "Is the Default Rolling Average the BufferedRollingAverage: " << (typeid(Mezzanine::DefaultRollingAverage<Mezzanine::Whole>::Type)==typeid(Mezzanine::BufferedRollingAverage<Mezzanine::Whole>)) << endl;
    cout << "Is the Default Rolling Average the BufferedRollingAverage: " << (typeid(Mezzanine::DefaultRollingAverage<Mezzanine::Whole>::Type)==typeid(Mezzanine::WeightedRollingAverage<Mezzanine::Whole,float>)) << endl;
    cout << "Creating a BufferedRollingAverage, WeightedRollingAverage and DefaultRollingAverage with Mezzanine::Whole (and sometimes float for math)" << endl;
    Mezzanine::BufferedRollingAverage<Mezzanine::Whole> RollingB(10);
    Mezzanine::WeightedRollingAverage<Mezzanine::Whole,float> RollingW(10);
    Mezzanine::DefaultRollingAverage<Mezzanine::Whole>::Type RollingD(10);

    cout << "Inserting into each: ";
    for(Mezzanine::Whole Counter=1; Counter<=20; Counter++)
    {
        cout << Counter << (20==Counter?".":", ");
        RollingB.Insert(Counter);
        RollingW.Insert(Counter);
        RollingD.Insert(Counter);
    }
    cout << endl;

    cout << "BufferedRollingAverage Result, should be 15: " << RollingB.GetAverage() << endl;
    ThrowOnFalse(RollingB.GetAverage()==15,"BufferedRollingAverage Result, should be 15");
    cout << "WeightedRollingAverage Result, should be about 10: " << RollingW.GetAverage() << endl;
    ThrowOnFalse(RollingW.GetAverage()>9||RollingW.GetAverage()<16,"WeightedRollingAverage Result, should be about 10");
    cout << "DefaultRollingAverage Result, should match its underlying type : " << RollingD.GetAverage() << endl;

    cout << "Creating a BufferedRollingAverage, WeightedRollingAverage and DefaultRollingAverage with floats" << endl;
    Mezzanine::BufferedRollingAverage<float> RollingB2(10);
    Mezzanine::WeightedRollingAverage<float,float> RollingW2(10);
    Mezzanine::DefaultRollingAverage<float>::Type RollingD2(10);

    cout << "Inserting into each: ";
    for(float Counter=1.0; Counter<=20.0; Counter++)
    {
        cout << Counter << (20.0<=Counter?".":", ");
        RollingB2.Insert(Counter);
        RollingW2.Insert(Counter);
        RollingD2.Insert(Counter);
    }
    cout << endl;

    cout << "BufferedRollingAverage Result, should be ~15.5: " << RollingB2.GetAverage() << endl;
    ThrowOnFalse(RollingB2.GetAverage()>15.4 && RollingB2.GetAverage()<15.6,"BufferedRollingAverage Result, should be ~15.5");
    cout << "WeightedRollingAverage Result, should be ~12.2158: " << RollingW2.GetAverage() << endl;
    ThrowOnFalse(RollingW2.GetAverage()>12.1 && RollingW2.GetAverage()<12.3,"WeightedRollingAverage Result, should be ~12.2158");
    cout << "DefaultRollingAverage Result, should match its underlying type : " << RollingD2.GetAverage() << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Timestamp Tests

/// @brief The 'timestamp' Test.
void TimeStamp()
{
    cout << "Starting timekeeping tests." << endl;
    cout << "Getting Timestamp1" << endl;
    Mezzanine::MaxInt Timestamp1 = Mezzanine::GetTimeStamp();

    cout << "Sleeping main thread for 300ms." << endl;
    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Getting Timestamp2" << endl;
    Mezzanine::MaxInt Timestamp2 = Mezzanine::GetTimeStamp();

    cout << "Timestamp1: " << Timestamp1 << endl;
    cout << "Timestamp2: " << Timestamp2 << endl;
    cout << "imestamp2 - Timestamp1 = " << Timestamp2-Timestamp1 << endl;
    cout << "Is Timestamp1 <= Timestamp2: " << (Timestamp1<=Timestamp2) << endl;
    cout << "Timer Resolution: " << GetTimeStampResolution() << " microsecond(s)" << endl;
    ThrowOnFalse((Timestamp1<=Timestamp2),"Is Timestamp1 <= Timestamp2");
    cout << "Is Timestamp1+300000-(2*TimerResolution) <= Timestamp2 = " << Timestamp1+300000-(2*GetTimeStampResolution()) << "<=" << Timestamp2 << endl;
    cout << "Is Timestamp1+300000-(2*TimerResolution) <= Timestamp2: " << (Timestamp1+300000-(2*GetTimeStampResolution())<=Timestamp2) << endl;
    ThrowOnFalse((Timestamp1+300000-(2*GetTimeStampResolution()))<=Timestamp2,"Is Timestamp1+300000-(2*TimerResolution) <= Timestamp2");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// WorkUnit tests

/// @brief An intentionally large and slow to process type chosen once here, to allow easy adjusting of these tests.
typedef long double PreciseFloat;


/// @brief A very inefficient way to calculate Pi with some spikes in execution time
/// @param Length How many iteration to use when calculating Pi
/// @param Spike if not 0 then this function will 1 in twentys increase in execution time by between 2 and 6 times.
/// @return Pi To varying degrees of precision
PreciseFloat MakePi(Mezzanine::Whole Length, Whole Spike = 0)
{
    if(Spike!=0 && 10==DiceD20())          // So one in twenty times
    {
        Spike=Dice2D3();        // Execution will take 2 to 6 times longer
        //* LocalLog << "<Bonuswork Multiple=\"" << Spike << "\" />" << endl;
    } else {
        Spike=1;
    }

    PreciseFloat Pi = 1.0;
    PreciseFloat Taylor = 3;
    for (Whole Count = 0; Count<(Length*Spike); ++Count)
    {
        Pi-=PreciseFloat(1.0)/Taylor;
        Taylor+=PreciseFloat(2.0);
        Pi+=PreciseFloat(1.0)/Taylor;
        Taylor+=PreciseFloat(2.0);
    }
    return PreciseFloat(4.0) * Pi;
}

/// @brief A samplework unit that calculates pi
/// @brief Used in @ref WorkUnitTests and other tests that require actual cpu consumption
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PiMakerWorkUnit : public Mezzanine::Threading::DefaultWorkUnit
{
    public:
        /// @brief How many iterations will we use when calculating Pi
        Mezzanine::Whole Length;

        /// @brief Name used for clarifying output and enhancing logging.
        Mezzanine::String Name;

        /// @brief should this randomly spike execution time.
        bool SpikesOn;

        /// @brief Create on of these workunit
        /// @param Length_ Defaults to 50 and is the number of iterations to use, more means slower execution
        /// @param Name Defaults to "Default" only used in output logs
        /// @param SpikesOn_ Defaults to true, used to make execution more dificult to predict
        PiMakerWorkUnit(Mezzanine::Whole Length_ = 50, Mezzanine::String Name_ = "Default", bool SpikesOn_=true)
            : Length(Length_), Name(Name_), SpikesOn(SpikesOn_)
            { }

        /// @brief Empty Virtual Deconstructor
        virtual ~PiMakerWorkUnit()
            { }

        /// @brief Calculate Pi and log it.
        /// @paramCurrentThreadStorage used to retrieve a valid logger.
        /// @brief CurrentFrameScheduler ignored
        virtual void DoWork(DefaultThreadSpecificStorage::Type& CurrentThreadStorage)
        {
            DoubleBufferedLogger& CurrentLogger = CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger);
            CurrentLogger.GetUsable() << "<MakePi Pi=\"" << MakePi(Length,SpikesOn) << "\" WorkUnitName=\"" << Name << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << endl;
        }
};


/// @brief The 'workunit' Test.
void WorkUnitTests()
{
    cout << "Starting WorkUnit Tests, 20 runs with WorkUnitSample1" << endl;
    PiMakerWorkUnit WorkUnitSample1(5000,"WorkUnitSample1",false);
    FrameScheduler TestScheduler(&cout,1);
    Mezzanine::Threading::DefaultThreadSpecificStorage::Type TestThreadStorage(&TestScheduler);
    // run work unit
    for(Whole Counter=0; Counter<20; Counter++)
        { WorkUnitSample1(TestThreadStorage); }
    cout << "Here is the complete log of Twenty Test Runs" << endl
         << TestThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable().str() // << endl // logs ends with a newline
         << "Average Execution Time (Microseconds): " << WorkUnitSample1.GetPerformanceLog().GetAverage() << endl;

    cout << endl << "Starting WorkUnit Dependent and Dependency count Tests. Creating a chain in which C depends on B which depends on A."
         << endl << "C --> B --> A" << endl;
    PiMakerWorkUnit* WorkUnitA = new PiMakerWorkUnit(50,"A",false);
    PiMakerWorkUnit* WorkUnitB = new PiMakerWorkUnit(50,"B",false);
    PiMakerWorkUnit* WorkUnitC = new PiMakerWorkUnit(50,"C",false);
    WorkUnitC->AddDependency(WorkUnitB);
    WorkUnitB->AddDependency(WorkUnitA);
    TestScheduler.AddWorkUnit(WorkUnitA);
    TestScheduler.AddWorkUnit(WorkUnitB);
    TestScheduler.AddWorkUnit(WorkUnitC);
    TestScheduler.UpdateDependentGraph();
    cout << "A dependency count: " << WorkUnitA->GetDependencyCount() << " \t A dependent count: " << WorkUnitA->GetDependentCount(TestScheduler) << endl;
    cout << "B dependency count: " << WorkUnitB->GetDependencyCount() << " \t B dependent count: " << WorkUnitB->GetDependentCount(TestScheduler) << endl;
    cout << "C dependency count: " << WorkUnitC->GetDependencyCount() << " \t C dependent count: " << WorkUnitC->GetDependentCount(TestScheduler) << endl;
    ThrowOnFalse(WorkUnitA->GetDependencyCount()==0, "A dependency count");
    ThrowOnFalse(WorkUnitA->GetDependentCount(TestScheduler)==2, "A dependent count");
    ThrowOnFalse(WorkUnitB->GetDependencyCount()==1, "B dependency count");
    ThrowOnFalse(WorkUnitB->GetDependentCount(TestScheduler)==1, "B dependent count");
    ThrowOnFalse(WorkUnitC->GetDependencyCount()==2, "C dependency count");
    ThrowOnFalse(WorkUnitC->GetDependentCount(TestScheduler)==0, "C dependent count");
    cout << "Creating a WorkUnit D which depends on B, So we should have:"
            << endl << "D --"
            << endl << "   |"
            << endl << "   v"
            << endl << "   B --> A"
            << endl << "   ^"
            << endl << "   |"
            << endl << "C --" << endl;
    PiMakerWorkUnit* WorkUnitD = new PiMakerWorkUnit(50,"D",false);
    WorkUnitD->AddDependency(WorkUnitB);
    TestScheduler.AddWorkUnit(WorkUnitD);
    TestScheduler.UpdateDependentGraph();
    cout << "A dependency count: " << WorkUnitA->GetDependencyCount() << " \t A dependent count: " << WorkUnitA->GetDependentCount(TestScheduler) << endl;
    cout << "B dependency count: " << WorkUnitB->GetDependencyCount() << " \t B dependent count: " << WorkUnitB->GetDependentCount(TestScheduler) << endl;
    cout << "C dependency count: " << WorkUnitC->GetDependencyCount() << " \t C dependent count: " << WorkUnitC->GetDependentCount(TestScheduler) << endl;
    cout << "D dependency count: " << WorkUnitD->GetDependencyCount() << " \t D dependent count: " << WorkUnitD->GetDependentCount(TestScheduler) << endl;
    ThrowOnFalse(WorkUnitA->GetDependencyCount()==0,"A dependency count");
    ThrowOnFalse(WorkUnitA->GetDependentCount(TestScheduler)==3,"A dependent count");
    ThrowOnFalse(WorkUnitB->GetDependencyCount()==1,"B dependency count");
    ThrowOnFalse(WorkUnitB->GetDependentCount(TestScheduler)==2,"A dependent count");
    ThrowOnFalse(WorkUnitC->GetDependencyCount()==2,"C dependency count");
    ThrowOnFalse(WorkUnitC->GetDependentCount(TestScheduler)==0,"A dependent count");
    ThrowOnFalse(WorkUnitD->GetDependencyCount()==2,"D dependency count");
    ThrowOnFalse(WorkUnitD->GetDependentCount(TestScheduler)==0,"A dependent count");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Monopoly Tests

class PiMakerMonopoly;

/// @brief Used to pass and accept data from the Pi calculating function with multiple threads.
/// @details Used in @ref MonopolyTest
/// @param PiMakerMonopoly_ a PiMakerMonopoly void pointer that provides all the required meta data for its execution
void PiMakerMonopolyHelper(void* PiMakerMonopoly_);

/// @brief The data a single thread in the Pi make monopoly needs.
struct PiMakerThreadData
{
    /// @brief A pointer to the one Pi maker monopoly the target thread cares about
    PiMakerMonopoly *Maker;

    /// @brief The Resources the target thread should use.
    DefaultThreadSpecificStorage::Type *Storage;

    /// @brief constructor
    /// @param Maker_ A pointer to a PiMakerMonopoly that the targe
    explicit PiMakerThreadData(PiMakerMonopoly *Maker_) : Maker(Maker_), Storage(new DefaultThreadSpecificStorage::Type(0)) // This is not exception safe, if we ran out of memory here this could leak.1
        {}

    /// @brief Deletes allocated data.
    ~PiMakerThreadData()
        { delete Storage; }
};

/// @brief PiMakerMonopoly Consume every thread for a brief period to calculate Pi.
/// @details Used in @ref MonopolyTest
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PiMakerMonopoly : public MonopolyWorkUnit
{
    public:
        /// @brief How many iterations will we use when calculating Pi
        Mezzanine::Whole Length;

        /// @brief Name used for clarifying output and enhancing logging.
        Mezzanine::String Name;

        /// @brief should this randomly spike execution time.
        bool SpikesOn;

        /// @brief  Count of thread to execute in
        Mezzanine::Whole HowManyThreads;

        /// @brief A way to allow workers to have access to thread specific resources
        FrameScheduler* Scheduler;

        /// @brief Set the amount of threads to use
        /// @param AmountToUse A whole indicating the amount of times Pi should uselessly be calculated in parrallel
        virtual void UseThreads(const Whole& AmountToUse)
            { HowManyThreads = AmountToUse; }

        /// @brief Get the current thread count
        /// @return A Whole
        virtual Whole UsingThreadCount()
            { return HowManyThreads; }

        /// @brief Constructor
        /// @param Length_ Defaults to 50 and is the number of iterations to use, more means slower execution
        /// @param Name Defaults to "Default" only used in output logs
        /// @param SpikesOn_ Defaults to true, used to make execution more dificult to predict
        /// @param AmountToUse Defaults to 1
        PiMakerMonopoly(Mezzanine::Whole Length_ = 50, Mezzanine::String Name_ = "Default", bool SpikesOn_=true, Whole AmountToUse = 1)
            : Length(Length_), Name(Name_), SpikesOn(SpikesOn_), HowManyThreads(AmountToUse)
        { }

        /// @brief Spawns several the amount of threads indicated by HowManyThreads then calculates Pi in each and logs teh results
        /// @param CurrentFrameScheduler
        virtual void DoWork(DefaultThreadSpecificStorage::Type& CurrentThreadStorage)
        {
            Scheduler = CurrentThreadStorage.GetFrameScheduler();
            vector<Thread*> ThreadIndex;
            vector<PiMakerThreadData*> ThreadData;
            for (Whole Count=0; Count<HowManyThreads; ++Count)      // Pretend making all this Pi simulates everyone in a Bakery baking at at once as hard as they can
            {
                PiMakerThreadData* Data = new PiMakerThreadData(this);
                ThreadData.push_back( Data );
                ThreadIndex.push_back( new Mezzanine::Threading::Thread (PiMakerMonopolyHelper, Data) );
            }
            for (Whole Count=0; Count<HowManyThreads; ++Count)
            {
                ThreadIndex[Count]->join();
                delete ThreadIndex[Count];
                delete ThreadData[Count];
            }
            Scheduler = 0;
        }
};

// Documented above at prototype line.
void PiMakerMonopolyHelper(void* Data)
{
    PiMakerThreadData* D = (PiMakerThreadData*)Data;
    PiMakerMonopoly* Mono = D->Maker;
    Logger& CurrentLog = D->Storage->GetUsableLogger();
    CurrentLog << "<PiMakerMonopolyHelper Pi=\"" << MakePi(Mono->Length,Mono->SpikesOn) << "\" WorkUnitName=\"" << Mono->Name<< "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << endl;
    MakePi(Mono->Length);
}

/// @brief The 'monopoly' Test. A smoke test for the monopoly
/// @todo intercept the log in the MonopolyTest and check that the log is good.
void MonopolyTest()
{
    cout << "Starting MonopolyWorkUnit test. Creating a monopoly that will calculate pi in a number of threads simultaneously." << endl;
    PiMakerMonopoly Pioply(50,"Pioply",false,4);
    FrameScheduler TestSchedulerMono(&cout,1);
    DefaultThreadSpecificStorage::Type PioplyStorage(&TestSchedulerMono);
    for(Whole Counter=0; Counter<20; Counter++)
        { Pioply(PioplyStorage); }
    cout << "Here is the un-aggregated (main thread only) log of Twenty Test Runs" << endl
         << PioplyStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable().str() // << endl // logs ends with a newline
         << "Average Execution Time (Microseconds): " << Pioply.GetPerformanceLog().GetAverage() << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Test for the logger workunits

/// @brief The 'logaggregator' Test. A smoke test for the monopoly
/// @todo intercept the log in the LogAggregatorTests and check that the log is good.
void LogAggregatorTests()
{
    cout << endl << "Creating a FrameScheduler with a monopoly Testing the logger workunits to get a handle on the monopolies logs, logging to cout: " << endl;
    PiMakerMonopoly Pioply(50,"Pioply",false,4);
    FrameScheduler TestSchedulerMono(&cout,1);
    DefaultThreadSpecificStorage::Type PioplyStorage(&TestSchedulerMono);
    for(Whole Counter=0; Counter<20; Counter++)
        { Pioply(PioplyStorage); }
    LogBufferSwapper Swapper;
    ThreadSpecificStorage SwapResource(&TestSchedulerMono);
    Swapper(SwapResource);
    LogAggregator Agg;
    Agg(SwapResource);
    cout << "Large log should have been emitted that showed PI being calculated 80 times and which thread it was calculated in. 20 iterations should have occurred in the main thread, and the rest each in fresh threads." << endl;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Tests for the WorkUnitKey

/// @brief The 'workunitkey' Test. A smoke test for the monopoly
void WorkUnitKeyTests()
{
    cout << "Creating a number of WorkUnitKeys and tesing their ability to sort: " << endl;
    cout << "\t" << "                   Deps, Time, Ptr" << endl;
    cout << "\t" << "WorkUnitKey First(   10, 500,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Second(   5, 600,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Third(    5, 500,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Fourth(   3, 500,  1  );" << endl;
    cout << "\t" << "WorkUnitKey Fifth(    3, 500,  0  );" << endl;
    WorkUnitKey First(10,500,0);
    WorkUnitKey Second(5,600,0);
    WorkUnitKey Third(5,500,0);
    WorkUnitKey Fourth(3,500,(iWorkUnit*)1);
    WorkUnitKey Fifth(3,500,0);

    cout << "Second<First: " << (Second < First) << "\t Third<First: " << (Third < First) << "\t Fourth<First: " << (Fourth < First) << "\t Fifth<First: " << (Fifth < First) << endl;
    ThrowOnFalse(Second < First,"Second < First");
    ThrowOnFalse(Third < First,"Third < First");
    ThrowOnFalse(Fourth < First,"Fourth < First");
    ThrowOnFalse(Fifth < First,"Fifth < First");
    cout << "Third<Second: " << (Third < Second) << "\t Fourth<Second: " << (Fourth < Second) << "\t Fifth<Second: " << (Third < Second) << endl;
    ThrowOnFalse(Third < Second,"Third < Second");
    ThrowOnFalse(Fourth < Second,"Fourth < Second");
    ThrowOnFalse(Fifth < Second,"Fifth < Second");
    cout << "Fourth<Third: " << (Fourth < Third) << "\t Fifth<Third: " << (Fifth < Third) << endl;
    ThrowOnFalse(Fourth < Third,"Fourth < Third");
    ThrowOnFalse(Fifth < Third,"Fifth < Third");
    cout << "Fifth<Fourth: " << (Fifth < Fourth) << endl;
    ThrowOnFalse(Fifth < Fourth,"Fifth < Fourth");

    cout << "First<Second: " << (First < Second) << "\t First<Third: " << (First < Third) << "\t First<Fourth: " << (First < Fourth) << "\t First<Fifth: " << (First < Fifth) << endl;
    ThrowOnFalse(!(First < Second),"!(First < Second)");
    ThrowOnFalse(!(First < Third),"!(First < Third)");
    ThrowOnFalse(!(First < Fourth),"!(First < Fourth)");
    ThrowOnFalse(!(First < Fifth),"!(First < Fifth)");
    cout << "Second<Third: " << (Second < Third) << "\t Second<Fourth: " << (Second < Fourth) << "\t Second<Fifth: " << (Second < Fifth) << endl;
    ThrowOnFalse(!(Second < Third),"!(Second < Third)");
    ThrowOnFalse(!(Second < Fourth),"!(Second < Fourth)");
    ThrowOnFalse(!(Second < Fifth),"!(Second < Fifth)");
    cout << "Third<Fourth: " << (Third < Fourth) << "\t Third<Fifth: " << (Third < Fifth) << endl;
    ThrowOnFalse(!(Third < Fourth),"!(Third < Fourth)");
    ThrowOnFalse(!(Third < Fifth),"!(Third < Fifth)");
    cout << "Fourth<Fifth: " << (Fourth<Fifth) << endl;
    ThrowOnFalse(!(Fourth<Fifth),"!(Fourth<Fifth)");

    cout << "Creating 4 WorkUnits for a sorting test with an set (be the only differrence between fourth and fifth was the address of the workunit, and we cannot control that.):" << endl;
    PiMakerWorkUnit *WorkUnitK1 = new PiMakerWorkUnit(500,"First",false);
    PiMakerWorkUnit *WorkUnitK2 = new PiMakerWorkUnit(500,"Second",false);
    PiMakerWorkUnit *WorkUnitK3 = new PiMakerWorkUnit(500,"Third",false);
    PiMakerWorkUnit *WorkUnitK4 = new PiMakerWorkUnit(500,"Fourth",false);
    First.Unit=WorkUnitK1;
    Second.Unit=WorkUnitK2;
    Third.Unit=WorkUnitK3;
    Fourth.Unit=WorkUnitK4;
    set<WorkUnitKey> WorkUnitKeyTest;
    WorkUnitKeyTest.insert(Second);
    WorkUnitKeyTest.insert(Fourth);
    WorkUnitKeyTest.insert(Third);
    WorkUnitKeyTest.insert(First);
    set<WorkUnitKey>::reverse_iterator Iter=WorkUnitKeyTest.rbegin();
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    ThrowOnFalse( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("First"), "Unit.Name==First" );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    ThrowOnFalse( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Second"), "Unit.Name==Second" );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    ThrowOnFalse( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Third"), "Unit.Name==Third" );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    ThrowOnFalse( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Fourth"), "Unit.Name==Fourth" );
    Iter++;
    cout << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Tests for the Framescheduler getting the next WorkUnit.


/// @brief A samplework unit that that just block the thread it is in
/// @details Used in @ref FrameSchedulerGetNext and other tests
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PausesWorkUnit : public DefaultWorkUnit
{
    public:
        /// @brief How many milliseconds should this thread block for.
        Mezzanine::Whole Length;

        /// @brief Name used for clarifying output and enhancing logging.
        Mezzanine::String Name;

        /// @brief Create one of these workunit
        /// @param Length_ Defaults to 50 and is the number of Milliseconds to wait
        /// @param Name Defaults to "Default" only used in output logs
        PausesWorkUnit(Mezzanine::Whole Length_ = 50, Mezzanine::String Name_ = "Default")
            : Length(Length_), Name(Name_)
            { }

        /// @brief Empty Virtual Deconstructor
        virtual ~PausesWorkUnit()
            { }

        /// @brief Wait and log it.
        /// @param CurrentThreadStorage used to retrieve a valid logger.
        /// @brief CurrentFrameScheduler ignored
        virtual void DoWork(DefaultThreadSpecificStorage::Type& CurrentThreadStorage)
        {
            DoubleBufferedLogger& CurrentLogger = CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger);
            CurrentLogger.GetUsable() << "<Pause PauseLength=\"" << Length << "\" WorkUnitName=\"" << Name << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << endl;
            Mezzanine::Threading::this_thread::sleep_for(Length);
        }
};

/// @brief The 'frameschedulergetnext' Test. A smoke test for the monopoly
void FrameSchedulerGetNext()
{
    cout <<  "Creating a simple dependency chain in 4 WorkUnits and inserting them into a Test FrameScheduler. Then they will be pulled out one at a time and mark them as completed: " << endl;

    PiMakerWorkUnit *WorkUnitK1 = new PiMakerWorkUnit(500,"First",false);
    PiMakerWorkUnit *WorkUnitK2 = new PiMakerWorkUnit(500,"Second",false);
    PiMakerWorkUnit *WorkUnitK3 = new PiMakerWorkUnit(500,"Third",false);
    PiMakerWorkUnit *WorkUnitK4 = new PiMakerWorkUnit(500,"Fourth",false);

    FrameScheduler SchedulingTest1(&cout,1);
    DefaultThreadSpecificStorage::Type Storage1(&SchedulingTest1);
    WorkUnitK4->AddDependency(WorkUnitK3);
    WorkUnitK3->AddDependency(WorkUnitK2);
    WorkUnitK2->AddDependency(WorkUnitK1);
    SchedulingTest1.AddWorkUnit(WorkUnitK1); // no deletes required the Scheduler takes ownership
    SchedulingTest1.AddWorkUnit(WorkUnitK2);
    SchedulingTest1.AddWorkUnit(WorkUnitK3);
    SchedulingTest1.AddWorkUnit(WorkUnitK4);
    SchedulingTest1.SortWorkUnitsMain();

    iWorkUnit* Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PiMakerWorkUnit*)Counter)->Name == String("First"), "Getting the WorkUnit Named First" );
    Counter->operator()(Storage1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PiMakerWorkUnit*)Counter)->Name == String("Second"), "Getting the WorkUnit Named Second" );
    Counter->operator()(Storage1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PiMakerWorkUnit*)Counter)->Name == String("Third"), "Getting the WorkUnit Named Third" );
    Counter->operator()(Storage1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PiMakerWorkUnit*)Counter)->Name == String("Fourth"), "Getting the WorkUnit Named Fourth" );
    Counter->operator()(Storage1);

    cout << endl << "Creating 3 WorkUnits with precise runtimes and inserting them into a Test FrameScheduler. Then they will be pulled out one at a time and mark them as completed: " << endl;
    FrameScheduler SchedulingTest2(&cout,1);
    DefaultThreadSpecificStorage::Type Storage2(&SchedulingTest2);

    PausesWorkUnit *FiveHundred = new PausesWorkUnit(500,"FiveHundred-ms");
    PausesWorkUnit *FiveThousand = new PausesWorkUnit(5000,"FiveThousand-ms");
    PausesWorkUnit *FiftyThousand = new PausesWorkUnit(50000,"FiftyThousand-ms");
    //PausesWorkUnit *FiveHundredThousand = new PausesWorkUnit(500000,"FiveHundredThousand-ms");
    cout << "Work Units (FiveHundred-ms, FiveThousand-ms, FiftyThousand-ms)[ms is microseconds in this context] Created, executing each ten times: " << endl;
    for(Int8 Counter = 0; Counter <10; ++Counter)
    {
        FiveHundred->operator()(Storage2);
        FiveThousand->operator()(Storage2);
        FiftyThousand->operator()(Storage2);
        //FiveHundredThousand->operator()(Storage2,SchedulingTest2);
    }
    SchedulingTest2.AddWorkUnit(FiveHundred);
    SchedulingTest2.AddWorkUnit(FiftyThousand);
    SchedulingTest2.AddWorkUnit(FiveThousand);
    //SchedulingTest2.AddWorkUnit(FiveHundredThousand);

    cout << "FiveHundred-ms   : " << FiveHundred->GetPerformanceLog().GetAverage() << endl;
    cout << "FiveThousand-ms  : " << FiveThousand->GetPerformanceLog().GetAverage() << endl;
    cout << "FiftyThousand-ms : " << FiftyThousand->GetPerformanceLog().GetAverage() << endl;
    //cout << "FiveHundredThousand-ms  : " << FiveHundredThousand->GetPerformanceLog().GetAverage() << endl;
    cout << "Marking each WorkUnit as usable for the next frame:" << endl;
    FiveHundred->PrepareForNextFrame();
    FiveThousand->PrepareForNextFrame();
    FiftyThousand->PrepareForNextFrame();
    //FiveHundredThousand->PrepareForNextFrame();
    SchedulingTest2.SortWorkUnitsMain();

    cout << "Extracting WorkUnits with the scheduling mechanism: " << endl;
    //Counter = SchedulingTest2.GetNextWorkUnit();
    //cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    //assert( ((PausesWorkUnit*)Counter)->Name == String("FiveHundredThousand-ms") );
    //Counter->operator()(Storage2, SchedulingTest2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PausesWorkUnit*)Counter)->Name == String("FiftyThousand-ms"), "Getting the WorkUnit Named FiftyThousand-ms" );
    Counter->operator()(Storage2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PausesWorkUnit*)Counter)->Name == String("FiveThousand-ms"), "Getting the WorkUnit Named FiveThousand-ms" );
    Counter->operator()(Storage2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    ThrowOnFalse( ((PausesWorkUnit*)Counter)->Name == String("FiveHundred-ms"), "Getting the WorkUnit Named FiveHundred-ms" );
    Counter->operator()(Storage2);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing Thread creation and destruction

/// @brief Performs basic checks on the logs
/// @details Used in @ref ThreadCreate
/// @param Log The Log to check
/// @param TargetThreadCount there better be this many threads.
/// @param WorkUnitCount There better be this many uniquely named WorkUnits.
/// @return A Set of WorkUnit names in case extra work needs to be performed on it.
set<String> CheckSchedulerLog(Mezzanine::Logger& Log, Whole TargetThreadCount_, Whole WorkUnitCount_)
{
    pugi::xml_document Doc;
    Doc.load(Log);
    pugi::xml_node TestLog = Doc.child("Frame");
    ThrowOnFalse (TestLog, "TestLog not present");
    Whole ThreadCount = 0;
    Whole WorkUnitCount = 0;
    pugi::xml_node LogCommit = TestLog.child("Thread");

    set<String> WorkUnitNames;
    while(LogCommit)
    {
        pugi::xml_node OneUnit = LogCommit.child("MakePi");
        while(OneUnit)
        {
            pugi::xml_attribute CurrentName = OneUnit.attribute("WorkUnitName");
            WorkUnitNames.insert(CurrentName.value());

            WorkUnitCount++;
            OneUnit = OneUnit.next_sibling("MakePi");
        }
        ThreadCount++;
        LogCommit = LogCommit.next_sibling("Thread");
    }
    cout << "Log inspection results: " << endl
         << "\t Found " << ThreadCount << " threads, expected " << TargetThreadCount_ << "." << endl
         << "\t Found " << WorkUnitNames.size() << " total WorkUnits run with " << WorkUnitCount << " different names and expected " << WorkUnitCount_ << "." << endl
         << "WorkUnit Names:" << endl;
    //sort(WorkUnitNames.begin(),WorkUnitNames.end());
    for(set<String>::iterator Iter=WorkUnitNames.begin(); Iter!=WorkUnitNames.end(); Iter++)
        { cout << *Iter << "\t"; }
    ThrowOnFalse(ThreadCount==TargetThreadCount_, "Thread count wrong");
    ThrowOnFalse(WorkUnitCount_==WorkUnitNames.size(),"Wrong number of Unique WorkUnit Names");
    ThrowOnFalse(WorkUnitCount_==WorkUnitCount,"Wrong number of WorkUnit Names");
    return WorkUnitNames;
}


/// @brief Converts anything streamable to a Mezzanine::String
/// @details Used in @ref ThreadCreate
/// @param Datum the item to convert
/// @return a string containing the lexographically casted data
template <typename T>
Mezzanine::String ToString(T Datum)
{
    stringstream Converter;
    Converter << Datum;
    return Converter.str();
}

/// @brief The 'threadcreate' Test. A smoke test for the monopoly
void ThreadCreate()
{
    cout << "Creating a FrameScheduler with 4 WorkUnits Running one frame with different thread counts: " << endl;
    stringstream LogCache;
    FrameScheduler ThreadCreationTest1(&LogCache,1);
    PiMakerWorkUnit* WorkUnitR1 = new PiMakerWorkUnit(50000,"Run1",false);
    PiMakerWorkUnit* WorkUnitR2 = new PiMakerWorkUnit(50000,"Run2",false);
    PiMakerWorkUnit* WorkUnitR3 = new PiMakerWorkUnit(50000,"Run3",false);
    PiMakerWorkUnit* WorkUnitR4 = new PiMakerWorkUnit(50000,"Run4",false);
    LogBufferSwapper Swapper2;
    LogAggregator Agg2;
    DefaultThreadSpecificStorage::Type SwapResource2(&ThreadCreationTest1);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR1);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR2);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR3);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR4);

    cout << "Thread count on initial creation: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2);
    Agg2(SwapResource2);
    cout << "Emitting log:" << endl;
    cout << LogCache.str() << endl;
    CheckSchedulerLog(LogCache,1,4);
    cout << "It ran correctly." << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(2);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2);
    Agg2(SwapResource2);
    cout << "Emitting log:" << endl;
    cout << LogCache.str() << endl;
    CheckSchedulerLog(LogCache,2,4);
    cout << "It ran correctly." << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(3);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2);
    Agg2(SwapResource2);
    cout << "Emitting log:" << endl;
    cout << LogCache.str() << endl;
    CheckSchedulerLog(LogCache,3,4);
    cout << "It ran correctly." << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(4);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2);
    Agg2(SwapResource2);
    cout << "Emitting log:" << endl;
    cout << LogCache.str() << endl;
    CheckSchedulerLog(LogCache,4,4);
    cout << "It ran correctly." << endl;
    LogCache.str("");

    //Whole Work = 8;
    Whole Work = 1000;
    cout << endl << "Leaving thread count alone and adding " << Work << " WorkUnits to the test scheduler" << endl;
    cout << "Running One Frame." << endl;
    for (Whole Counter=0; Counter<Work; ++Counter)
        { ThreadCreationTest1.AddWorkUnit( new PiMakerWorkUnit(50000,"Dyn"+ToString(Counter),false) ); }
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2);
    Agg2(SwapResource2);
    //CheckSchedulerLog(LogCache,4,12);
    //cout << LogCache.str() << endl;
    CheckSchedulerLog(LogCache,4,1004);
    cout << "It ran correctly." << endl;
    LogCache.str("");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing FrameScheduler thread restart

/// @brief Used in @ref ThreadRestart
class RestartMetric
{
    public:
        /// @brief when the given unit started
        String UnitStart;

        /// @brief when the given unit completed
        String UnitEnd;

        /// @brief name of the given unit
        String Name;

        /// @brief The thread the unit ran in.
        String Threadid;
};

/// @brief Used to easily output a work unit metrics
/// @details Used in @ref ThreadCreate
ostream& operator<< (ostream& out, RestartMetric Lhs)
{
    out << "Name: " << Lhs.Name << " \tStarted: " << Lhs.UnitStart << " \tEnded: " << Lhs.UnitEnd << " \tThread: " << Lhs.Threadid;
    return out;
}

/// @brief Converts anything streamable to anything streamable Mezzanine::String
/// @details Used in @ref ThreadRestart
/// @param Datum the item to convert
/// @return a string containing the lexographically casted data
template <typename To, typename From>
To ToWhatever(From Datum)
{
    stringstream Converter;
    Converter << Datum;
    To Results;
    Converter >> Results;
    return Results;
}

/// @brief The 'threadrestart' Test. A smoke test for the monopoly
void ThreadRestart()
{
    cout << "Creating a few WorkUnits with a " << endl;
    stringstream LogCache;
    cout << "Creating WorkUnits a Dependency chain as follows: "
            << endl << "    +--->B"
            << endl << "    |"
            << endl << "A---+"
            << endl << "    |"
            << endl << "    +--->C"
            << endl;
    PausesWorkUnit *RestartA = new PausesWorkUnit(100000,"A");
    PausesWorkUnit *RestartB = new PausesWorkUnit(100000,"B");
    PausesWorkUnit *RestartC = new PausesWorkUnit(100000,"C");
    RestartB->AddDependency(RestartA);
    RestartC->AddDependency(RestartA);
    LogCache.str("");
    FrameScheduler RestartScheduler1(&LogCache,2);
    LogBufferSwapper Swapper3;
    LogAggregator Agg3;
    DefaultThreadSpecificStorage::Type SwapResource3(&RestartScheduler1);
    RestartScheduler1.AddWorkUnit(RestartA);
    RestartScheduler1.AddWorkUnit(RestartB);
    RestartScheduler1.AddWorkUnit(RestartC);
    RestartScheduler1.SortWorkUnitsMain();
    RestartScheduler1.DoOneFrame();
    Swapper3(SwapResource3);
    Agg3(SwapResource3);
    // Check that two threads exist and that B and C run in different thread, and after A finished

    cout << LogCache.str() << "Parsing log to determine if everything happened correctly" << endl;
    pugi::xml_document Doc;
    Doc.load(LogCache);

    pugi::xml_node Thread1Node = Doc.child("Frame").first_child();
    pugi::xml_node Thread2Node = Doc.child("Frame").last_child();
    ThrowOnFalse(Thread1Node,"Could not find first Frame node");
    ThrowOnFalse(Thread2Node,"Could not find second Frame node");

    #ifdef MEZZ_DEBUG
        vector<RestartMetric> UnitTracking;
        UnitTracking.push_back(RestartMetric());
        UnitTracking.push_back(RestartMetric());
        UnitTracking.push_back(RestartMetric());
        UnitTracking.push_back(RestartMetric());


        // gather all the data that might be useful in this test.
        UnitTracking[0].UnitStart = String(Thread1Node.child("WorkunitStart").attribute("BeginTimeStamp").as_string());
        UnitTracking[0].Name = String(Thread1Node.child("WorkunitStart").next_sibling().attribute("WorkUnitName").as_string());
        UnitTracking[0].Threadid = String(Thread1Node.child("WorkunitStart").next_sibling().attribute("ThreadID").as_string());
        UnitTracking[0].UnitEnd = String(Thread1Node.child("WorkunitStart").next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
        cout << UnitTracking[0] << endl;
        UnitTracking[1].UnitStart = String(Thread1Node.child("WorkunitEnd").next_sibling().attribute("BeginTimeStamp").as_string());
        UnitTracking[1].Name = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("WorkUnitName").as_string());
        UnitTracking[1].Threadid = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("ThreadID").as_string());
        UnitTracking[1].UnitEnd = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
        cout << UnitTracking[1] << endl;
        UnitTracking[2].UnitStart = String(Thread2Node.child("WorkunitStart").attribute("BeginTimeStamp").as_string());
        UnitTracking[2].Name = String(Thread2Node.child("WorkunitStart").next_sibling().attribute("WorkUnitName").as_string());
        UnitTracking[2].Threadid = String(Thread2Node.child("WorkunitStart").next_sibling().attribute("ThreadID").as_string());
        UnitTracking[2].UnitEnd = String(Thread2Node.child("WorkunitStart").next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
        cout << UnitTracking[2] << endl;
        UnitTracking[3].UnitStart = String(Thread2Node.child("WorkunitEnd").next_sibling().attribute("BeginTimeStamp").as_string());
        UnitTracking[3].Name = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("WorkUnitName").as_string());
        UnitTracking[3].Threadid = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("ThreadID").as_string());
        UnitTracking[3].UnitEnd = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
        cout << UnitTracking[3] << endl;

        // Get exactly what we need.
        String BThread;
        String CThread;
        String AEnd;
        String BStart;
        String CStart;
        for(vector<RestartMetric>::iterator Iter = UnitTracking.begin(); Iter != UnitTracking.end(); ++Iter)
        {
            if(Iter->Name=="A")
            {
                AEnd = Iter->UnitEnd;
            }
            if(Iter->Name=="B")
            {
                BStart = Iter->UnitStart;
                BThread = Iter->Threadid;
            }
            if(Iter->Name=="C")
            {
                CStart = Iter->UnitStart;
                CThread = Iter->Threadid;
            }
        }

        cout << "The timer cannot resolve times less then: " << GetTimeStampResolution() << endl;
        cout << "Was A complete before B started: " << (ToWhatever<MaxInt>(AEnd)<=ToWhatever<MaxInt>(BStart)+GetTimeStampResolution()) << endl;
        cout << "Was A complete before B started if the clock resolution could cause error: " << (ToWhatever<MaxInt>(AEnd)<=(ToWhatever<MaxInt>(BStart)+GetTimeStampResolution())) << endl;
        ThrowOnFalse((ToWhatever<MaxInt>(AEnd)<=(ToWhatever<MaxInt>(BStart)+GetTimeStampResolution())),"Was A complete before B started");
        cout << "Was A complete before C started: " << (ToWhatever<MaxInt>(AEnd)<=ToWhatever<MaxInt>(CStart)) << endl;
        cout << "Was A complete before C started if the clock resolution could cause error: " << (ToWhatever<MaxInt>(AEnd)<=(ToWhatever<MaxInt>(CStart)+GetTimeStampResolution())) << endl;
        ThrowOnFalse((ToWhatever<MaxInt>(AEnd)<=(ToWhatever<MaxInt>(CStart)+GetTimeStampResolution())),"Was A complete before C started");
        cout << "Were B and C run in different threads: " << (BThread!=CThread) << endl;
        ThrowOnFalse(BThread!=CThread,"Were B and C run in different threads");
    #else
        cout << "This test does not validate when not in debug mode. The log is missing much meta data.";
        // can still do some tests here
    #endif

}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing FrameScheduler Timings

/// @brief The 'timing' Test. A smoke test for the monopoly
void Timing()
{
    cout << "Creating a few Schedulers with work units and testing a variety of framerates timing accuracies." << endl;
    vector<Whole> Rates;
    Rates.push_back(10);
    Rates.push_back(25);
    Rates.push_back(28);
    Rates.push_back(30);
    Rates.push_back(60);
    Rates.push_back(100);
    BufferedRollingAverage<double> VarianceTotal(Rates.size());

    for(vector<Whole>::iterator Iter = Rates.begin(); Iter!=Rates.end(); ++Iter)
    {
        stringstream LogCache;
        cout << "Creating a Scheduler with only one work unit " << *Iter << " Frame Per Second running " << *Iter << " frames. " << endl;
        FrameScheduler TimingTest(&LogCache,1);
        PiMakerWorkUnit* WorkUnitTT1 = new PiMakerWorkUnit(50,"ForeverAlone",false);
        TimingTest.AddWorkUnit(WorkUnitTT1);
        TimingTest.SetFrameRate(*Iter);
        MaxInt TimingTestStart = GetTimeStamp();
        for(Whole Counter=0; Counter<*Iter; ++Counter)
        {
            TimingTest.DoOneFrame();
        }
        MaxInt TimingTestEnd = GetTimeStamp();
        Whole TestLength = TimingTestEnd-TimingTestStart;
        cout << "  " << *Iter << " Frames took " << TestLength << " microseconds to run, should be around 1000000 (one million)." << endl;
        Integer Error = TestLength - 1000000;
        Error = (Error>0.0) ? Error : -Error;
        double Variance = (double(Error))/double(1000000) * 100;
        cout << "  " << "This is a variance of " << Error << " Frames or " << Variance << "%. Which is " << endl;
        VarianceTotal.Insert(Variance);
        //ThrowOnFalse(3>Variance,"3% variance exceeded"); // Allow a 3% variance - incosistent achievable even on even on winxp with its crappy 3.5 millisecond timer
        //assert(0.1>Variance); // Allow a .1% variance - This is very achievable with an accurate microsecond timer
    }
    cout << "Average Variance: " << VarianceTotal.GetAverage() << "%" << endl;
}

/// @brief The 'performanceframes' Test. A smoke test for the monopoly
void PerformanceFrames()
{
    cout << "|Testing the FrameScheduler with a framrate of 0 to see max performance in a fixed number of frames: " << endl;
    vector<Whole> Durations;
    Durations.push_back(10);
    Durations.push_back(25);
    Durations.push_back(28);
    Durations.push_back(30);
    Durations.push_back(60);
    Durations.push_back(100);
    Durations.push_back(200);
    Durations.push_back(300);
    Durations.push_back(400);
    Durations.push_back(500);
    Durations.push_back(600);
    Durations.push_back(700);
    Durations.push_back(800);
    Durations.push_back(900);
    Durations.push_back(1000);
    Durations.push_back(2000);
    Durations.push_back(3000);
    Durations.push_back(4000);
    Durations.push_back(5000);
    Durations.push_back(6000);
    Durations.push_back(7000);
    Durations.push_back(8000);
    Durations.push_back(9000);
    Durations.push_back(10000);
    Durations.push_back(20000);
    Durations.push_back(30000);
    Durations.push_back(40000);
    Durations.push_back(50000);
    Durations.push_back(60000);
    Durations.push_back(70000);
    Durations.push_back(80000);
    Durations.push_back(90000);
    Durations.push_back(100000);
    Durations.push_back(200000);
    Durations.push_back(300000);
    Durations.push_back(400000);
    Durations.push_back(500000);
    Durations.push_back(600000);
    Durations.push_back(700000);
    Durations.push_back(800000);
    Durations.push_back(900000);
    Durations.push_back(1000000);
    Durations.push_back(10000000);
    Durations.push_back(100000000);

    Whole EmptyMaxFR = 0;
    Whole EmptyMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> EmptyResults;

    Whole OneMaxFR = 0;
    Whole OneMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> OneResults;

    Whole ChainMaxFR = 0;
    Whole ChainMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> ChainResults;

    for(vector<Whole>::iterator Iter = Durations.begin(); Iter!=Durations.end(); ++Iter)
    {
        stringstream LogCache;
        cout << "Creating a Scheduler with a variety of WorkUnits running at full speed. " << endl;
        FrameScheduler TimingTest(&LogCache,1);
        TimingTest.SetFrameRate(0);
        MaxInt TimingTestStart = GetTimeStamp();
        for(Whole Counter=0; Counter<*Iter; ++Counter)
            { TimingTest.DoOneFrame(); }
        MaxInt TimingTestEnd = GetTimeStamp();
        Whole TestLength = TimingTestEnd-TimingTestStart;
        Whole FrameRate = double(*Iter)/(double(TestLength)/double(1000000));
        cout << "  " << *Iter << " Empty Frames took " << TestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>EmptyMaxFR)
            { EmptyMaxFR = FrameRate; }
        if(FrameRate<EmptyMinFR)
            { EmptyMinFR = FrameRate; }
        EmptyResults.push_back(FrameRate);
        if(3000000<TestLength)
            { cout << "Single Test longer than three seconds, bailing from other performace tests" << endl; break; }

        //WorkUnit* WorkUnitTT2 = new PausesWorkUnit(0,"ForeverAlone");
        iWorkUnit* WorkUnitTT2 = new PiMakerWorkUnit(0,"ForeverAlone",false);
        TimingTest.AddWorkUnit(WorkUnitTT2);
        TimingTestStart = GetTimeStamp();
        for(Whole Counter=0; Counter<*Iter; ++Counter)
            { TimingTest.DoOneFrame(); }
        TimingTestEnd = GetTimeStamp();
        TestLength = TimingTestEnd-TimingTestStart;
        FrameRate = double(*Iter)/(double(TestLength)/double(1000000));
        cout << "  " << *Iter << " Single WorkUnit Frames took " << TestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>OneMaxFR)
            { OneMaxFR = FrameRate; }
        if(FrameRate<OneMinFR)
            { OneMinFR = FrameRate; }
        OneResults.push_back(FrameRate);
        if(3000000<TestLength)
            { cout << "Single Test longer than three seconds, bailing from other performace tests" << endl; break; }

        //WorkUnit* WorkUnitTT2A = new PausesWorkUnit(0,"ForeverAlone");
        iWorkUnit* WorkUnitTT2A = new PiMakerWorkUnit(0,"A",false);
        iWorkUnit* WorkUnitTT2B = new PiMakerWorkUnit(0,"B",false);
        iWorkUnit* WorkUnitTT2C = new PiMakerWorkUnit(0,"C",false);
        TimingTest.AddWorkUnit(WorkUnitTT2A);
        TimingTest.AddWorkUnit(WorkUnitTT2B);
        TimingTest.AddWorkUnit(WorkUnitTT2C);
        WorkUnitTT2C->AddDependency(WorkUnitTT2B);
        WorkUnitTT2B->AddDependency(WorkUnitTT2A);
        TimingTest.SortWorkUnitsAll();
        TimingTestStart = GetTimeStamp();
        for(Whole Counter=0; Counter<*Iter; ++Counter)
            { TimingTest.DoOneFrame(); }
        TimingTestEnd = GetTimeStamp();
        TestLength = TimingTestEnd-TimingTestStart;
        FrameRate = double(*Iter)/(double(TestLength)/double(1000000));
        cout << "  " << *Iter << " Frames with the previous and an extra dependency set (A->B->C) took " << TestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>ChainMaxFR)
            { ChainMaxFR = FrameRate; }
        if(FrameRate<ChainMinFR)
            { ChainMinFR = FrameRate; }
        ChainResults.push_back(FrameRate);
        if(3000000<TestLength)
            { cout << "Single Test longer than three seconds, bailing from other performance tests" << endl; break; }
        cout << endl;
    }

    std::vector<String> Output;
    Output.push_back("|");
    Output.push_back("Min");
    Output.push_back("Mean");
    Output.push_back("Max");
    Output.push_back("|Empty");
    Output.push_back(ToString(EmptyMinFR));
    Output.push_back(ToString(std::accumulate<>(EmptyResults.begin(),EmptyResults.end(),0)/EmptyResults.size()));
    Output.push_back(ToString(EmptyMaxFR));
    Output.push_back("|One");
    Output.push_back(ToString(OneMinFR));
    Output.push_back(ToString(std::accumulate<>(OneResults.begin(),OneResults.end(),0)/OneResults.size()));
    Output.push_back(ToString(OneMaxFR));
    Output.push_back("|Chain");
    Output.push_back(ToString(ChainMinFR));
    Output.push_back(ToString(std::accumulate<>(ChainResults.begin(),ChainResults.end(),0)/ChainResults.size()));
    Output.push_back(ToString(ChainMaxFR));

    cout << "Scheduler timings for X frames in any time:" << endl;

    Whole ColumnWidth=14;
    Whole ColumnCount=4;
    Whole WhichColumn=0;
    String CurrentOutput;
    for(std::vector<String>::iterator Iter=Output.begin(); Iter!=Output.end(); ++Iter)
    {
        if(!(WhichColumn % ColumnCount))
        {
            WhichColumn=0;
            cout << endl << "  ";
        }
        WhichColumn++;


        for(CurrentOutput = *Iter; CurrentOutput.size()<ColumnWidth; CurrentOutput += " ") {}
        cout << CurrentOutput;
    }
    cout << endl << endl;

}

//PerformanceSeconds
/// @brief The 'performanceseconds' Test. A smoke test for the monopoly
void PerformanceSeconds()
{
    cout << "|Testing the FrameScheduler setup with a framerate of 0 to see max performance over fixed length of time: " << endl;
    vector<Whole> Durations;
    Durations.push_back(10);
    Durations.push_back(100);
    Durations.push_back(1000);
    Durations.push_back(10000);
    Durations.push_back(100000);
    Durations.push_back(1000000); // one second
    Durations.push_back(10000000);

    Whole EmptyMaxFR = 0;
    Whole EmptyMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> EmptyResults;

    Whole OneMaxFR = 0;
    Whole OneMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> OneResults;

    Whole ChainMaxFR = 0;
    Whole ChainMinFR = std::numeric_limits<Whole>::max();
    std::vector<Whole> ChainResults;

    for(vector<Whole>::iterator Iter = Durations.begin(); Iter!=Durations.end(); ++Iter)
    {
        stringstream LogCache;
        cout << "Creating a Scheduler with a variety of WorkUnits running at full speed. " << endl;
        FrameScheduler TimingTest1(&LogCache,1);
        TimingTest1.SetFrameRate(0);
        MaxInt TimingTestStart = GetTimeStamp();
        MaxInt TimingTestEnd = TimingTestStart + *Iter;
        while(GetTimeStamp()<TimingTestEnd)
            { TimingTest1.DoOneFrame(); }
        TimingTestEnd = GetTimeStamp();
        Whole FrameCount = TimingTest1.GetFrameCount();
        Whole TimingTestLength = TimingTestEnd-TimingTestStart;
        Whole FrameRate = 0;
        if(FrameCount)
            { FrameRate = double(FrameCount) / (double(TimingTestLength)/double(1000000))  ; }
        cout << "  " << FrameCount << " Empty Frames took " << TimingTestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>EmptyMaxFR)
            { EmptyMaxFR = FrameRate; }
        if(FrameRate<EmptyMinFR)
            { EmptyMinFR = FrameRate; }
        EmptyResults.push_back(FrameRate);

        FrameScheduler TimingTest2(&LogCache,1);
        TimingTest2.SetFrameRate(0);
        //WorkUnit* WorkUnitTT2 = new PausesWorkUnit(0,"ForeverAlone");
        iWorkUnit* WorkUnitTT2 = new PiMakerWorkUnit(0,"ForeverAlone",false);
        TimingTest2.AddWorkUnit(WorkUnitTT2);
        TimingTestStart = GetTimeStamp();
        TimingTestEnd = TimingTestStart + *Iter;
        while(GetTimeStamp()<TimingTestEnd)
            { TimingTest1.DoOneFrame(); }
        TimingTestEnd = GetTimeStamp();
        FrameCount = TimingTest1.GetFrameCount();
        TimingTestLength = TimingTestEnd-TimingTestStart;
        FrameRate = 0;
        if(FrameCount)
            { FrameRate = double(FrameCount) / (double(TimingTestLength)/double(1000000))  ; }
        cout << "  " << FrameCount << " Single WorkUnit Frames took " << TimingTestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>OneMaxFR)
            { OneMaxFR = FrameRate; }
        if(FrameRate<OneMinFR)
            { OneMinFR = FrameRate; }
        OneResults.push_back(FrameRate);

        FrameScheduler TimingTest3(&LogCache,1);
        TimingTest3.SetFrameRate(0);
        //WorkUnit* WorkUnitTT2A = new PausesWorkUnit(0,"ForeverAlone");
        iWorkUnit* WorkUnitTT3 = new PiMakerWorkUnit(0,"ForeverAlone",false);
        iWorkUnit* WorkUnitTT2A = new PiMakerWorkUnit(0,"A",false);
        iWorkUnit* WorkUnitTT2B = new PiMakerWorkUnit(0,"B",false);
        iWorkUnit* WorkUnitTT2C = new PiMakerWorkUnit(0,"C",false);
        TimingTest3.AddWorkUnit(WorkUnitTT2A);
        TimingTest3.AddWorkUnit(WorkUnitTT2B);
        TimingTest3.AddWorkUnit(WorkUnitTT2C);
        TimingTest3.AddWorkUnit(WorkUnitTT3);

        WorkUnitTT2C->AddDependency(WorkUnitTT2B);
        WorkUnitTT2B->AddDependency(WorkUnitTT2A);
        TimingTest3.SortWorkUnitsAll();
        TimingTestStart = GetTimeStamp();
        TimingTestEnd = TimingTestStart + *Iter;
        while(GetTimeStamp()<TimingTestEnd)
            { TimingTest1.DoOneFrame(); }
        TimingTestEnd = GetTimeStamp();
        FrameCount = TimingTest1.GetFrameCount();
        TimingTestLength = TimingTestEnd-TimingTestStart;
        FrameRate = 0;
        if(FrameCount)
            { FrameRate = double(FrameCount) / (double(TimingTestLength)/double(1000000))  ; }
        cout << "  " << FrameCount << " Frames with the previous and an extra dependency set (A->B->C) took " << TimingTestLength << " microseconds to run, which is " << FrameRate << " frames per second." << endl;
        if(FrameRate>ChainMaxFR)
            { ChainMaxFR = FrameRate; }
        if(FrameRate<ChainMinFR)
            { ChainMinFR = FrameRate; }
        ChainResults.push_back(FrameRate);
        cout << endl;
    }
    std::vector<String> Output;
    Output.push_back("|");
    Output.push_back("Min");
    Output.push_back("Mean");
    Output.push_back("Max");
    Output.push_back("|Empty");
    Output.push_back(ToString(EmptyMinFR));
    Output.push_back(ToString(std::accumulate<>(EmptyResults.begin(),EmptyResults.end(),0)/EmptyResults.size()));
    Output.push_back(ToString(EmptyMaxFR));
    Output.push_back("|One");
    Output.push_back(ToString(OneMinFR));
    Output.push_back(ToString(std::accumulate<>(OneResults.begin(),OneResults.end(),0)/OneResults.size()));
    Output.push_back(ToString(OneMaxFR));
    Output.push_back("|Chain");
    Output.push_back(ToString(ChainMinFR));
    Output.push_back(ToString(std::accumulate<>(ChainResults.begin(),ChainResults.end(),0)/ChainResults.size()));
    Output.push_back(ToString(ChainMaxFR));


    cout << "Scheduler timings for X frames in any time:" << endl;

    Whole ColumnWidth=14;
    Whole ColumnCount=4;
    Whole WhichColumn=0;
    String CurrentOutput;
    for(std::vector<String>::iterator Iter=Output.begin(); Iter!=Output.end(); ++Iter)
    {
        if(!(WhichColumn % ColumnCount))
        {
            WhichColumn=0;
            cout << endl << "  ";
        }
        WhichColumn++;


        for(CurrentOutput = *Iter; CurrentOutput.size()<ColumnWidth; CurrentOutput += " ") {}
        cout << CurrentOutput;
    }
    cout << endl << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing FrameScheduler Timings

/// @brief The 'performanceseconds' Test. A smoke test for the monopoly
void ThreadAffinity()
{
    stringstream LogCache;
    cout << "Creating WorkUnits a Dependency chain as follows: "
            << endl << "A---+                  +--->B"
            << endl << "    |                  |"
            << endl << "    +-->AffinityUnit---+"
            << endl << "    |                  |"
            << endl << "B---+                  +--->C"
            << endl;
    PausesWorkUnit *AffinityA = new PausesWorkUnit(10000,"A");
    PausesWorkUnit *AffinityB = new PausesWorkUnit(10000,"B");
    PausesWorkUnit *AffinityAffinity = new PausesWorkUnit(10000,"Affinity");
    PausesWorkUnit *AffinityC = new PausesWorkUnit(10000,"C");
    PausesWorkUnit *AffinityD = new PausesWorkUnit(10000,"D");
    /*PiMakerWorkUnit *AffinityA = new PiMakerWorkUnit(100000,"A",false);
    PiMakerWorkUnit *AffinityB = new PiMakerWorkUnit(100000,"B",false);
    PiMakerWorkUnit *AffinityAffinity = new PiMakerWorkUnit(10000,"Affinity",false);
    PiMakerWorkUnit *AffinityC = new PiMakerWorkUnit(100000,"C",false);
    PiMakerWorkUnit *AffinityD = new PiMakerWorkUnit(100000,"D",false);*/
    AffinityAffinity->AddDependency(AffinityA);
    AffinityAffinity->AddDependency(AffinityB);
    AffinityC->AddDependency(AffinityAffinity);
    AffinityD->AddDependency(AffinityAffinity);

    FrameScheduler Scheduler1(&LogCache,2);
    LogBufferSwapper Swapper1;
    LogAggregator Agg1;
    DefaultThreadSpecificStorage::Type SwapResource(&Scheduler1);
    Scheduler1.AddWorkUnit(AffinityA);
    Scheduler1.AddWorkUnit(AffinityB);
    Scheduler1.AddWorkUnitAffinity(AffinityAffinity);
    Scheduler1.AddWorkUnit(AffinityC);
    Scheduler1.AddWorkUnit(AffinityD);
    Scheduler1.SortWorkUnitsMain();
    Scheduler1.DoOneFrame();
    Swapper1(SwapResource);
    Agg1(SwapResource);
    // Check that two threads exist and that B and C run in different thread, and after A finished
    cout << "Affinity should run in this This thread and this thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    cout << LogCache.str() << "Parsing log to determine if everything happened correctly" << endl;

    #ifdef MEZZ_DEBUG
        /// @todo make a function that checks the order works units ran in returns that as a vector
    #else
        cout << "This test does not validate when not in debug mode. The log is missing much meta data.";
        // can still do some tests here
    #endif
    // Affinity starts after A and B end
    /*pugi::xml_document Doc;
    Doc.load(LogCache);
    pugi::xml_node Thread1Node = Doc.child("Frame").first_child();
    pugi::xml_node Thread2Node = Doc.child("Frame").last_child();
    ThrowOnFalse(Thread1Node,"Could not find first Frame node");
    ThrowOnFalse(Thread2Node,"Could not find second Frame node");
    vector<RestartMetric> UnitTracking;
    UnitTracking.push_back(RestartMetric());
    UnitTracking.push_back(RestartMetric());
    UnitTracking.push_back(RestartMetric());
    UnitTracking.push_back(RestartMetric());

    // gather all the data that might be useful in this test.
    UnitTracking[0].UnitStart = String(Thread1Node.child("WorkunitStart").attribute("BeginTimeStamp").as_string());
    UnitTracking[0].Name = String(Thread1Node.child("WorkunitStart").next_sibling().attribute("WorkUnitName").as_string());
    UnitTracking[0].Threadid = String(Thread1Node.child("WorkunitStart").next_sibling().attribute("ThreadID").as_string());
    UnitTracking[0].UnitEnd = String(Thread1Node.child("WorkunitStart").next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
    cout << UnitTracking[0] << endl;
    UnitTracking[1].UnitStart = String(Thread1Node.child("WorkunitEnd").next_sibling().attribute("BeginTimeStamp").as_string());
    UnitTracking[1].Name = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("WorkUnitName").as_string());
    UnitTracking[1].Threadid = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("ThreadID").as_string());
    UnitTracking[1].UnitEnd = String(Thread1Node.child("WorkunitEnd").next_sibling().next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
    cout << UnitTracking[1] << endl;
    UnitTracking[2].UnitStart = String(Thread2Node.child("WorkunitStart").attribute("BeginTimeStamp").as_string());
    UnitTracking[2].Name = String(Thread2Node.child("WorkunitStart").next_sibling().attribute("WorkUnitName").as_string());
    UnitTracking[2].Threadid = String(Thread2Node.child("WorkunitStart").next_sibling().attribute("ThreadID").as_string());
    UnitTracking[2].UnitEnd = String(Thread2Node.child("WorkunitStart").next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
    cout << UnitTracking[2] << endl;
    UnitTracking[3].UnitStart = String(Thread2Node.child("WorkunitEnd").next_sibling().attribute("BeginTimeStamp").as_string());
    UnitTracking[3].Name = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("WorkUnitName").as_string());
    UnitTracking[3].Threadid = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().attribute("ThreadID").as_string());
    UnitTracking[3].UnitEnd = String(Thread2Node.child("WorkunitEnd").next_sibling().next_sibling().next_sibling().attribute("EndTimeStamp").as_string());
    cout << UnitTracking[3] << endl;

    // Get exactly what we need.
    String BThread;
    String CThread;
    String AEnd;
    String BStart;
    String CStart;
    for(vector<RestartMetric>::iterator Iter = UnitTracking.begin(); Iter != UnitTracking.end(); ++Iter)
    {
        if(Iter->Name=="A")
        {
            AEnd = Iter->UnitEnd;
        }
        if(Iter->Name=="B")
        {
            BStart = Iter->UnitStart;
            BThread = Iter->Threadid;
        }
        if(Iter->Name=="C")
        {
            CStart = Iter->UnitStart;
            CThread = Iter->Threadid;
        }
    }

    cout << "Was A complete before B started: " << (AEnd<=BStart) << endl; // This relies  on lexigraphical ordering matching numeric ordering
    ThrowOnFalse(AEnd<=BStart,"Was A complete before B started");
    cout << "Was A complete before C started: " << (AEnd<=CStart) << endl; // if it doesn't then these numbers need to be converted.
    ThrowOnFalse(AEnd<=CStart,"Was A complete before C started");
    cout << "Were B and C run in different threads: " << (BThread!=CThread) << endl;
    ThrowOnFalse(BThread!=CThread,"Were B and C run in different threads");
    */
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing Barrier synchronization primitive

/// @brief The Barrier instance to be used in the test 'barrier'
Barrier TestBarrier(4);

vector<Whole> BarrierData1;
vector<Whole> BarrierData2;


/// @brief A simple function to synchronize in the 'barrier' test
void BarrierTestHelper(void* ThreadID)
{
    Int32 Position = *((Int32*)ThreadID);
    stringstream Output;
    Output << "-------------------" << endl
           << "This is the thread with id: " <<  Mezzanine::Threading::this_thread::get_id() << endl
           << "For this test it requires the data in position: " << Position-1 << endl
           << "doubling data in position: " << Position%4 << endl;
    BarrierData1[Position-1] *= 2;
    cout << Output.str();
    Output.str("");


    //Mezzanine::Threading::this_thread::sleep_for(10000*DiceD20()); // just because the standard says streams need to synchronize output does not mean they actually output data in correct order, this lets cout catchup and makes guessing which thread makes it to the barrier last impossible
    if(TestBarrier.Wait())
    {
        Output << "-------------------" << endl
               << "This is the thread with id: " <<  Mezzanine::Threading::this_thread::get_id() << endl
               << "This thread broke the barrier" << endl
               << "Copy data in position:" << Position%4 << endl;
    }else{
        Output << "-------------------" << endl
               << "This is the thread with id: " <<  Mezzanine::Threading::this_thread::get_id() << endl
               << "This thread waited for another to break it." << endl
               << "Copy data in position: " << Position%4 << endl;
    }
    BarrierData2[Position%4]=BarrierData1[Position%4];
    Output << "Data: " << BarrierData2[Position%4] << endl;

    cout << Output.str();
    Output.str("");
}

/// @brief The 'barrier' Test. A smoke test for the monopoly
void BarrierTest()
{
    cout << "Testing Basic Thread Barrier functionality." << endl
         << "This Threads id: " <<  Mezzanine::Threading::this_thread::get_id() << endl
         << "A group of data has been populated with 5,10,15 and 20, this should be doubled and copied into a new field of data and will be done by 4 threads. Each thread will be indexed, and will adjust the data from some other thread then synchronize and copy its own data." << endl;

    Int32 One = 1;
    Int32 Two = 2;
    Int32 Three = 3;
    Int32 Four = 4;
    BarrierData1.push_back(5);
    BarrierData1.push_back(10);
    BarrierData1.push_back(15);
    BarrierData1.push_back(20);
    BarrierData2.push_back(0);
    BarrierData2.push_back(0);
    BarrierData2.push_back(0);
    BarrierData2.push_back(0);

    Mezzanine::Threading::Thread T1(BarrierTestHelper,&One);
    Mezzanine::Threading::Thread T2(BarrierTestHelper,&Two);
    Mezzanine::Threading::Thread T3(BarrierTestHelper,&Three);
    Mezzanine::Threading::Thread T4(BarrierTestHelper,&Four);
    T1.join();
    T2.join();
    T3.join();
    T4.join();

    ThrowOnFalse(10==BarrierData2[0], "This thread should have copied 10");
    ThrowOnFalse(20==BarrierData2[1], "This thread should have copied 20");
    ThrowOnFalse(30==BarrierData2[2], "This thread should have copied 30");
    ThrowOnFalse(40==BarrierData2[3], "This thread should have copied 40");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Testing Asynchronous Workunit synchronization primitive

/// @brief Create a string suitable for output that converts IO in bytes and time in  microseconds to easily human readable text
String GetPerfString(double IOSize, double Duration)
{
    stringstream Maker;
    Maker << fixed << showpoint << setprecision(2);
    vector<String> Terms;
    Terms.push_back(" Bytes/sec");
    Terms.push_back(" KB/sec");
    Terms.push_back(" MB/sec");
    Terms.push_back(" GB/sec");
    Terms.push_back(" TB/sec");
    Whole WhichTerm = 0;

    IOSize = IOSize/Duration * double(1000000); // convert to bytes/sec

    while(1024<IOSize && 4!=WhichTerm)
    {
        WhichTerm++;
        IOSize/=1024;
    }
    Maker << IOSize << Terms[WhichTerm];
    return Maker.str();
}

/// @brief The 'async' Test. A smoke test for the monopoly
void Async()
{
    cout << "Creating three files that might take up to a whole seconds to write." << endl;
    MaxInt MaxTime = 1000000;
    Whole MaxFileWrites = 100000;
    Whole CurrentCount = 0;
    MaxInt TimeStarted = GetTimeStamp();

    vector<String> Files;
    Files.push_back(String("File1.txt"));
    Files.push_back(String("File2.txt"));
    Files.push_back(String("File3.txt"));

    ofstream File1a(Files[0].c_str());
    ofstream File2a(Files[1].c_str());
    ofstream File3a(Files[2].c_str());
    while(GetTimeStamp()<TimeStarted+MaxTime && CurrentCount<MaxFileWrites)
    {
        CurrentCount++;
        File1a.write("Packets1Packets2Packets3", 24);
        File2a.write("01", 2);
        File3a.write("-", 1);
    }
    Whole Duration = GetTimeStamp()-TimeStarted;
    Whole WriteSize = 27*CurrentCount;
    String PerfString(GetPerfString(WriteSize,Duration));
    File1a.close();
    File2a.close();
    File3a.close();
    cout << fixed << showpoint << setprecision(2);
    cout << "Creating files took " << Duration << " microseconds " << endl;
    cout << "A total of " << WriteSize << " Bytes were written or " << PerfString << endl;

    cout << "Creating an AsynchronousFileLoadWorkUnit to load the contents of these files." << endl;
    AsynchronousFileLoadWorkUnit Testable;
    Testable.BeginLoading(Files);

    FrameScheduler Scheduler1(&cout,2);
    DefaultThreadSpecificStorage::Type AResource(&Scheduler1);
    TimeStarted = GetTimeStamp();
    while(Complete!=Testable.IsWorkDone())
    {
        Testable.DoWork(AResource);
        ThrowOnFalse(GetTimeStamp()<TimeStarted+MaxTime*20,"Reading the file took more than 20 times as long writing the files");
    }
    Duration = GetTimeStamp()-TimeStarted;
    Whole ReadSize = Testable.GetFile(0)->Size+Testable.GetFile(1)->Size+Testable.GetFile(2)->Size;
    PerfString = GetPerfString(ReadSize,Duration);
    cout << "Reading file took " << Duration << " microseconds " << endl;
    cout << "A total of " << ReadSize << " Bytes were read or " << PerfString << endl;


    cout << "The files have been loaded. performing a basic consistency check." << endl;
    ThrowOnFalse( ReadSize == WriteSize  ,"Wrote and Read different amounts, what is going on");
    ThrowOnFalse(Testable.GetFile(0)->Size>0,"First file is too short");
    ThrowOnFalse(Testable.GetFile(0)->Data[0]=='P' ,"First file is wrong");
    ThrowOnFalse(Testable.GetFile(1)->Size>0,"Second file is too short");
    ThrowOnFalse(Testable.GetFile(1)->Data[0]=='0' ,"Second file is wrong");
    ThrowOnFalse(Testable.GetFile(2)->Size>0,"Third file is too short");
    ThrowOnFalse(Testable.GetFile(2)->Data[0]=='-' ,"Third file is wrong");
    cout << "Files seem at least superficially consistent, trunctating files on disk to conserve space." << endl;

    ofstream File1b(Files[0].c_str());
    ofstream File2b(Files[1].c_str());
    ofstream File3b(Files[2].c_str());
    File1b.close();
    File2b.close();
    File3b.close();
}

/// @brief The 'helperunits' Test. A smoke test for the monopoly
void HelperUnits()
{
    cout << "Creating a frame scheduler with a variety of Work units for integration testing of the helper workunits." << endl;

    LogAggregator* LA = new LogAggregator;
    LogBufferSwapper* LBS = new LogBufferSwapper;
    WorkSorter* WS = new WorkSorter;
    LBS->AddDependency(LA);
    #ifdef MEZZ_DEBUG
    LBS->AddDependency(WS);
    #endif

    PiMakerWorkUnit *AffinityA = new PiMakerWorkUnit(100000,"A",false);
    LBS->AddDependency(AffinityA);
    PiMakerWorkUnit *AffinityB = new PiMakerWorkUnit(100000,"B",false);
    LBS->AddDependency(AffinityB);
    PiMakerWorkUnit *AffinityAffinity = new PiMakerWorkUnit(10000,"Affinity",false);
    LBS->AddDependency(AffinityAffinity);
    AffinityAffinity->AddDependency(AffinityA);
    AffinityAffinity->AddDependency(AffinityB);
    PiMakerWorkUnit *AffinityC = new PiMakerWorkUnit(100000,"C",false);
    LBS->AddDependency(AffinityC);
    AffinityC->AddDependency(AffinityAffinity);
    PiMakerWorkUnit *AffinityD = new PiMakerWorkUnit(100000,"D",false);
    LBS->AddDependency(AffinityD);
    AffinityD->AddDependency(AffinityAffinity);

    PiMakerWorkUnit *AffinityFog1 = new PiMakerWorkUnit(100000,"Fog1",false);
    LBS->AddDependency(AffinityFog1);
    PiMakerWorkUnit *AffinityFog2 = new PiMakerWorkUnit(100000,"Fog2",false);
    LBS->AddDependency(AffinityFog2);
    PiMakerWorkUnit *AffinityFog3 = new PiMakerWorkUnit(100000,"Fog3",false);
    LBS->AddDependency(AffinityFog3);

    PiMakerMonopoly* Pioply = new PiMakerMonopoly(50,"Pioply",false,4);

    stringstream LogCache;

    FrameScheduler IntegrationTester(&LogCache,4);
    IntegrationTester.AddWorkUnit(LA);
    IntegrationTester.AddWorkUnit(LBS);
    IntegrationTester.AddWorkUnit(WS);
    IntegrationTester.AddWorkUnit(AffinityA);
    IntegrationTester.AddWorkUnit(AffinityB);
    IntegrationTester.AddWorkUnit(AffinityC);
    IntegrationTester.AddWorkUnit(AffinityD);
    IntegrationTester.AddWorkUnit(AffinityFog1);
    IntegrationTester.AddWorkUnit(AffinityFog2);
    IntegrationTester.AddWorkUnit(AffinityFog3);

    IntegrationTester.AddWorkUnitMonopoly(Pioply);
    IntegrationTester.AddWorkUnitAffinity(AffinityAffinity);

    IntegrationTester.SetFrameRate(0); //MAX Frame Rate !!!!1!!11!!!!1!one!!
    MaxInt ExternalFrameStart = 0;
    for(Whole Counter=0; Counter<MEZZ_FRAMESTOTRACK*4; Counter++)
    {
        ExternalFrameStart = GetTimeStamp();
        IntegrationTester.DoOneFrame();
        cout << "Frame " << IntegrationTester.GetFrameCount() << " Took " << GetTimeStamp()-ExternalFrameStart << " microseconds." << endl;
    }

    cout    << "Log from 20 frames of execution" << endl
            << LogCache.str() << endl;

}






int main (int argc, char** argv)
{
    // Make a vector of Test names to run
    vector<String> TargetTests;
    String ThisExecutable(argv[0]);
    for(int Counter=1; Counter<argc; Counter++)
        { TargetTests.push_back(argv[Counter]); }
    for(vector<String>::iterator Iter=TargetTests.begin(); Iter!=TargetTests.end(); ++Iter)
        { transform(Iter->begin(), Iter->end(), Iter->begin(), ::tolower); }

    // The mapping of all the tests to their name
    TestGroup AllTheTests;
    AllTheTests["sizes"]=Sizes;
    AllTheTests["infercachesize"]=InferCacheSize;
    AllTheTests["untestable"]=Untestable;
    AllTheTests["basicthreading"]=BasicThreading;
    AllTheTests["basicmutex"]=BasicMutex;
    AllTheTests["basicthreadingpassing"]=BasicThreadingPassing;
    AllTheTests["basicmutextry"]=BasicMutexTry;
    AllTheTests["rollingaverage"]=RollingAverageTests;
    AllTheTests["timestamp"]=TimeStamp;
    AllTheTests["random"]=RandomTests;
    AllTheTests["workunit"]=WorkUnitTests;
    AllTheTests["monopoly"]=MonopolyTest;
    AllTheTests["logaggregator"]=LogAggregatorTests;
    AllTheTests["workunitkey"]=WorkUnitKeyTests;
    AllTheTests["frameschedulergetnext"]=FrameSchedulerGetNext;
    AllTheTests["threadcreate"]=ThreadCreate;
    AllTheTests["threadrestart"]=ThreadRestart;
    AllTheTests["timing"]=Timing;
    AllTheTests["performanceframes"]=PerformanceFrames;
    AllTheTests["performanceseconds"]=PerformanceSeconds;
    AllTheTests["threadaffinity"]=ThreadAffinity;
    AllTheTests["barrier"]=BarrierTest;
    AllTheTests["async"]=Async;
    AllTheTests["helperunits"]=HelperUnits;
    //AllTheTests["basicthreading"]=BasicThreading;
    //AllTheTests["basicthreading"]=BasicThreading;
    //AllTheTests["basicthreading"]=BasicThreading;

    if(TargetTests.size())
    {
        for(vector<String>::iterator Iter=TargetTests.begin(); Iter!=TargetTests.end(); ++Iter) // Check for invalid tests
        {
            if(AllTheTests.find(*Iter)==AllTheTests.end())
            {
                cout << "\"" << *Iter << "\" is not a valid test name." <<  endl << endl;
                Usage(ThisExecutable, AllTheTests);
                exit(EXIT_SUCCESS);
            }
        }
        for(vector<String>::iterator Iter=TargetTests.begin(); Iter!=TargetTests.end(); ++Iter)
            { AllTheTests[*Iter](); }
    }else{

        for(TestGroup::iterator Iter=AllTheTests.begin(); Iter!=AllTheTests.end(); ++Iter)
        {
            cout << endl << endl << "Beginning test '" << Iter->first << "' :" << endl;
            Iter->second();
            cout << endl << endl;
        }

    }

    #ifdef _MSC_VER
    system("pause");
    #endif

    exit(EXIT_SUCCESS);
}
#endif
