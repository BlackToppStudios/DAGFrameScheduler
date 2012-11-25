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
#ifndef _main_cpp
#define _main_cpp

/// @file
/// @brief This file defines a number of tests for the rest of the library, and is not included in binary form with the library when compiled.

#include "dagframescheduler.h"

#include <iostream>
#include <cassert>
#include <typeinfo>
#include <cstdlib>
#include <sstream>
#include <set>

#include "pugixml.h" // Not needed for regular operation of the library, just needed for tests.

using std::cout;
using std::endl;
using std::vector;
using namespace Mezzanine;
using namespace Mezzanine::Threading;

/// @brief Simulate a 20 sided die
/// @return A Mezzanine::Whole containing a a random number between 1 and 20 inclusive with equal chance.
Mezzanine::Whole DiceD20()
    { return rand()%20+1; }

/// @brief Simulate 2x 3 sided die being added
/// @return A Mezzanine::Whole containing a a random number between 2 and 6 inclusive with a bell curve probability. This actually generates 2 numbers between 1 and 3 inclusive adds them, then returns that.
Mezzanine::Whole Dice2D3()
    { return ((rand()%3) + (rand()%3) + 2); }

/// @brief Converts anything streamable to a Mezzanine::String
/// @param Datum the item to convert
/// @return a string containing the lexographically casted data
template <typename T>
Mezzanine::String ToString(T Datum)
{
    std::stringstream Converter;
    Converter << Datum;
    return Converter.str();
}

/// @brief An intentionally large and slow to process type chosen once here, to allwo easy adjusting of these tests.
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
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PiMakerWorkUnit : public Mezzanine::Threading::WorkUnit
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
        virtual void DoWork(ThreadSpecificStorage& CurrentThreadStorage, FrameScheduler& CurrentFrameScheduler)
        {
            DoubleBufferedLogger& CurrentLogger = CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger);
            CurrentLogger.GetUsable() << "<MakePi Pi=\"" << MakePi(Length,SpikesOn) << "\" WorkUnitName=\"" << Name << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << endl;
        }
};

/// @brief A samplework unit that that just block the thread it is in
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PausesWorkUnit : public Mezzanine::Threading::WorkUnit
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
        virtual void DoWork(ThreadSpecificStorage& CurrentThreadStorage, FrameScheduler& CurrentFrameScheduler)
        {
            DoubleBufferedLogger& CurrentLogger = CurrentThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger);
            CurrentLogger.GetUsable() << "<Pause Pause=\"" << Length << "\" WorkUnitName=\"" << Name << "\" ThreadID=\"" << Mezzanine::Threading::this_thread::get_id() << "\" />" << endl;
            Mezzanine::Threading::this_thread::sleep_for(Length);
        }
};


/// @brief Used to pass and accept data from the Pi calculating function with multiple threads.
/// @param PiMakerMonopoly_ a PiMakerMonopoly void pointer that provides all the required meta data for its execution
void PiMakerMonopolyHelper(void* PiMakerMonopoly_);

class PiMakerMonopoly;

/// @brief The data a single thread in the Pi make monopoly needs.
struct PiMakerThreadData
{
    /// @brief A pointer to the one Pi maker monopoly the target thread cares about
    PiMakerMonopoly *Maker;

    /// @brief The Resources the target thread should use.
    ThreadSpecificStorage *Storage;

    /// @brief constructor
    /// @param Maker_ A pointer to a PiMakerMonopoly that the targe
    explicit PiMakerThreadData(PiMakerMonopoly *Maker_) : Maker(Maker_), Storage(new ThreadSpecificStorage(0)) // This is not exception safe, if we ran out of memory here this could leak.1
        {}

    /// @brief Deletes allocated data.
    ~PiMakerThreadData()
        { delete Storage; }
};

