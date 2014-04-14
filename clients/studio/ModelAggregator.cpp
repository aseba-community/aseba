#include "ModelAggregator.h"

namespace Aseba
{
	ModelAggregator::ModelAggregator(QObject* parent) :
		QAbstractItemModel(parent)
	{

	}

	void ModelAggregator::addModel(QAbstractItemModel* model, unsigned int column)
	{
		ModelDescription description;
		description.model = model;
		description.column = column;
		models.append(description);
	}

	int ModelAggregator::columnCount(const QModelIndex &parent) const
	{
		return 1;
	}

	int ModelAggregator::rowCount(const QModelIndex & /*parent*/) const
	{
		int count = 0;
		for (ModelList::ConstIterator it = models.begin(); it != models.end(); it++)
			count += (*it).model->rowCount();
		return count;
	}

	QVariant ModelAggregator::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid())
			return QVariant();

		int count = 0;
		int previousCount = 0;
		int modelID = 0;
		for (ModelList::ConstIterator it = models.begin(); it != models.end(); it++, modelID++)
		{
			previousCount = count;
			count += (*it).model->rowCount();
			if (index.row() < count)
			{
				QAbstractItemModel* thisModel = (*it).model;
				unsigned int thisColumn = (*it).column;
				QModelIndex newIndex = thisModel->index(index.row()-previousCount, thisColumn);
				return thisModel->data(newIndex, role);
			}
		}

		return QVariant();
	}

	bool ModelAggregator::hasIndex(int row, int column, const QModelIndex &parent) const
	{
		if (parent.isValid())
			return false;
		if (column >= columnCount())
			return false;
		if (row > rowCount())
			return false;
		return true;
	}

	QModelIndex ModelAggregator::index(int row, int column, const QModelIndex &parent) const
	{
		return hasIndex(row, column, parent) ? createIndex(row, column, 0) : QModelIndex();
	}

	QModelIndex ModelAggregator::parent(const QModelIndex &child) const
	{
		return QModelIndex();
	}


	// *** TreeChainsawFilter ***
	void TreeChainsawFilter::setSourceModel(QAbstractItemModel *sourceModel)
	{
		QAbstractItemModel* oldSource = this->sourceModel();
		if (oldSource)
			oldSource->disconnect(this);
		QAbstractProxyModel::setSourceModel(sourceModel);
		connect(sourceModel, SIGNAL(modelReset()), this, SLOT(resetInternalData()));
		sort(0, Qt::AscendingOrder);
	}

	int TreeChainsawFilter::rowCount(const QModelIndex &parent) const
	{
		return indexList.count();
	}

	int TreeChainsawFilter::columnCount(const QModelIndex &parent) const
	{
		return 1;
	}

	bool TreeChainsawFilter::hasIndex(int row, int column, const QModelIndex &parent) const
	{
		if (parent.isValid())
			return false;
		if (column >= columnCount())
			return false;
		if (row > rowCount())
			return false;
		return true;
	}

	QModelIndex TreeChainsawFilter::index(int row, int column, const QModelIndex &parent) const
	{
		return hasIndex(row, column, parent) ? createIndex(row, column, 0) : QModelIndex();
	}

	QModelIndex TreeChainsawFilter::parent(const QModelIndex &child) const
	{
		return QModelIndex();
	}

	void TreeChainsawFilter::resetInternalData(void)
	{
		sort(0, Qt::AscendingOrder);
	}

	// sourceIndex -> proxyIndex
	QModelIndex TreeChainsawFilter::mapFromSource(const QModelIndex &sourceIndex) const
	{
		if (!sourceIndex.isValid())
			return QModelIndex();

		IndexLinkList::ConstIterator it;
		for (it = indexList.constBegin(); it != indexList.constEnd(); it++)
			if (sourceIndex == it->source)
				return it->proxy;

		// not found
		return QModelIndex();
	}

	// proxyIndex -> sourceIndex
	QModelIndex TreeChainsawFilter::mapToSource(const QModelIndex &proxyIndex) const
	{
		if (!proxyIndex.isValid())
			return QModelIndex();

		IndexLinkList::ConstIterator it;
		for (it = indexList.constBegin(); it != indexList.constEnd(); it++)
			if (proxyIndex == it->proxy)
				return it->source;

		// not found
		return QModelIndex();
	}

	// rebuild the internal data structure
	void TreeChainsawFilter::sort(int /* column */, Qt::SortOrder /* order */)
	{
		if (!sourceModel())
			return;

		indexList.clear();
		// get the root and walk the tree
		sortWalkTree(QModelIndex());
	}

	// rebuild the internal data structure, private function
	void TreeChainsawFilter::sortWalkTree(const QModelIndex &parent)
	{
		static int rowCounter = 0;

		if (!parent.isValid())
			// root node
			rowCounter = 0;

		QAbstractItemModel* model = sourceModel();
		int childCount = model->rowCount(parent);
		if (childCount == 0)
		{
			// leaf!
			ModelIndexLink indexLink;
			indexLink.source = parent;
			indexLink.proxy = createIndex(rowCounter++, 0, 0);
			indexList.append(indexLink);
			return;
		}

		// iterate on children
		for (int row = 0; row < childCount; row++)
		{
			//QModelIndex child = parent.child(row, 0);
			QModelIndex child = model->index(row, 0, parent);
			sortWalkTree(child);
		}
	}
};
