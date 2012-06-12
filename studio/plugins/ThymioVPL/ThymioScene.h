#ifndef THYMIO_SCENE_H
#define THYMIO_SCENE_H

#include <QGraphicsScene>

#include "ThymioButtons.h"

namespace Aseba
{	
	class ThymioScene : public QGraphicsScene
	{
		Q_OBJECT
		
	public:
		ThymioScene(QObject *parent = 0);
		
		QGraphicsItem *addAction(ThymioButton *item);
		QGraphicsItem *addEvent(ThymioButton *item);

		bool isEmpty();
		void reset();
		void clear();
		void setColorScheme(QColor eventColor, QColor actionColor);
		bool isModified() { return sceneModified; }
		void setModified(bool mod) { sceneModified=mod; }
		void setScale(qreal scale);
		
		QString getErrorMessage();
		QList<QString> getCode();
		
		bool isSuccessful() { return  thymioCompiler.isSuccessful(); }
				
		typedef QList<ThymioButtonSet *>::iterator ButtonSetItr;
		
		ButtonSetItr buttonsBegin() { return buttonSets.begin(); }
		ButtonSetItr buttonsEnd() { return buttonSets.end(); }
		
	signals:
		void stateChanged();
		
	private slots:
		void buttonUpdateDetected(int);
		
	protected:
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

		void removeButton(int row);
		void insertButton(int row);
		void rearrangeButtons(int row=0);
		
	private:
		bool prevNewEventButton;
		bool prevNewActionButton;
		int lastFocus;
		
		QList<ThymioButtonSet *> buttonSets;
		ThymioCompiler thymioCompiler;
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		bool sceneModified;
		double scaleFactor;
		bool newRow;
		qreal buttonSetHeight;
	};
};

#endif
