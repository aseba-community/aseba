#ifndef MODELAGGREGATOR_H
#define MODELAGGREGATOR_H

#include <QAbstractItemModel>
#include <QList>

namespace Aseba
{
	class ModelAggregator : public QAbstractItemModel
	{
	public:
		ModelAggregator(QObject* parent = 0);

		// interface for the aggregator
		void addModel(QAbstractItemModel* model, unsigned int column = 0);

		// interface for QAbstractItemModel
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		bool hasIndex(int row, int column, const QModelIndex &parent) const;
		QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex &child) const;

	protected:
		struct ModelDescription
		{
			QAbstractItemModel* model;
			unsigned int column;
		};

		typedef QList<ModelDescription> ModelList;
		ModelList models;
	};
};

#endif // MODELAGGREGATOR_H
