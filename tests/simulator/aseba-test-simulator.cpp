/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#include "targets/playground/EnkiGlue.h"
#include "targets/playground/Robots.h"
#include "common/msg/NodesManager.h"
#include "compiler/compiler.h"
#include <iostream>
#include <iterator>

using namespace Aseba;
using namespace Enki;
using namespace std;

struct TestNodesManager: NodesManager
{
	DirectAsebaThymio2* thymio;

	TestNodesManager(DirectAsebaThymio2* thymio): thymio(thymio) {}

	virtual void sendMessage(const Message& message)
	{
		thymio->inQueue.emplace(message.clone());
	}

	void step()
	{
		while (!thymio->outQueue.empty())
		{
			wcout << L"M ";
			thymio->outQueue.front()->dump(wcout);
			wcout << endl;
			processMessage(thymio->outQueue.front().get());
			thymio->outQueue.pop();
		}
	}
};

struct TestSimulatorEnvironment: SimulatorEnvironment
{
	World& world;

	TestSimulatorEnvironment(World& world): world(world) {}

	virtual void notify(const EnvironmentNotificationType type, const string& description, const strings& arguments) override
	{
		cerr << "N " << description;
		copy(arguments.begin(), arguments.end(), ostream_iterator<string>(cerr, " "));
		cerr << endl;
	}

	virtual string getSDFilePath(const string& robotName, unsigned fileNumber) const override
	{
		return string("SD_FILE_") + to_string(fileNumber) + ".DAT";
	}

	virtual World* getWorld() const override
	{
		return &world;
	}
};

int main()
{
	// parameters
	const double dt(0.03);

	// create world, robot and nodes manager
	World world(40, 20);
	simulatorEnvironment.reset(new TestSimulatorEnvironment(world));

	DirectAsebaThymio2* thymio(new DirectAsebaThymio2("thymio2_0", 1));
	thymio->pos = {10, 10};
	world.addObject(thymio);

	TestNodesManager testNodesManager(thymio);

	cout << "\n* Discovering robot\n" << endl;

	// list the nodes and step, the robot should send its description to the nodes manager
	thymio->inQueue.emplace(ListNodes().clone());

	// we define a step lambda
	auto step = [&]()
	{
		world.step(dt);
		testNodesManager.step();
		//wcout << L"- stepped, robot pos: " << thymio->pos.x << L", " << thymio->pos.y << endl;
	};

	// step twice for the detection and enumeration round-trip
	step();
	step();

	// check that the nodes manager has received the description from the robot
	bool ok(false);
	unsigned nodeId(testNodesManager.getNodeId(L"thymio-II", 0, &ok));
	if (!ok)
	{
		cerr << "nodes manager did not find \"thymio-II\"" << endl;
		return 1;
	}
	if (nodeId != 1)
	{
		cerr << "nodes manager did not return the right nodeId for \"thymio-II\", should be 1, was " << nodeId << endl;
		return 2;
	}
	const TargetDescription *targetDescription(testNodesManager.getDescription(nodeId));
	if (!targetDescription)
	{
		cerr << "nodes manager did not return a target description for \"thymio-II\"" << endl;
		return 3;
	}

	// we define a lambda to load and run a program
	auto loadAndRun = [&](const wchar_t program[])
	{
		// compile a small code
		Compiler compiler;
		CommonDefinitions commonDefinitions;
		compiler.setTargetDescription(targetDescription);
		compiler.setCommonDefinitions(&commonDefinitions);
		wistringstream programStream(program);
		BytecodeVector bytecode;
		unsigned allocatedVariablesCount;
		Error errorDescription;
		const bool compilationResult(compiler.compile(programStream, bytecode, allocatedVariablesCount, errorDescription));
		if (!compilationResult)
		{
			wcerr << L"compilation error: " << errorDescription.toWString() << endl;
			exit(4);
		}

		// fill the bytecode messages
		vector<unique_ptr<Message>> setBytecodeMessages;
		sendBytecode(setBytecodeMessages, nodeId, vector<uint16_t>(bytecode.begin(), bytecode.end()));
		for_each(setBytecodeMessages.begin(), setBytecodeMessages.end(), [=](unique_ptr<Message>& message){ thymio->inQueue.emplace(move(message)); });

		// then run the code...
		thymio->inQueue.emplace(new Run(nodeId));
	};

	cout << "\n* Testing movement\n" << endl;

	// let the robot move until it sees an obstacle
	loadAndRun(
		L"onevent prox\n"
		L"when prox.horizontal[2] <= 1000 do\n"
			L"motor.left.target = 250\n"
			L"motor.right.target = 250\n"
		L"end\n"
		L"when prox.horizontal[2] >= 2000 do\n"
			L"motor.left.target = 0\n"
			L"motor.right.target = 0\n"
		L"end\n"
	);

	// ...run for hundred time steps
	for (unsigned i(0); i<100; ++i)
		step();

	// and check the robot has stopped
	const Point robotPos(thymio->pos);
	step();
	const Point deltaPos(thymio->pos - robotPos);

	// if not it is an error
	if (deltaPos.x > numeric_limits<double>::epsilon() || deltaPos.y > numeric_limits<double>::epsilon())
	{
		cerr << "Robot is still moving after 100 time steps, delta: " << deltaPos.x << ", " << deltaPos.y << endl;
		return 5;
	}

	cout << "\n* Testing SD card\n" << endl;

	// write some data
	loadAndRun(
		L"var result\n"
		L"call sd.open(0, result)\n"
		L"call sd.write([1,2,3,4], result)\n"
	);
	step();

	// check result
	if (thymio->variables.freeSpace[0] != 4)
	{
		cerr << "Result of sd.write is " << thymio->variables.freeSpace[0] << " instead of 4" << endl;
		return 6;
	}

	// load back the data and compare
	loadAndRun(
		L"var data[8]\n"
		L"var result\n"
		L"call sd.open(0, result)\n"
		L"call sd.read(data[0:3], result)\n"
		L"call sd.seek(0, result)\n"
		L"call sd.read(data[4:7], result)\n"
	);
	step();

	// check result
	auto expected = {1, 2, 3, 4, 1, 2, 3, 4};
	if (!equal(expected.begin(), expected.end(), &thymio->variables.freeSpace[0]))
	{
		cerr << "Reading back result of SD is ";
		copy(&thymio->variables.freeSpace[0], &thymio->variables.freeSpace[8], ostream_iterator<int>(cerr, " "));
		cerr << " instead of ";
		copy(expected.begin(), expected.end(), ostream_iterator<int>(cerr, " "));
		cerr << endl;
		return 7;
	}

	return 0;
}
