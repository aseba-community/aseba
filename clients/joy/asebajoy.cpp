#include <dashel/dashel.h>
#include "../../common/msg/msg.h"
#include "../../common/utils/utils.h"
#include <SDL.h>
#include <cmath>
#include <cassert>
#include <algorithm>

using namespace Dashel;
using namespace Aseba;
using namespace std;

enum ExitCodes
{
	EXIT_OK = 0,
	EXIT_DASHEL_EXCEPTION_OCCURED,
	EXIT_COULD_NOT_INIT_SDL,
	EXIT_NOT_SUITABLE_JOY_FOUND,
};

enum MessagesTypes
{
	MESSAGE_ID_AXIS0 = 0,
	MESSAGE_ID_AXIS1,
	MESSAGE_ID_AXIS2,
	MESSAGE_ID_AXIS3,
	MESSAGE_ID_AXIS4,
	MESSAGE_ID_HAT,
	MESSAGE_ID_BUTTON0,
	MESSAGE_ID_BUTTON1,
	MESSAGE_ID_BUTTON2,
	MESSAGE_ID_BUTTON3,
	MESSAGE_ID_BUTTON4,
	MESSAGE_ID_BUTTON5,
	MESSAGE_ID_BUTTON6,
	MESSAGE_ID_BUTTON7,
	MESSAGE_ID_BUTTON8,
};

class Joystick
{
protected:
	int number;
	SDL_Joystick *joy;
	int oa[5];
	int ob[9];
	int ohat;
	
public:
	Joystick(int number, SDL_Joystick *joy):
		number(number), joy(joy)
	{
		fill(oa, oa+5, 0);
		fill(ob, ob+9, 0);
		ohat = 0;
	}
	
	~Joystick()
	{
		SDL_JoystickClose(joy);
	}
	
	void process(Stream* stream)
	{
		VariablesDataVector data(2);
		data[0] = number;
		
		// axes
		for (int i = 0; i < std::min(5, SDL_JoystickNumAxes(joy)); ++i)
		{
			int a = SDL_JoystickGetAxis(joy, i);
			if (a != oa[i])
			{
				data[1] = short(a);
				UserMessage(MESSAGE_ID_AXIS0 + i, data).serialize(stream);
				stream->flush();
				oa[i] = a;
			}
		}
		
		// hat
		if (SDL_JoystickNumHats(joy) > 0)
		{
			int hat = SDL_JoystickGetHat(joy, 0);
			if (hat != ohat)
			{
				data[1] = short(hat);
				UserMessage(MESSAGE_ID_HAT, data).serialize(stream);
				stream->flush();
				ohat = hat;
			}
		}
		
		// buttons
		for (int i = 0; i < std::min(9, SDL_JoystickNumButtons(joy)); ++i)
		{
			int b = SDL_JoystickGetButton(joy, i);
			if (b != ob[i])
			{
				data[1] = short(b);
				UserMessage(MESSAGE_ID_BUTTON0 + i, data).serialize(stream);
				stream->flush();
				ob[i] = b;
			}
		}
	}
};

class JoystickReader : public Hub
{
protected:
	Stream* stream;
	vector<Joystick*> joysticks;
	
public:
	JoystickReader(const char* target)
	{
		// SDL inint stuff
		if((SDL_Init(SDL_INIT_JOYSTICK)==-1))
		{
			cerr << "Error : Could not initialize SDL: " << SDL_GetError() << endl;
			exit(EXIT_COULD_NOT_INIT_SDL);
		}
		
		// list all joysticks
		const int count = SDL_NumJoysticks();
		for (int i = 0; i < count; ++i)
		{
			SDL_Joystick* joy = SDL_JoystickOpen(i);
			if (!joy)
			{
				cerr << "Warning: cannot open joystick " << i << endl;
				continue;
			}
			if (SDL_JoystickNumAxes(joy) < 2)
			{
				cerr << "Warning: not enough axis on joystick " << i << endl;
				continue;
			}
			
			joysticks.push_back(new Joystick(i, joy));
		}
		
		// we need at least one
		if (joysticks.empty())
		{
			cerr << "Error: not suitable joystick found" << endl;
			exit(EXIT_NOT_SUITABLE_JOY_FOUND);
		}
		
		cerr << "Found " << joysticks.size() << " joysticks" << endl;
		
		// connect to Dashel
		stream = Hub::connect(target);
		cout << "Connected to " << stream->getTargetName() << endl;
	}
	
	~JoystickReader()
	{
		for (size_t i = 0; i < joysticks.size(); ++i)
			delete joysticks[i];
		SDL_Quit();
	}

public:
	void processJoysticks()
	{
		SDL_JoystickUpdate();
		for (size_t i = 0; i < joysticks.size(); ++i)
			joysticks[i]->process(stream);
	}
	
protected:
	void incomingData(Stream *stream)
	{
		delete Message::receive(stream);
	}
	
	void connectionClosed(Stream *stream, bool abnormal)
	{
		dumpTime(cerr);
		cerr << "Connection closed to " << stream->getTargetName();
		if (abnormal)
			cerr << " : " << stream->getFailReason();
		cerr << endl;
		stop();
	}
};

int main(int argc, char *argv[])
{
	const char *target = ASEBA_DEFAULT_TARGET;
	
	if (argc >= 2)
		target = argv[1];
	
	try
	{
		JoystickReader reader(target);
		while (reader.step(10))
			reader.processJoysticks();
		return 0;
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_DASHEL_EXCEPTION_OCCURED;
	}
}
