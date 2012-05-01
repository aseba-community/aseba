/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#include "FunctionParametersDialog.h"
#include "CustomDelegate.h"
#include <QtGui>

#include <FunctionParametersDialog.moc>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	FunctionParametersDialog::FunctionParametersDialog(const QString &name, const std::vector<unsigned> &parameters)
	{
		QVBoxLayout *mainLayout = new QVBoxLayout;
		
		// title
		QHBoxLayout *titleLayout = new QHBoxLayout;
		titleLayout->addWidget(new QLabel(QString("<b>%0</b>").arg(name)));
		titleLayout->addStretch();
		
		addParameterButton = new QPushButton(QIcon(":/images/add.png"), "");
		addParameterButton->setMinimumSize(20, 20);
		titleLayout->addWidget(addParameterButton);
		
		delParameterButton = new QPushButton(QIcon(":/images/remove.png"), "");
		delParameterButton->setMinimumSize(20, 20);
		delParameterButton->setDisabled(true);
		titleLayout->addWidget(delParameterButton);
		
		// parameter table
		parametersTable = new QTableWidget(0, 3);
		parametersTable->setShowGrid(false);
		parametersTable->verticalHeader()->hide();
		parametersTable->horizontalHeader()->hide();
		//parametersTable->setSelectionMode(QAbstractItemView::NoSelection);
		parametersTable->setSelectionMode(QAbstractItemView::SingleSelection);
		parametersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		parametersTable->setItemDelegateForColumn(0, &spinBoxDelegate);
		
		// fill table
		parametersTable->resizeColumnToContents(1);
		parametersTable->resizeColumnToContents(2);
		for (size_t i = 0; i < parameters.size(); i++)
			addEntry(parameters[i]);
		parametersTable->resizeRowsToContents();
		
		// ok button
		QPushButton *okButton = new QPushButton(tr("OK"));
		okButton->setDefault(true);
		
		// connections
		connect(addParameterButton, SIGNAL(clicked()), SLOT(addParameter()));
		connect(delParameterButton, SIGNAL(clicked()), SLOT(deleteParameter()));
		connect(parametersTable, SIGNAL(itemSelectionChanged()), SLOT(argumentSelectionChanged()));
		connect(parametersTable, SIGNAL(cellClicked ( int , int )), SLOT(cellClicked ( int , int )));
		connect(okButton, SIGNAL(clicked()), SLOT(accept()));
		
		// layout
		mainLayout->addLayout(titleLayout);
		mainLayout->addWidget(parametersTable);
		mainLayout->addWidget(okButton);
		
		// main
		setLayout(mainLayout);
		resize(240, 240);
		setWindowTitle(tr("Native function"));
	}
	
	std::vector<unsigned> FunctionParametersDialog::getParameters() const
	{
		std::vector<unsigned> parameters;
		for (int i = 0; i < parametersTable->rowCount(); i++)
			parameters.push_back(parametersTable->item(i, 0)->data(Qt::DisplayRole).toUInt());
		return parameters;
	}
	
	void FunctionParametersDialog::addParameter()
	{
		addEntry(1);
	}
	
	void FunctionParametersDialog::deleteParameter()
	{
		parametersTable->removeRow(parametersTable->currentRow());
	}
	
	void FunctionParametersDialog::argumentSelectionChanged()
	{
		delParameterButton->setEnabled(!parametersTable->selectedItems().isEmpty());
	}
	
	void FunctionParametersDialog::cellClicked(int row, int column)
	{
		if (column == 1)
		{
			if (row > 0)
			{
				QString text = parametersTable->item(row, 0)->text();
				parametersTable->item(row, 0)->setText(parametersTable->item(row - 1, 0)->text());
				parametersTable->item(row - 1, 0)->setText(text);
				parametersTable->setCurrentCell(row - 1, 0);
			}
		}
		else if (column == 2)
		{
			if (row + 1 < parametersTable->rowCount())
			{
				QString text = parametersTable->item(row, 0)->text();
				parametersTable->item(row, 0)->setText(parametersTable->item(row + 1, 0)->text());
				parametersTable->item(row + 1, 0)->setText(text);
				parametersTable->setCurrentCell(row + 1, 0);
			}
		}
	}
	
	void FunctionParametersDialog::addEntry(int value)
	{
		int rowCount = parametersTable->rowCount();
		parametersTable->insertRow(rowCount);
		QTableWidgetItem *item;
		
		item = new QTableWidgetItem(QString("%0").arg(value));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		parametersTable->setItem(rowCount, 0, item);
		
		item = new QTableWidgetItem(QIcon(":/images/up.png"), "");
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		parametersTable->setItem(rowCount, 1, item);
		
		item = new QTableWidgetItem(QIcon(":/images/down.png"), "");
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		parametersTable->setItem(rowCount, 2, item);
		
		parametersTable->resizeColumnToContents(1);
		parametersTable->resizeColumnToContents(2);
	}
	
	/*@}*/
}

