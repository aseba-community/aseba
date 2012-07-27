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
		~ThymioScene();
		
		QGraphicsItem *addAction(ThymioButton *item);
		QGraphicsItem *addEvent(ThymioButton *item);

		bool isEmpty() const;
		void reset();
		void clear();
		void setColorScheme(QColor eventColor, QColor actionColor);
		bool isModified() const { return sceneModified; }
		void setModified(bool mod) { sceneModified=mod; }
		void setScale(qreal scale);
		int getNumberOfButtonSets() const { return buttonSets.size(); }
		
		QString getErrorMessage() const;
		QList<QString> getCode() const;
		
		bool isSuccessful() const { return  thymioCompiler.isSuccessful(); }
		
		typedef QList<ThymioButtonSet *>::iterator ButtonSetItr;
		
		ButtonSetItr buttonsBegin() { return buttonSets.begin(); }
		ButtonSetItr buttonsEnd() { return buttonSets.end(); }
		
	signals:
		void stateChanged();
		
	private slots:
		void buttonUpdateDetected();
		
	protected:
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	private:
		void removeButton(int row);
		void insertButton(int row);
		void rearrangeButtons(int row=0);
		
		ThymioButtonSet *createNewButtonSet();

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
