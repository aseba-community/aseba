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

#ifndef VPL_USAGE_LOGGER_H
#define VPL_USAGE_LOGGER_H

#ifdef PROTOBUF_FOUND 

#include <google/protobuf/stubs/common.h>

#define USAGE_LOG(x) UsageLogger::getLogger().x
#define ENABLE_USAGE_LOG() Aseba::ThymioVPL::UsageLogger::setLoggingState(true)

#include <iostream>
#include <fstream>
#include <string>

#include <QGraphicsSceneMouseEvent>
#include <QString>
#include <QMouseEvent>
#include <QDrag>
#include <QDropEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QObject>


#include "Scene.h"
#include "Buttons.h"
#include "../../../../compiler/compiler.h"
#include <UsageProfile.pb.h>
#include "LogSignalMapper.h"


namespace Aseba { namespace ThymioVPL
{
	
class UsageLogger : public QObject
{
	Q_OBJECT
	
public:
	static UsageLogger & getLogger();
	static int getRow(Block * b);
	static void setLoggingState(bool enabled);
	
	public slots:
	void logGUIEvents(unsigned int senderId, QObject *originalSender, QObject * logicalParent);

	public:
	void setScene(Scene * scene);
	
	// Actions logged
	void logOpenHelp();
	void logSaveSnapshot();
	void logNewFile();
	void logOpenFile();
	void logSave();
	void logSaveAs();
	void logCloseFile();
	void logStop();
	void logRun();
	void logInsertSet(int row);
	void logRemoveSet(int row);
	void logBlockMouseMove(QString name, QString type, QGraphicsSceneMouseEvent *event);
	void logBlockMouseRelease(QString name, QString type, QGraphicsSceneMouseEvent *event);
	void logButtonDrag(QString name, QString type, QMouseEvent *event, QDrag *drag);
	void logSetAdvanced(bool advanced);
	void logAddEventBlock(int row, Block *block);
	void logAddActionBlock(int row, Block *block, int position);
	void logActionSetDrag(int row,QGraphicsSceneMouseEvent *event, QDrag *drag);
	void logAccEventBlockMode(QString name, QString type, int mode);
	void logDropButton(BlockButton *block, QDropEvent *event);
	void logEventActionSetDrop(int row, QGraphicsSceneDragDropEvent *event);
	void logUserEvent(unsigned id, const VariablesDataVector& data);
	void logTabletData(const VariablesDataVector& data);
	void logSignal(const QObject * sender, const char * signal, unsigned int senderId, QObject * logicalParent);
	void logBlockAction(BlockActionType type, QString blockName, QString blockType, int row, int elementId, int * sliderValue, unsigned int * soundValue, unsigned int * timeValue, int * buttonValue);
	
private:
	UsageLogger();
	virtual ~UsageLogger();
	Action *action;
	LogSignalMapper signalMapper;
	QString groupName;
	static bool loggingEnabled;
	
protected:
	virtual void storeAction(Action * action);
	Action * getActionWithCurrentState();
	QString getTimeStampString();
	void gen_random(char *s, const int len);
	MouseButton mapButtons(Qt::MouseButton b);
	MouseButton mapButtons(Qt::MouseButtons b);
	void logMenuAction(MenuEntry entry);
	void logMouseAction(MouseActionType type, double xPos, double yPos, MouseButton button, const int * row, const char * blockName, const char * blockType);
	void logSetAction(RowAction_ActionType type, int row);
	void logAddBlock(BlockType type,int row, Block *block);
	void askForGroupName();
	unsigned int getMilliseconds();
	
	std::ofstream * fileOut;
	Scene * scene;
};

}}

#else /*PROTOBUF_FOUND*/

#define USAGE_LOG(x) 
#define ENABLE_USAGE_LOG() 

#endif

#endif // VPL_USAGE_LOGGER_H
