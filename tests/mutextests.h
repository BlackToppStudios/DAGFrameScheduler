//© Copyright 2010 - 2013 BlackTopp Studios Inc.
/* This file is part of The Mezzanine Engine.

    The Mezzanine Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Mezzanine Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Mezzanine Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
/* The original authors have included a copy of the license specified above in the
   'Docs' folder. See 'gpl.txt'
*/
/* We welcome the use of the Mezzanine engine to anyone, including companies who wish to
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
#ifndef _mutextests_h
#define _mutextests_h

#include "mezztest.h"

#include "dagframescheduler.h"

/// @file
/// @brief Basic tests of the mutex class

using namespace std;
using namespace Mezzanine;
using namespace Mezzanine::Testing;
using namespace Mezzanine::Threading;

/// @brief Used in testing Basic Mutex
static Mezzanine::Threading::Thread::id ThreadIDTest=0;
/// @brief Used in testing Basic Mutex
static Mezzanine::Threading::Mutex ThreadIDLock;
/// @brief Used in testing Basic Mutex
void PutIdInGlobal(void*)
{
    cout << "Thread T2 trying to lock mutex ThreadIDLock, thread has id: " << Mezzanine::Threading::this_thread::get_id() << endl;
    ThreadIDLock.Lock();
    cout << "Thread T2 locked mutex: " << endl;
    ThreadIDTest = Mezzanine::Threading::this_thread::get_id();
    cout << "Thread T2 work complete unlocking mutex: " << endl;
    ThreadIDLock.Unlock();
}

/// @brief Used in mutex try_lock tests
static Mezzanine::Integer TryLockTest=0;
/// @brief Used in mutex try_lock tests
static Mezzanine::Threading::Mutex TryLock;
/// @brief Used in mutex try_lock tests
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


/// @brief Tests for the mutex class
class mutextests : public UnitTestGroup
{
    public:
        /// @copydoc Mezzanine::Testing::UnitTestGroup::Name
        /// @return Returns a String containing "thread"
        virtual String Name()
            { return String("mutex"); }

        /// @copydoc Mezzanine::Testing::UnitTestGroup::RunTests
        /// @detail Test if the mutex works correctly were possible
        virtual void RunTests(bool RunAutomaticTests, bool RunInteractiveTests)
        {
            RunInteractiveTests = false; //prevent warnings

            if (RunAutomaticTests)
            {
                TestResult temp;

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
                if(ThreadIDTest == T2id)
                    { temp=Testing::Success; }
                else
                    { temp=Testing::Failed; }
                AddTestResult("DAGFrameScheduler::mutex::lock", temp);
                ThreadIDLock.Unlock();

                cout << "Joining T2" << endl;
                T2.join();
            }else{
                AddTestResult("DAGFrameScheduler::mutex::lock", Testing::Skipped);

            }

            if (RunAutomaticTests)
            {
                TestResult temp;
                cout << "Testing Mutex try_lock()" << endl;

                cout << "Locking TryLock in main thread with id: " << Mezzanine::Threading::this_thread::get_id() << endl;
                if(TryLock.TryLock())
                    { temp=Testing::Success; }
                else
                    { temp=Testing::Failed; }
                AddTestResult("DAGFrameScheduler::Thread::TryLock", temp);

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
                if(TryLockTest == Value)
                    { temp=Testing::Success; }
                else
                    { temp=Testing::Failed; }
                AddTestResult("DAGFrameScheduler::Thread::TryLockExclude", temp);

            }else{
                AddTestResult("DAGFrameScheduler::Thread::TryLock", Skipped);
                AddTestResult("DAGFrameScheduler::Thread::TryLockExclude", Skipped);
            }

            /*if (RunInteractiveTests)
            {   
                AddTestResult("Threading::Interactive", Success
                              );
            }else{
                AddTestResult("Threading::Interactive", Skipped);
            }*/
        }
};

#endif

