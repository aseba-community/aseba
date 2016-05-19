/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
        Stephane Magnenat <stephane at magnenat dot net>
        (http://stephane.magnenat.net)
        and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dashel/dashel.h>
#include "../../../../common/consts.h"
#include "../../../../common/msg/msg.h"
#include "../../../../common/utils/utils.h"
#include "../../../../transport/dashel_plugins/dashel-plugins.h"
#include <time.h>
#include <iostream>
#include <cstring>
#include <string>
#include <thread>

// MockAsebascratch imitates the sequence of Aseba messages that asebascratch would generate from
// a simple Scratch program that combines wait blocks with instantaneous blocks. The robot *must*
// be running the thymio_motion_10Hz.aesl program. MockAsebascratch relies on Q_motion events and
// R_state_update to know which threads are busy, and waits for busy threads as does Scratch.

namespace Aseba
{
    using namespace Dashel;
    using namespace std;
    
    class MockAsebascratch : public Hub
    {
    private:
        Stream* in;
        bool do_R_state_update;

    public:
        std::set<short> busy;
        
    public:
        MockAsebascratch() :
        do_R_state_update(false)
        {
        }
        
        void sendEvent(const unsigned eventid, const UserMessage::DataVector data)
        {
            UserMessage userMessage(eventid, data);
            for (auto stream: dataStreams)
            {
                userMessage.serialize(stream);
                stream->flush();
            }
        }
        
       
    protected:
        void incomingUserMsg(const UserMessage *userMsg)
        {
            if (userMsg->type == eventid["R_state_update"] && do_R_state_update)
            {
                assert (userMsg->data.size() >= 27);
                
                busy.clear();
                std::vector<short> Qid(userMsg->data.begin()+23, userMsg->data.begin()+26);
                for( auto id: Qid)
                    if (id != 0)
                        busy.insert(id);
            }
            else if (userMsg->type == eventid["Q_motion_added"])
            {
                assert (userMsg->data.size() >= 5);
                
                busy.insert(userMsg->data[0]);
                cout << "Q_motion_added " << userMsg->data[0] << endl;
            }
            else if (userMsg->type == eventid["Q_motion_ended"])
            {
                assert (userMsg->data.size() >= 5);
                
                busy.erase(userMsg->data[0]);
                cout << "Q_motion_ended " << userMsg->data[0] << endl;
            }

            // cout << "busy size " << busy.size() << endl;
        }
        
        void connectionCreated(Stream *stream)
        {
            cout << "Connected to " << stream->getTargetName()  << endl;
        }
        
        void incomingData(Stream *stream)
        {
            Message *message(Message::receive(stream));
            const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
            if (userMsg)
                incomingUserMsg(userMsg);
        }
        
        void connectionClosed(Stream *stream, bool abnormal)
        {
            if (stream == in)
                stop();
        }
        
    public:
        std::map<std::string, unsigned > eventid =
        std::map<std::string, unsigned >({ {"Q_add_motion",0},{"Q_cancel_motion",1},{"Q_motion_added",2},
            {"Q_motion_cancelled",3},{"Q_motion_started",4},{"Q_motion_ended",5},{"Q_motion_noneleft",6},
            {"Q_set_odometer",7},{"V_leds_prox_h",8},{"V_leds_circle",9},{"V_leds_top",10},
            {"V_leds_bottom",11},{"V_leds_prox_v",12},{"V_leds_buttons",13},{"V_leds_rc",14},
            {"V_leds_temperature",15},{"V_leds_sound",16},{"A_sound_freq",17},{"A_sound_play",18},
            {"A_sound_system",19},{"A_sound_replay",20},{"A_sound_record",21},{"M_motor_left",22},
            {"M_motor_right",23},{"R_state_update",24} });
    };
    
    class EventGenerator
    {
    public:
        EventGenerator(Aseba::MockAsebascratch * mock, unsigned duration, unsigned timeout, unsigned delay) :
        mock(mock),
        duration(duration),
        timeout(timeout),
        delay(UnifiedTime(delay))
        {
        }
        
        void waitFor(unsigned jobid)
        {
            const UnifiedTime startTime;
            const UnifiedTime endTime(timeout + startTime.value);

            cout << "Wait for " << jobid << " in busy" << endl;
            while (mock->busy.count(jobid) != 0)
            {
                const UnifiedTime scratch_video_refresh(33);
                scratch_video_refresh.sleep();
                const UnifiedTime now;
                if (now > endTime)
                {
                    cerr << "Timeout for " << jobid << endl;
                    cout << "Q_cancel_motion " << jobid << endl;
                    mock->sendEvent(mock->eventid["Q_cancel_motion"],
                                    UserMessage::DataVector({(sint16)jobid}));
                    break;
                }
                std::this_thread::yield();
            }
        }
        
        void run()
        {
	  for (sint16 jobid = 1; jobid <= sint16(duration) * 2; jobid++)
            {
                cout << "EventGenerator::run jobid " << jobid << endl;

                // Each step combines wait events and instantaneous events.
                // The pattern is: long arc forward (wait), short arc backwards (wait), beep.
                // The top LEDs are red while a job is in progress, green otherwise.
                
                mock->sendEvent(mock->eventid["V_leds_top"],
                                UserMessage::DataVector({8,0,0}));
                
                cout << "Q_add_motion " << jobid << endl;
                mock->sendEvent(mock->eventid["Q_add_motion"],
                                jobid % 2
                                ? UserMessage::DataVector({jobid,350,400,324})
                                : UserMessage::DataVector({jobid,187,-400,-20}));
                mock->busy.insert(jobid);
                waitFor(jobid);
                
                mock->sendEvent(mock->eventid["V_leds_top"],
                                UserMessage::DataVector({0,8,0}));
                if ((jobid % 2) == 0)
                {
                    mock->sendEvent(mock->eventid["A_sound_system"],
                                    UserMessage::DataVector({5}));
                    if (delay.value > 0)
                        delay.sleep();
                }
            }
            
            mock->stop();
        }
    
    protected:
        Aseba::MockAsebascratch * mock;
        unsigned duration;
        unsigned timeout;
        UnifiedTime delay;
    };
}

void dumpHelp(std::ostream &stream, const char *programName)
{
    stream << "Mock-scratch, imitate Scratch protocol with thymio_motion.aesl, usage:\n";
    stream << programName << " [options] [targets]*\n";
    stream << "Options:\n";
    stream << "--EVENT=ID      : redefine an AESL event id\n";
    stream << "-k ITERATIONS   : run for ITERATIONS steps (default: 13)\n";
    stream << "-t TIMEOUT      : maximum wait for motion event (default: 20000 ms)\n";
    stream << "-w WAIT         : wait between iterations (default: 0 ms)\n";
    stream << "-h, --help      : shows this help\n";
    stream << "-V, --version   : shows the version number\n";
    stream << "Targets are any valid Dashel targets." << std::endl;
    stream << "Report bugs to: david.sherman@inria.fr" << std::endl;
}

void dumpVersion(std::ostream &stream)
{
    stream << "Mock Scratch " << ASEBA_VERSION << std::endl;
    stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
    stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

int main(int argc, char *argv[])
{
    Dashel::initPlugins();
    std::vector<std::string> targets;
    unsigned iterations = 13;  //
    unsigned timeout = 20000;  // roughly 4 meters at Thymio top speed
    unsigned delay = 0;        // if necessary to imitate Scratch
    
    int argCounter = 1;
    
    while (argCounter < argc)
    {
        const char *arg = argv[argCounter];
        
        if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
        {
            dumpHelp(std::cout, argv[0]);
            return 0;
        }
        else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
        {
            dumpVersion(std::cout);
            return 0;
        }
        else if (strcmp(arg, "-k") == 0)
        {
            argCounter++;
            if (argCounter >= argc)
            {
                dumpHelp(std::cout, argv[0]);
                return 1;
            }
            else
                iterations = atoi(argv[argCounter]);
        }
        else if (strcmp(arg, "-t") == 0)
        {
            argCounter++;
            if (argCounter >= argc)
            {
                dumpHelp(std::cout, argv[0]);
                return 1;
            }
            else
                timeout = atoi(argv[argCounter]);
        }
        else if (strcmp(arg, "-w") == 0)
        {
            argCounter++;
            if (argCounter >= argc)
            {
                dumpHelp(std::cout, argv[0]);
                return 1;
            }
            else
                delay = atoi(argv[argCounter]);
        }
        else
        {
            targets.push_back(argv[argCounter]);
        }
        argCounter++;
    }
    
    if (targets.empty())
        targets.push_back(ASEBA_DEFAULT_TARGET);
    
    try
    {
        // Create MockAsebascratch hub with given targets
        Aseba::MockAsebascratch mock;
        for (size_t i = 0; i < targets.size(); i++)
            mock.connect(targets[i]);

        // Create EventGenerator thread with chosen number of iterations
        std::thread gen_thread(&Aseba::EventGenerator::run,
                               Aseba::EventGenerator(&mock,iterations,timeout,delay));
        
        // Run threads. Mock will be stopped by EventGenerator::run
        mock.run();
        gen_thread.join();
    }
    catch(Dashel::DashelException& e)
    {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