/// @brief PiMakerMonopoly Consume every thread for a brief period to calculate Pi.
/// @warning Everything on these samples has a public access specifier, for production code that is poor form, encapsulate your stuff.
class PiMakerMonopoly : public Mezzanine::Threading::MonopolyWorkUnit
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
        virtual void UseThreads(Whole AmountToUse)
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
        virtual void DoWork(FrameScheduler& CurrentFrameScheduler)
        {
            Scheduler = &CurrentFrameScheduler;
            std::vector<thread*> ThreadIndex;
            std::vector<PiMakerThreadData*> ThreadData;
            for (Whole Count=0; Count<HowManyThreads; ++Count)      // Pretend making all this Pi simulates everyone in a Bakery baking at at once as hard as they can
            {
                PiMakerThreadData* Data = new PiMakerThreadData(this);
                ThreadData.push_back( Data );
                ThreadIndex.push_back( new Mezzanine::Threading::thread (PiMakerMonopolyHelper, Data) );
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

/// @brief Performs basic checks on the logs
/// @param Log The Log to check
/// @param TargetThreadCount there better be this many threads.
/// @param WorkUnitCount There better be this many uniquely named WorkUnits.
/// @return A Set of WorkUnit names in case extra work needs to be performed on it.
std::set<String> CheckSchedulerLog(Mezzanine::Logger& Log, Whole TargetThreadCount_, Whole WorkUnitCount_)
{
    pugi::xml_document Doc;
    Doc.load(Log);
    pugi::xml_node TestLog = Doc.child("Frame");
    assert (TestLog);
    Whole ThreadCount = 0;
    Whole WorkUnitCount = 0;
    pugi::xml_node LogCommit = TestLog.child("Thread");

    std::set<String> WorkUnitNames;
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
    assert(ThreadCount==TargetThreadCount_);
    assert(WorkUnitCount_==WorkUnitNames.size()); // unique names vs expected
    assert(WorkUnitCount_==WorkUnitCount); // actual work units vs expected
    return WorkUnitNames;
}

static Mezzanine::Integer TryLockTest=0;
static Mezzanine::Threading::mutex TryLock;
void TryToSquareInThread(void* Value)
{
    cout << "Thread T4 trying to lock mutex ThreadPassLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    if (TryLock.try_lock())
    {
        cout << "Thread T4 locked mutex, Squaring the value " << endl;
        TryLockTest = *(Mezzanine::Integer*)Value * *(Mezzanine::Integer*)Value;
        TryLock.unlock();
    }else{
        cout << "Thread T4 could not acquire lock, no work done" << endl;
    }
}

static Mezzanine::Integer ThreadPassTest=0;
static Mezzanine::Threading::mutex ThreadPassLock;
void SquareInThread(void* Value)
{
    cout << "Thread T3 waiting for lock on mutex ThreadPassLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadPassLock.lock();
    cout << "Thread T3 locked mutex: " << endl;
    ThreadPassTest = *(Mezzanine::Integer*)Value * *(Mezzanine::Integer*)Value;
    cout << "Thread T3 work complete unlocking mutex: " << endl;
    ThreadPassLock.unlock();
}

static Mezzanine::Threading::thread::id ThreadIDTest=0;
static Mezzanine::Threading::mutex ThreadIDLock;
void PutIdInGlobal(void*)
{
    cout << "Thread T2 trying to lock mutex ThreadIDLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadIDLock.lock();
    cout << "Thread T2 locked mutex: " << endl;
    ThreadIDTest = Mezzanine::Threading::this_thread::get_id();
    cout << "Thread T2 work complete unlocking mutex: " << endl;
    ThreadIDLock.unlock();
}

void PrintHello(void*)
    { cout << "Hello from thread T1 with id: " << Mezzanine::Threading::this_thread::get_id() << endl; }

int main (int argc, char** argv)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    cout << "Determining sizeof() important types that are used throughout:" << endl
         << "WorkUnit: " << sizeof(WorkUnit) << endl
         << "DefaultRollingAverage<Whole>::Type: " << sizeof(DefaultRollingAverage<Whole>::Type) << endl
         << "WeightedRollingAverage<Whole,Whole>: " << sizeof(WeightedRollingAverage<Whole,Whole>) << endl
         << "BufferedRollingAverage<Whole>: " << sizeof(BufferedRollingAverage<Whole>) << endl
         << "WorkUnitMonpoly: " << sizeof(MonopolyWorkUnit) << endl
         << "ThreadSpecificStorage: " << sizeof(ThreadSpecificStorage) << endl
         << "FrameScheduler: " << sizeof(FrameScheduler) << endl
         << "thread: " << sizeof(thread) << endl
         << "mutex: " << sizeof(mutex) << endl
         << "vector<Whole>: " << sizeof(vector<Whole>) << endl
         << "vector<WorkUnit*>: " << sizeof(vector<Whole*>) << endl
         << "set<WorkUnit*>: " << sizeof(std::set<WorkUnit*>) << endl
         << "volatile int32_t: " << sizeof(volatile int32_t) << endl
         << "std::ostream*: " << sizeof(std::ostream*) << endl
         << "MaxInt: " << sizeof(MaxInt) << endl
         << "Whole: " << sizeof(Whole) << endl;

    cout << endl << "Displaying Output of untestable functions. There is no way to have known when this was written, what the results of these would be:" << endl;
    cout << "The current time in microseconds GetTimeStamp(): " << GetTimeStamp() << endl;
    cout << "What is the smallest amount of time the clock can measure in microseconds GetTimeStampResolution(): " << GetTimeStampResolution() << endl;
    cout << "Current Logical Processor Count GetCPUCount(): " << GetCPUCount() << endl;
    //cout << "Cache size available before using RAM GetCacheSize(): " << GetCacheSize() << endl;
    //cout << "Size of one entry in the fastest cache GetCachelineSize(): " << GetCachelineSize() << endl;



    ////////////////////////////////////////////////////////////////////////////////////////////////
    cout << endl << "Testing Basic Thread functionality." << endl;
    cout << "This Threads id: " <<  Mezzanine::Threading::this_thread::get_id() << endl;

    cout << "Creating a thread with identifier T1 and unkown id." << endl;
    Mezzanine::Threading::thread T1(PrintHello);
    cout << "T1 should have an id of: " << T1.get_id() << endl;

    cout << "Is T1 joinable: " << T1.joinable() << endl;
    cout << "Joining T1" << endl;
    T1.join();
    cout << "Is T1 joinable: " << T1.joinable() << endl;

    cout << "Sleeping main thread for 300ms." << endl;
    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Yielding thread to OS scheduler." << endl;
    Mezzanine::Threading::this_thread::yield();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    cout << endl << "Testing basic mutex functionality" << endl;
    cout << "Locking ThreadIDLock in thread: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadIDLock.lock();

    cout << "Creating a thread with identifier T2 and unkown id." << endl;
    Mezzanine::Threading::thread T2(PutIdInGlobal);

    cout << "Storing T2's id: " << T2.get_id() << endl;
    cout << "Unlocking ThreadIDLock from main and sleeping for 300 ms." << endl;
    Mezzanine::Threading::thread::id T2id = T2.get_id();
    ThreadIDLock.unlock();
    Mezzanine::Threading::this_thread::sleep_for(300000);

    ThreadIDLock.lock();
    cout << "Does the thread report the same ID as we gathered: " << (ThreadIDTest == T2id) << endl;
    assert(ThreadIDTest == T2id);
    ThreadIDLock.unlock();

    cout << "Joining T2" << endl;
    T2.join();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    cout << endl << "Testing passing to thread functionality" << endl;
    cout << "Locking ThreadPassLock in thread: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadPassLock.lock();

    cout << "Creating a thread with identifier T3 and unkown id." << endl;
    Mezzanine::Integer Value = 9;
    cout << "Passing " << Value << " into thread T3." << endl;
    Mezzanine::Threading::thread T3(SquareInThread, &Value);

    cout << "Unlocking ThreadPassLock from main and sleeping for 300 ms." << endl;
    ThreadPassLock.unlock();
    Mezzanine::Threading::this_thread::sleep_for(300000);


    ThreadPassLock.lock();
    cout << "Thread gives us: " << ThreadPassTest << endl;
    cout << "Does the thread give us the square of what we passed it: " << (Value*Value == ThreadPassTest) << endl;
    assert(Value*Value == ThreadPassTest);
    ThreadPassLock.unlock();

    cout << "Joining T3" << endl;
    T3.join();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    cout << endl << "Testing Mutex try_lock()" << endl;
    cout << "Locking TryLock in main thread with id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    assert(TryLock.try_lock());

    cout << "Creating a thread with identifier T4 and unkown id." << endl;
    cout << "Passing " << Value << " into thread T4, and assigning to output and waiting 200ms." << endl;
    TryLockTest = Value;
    Mezzanine::Threading::thread T4(TryToSquareInThread, &Value);

    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Joining T4" << endl;
    T4.join();

    cout << "Unlocking TryLock." << endl;
    TryLock.unlock();

    cout << "Value from thread's return point is " << TryLockTest << " it should be " << Value << " if it wasn't able to get mutex" << endl;
    cout << "Did T4 not get the mutex and proceed past mutex as expected: " << (TryLockTest == Value) << endl;
    assert(TryLockTest == Value);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // rolling average tests

    cout << endl << "Starting Rolling Average Tests" << endl;
    cout << "Is the Default Rolling Average the BufferedRollingAverage: " << (typeid(Mezzanine::DefaultRollingAverage<Mezzanine::Whole>::Type)==typeid(Mezzanine::BufferedRollingAverage<Mezzanine::Whole>)) << endl;
    //assert((typeid(Mezzanine::DefaultRollingAverage<Mezzanine::Whole>::Type)==typeid(Mezzanine::BufferedRollingAverage<Mezzanine::Whole>)));

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
    assert(RollingB.GetAverage()==15);
    cout << "WeightedRollingAverage Result, should be about 10: " << RollingW.GetAverage() << endl;
    assert(RollingW.GetAverage()>9||RollingW.GetAverage()<16);
    cout << "DefaultRollingAverage Result, should match its underlying type : " << RollingD.GetAverage() << endl;
    //assert(RollingD.GetAverage()==15);

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
    assert(RollingB2.GetAverage()>15.4 && RollingB2.GetAverage()<15.6);
    cout << "WeightedRollingAverage Result, should be ~12.2158: " << RollingW2.GetAverage() << endl;
    assert(RollingW2.GetAverage()>12.1 && RollingW2.GetAverage()<12.3);
    cout << "DefaultRollingAverage Result, should match its underlying type : " << RollingD2.GetAverage() << endl;
    //assert(RollingD2.GetAverage()>15.4 && RollingD2.GetAverage()<15.6);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Timestamp Tests

    cout << endl << "Starting timekeeping tests." << endl;
    cout << "Getting Timestamp1" << endl;
    Mezzanine::MaxInt Timestamp1 = Mezzanine::GetTimeStamp();

    cout << "Sleeping main thread for 300ms." << endl;
    Mezzanine::Threading::this_thread::sleep_for(300000);

    cout << "Getting Timestamp2" << endl;
    Mezzanine::MaxInt Timestamp2 = Mezzanine::GetTimeStamp();

    cout << "Timestamp1: " << Timestamp1 << endl;
    cout << "Timestamp2: " << Timestamp2 << endl;
    cout << "Is Timestamp2 - Timestamp1 = " << Timestamp2-Timestamp1 << endl;
    cout << "Is Timestamp1 <= Timestamp2: " << (Timestamp1<=Timestamp2) << endl;
    cout << "Timer Resolution: " << GetTimeStampResolution() << " microsecond(s)" << endl;
    assert((Timestamp1<=Timestamp2));
    cout << "Is Timestamp1+300000-(2*TimerResolution) <= Timestamp2 = " << Timestamp1+300000-(2*GetTimeStampResolution()) << "<=" << Timestamp2 << endl;
    cout << "Is Timestamp1+300000-(2*TimerResolution) <= Timestamp2: " << (Timestamp1+300000-(2*GetTimeStampResolution())<=Timestamp2) << endl;
    assert((Timestamp1+300000-(2*GetTimeStampResolution()))<=Timestamp2);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Random Number facilities
    cout << endl << "Starting random number generation tests. Not part of the library, but required for testing." << endl;
    srand((int)Timestamp1);
    Whole TestRuns = 1000000;
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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // WorkUnit tests

    cout << endl << "Starting WorkUnit Tests, 20 runs with WorkUnitSample1" << endl;
    PiMakerWorkUnit WorkUnitSample1(5000,"WorkUnitSample1",false);
    FrameScheduler TestScheduler(&std::cout,1);
    Mezzanine::Threading::ThreadSpecificStorage TestThreadStorage(&TestScheduler);
    // run work unit
    for(Whole Counter=0; Counter<20; Counter++)
        { WorkUnitSample1(TestThreadStorage, TestScheduler); }
    cout << "Here is the complete log of Twenty Test Runs" << endl
         << TestThreadStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable().str() // << endl // logs ends with a newline
         << "Average Execution Time (Microseconds): " << WorkUnitSample1.GetPerformanceLog().GetAverage() << endl;

    cout << endl << "Starting WorkUnit Dependent and Dependency count Tests. Creating a chain in which C depends on B which depends on A." << endl;
    PiMakerWorkUnit WorkUnitA(50,"A",false);
    PiMakerWorkUnit WorkUnitB(50,"B",false);
    PiMakerWorkUnit WorkUnitC(50,"C",false);
    WorkUnitC.AddDependency(&WorkUnitB);
    WorkUnitB.AddDependency(&WorkUnitA);
    cout << "A dependency count: " << WorkUnitA.GetDependencyCount() << " \t A dependent count: " << WorkUnitA.GetDependentCount() << endl;
    cout << "B dependency count: " << WorkUnitB.GetDependencyCount() << " \t B dependent count: " << WorkUnitB.GetDependentCount() << endl;
    cout << "C dependency count: " << WorkUnitC.GetDependencyCount() << " \t C dependent count: " << WorkUnitC.GetDependentCount() << endl;
    assert(WorkUnitA.GetDependencyCount()==0);
    assert(WorkUnitA.GetDependentCount()==2);
    assert(WorkUnitB.GetDependencyCount()==1);
    assert(WorkUnitB.GetDependentCount()==1);
    assert(WorkUnitC.GetDependencyCount()==2);
    assert(WorkUnitC.GetDependentCount()==0);
    cout << "Creating a WorkUnit D which depends on B, So we should have:"
            << endl << "D --"
            << endl << "   |"
            << endl << "   v"
            << endl << "   B --> A"
            << endl << "   ^"
            << endl << "   |"
            << endl << "C --" << endl;
    PiMakerWorkUnit WorkUnitD(50,"D",false);
    WorkUnitD.AddDependency(&WorkUnitB);
    cout << "A dependency count: " << WorkUnitA.GetDependencyCount() << " \t A dependent count: " << WorkUnitA.GetDependentCount() << endl;
    cout << "B dependency count: " << WorkUnitB.GetDependencyCount() << " \t B dependent count: " << WorkUnitB.GetDependentCount() << endl;
    cout << "C dependency count: " << WorkUnitC.GetDependencyCount() << " \t C dependent count: " << WorkUnitC.GetDependentCount() << endl;
    cout << "D dependency count: " << WorkUnitD.GetDependencyCount() << " \t D dependent count: " << WorkUnitD.GetDependentCount() << endl;
    assert(WorkUnitA.GetDependencyCount()==0);
    assert(WorkUnitA.GetDependentCount()==3);
    assert(WorkUnitB.GetDependencyCount()==1);
    assert(WorkUnitB.GetDependentCount()==2);
    assert(WorkUnitC.GetDependencyCount()==2);
    assert(WorkUnitC.GetDependentCount()==0);
    assert(WorkUnitD.GetDependencyCount()==2);
    assert(WorkUnitD.GetDependentCount()==0);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // A smoke test for the monopoly
    cout << endl << "Starting MonopolyWorkUnit test. Creating a monopoly that will calculate pi in a number of threads simultaneously." << endl;
    PiMakerMonopoly Pioply(50,"Pioply",false,4);
    ThreadSpecificStorage PioplyStorage(&TestScheduler);
    for(Whole Counter=0; Counter<20; Counter++)
        { Pioply(PioplyStorage, TestScheduler); }
    cout << "Here is the un-aggregated (main thread only) log of Twenty Test Runs" << endl
         << PioplyStorage.GetResource<DoubleBufferedLogger>(DBRLogger).GetUsable().str() // << endl // logs ends with a newline
         << "Average Execution Time (Microseconds): " << Pioply.GetPerformanceLog().GetAverage() << endl;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Test for the logger workunits
    cout << endl << "Testing the logger workunits to get a handle on the monopolies logs, logging to cout: " << endl;
    LogBufferSwapper Swapper;
    ThreadSpecificStorage SwapResource(&TestScheduler);
    Swapper(SwapResource, TestScheduler);
    LogAggregator Agg;
    Agg(SwapResource, TestScheduler);
    cout << "Large log should have been emitted that showed PI being calculated 80 times and which thread it was calculated in. 20 iterations should have occurred in the main thread, and the rest each in fresh threads." << endl;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Tests for the WorkUnitKey
    cout << endl << "Creating a number of WorkUnitKeys and tesing their ability to sort: " << endl;
    cout << "\t" << "                   Deps, Time, Ptr" << endl;
    cout << "\t" << "WorkUnitKey First(   10, 500,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Second(   5, 600,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Third(    5, 500,  0  );" << endl;
    cout << "\t" << "WorkUnitKey Fourth(   3, 500,  1  );" << endl;
    cout << "\t" << "WorkUnitKey Fifth(    3, 500,  0  );" << endl;
    WorkUnitKey First(10,500,0);
    WorkUnitKey Second(5,600,0);
    WorkUnitKey Third(5,500,0);
    WorkUnitKey Fourth(3,500,(WorkUnit*)1);
    WorkUnitKey Fifth(3,500,0);

    cout << "Second<First: " << (Second < First) << "\t Third<First: " << (Third < First) << "\t Fourth<First: " << (Fourth < First) << "\t Fifth<First: " << (Fifth < First) << endl;
    assert(Second < First);
    assert(Third < First);
    assert(Fourth < First);
    assert(Fifth < First);
    cout << "Third<Second: " << (Third < Second) << "\t Fourth<Second: " << (Fourth < Second) << "\t Fifth<Second: " << (Third < Second) << endl;
    assert(Third < Second);
    assert(Fourth < Second);
    assert(Fifth < Second);
    cout << "Fourth<Third: " << (Fourth < Third) << "\t Fifth<Third: " << (Fifth < Third) << endl;
    assert(Fourth < Third);
    assert(Fifth < Third);
    cout << "Fifth<Fourth: " << (Fifth < Fourth) << endl;
    assert(Fifth < Fourth);

    cout << "First<Second: " << (First < Second) << "\t First<Third: " << (First < Third) << "\t First<Fourth: " << (First < Fourth) << "\t First<Fifth: " << (First < Fifth) << endl;
    assert(!(First < Second));
    assert(!(First < Third));
    assert(!(First < Fourth));
    assert(!(First < Fifth));
    cout << "Second<Third: " << (Second < Third) << "\t Second<Fourth: " << (Second < Fourth) << "\t Second<Fifth: " << (Second < Fifth) << endl;
    assert(!(Second < Third));
    assert(!(Second < Fourth));
    assert(!(Second < Fifth));
    cout << "Third<Fourth: " << (Third < Fourth) << "\t Third<Fifth: " << (Third < Fifth) << endl;
    assert(!(Third < Fourth));
    assert(!(Third < Fifth));
    cout << "Fourth<Fifth: " << (Fourth<Fifth) << endl;
    assert(!(Fourth<Fifth));

    cout << "Creating 4 WorkUnits for a sorting test with an std::set (be the only differrence between fourth and fifth was the address of the workunit, and we cannot control that.):" << endl;
    PiMakerWorkUnit *WorkUnitK1 = new PiMakerWorkUnit(500,"First",false);
    PiMakerWorkUnit *WorkUnitK2 = new PiMakerWorkUnit(500,"Second",false);
    PiMakerWorkUnit *WorkUnitK3 = new PiMakerWorkUnit(500,"Third",false);
    PiMakerWorkUnit *WorkUnitK4 = new PiMakerWorkUnit(500,"Fourth",false);
    First.Unit=WorkUnitK1;
    Second.Unit=WorkUnitK2;
    Third.Unit=WorkUnitK3;
    Fourth.Unit=WorkUnitK4;
    std::set<WorkUnitKey> WorkUnitKeyTest;
    WorkUnitKeyTest.insert(Second);
    WorkUnitKeyTest.insert(Fourth);
    WorkUnitKeyTest.insert(Third);
    WorkUnitKeyTest.insert(First);
    std::set<WorkUnitKey>::reverse_iterator Iter=WorkUnitKeyTest.rbegin();
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    assert( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("First") );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    assert( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Second") );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    assert( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Third") );
    Iter++;
    cout << ((PiMakerWorkUnit*)(Iter->Unit))->Name << " ";
    assert( ((PiMakerWorkUnit*)(Iter->Unit))->Name == String("Fourth") );
    Iter++;
    cout << endl;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Tests for the Framescheduler getting the next WorkUnit.
    cout << endl << "Creating a simple dependency chain in 4 WorkUnits and inserting them into a Test FrameScheduler. Then they will be pulled out one at a time and mark them as completed: " << endl;

    FrameScheduler SchedulingTest1(&cout,1);
    ThreadSpecificStorage Storage1(&SchedulingTest1);
    WorkUnitK4->AddDependency(WorkUnitK3);
    WorkUnitK3->AddDependency(WorkUnitK2);
    WorkUnitK2->AddDependency(WorkUnitK1);
    SchedulingTest1.AddWorkUnit(WorkUnitK1); // no deletes required the Scheduler takes ownership
    SchedulingTest1.AddWorkUnit(WorkUnitK2);
    SchedulingTest1.AddWorkUnit(WorkUnitK3);
    SchedulingTest1.AddWorkUnit(WorkUnitK4);
    SchedulingTest1.SortWorkUnits();

    WorkUnit* Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PiMakerWorkUnit*)Counter)->Name == String("First") );
    Counter->operator()(Storage1, SchedulingTest1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PiMakerWorkUnit*)Counter)->Name == String("Second") );
    Counter->operator()(Storage1, SchedulingTest1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PiMakerWorkUnit*)Counter)->Name == String("Third") );
    Counter->operator()(Storage1, SchedulingTest1);
    Counter = SchedulingTest1.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PiMakerWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PiMakerWorkUnit*)Counter)->Name == String("Fourth") );
    Counter->operator()(Storage1, SchedulingTest1);

    cout << endl << "Creating 4 WorkUnits with precise runtimes and inserting them into a Test FrameScheduler. Then they will be pulled out one at a time and mark them as completed: " << endl;
    FrameScheduler SchedulingTest2(&cout,1);
    ThreadSpecificStorage Storage2(&SchedulingTest2);

    PausesWorkUnit *FiveHundred = new PausesWorkUnit(500,"FiveHundred-ms");
    PausesWorkUnit *FiveThousand = new PausesWorkUnit(5000,"FiveThousand-ms");
    PausesWorkUnit *FiftyThousand = new PausesWorkUnit(50000,"FiftyThousand-ms");
    //PausesWorkUnit *FiveHundredThousand = new PausesWorkUnit(500000,"FiveHundredThousand-ms");
    cout << "Work Units (FiveHundred-ms, FiveThousand-ms, FiftyThousand-ms)[ms is microseconds in this context] Created, executing each ten times: " << endl;
    for(Int8 Counter = 0; Counter <10; ++Counter)
    {
        FiveHundred->operator()(Storage2,SchedulingTest2);
        FiveThousand->operator()(Storage2,SchedulingTest2);
        FiftyThousand->operator()(Storage2,SchedulingTest2);
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
    SchedulingTest2.SortWorkUnits();

    cout << "Extracting WorkUnits with the scheduling mechanism: " << endl;
    //Counter = SchedulingTest2.GetNextWorkUnit();
    //cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    //assert( ((PausesWorkUnit*)Counter)->Name == String("FiveHundredThousand-ms") );
    //Counter->operator()(Storage2, SchedulingTest2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PausesWorkUnit*)Counter)->Name == String("FiftyThousand-ms") );
    Counter->operator()(Storage2, SchedulingTest2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PausesWorkUnit*)Counter)->Name == String("FiveThousand-ms") );
    Counter->operator()(Storage2, SchedulingTest2);
    Counter = SchedulingTest2.GetNextWorkUnit();
    cout << "Getting the WorkUnit Named " << ((PausesWorkUnit*)Counter)->Name << " and marking it as complete." << endl;
    assert( ((PausesWorkUnit*)Counter)->Name == String("FiveHundred-ms") );
    Counter->operator()(Storage2, SchedulingTest2);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Thread creation and destruction
    cout << endl << "Creating a FrameScheduler with 4 WorkUnits Running one frame with different thread counts: " << endl;
    std::stringstream LogCache;
    FrameScheduler ThreadCreationTest1(&LogCache,1);
    PiMakerWorkUnit* WorkUnitR1 = new PiMakerWorkUnit(50000,"Run1",false);
    PiMakerWorkUnit* WorkUnitR2 = new PiMakerWorkUnit(50000,"Run2",false);
    PiMakerWorkUnit* WorkUnitR3 = new PiMakerWorkUnit(50000,"Run3",false);
    PiMakerWorkUnit* WorkUnitR4 = new PiMakerWorkUnit(50000,"Run4",false);
    LogBufferSwapper Swapper2;
    LogAggregator Agg2;
    ThreadSpecificStorage SwapResource2(&ThreadCreationTest1);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR1);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR2);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR3);
    ThreadCreationTest1.AddWorkUnit(WorkUnitR4);

    cout << "Thread count on initial creation: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2, ThreadCreationTest1);
    Agg2(SwapResource2, ThreadCreationTest1);
    CheckSchedulerLog(LogCache,1,4);
    cout << "It ran correctly. Emitting log." << endl;
    cout << LogCache.str() << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(2);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2, ThreadCreationTest1);
    Agg2(SwapResource2, ThreadCreationTest1);
    CheckSchedulerLog(LogCache,2,4);
    cout << "It ran correctly. Emitting log." << endl;
    cout << LogCache.str() << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(3);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2, ThreadCreationTest1);
    Agg2(SwapResource2, ThreadCreationTest1);
    CheckSchedulerLog(LogCache,3,4);
    cout << "It ran correctly. Emitting log." << endl;
    cout << LogCache.str() << endl;
    LogCache.str("");

    ThreadCreationTest1.SetThreadCount(4);
    cout << endl << "Thread count after setting to: " << ThreadCreationTest1.GetThreadCount() << endl;
    cout << "Running One Frame." << endl;
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2, ThreadCreationTest1);
    Agg2(SwapResource2, ThreadCreationTest1);
    CheckSchedulerLog(LogCache,4,4);
    cout << "It ran correctly. Emitting log." << endl;
    cout << LogCache.str() << endl;
    LogCache.str("");

    Whole Work = 1000;
    cout << endl << "Leaving thread count alone and adding " << Work << " WorkUnits to the test scheduler" << endl;
    cout << "Running One Frame." << endl;
    for (Whole Counter=0; Counter<Work; ++Counter)
        { ThreadCreationTest1.AddWorkUnit( new PiMakerWorkUnit(50000,"Dyn"+ToString(Counter),false) ); }
    ThreadCreationTest1.DoOneFrame();
    Swapper2(SwapResource2, ThreadCreationTest1);
    Agg2(SwapResource2, ThreadCreationTest1);
    CheckSchedulerLog(LogCache,4,1004);
    cout << "It ran correctly." << endl;
    LogCache.str("");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing FrameScheduler timing
    cout << endl << "Creating a few Schedulers with only one work unit and testing a variety of framerates." << endl;
    std::vector<Whole> Rates;
    Rates.push_back(10);
    Rates.push_back(25);
    Rates.push_back(28);
    Rates.push_back(30);
    Rates.push_back(60);
    Rates.push_back(100);

    for(std::vector<Whole>::iterator Iter = Rates.begin(); Iter!=Rates.end(); ++Iter)
    {
        cout << "Creating a Scheduler with only one work unit " << *Iter << " Frame Per Second running " << *Iter << " frames. " << endl;
        FrameScheduler TimingTest(&LogCache,1);
        PiMakerWorkUnit* WorkUnitTT1 = new PiMakerWorkUnit(50,"ForeverAlone",false);
        TimingTest.AddWorkUnit(WorkUnitTT1);
        Whole TimingTestStart = GetTimeStamp();
        for(Whole Counter=0; Counter<*Iter; ++Counter)
        {
            TimingTest.SetFrameRate(*Iter);
            TimingTest.DoOneFrame();
        }
        Whole TimingTestEnd = GetTimeStamp();
        Whole TestLength = TimingTestEnd-TimingTestStart;
        cout << "  " << *Iter << " Frames took " << TestLength << " microseconds to run, should be around 1000000 (one million)." << endl;
        Integer Error = TestLength - 1000000;
        Error = (Error>0.0) ? Error : -Error;
        double Variance = (double(Error))/double(1000000) * 100;
        double ErrorPerFrame = double(Error)/double(*Iter);
        cout << "  " << "This is a variance of " << Error << " Frames or " << Variance << "%. Which is " << ErrorPerFrame << " microsecond drift per frame." << endl;
        //assert(0.3>Variance); // Allow a .3% variance - incosistent achievable even on even on winxp with its crappy 3.5 millisecond timer
        //assert(0.1>Variance); // Allow a .1% variance - This is very achievable with an accurate timer, bu
    }


    //put verifcation




    #if defined(_MEZZ_THREAD_WIN32_)
    system("pause");
    #endif
}
#endif
