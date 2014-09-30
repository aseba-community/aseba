#include "UsageLogger.h"

#include <ctime>
#include <sys/time.h>
#include <iostream>

#include <QSlider>
#include <QInputDialog>
#include <QDir>
#include <QSettings>

#include "EventActionsSet.h"
#include "Block.h"

using namespace std;

namespace Aseba{ namespace ThymioVPL
{
	
bool UsageLogger::loggingEnabled = false;
UsageLogger::UsageLogger()
{
	if(loggingEnabled){
		askForGroupName();
	
		QString homePath = QDir::homePath();
		QString filePath = homePath + "/" + groupName + "_" + getTimeStampString() + ".log";
		
		fileOut = new ofstream(filePath.toUtf8().constData(), ios::app | ios::binary);
		scene = 0;
		
		connect(&signalMapper, SIGNAL(mapped(unsigned int, QObject *, QObject *)),this, SLOT(logGUIEvents(unsigned int, QObject*, QObject *)));
	}
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

void UsageLogger::setLoggingState(bool enabled){
	loggingEnabled = enabled;
}

void UsageLogger::askForGroupName(){
	QSettings settings;
	groupName = settings.value("logging/groupname","group").toString();
	bool ok;
	QString text = QInputDialog::getText(0, tr("Login"),
				  tr("Please enter your user or group name:"), QLineEdit::Normal,
				  groupName, &ok);
	if (ok && !text.isEmpty()){
		groupName = text;
		settings.setValue("logging/groupname", groupName); 
	}
}

int UsageLogger::getRow(Block * b){
	int row = -1;
	QGraphicsItem  *parent = b->parentItem();
	if(parent){
		EventActionsSet * eas = dynamic_cast<EventActionsSet*>(parent);
		if(eas){
			row = eas->getRow();
		}
	}
	return row;
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

void UsageLogger::logGUIEvents(unsigned int senderId, QObject *originalSender, QObject * logicalParent){
	if(originalSender == 0){
		return;
	}
	
	Block *b = dynamic_cast<Block*>(logicalParent);
	if(b != 0){
		int row = getRow(b);
		
		QSlider * slider = dynamic_cast<QSlider*>(originalSender);
		GeometryShapeButton * button = dynamic_cast<GeometryShapeButton*>(originalSender);
		
		if(slider != 0){
			int sliderValue = slider->value();
			logBlockAction(SLIDER,b->getName(),b->getType(), row, senderId, &sliderValue, NULL, NULL,NULL);
		}
		else if(button != 0){
			int buttonValue = button->getValue();
			logBlockAction(BUTTON,b->getName(),b->getType(), row, senderId, NULL, NULL, NULL, &buttonValue);
		}
	}
}


void UsageLogger::logBlockAction(BlockActionType type, QString blockName, QString blockType, int row, int elementId, int * sliderValue, unsigned int * soundValue, unsigned int * timerValue, int * buttonValue){
	Action * wrapper = getActionWithCurrentState();
	BlockAction *a = new BlockAction();
	
	a->set_type(type);
	a->set_blockname(blockName.toUtf8().constData());
	a->set_blocktype(blockType.toUtf8().constData());
	a->set_row(row);
	a->set_elementid(elementId);
	
	if(sliderValue){
		a->set_slidervalue(*sliderValue);
	}
	if(soundValue){
		a->set_soundvalue(*soundValue);
	}
	if(timerValue){
		a->set_timervalue(*timerValue);
	}
	if(buttonValue){
		a->set_buttonvalue(*buttonValue);
	}
	
	wrapper->set_type(Action_ActionType_BLOCK_ACTION);
	wrapper->set_allocated_blockaction(a);
	storeAction(wrapper);
}

void UsageLogger::logSignal(const QObject * sender, const char * signal, unsigned int senderId, QObject * logicalParent){
	connect(sender, signal, &signalMapper, SLOT(map()));
	signalMapper.setMapping(sender, senderId, logicalParent);
}

void UsageLogger::logInsertSet(int row)
{
	logSetAction(RowAction::INSERT,row);
}

void UsageLogger::logRemoveSet(int row){
	logSetAction(RowAction::REMOVE,row);
}

void UsageLogger::logSetAction(RowAction_ActionType type, int row){
	Action * wrapper = getActionWithCurrentState();
	RowAction * rowAction = new RowAction();
	rowAction->set_row(row);
	rowAction->set_type(type);
	
	wrapper->set_type(Action_ActionType_ROW);
	wrapper->set_allocated_rowaction(rowAction);
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
	logAddBlock(EVENT,row,block);
}
void UsageLogger::logAddActionBlock(int row, Block *block, int position){	
	logAddBlock(ACTION,row,block);
}

void UsageLogger::logAddBlock(BlockType type,int row, Block *block){
	Action * wrapper = getActionWithCurrentState();
	
	AddBlockAction *a = new AddBlockAction();
	a->set_type(type);
	a->set_blockname(block->getName().toUtf8().constData());
	a->set_blocktype(block->getType().toUtf8().constData());
	a->set_row(row);
	wrapper->set_type(Action_ActionType_ADD_BLOCK);
	wrapper->set_allocated_addblockaction(a);
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

void UsageLogger::logBlockMouseMove(QString name, QString type, QGraphicsSceneMouseEvent *event){
	logMouseAction(MOVE_BLOCK,event->scenePos().x(),event->scenePos().y(),mapButtons(event->button()),NULL,name.toUtf8().constData(),type.toUtf8().constData());
}

void UsageLogger::logBlockMouseRelease(QString name, QString type, QGraphicsSceneMouseEvent *event){
	logMouseAction(RELEASE_BLOCK,event->scenePos().x(),event->scenePos().y(),mapButtons(event->button()),NULL,name.toUtf8().constData(),type.toUtf8().constData());
}
void UsageLogger::logButtonDrag(QString name, QString type, QMouseEvent *event, QDrag *drag){
	logMouseAction(DRAG_BUTTON,event->posF().x(),event->posF().y(),mapButtons(event->button()),NULL,name.toUtf8().constData(),type.toUtf8().constData());
}

void UsageLogger::logActionSetDrag(int row,QGraphicsSceneMouseEvent *event, QDrag *drag){
	logMouseAction(DRAG_ACTION_SET,event->scenePos().x(),event->scenePos().y(),mapButtons(event->button()),&row,NULL,NULL);
}

void UsageLogger::logDropButton(BlockButton *button, QDropEvent *event){
	logMouseAction(DROP_BUTTON,event->pos().x(),event->pos().y(),mapButtons(event->mouseButtons()),NULL,button->getName().toUtf8().constData(),NULL);
}

void UsageLogger::logEventActionSetDrop(int row, QGraphicsSceneDragDropEvent *event){
	logMouseAction(DROP_ACTION_SET,event->pos().x(),event->pos().y(),mapButtons(event->buttons()),&row,NULL,NULL);
}

void UsageLogger::logMouseAction(MouseActionType type, double xPos, double yPos, MouseButton button, const int * row, const char * blockName, const char * blockType){
	Action * wrapper = getActionWithCurrentState();
	
	MouseAction *a = new MouseAction();
	a->set_type(type);
	a->set_button(button);
	a->set_xpos(xPos);
	a->set_ypos(yPos);
	
	if(row != 0){
		a->set_row(*row);
	}
	if(blockName != 0){
		a->set_blockname(blockName);
	}
	if(blockType != 0){
		a->set_blocktype(blockType);
	}
	
	wrapper->set_type(Action_ActionType_MOUSE_ACTION);
	wrapper->set_allocated_mouseaction(a);
	
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
void UsageLogger::logRun(){
	logMenuAction(RUN);	
}
void UsageLogger::logUserEvent(unsigned id, const VariablesDataVector& data){
	Action * wrapper = getActionWithCurrentState();
	DeviceAction *a = new DeviceAction();
	a->set_id(id);
	for(unsigned i=0; i < data.size(); i++){
		a->add_variable(data[i]);
	}
	
	wrapper->set_type(Action_ActionType_DEVICE_ACTION);
	wrapper->set_allocated_deviceaction(a);
	storeAction(wrapper);
}

void UsageLogger::storeAction(Action * a){
	if(loggingEnabled){
		int size = a->ByteSize();
		fileOut->write((char*)&size,4);
		fileOut->flush();
		a->SerializeToOstream(fileOut);
	}
}

Action * UsageLogger::getActionWithCurrentState()
{
	action->Clear();
	if(scene != 0){
		action->set_programstateasxml(scene->toString().toUtf8().constData());
	}
	
	TimeStamp *t = new TimeStamp();
	t->set_timestamp(time(NULL));
	t->set_milliseconds(getMilliseconds());
	action->set_allocated_time(t);
	
	return action;
}

QString UsageLogger::getTimeStampString(){
	// For debugging
	if(groupName == "test"){
		return QString("000");
	}
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[100];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime(buffer,80,"%Y%m%d_%I-%M-%S_",timeinfo);
	QString str(buffer);
	return str;
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

unsigned int UsageLogger::getMilliseconds(){
	unsigned int millis = 0;
	timeval t;
	gettimeofday(&t, NULL);
	millis = t.tv_usec;
	return millis;
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
		default:
			button = NO;
			break;
	}
	return button;
}

MouseButton UsageLogger::mapButtons(Qt::MouseButtons b){
	MouseButton button;
	if(b.testFlag(Qt::LeftButton)){
		button = LEFT;
	}
	else if(b.testFlag(Qt::RightButton)){
		button = RIGHT;
	}
	else if(b.testFlag(Qt::MidButton)){
		button = MIDDLE;
	}
	else{
		button = NO;
	}
	return button;
}

}
}


