#ifndef VPL_EVENT_ACTIONS_SET_H
#define VPL_EVENT_ACTIONS_SET_H

#include <QGraphicsItem>
#include <QDomElement>

#include "Compiler.h"

class QMimeData;
class QDomDocument;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class Block;
	class AddRemoveButton;
	class RemoveBlockButton;
	class StateFilterEventBlock;
	
	class EventActionsSet : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		EventActionsSet(int row, bool advanced, QGraphicsItem *parent=0);
		
		// from QGraphicsObject
		virtual QRectF boundingRect() const;
		
		// specific
		QRectF innerBoundingRect() const;
		
		void setRow(int row);
		int getRow() const { return row; }
		
		void addEventBlock(Block *block);
		void addActionBlock(Block *block, int number = -1);
		const bool hasEventBlock() const;
		const Block *getEventBlock() const;
		const Block *getStateFilterBlock() const;
		QString getEventAndStateFilterHash() const;
		
		bool hasAnyActionBlock() const;
		bool hasActionBlock(const QString& blockName) const;
		int getActionBlockIndex(const QString& blockName) const;
		const Block *getActionBlock(int index) const;
		const Block *getActionBlock(const QString& blockName) const;
		int actionBlocksCount() const;
		
		void removeBlock(Block* block);
		
		bool isAnyAdvancedFeature() const;
		bool isEmpty() const; 
		
		void setAdvanced(bool advanced);
		bool isAdvanced() const;
	
		void setErrorType(Compiler::ErrorType errorType);
		void blink();
		
		QVector<quint16> getContentCompressed() const;
		
		QDomElement serialize(QDomDocument& document) const;
		void deserialize(const QDomElement& element);
		void deserialize(const QByteArray& data);
		
		void repositionElements();
		
	signals:
		friend class Block;
		void contentChanged();
		void undoCheckpoint();
		
	public slots:
		void setSoleSelection();
		
	protected slots:
		void removeClicked();
		void addClicked();
		void removeBlockClicked();
		void clearBlink();
		
	protected:
		// from QGraphicsObject
		virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
		
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		
		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		void drawBlockArea(QPainter * painter, const QString& type, const QPointF& pos, bool highlight) const;
		
		// specific
		void setVisualFromEvent(QGraphicsSceneDragDropEvent *event);
		void clearVisualFromEvent(QGraphicsSceneDragDropEvent *event);
		bool isDnDValid(QGraphicsSceneDragDropEvent *event) const;
		bool isDnDAction(QGraphicsSceneDragDropEvent *event) const;
		bool isDnDNewAction(QGraphicsSceneDragDropEvent *event) const;
		QMimeData* mimeData() const;
		
		void resetSet();
		void ensureOneEmptyActionHolderAtEnd();
		void updateActionPositions(qreal dropXPos);
		void updateDropIndex(qreal dropXPos);
		
		void addActionBlockNoEmit(Block *block, int number = -1);
		void setBlock(Block*& blockPointer, Block* newBlock);
		Block *getActionBlock(const QString& blockName);
	
	protected:
		Block* event;
		Block* stateFilter;
		QList<Block*> actions;
		int isBlinking; //!< show blink background
		
		AddRemoveButton *deleteButton;
		AddRemoveButton *addButton;
		RemoveBlockButton *deleteBlockButton;
		
		enum HighlightMode
		{
			HIGHLIGHT_NONE = 0,
			HIGHLIGHT_EVENT,
			HIGHLIGHT_NEW_ACTION,
			HIGHLIGHT_EXISTING_ACTION,
			HIGHLIGHT_SET
		} highlightMode;
		
		int removeBlockIndex; // -1: none, -2: event, 0 or larger: action
		qreal dropAreaXPos;
		int dropIndex;
		qreal currentWidth;
		qreal basicWidth;
		qreal totalWidth;
		qreal columnPos;
		int row;
		
		Compiler::ErrorType errorType;
		bool beingDragged;
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
