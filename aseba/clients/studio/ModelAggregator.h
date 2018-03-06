#ifndef MODELAGGREGATOR_H
#define MODELAGGREGATOR_H

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QModelIndex>
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


	// keep only the leaves of a tree, and present them as a table
	class TreeChainsawFilter : public QAbstractProxyModel
	{
		Q_OBJECT

	public:
		TreeChainsawFilter(QObject* parent = 0) : QAbstractProxyModel(parent) { }

		// model interface
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		bool hasIndex(int row, int column, const QModelIndex &parent) const;
		QModelIndex index(int row, int column, const QModelIndex &parent) const;
		QModelIndex parent(const QModelIndex &child) const;

	public slots:
		void resetInternalData(void);

	public:
		// proxy interface
		void setSourceModel(QAbstractItemModel *sourceModel);
		QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
		QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

		void sort(int column, Qt::SortOrder order);

	protected:
		struct ModelIndexLink
		{
			QModelIndex source;
			QModelIndex proxy;
		};

		void sortWalkTree(const QModelIndex& parent);

		typedef QList<ModelIndexLink> IndexLinkList;
		IndexLinkList indexList;
	};
};

#endif // MODELAGGREGATOR_H
