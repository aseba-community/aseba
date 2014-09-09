#include "UsageLogger.h"

#include <ctime>
#include "Block.h"

using namespace std;

namespace Aseba{ namespace ThymioVPL
{

#if defined(PROTOBUF_FOUND)

UsageLogger::UsageLogger()
{
	fileOut = new ofstream("test.log"/*getFileName().c_str()*/, ios::app | ios::binary);
	scene = 0;
	action = new Action();
}

UsageLogger::~UsageLogger()
{
	delete action;
	if(fileOut != 0){
		fileOut->close();
		delete fileOut;
	}
}

void UsageLogger::setScene(Scene * scene)
{
	this->scene = scene;
}

UsageLogger& UsageLogger::getLogger()
{
	static UsageLogger instance;
	return instance;
}

void UsageLogger::logInsertSet(int row)
{
	Action * wrapper = getActionWithCurrentState();
	RowAction * rowAction = new RowAction();
	rowAction->set_row(row);
	rowAction->set_type(RowAction::INSERT);
	
	wrapper->set_type(Action_ActionType_ROW);
	wrapper->set_allocated_rowaction(rowAction);
	storeAction(wrapper);
}

void UsageLogger::logRemoveSet(int row){
	Action * wrapper = getActionWithCurrentState();
	RowAction * rowAction = new RowAction();
	rowAction->set_row(row);
	rowAction->set_type(RowAction::REMOVE);
	
	wrapper->set_type(Action_ActionType_ROW);
	wrapper->set_allocated_rowaction(rowAction);
	storeAction(wrapper);
}
void UsageLogger::logBlockMouseMove(QString name, QString type, QGraphicsSceneMouseEvent *event){
	MouseButton button = mapButtons(event->button());
	
	Action * wrapper = getActionWithCurrentState();
	BlockMouseMoveAction * a = new BlockMouseMoveAction();
	a->set_button(button);
	a->set_blockname(name.toUtf8().constData());
	a->set_blocktype(type.toUtf8().constData());
	a->set_x(event->scenePos().x());
	a->set_y(event->scenePos().y());
	wrapper->set_type(Action_ActionType_BLOCK_MOUSE_MOVE);
	wrapper->set_allocated_blockmousemoveaction(a);
	storeAction(wrapper);
}
void UsageLogger::logBlockMouseRelease(QString name, QString type, QGraphicsSceneMouseEvent *event){
	MouseButton button = mapButtons(event->button());
	
	Action * wrapper = getActionWithCurrentState();
	BlockMouseReleaseAction * a = new BlockMouseReleaseAction();
	a->set_button(button);
	a->set_blockname(name.toUtf8().constData());
	a->set_blocktype(type.toUtf8().constData());
	a->set_x(event->scenePos().x());
	a->set_y(event->scenePos().y());
	wrapper->set_type(Action_ActionType_BLOCK_MOUSE_RELEASE);
	wrapper->set_allocated_blockmousereleaseaction(a);
	storeAction(wrapper);
}
void UsageLogger::logButtonDrag(QString name, QString type, QMouseEvent *event, QDrag *drag){
	MouseButton button = mapButtons(event->button());
	
	Action * wrapper = getActionWithCurrentState();
	ButtonDragAction *a = new ButtonDragAction();
	a->set_button(button);
	a->set_blockname(name.toUtf8().constData());
	a->set_blocktype(type.toUtf8().constData());
	a->set_x(event->posF().x());
	a->set_y(event->posF().y());
	
	wrapper->set_type(Action_ActionType_BUTTON_DRAG);
	wrapper->set_allocated_buttondragaction(a);
	storeAction(wrapper);
}
void UsageLogger::logSetAdvanced(bool advanced){
	Action * wrapper = getActionWithCurrentState();
	AdvancedModeAction *a = new AdvancedModeAction();
	a->set_isadvanced(advanced);
	
	wrapper->set_type(Action_ActionType_ADVANCED_MODE);
	wrapper->set_allocated_advancedmodeaction(a);
	storeAction(wrapper);
}
void UsageLogger::logAddEventBlock(int row, Block *block){
	Action * wrapper = getActionWithCurrentState();
	
	AddBlockAction *a = new AddBlockAction();
	a->set_type(EVENT);
	a->set_blockname(block->getName().toUtf8().constData());
	a->set_blocktype(block->getType().toUtf8().constData());
	a->set_row(row);
	wrapper->set_type(Action_ActionType_ADD_BLOCK);
	wrapper->set_allocated_addblockaction(a);
	storeAction(wrapper);
}
void UsageLogger::logAddActionBlock(int row, Block *block, int position){
	Action * wrapper = getActionWithCurrentState();
	
	AddBlockAction *a = new AddBlockAction();
	a->set_type(ACTION);
	a->set_blockname(block->getName().toUtf8().constData());
	a->set_blocktype(block->getType().toUtf8().constData());
	a->set_row(row);
	wrapper->set_type(Action_ActionType_ADD_BLOCK);
	wrapper->set_allocated_addblockaction(a);
	storeAction(wrapper);
}
void UsageLogger::logActionSetDrag(int row,QGraphicsSceneMouseEvent *event, QDrag *drag){
	Action * wrapper = getActionWithCurrentState();
	ActionSetDragAction *a = new ActionSetDragAction();
	
	a->set_button(mapButtons(event->button()));
	a->set_row(row);
	a->set_x(event->scenePos().x());
	a->set_y(event->scenePos().y());
	
	wrapper->set_type(Action_ActionType_ACTION_SET_DRAG);
	wrapper->set_allocated_actionsetdragaction(a);
	storeAction(wrapper);
}
void UsageLogger::logAccEventBlockMode(QString name, QString type, int mode){
	Action * wrapper = getActionWithCurrentState();
	AccBlockModeAction *a = new AccBlockModeAction();
	
	a->set_blockname(name.toUtf8().constData());
	a->set_blocktype(type.toUtf8().constData());
	a->set_mode(mode);
	wrapper->set_type(Action_ActionType_ACC_BLOCK_MODE);
	wrapper->set_allocated_accblockmodeaction(a);
	storeAction(wrapper);
}

void UsageLogger::logMenuAction(MenuEntry entry){
	Action * wrapper = getActionWithCurrentState();
	
	MenuAction *a = new MenuAction();
	a->set_entry(entry);
	wrapper->set_type(Action_ActionType_MENU);
	wrapper->set_allocated_menuaction(a);
	
	storeAction(wrapper);
}

void UsageLogger::logOpenHelp(){
	logMenuAction(OPEN_HELP);
}
void UsageLogger::logSaveSnapshot(){
	logMenuAction(SAVE_SNAPSHOT);
}
void UsageLogger::logSave(){
	logMenuAction(SAVE);
}
void UsageLogger::logNewFile(){
	logMenuAction(NEW_FILE);
}
void UsageLogger::logOpenFile(){
	logMenuAction(OPEN_FILE);
	}
void UsageLogger::logSaveAs(){
	logMenuAction(SAVE_AS);
	}
void UsageLogger::logCloseFile(){
	logMenuAction(CLOSE_FILE);
}
void UsageLogger::logStop(){
	logMenuAction(STOP);	
}

void UsageLogger::storeAction(Action * a){
	int size = a->ByteSize();
	fileOut->write((char*)&size,4);
	a->SerializeToOstream(fileOut);
}

Action * UsageLogger::getActionWithCurrentState()
{
	action->Clear();
	if(scene != 0){
		action->set_programstateasxml(scene->toString().toUtf8().constData());
	}
	
	return action;
}

string UsageLogger::getFileName(){
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[100];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime(buffer,80,"%Y%m%d_%I-%M-%S_",timeinfo);
	gen_random(buffer+strlen(buffer), 10);
	string str(buffer);
	return str + ".log";
}

// Taken from stackoverflow
void UsageLogger::gen_random(char *s, const int len) {
	srand(time(NULL));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

MouseButton UsageLogger::mapButtons(Qt::MouseButton b){
	MouseButton button;
	switch(b){
		case Qt::NoButton:
		button = NO;
		break;
		case Qt::LeftButton:
		button = LEFT;
		break;
		case Qt::RightButton:
		button = RIGHT;
		break;
		case Qt::MidButton:
		button = MIDDLE;
		break;
	}
	return button;
}



#else
// Provide dummy implementation in case no usage logging is desired.
UsageLogger::UsageLogger(){}
UsageLogger::~UsageLogger(){}
UsageLogger& UsageLogger::getLogger()
{
	static UsageLogger instance;
	return instance;
}
void UsageLogger::logSave(){}
void UsageLogger::logRemoveSet(int row){}
void UsageLogger::logBlockMouseMove(QString name, QString type, QGraphicsSceneMouseEvent *event){}
void UsageLogger::logBlockMouseRelease(QString name, QString type, QGraphicsSceneMouseEvent *event){}
void UsageLogger::logButtonDrag(QString name, QString type, QMouseEvent *event, QDrag *drag){}
void UsageLogger::logSetAdvanced(bool advanced){}
void UsageLogger::logAddEventBlock(int row, Block *block){}
void UsageLogger::logAddActionBlock(int row, Block *block, int position){}
void UsageLogger::logActionSetDrag(int row,QGraphicsSceneMouseEvent *event, QDrag *drag){}
void UsageLogger::logAccEventBlockMode(QString name, QString type, int mode){}
void UsageLogger::logOpenHelp(){}
void UsageLogger::logSaveSnapshot(){}
void UsageLogger::logSave(){}
void UsageLogger::logNewFile(){}
void UsageLogger::logOpenFile(){}
void UsageLogger::logSaveAs(){}
void UsageLogger::logCloseFile(){}
void UsageLogger::logStop(){}

	
#endif //#ifdef PROTOBUF_FOUND


}
}


