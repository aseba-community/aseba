#ifndef VPL_USAGE_LOGGER_H
#define VPL_USAGE_LOGGER_H

#ifdef PROTOBUF_FOUND
#define LOG(x) UsageLogger::getLogger().x

#include <iostream>
#include <fstream>
#include <string>

#include <QGraphicsSceneMouseEvent>
#include <QString>
#include <QMouseEvent>
#include <QDrag>
#include <QDropEvent>
#include <QGraphicsSceneDragDropEvent>


#include "Scene.h"
#include "Buttons.h"
#include "../../../../compiler/compiler.h"
#include "UsageProfile.pb.h"


namespace Aseba { namespace ThymioVPL
{

class UsageLogger
{
public:
	static UsageLogger & getLogger();
	

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
	
private:
	UsageLogger();
	~UsageLogger();
	Action *action;
	
protected:
	void storeAction(Action * action);
	Action * getActionWithCurrentState();
	std::string getFileName();
	void gen_random(char *s, const int len);
	MouseButton mapButtons(Qt::MouseButton b);
	MouseButton mapButtons(Qt::MouseButtons b);
	void logMenuAction(MenuEntry entry);
	void logMouseAction(MouseActionType type, double xPos, double yPos, MouseButton button, const int * row, const char * blockName, const char * blockType);
	void logSetAction(RowAction_ActionType type, int row);
	void logAddBlock(BlockType type,int row, Block *block);
	unsigned int getMilliseconds();
	
	std::ofstream * fileOut;
	Scene * scene;
};

}}

#else /*PROTOBUF_FOUND*/
#define LOG(x) 
#endif

#endif // VPL_USAGE_LOGGER_H
