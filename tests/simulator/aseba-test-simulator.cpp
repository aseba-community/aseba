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

#include "../../targets/playground/DirectAsebaGlue.h"
#include "../../common/msg/NodesManager.h"
#include <iostream>

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
			thymio->outQueue.front()->dump(wcout);
			wcout << endl;
			processMessage(thymio->outQueue.front().get());
			thymio->outQueue.pop();
		}
	}
};

int main()
{
	// parameters
	const double dt(0.03);

	// create world, robot and nodes manager
	World world(40, 20);

	DirectAsebaThymio2* thymio(new DirectAsebaThymio2());
	thymio->pos = {10, 10};
	world.addObject(thymio);

	TestNodesManager testNodesManager(thymio);

	// list the nodes and step, the robot should send its description to the nodes manager
	thymio->inQueue.emplace(ListNodes().clone());

	// we define a step lambda
	auto step = [&]()
	{
		world.step(dt);
		testNodesManager.step();
		wcout << L"- stepped, robot pos: " << thymio->pos.x << L", " << thymio->pos.y << endl;
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

	// compile a small code
	Compiler compiler;
	CommonDefinitions commonDefinitions;
	compiler.setTargetDescription(targetDescription);
	compiler.setCommonDefinitions(&commonDefinitions);
	const wchar_t program[] =
		L"onevent prox\n"
		L"when prox.horizontal[2] <= 1000 do\n"
			L"motor.left.target = 250\n"
			L"motor.right.target = 250\n"
		L"end\n"
		L"when prox.horizontal[2] >= 2000 do\n"
			L"motor.left.target = 0\n"
			L"motor.right.target = 0\n"
		L"end\n";
	wistringstream programStream(program);
	BytecodeVector bytecode;
	unsigned allocatedVariablesCount;
	Error errorDescription;
	const bool compilationResult(compiler.compile(programStream, bytecode, allocatedVariablesCount, errorDescription));
	if (!compilationResult)
	{
		wcerr << L"compilation error: " << errorDescription.toWString() << endl;
		return 4;
	}

	// fill the bytecode messages
	vector<Message*> setBytecodeMessages;
	sendBytecode(setBytecodeMessages, nodeId, vector<uint16>(bytecode.begin(), bytecode.end()));
	for_each(setBytecodeMessages.begin(), setBytecodeMessages.end(), [=](Message* message){ thymio->inQueue.emplace(message); });

	// then run the code...
	thymio->inQueue.emplace(new Run(nodeId));

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

	return 0;
}