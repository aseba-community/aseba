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

#ifndef THYMIO_VPL_STANDALONE_H
#define THYMIO_VPL_STANDALONE_H

#include "Plugin.h"
#include "Target.h"
#include "TargetModels.h"

class QTranslator;
class QVBoxLayout;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioVPLStandalone;
	
	struct ThymioVPLStandaloneInterface: DevelopmentEnvironmentInterface
	{
		ThymioVPLStandaloneInterface(ThymioVPLStandalone* vplStandalone);
		
		Target * getTarget();
		unsigned getNodeId();
		void displayCode(const QList<QString>& code, int line);
		void loadAndRun();
		void stop();
		TargetVariablesModel * getVariablesModel();
		void setVariableValues(unsigned, const VariablesDataVector &);
		bool saveFile(bool as=false);
		void openFile();
		bool newFile();
		
	private:
		ThymioVPLStandalone* vplStandalone;
	};
	
	class ThymioVisualProgramming;
	class AeslEditor;
	
	//! Container for VPL standalone and its code viewer
	class ThymioVPLStandalone: public QWidget, public VariableListener
	{
		Q_OBJECT
		
	public:
		ThymioVPLStandalone(QVector<QTranslator*> translators, const QString& commandLineTarget);
		~ThymioVPLStandalone();
	
	protected:
		void setupWidgets();
		void setupConnections();
		void variableValueUpdated(const QString& name, const VariablesDataVector& values);
		void closeEvent ( QCloseEvent * event );
		void timerEvent ( QTimerEvent * event );
		bool saveFile(bool as);
		void openFile();
		
	protected slots:
		void editorContentChanged();
		void nodeConnected(unsigned node);
		void nodeDisconnected(unsigned node);
		void networkDisconnected();
		void variablesMemoryEstimatedDirty(unsigned node);
		void variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables);
		
	protected:
		friend class ThymioVPLStandaloneInterface;
		
		std::auto_ptr<Target> target; //!< pointer to target
		
		unsigned id; //!< node identifier
		
		QVBoxLayout* layout; //!< layout to add widgets to
		ThymioVisualProgramming* vpl; //!< VPL widget 
		QDomDocument savedContent; //!< saved VPL content across disconnections
		AeslEditor* editor; //! viewer of code produced by VPL
		BytecodeVector bytecode; //!< bytecode resulting of last successfull compilation
		unsigned allocatedVariablesCount; //!< number of allocated variables
		int getDescriptionTimer; //!< timer to periodically get description after a reconnection
		QString fileName; //!< file name of last saved/opened file
	};
	
	/*@}*/
}; // Aseba

#endif // THYMIO_VPL_STANDALONE_H