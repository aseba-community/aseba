#ifndef THYMIO_WNET_CONFIG_H
#define THYMIO_WNET_CONFIG_H

#include <QWidget>
#include <dashel/dashel.h>

class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QRadioButton;
class QSpinBox;
class QLabel;

namespace Aseba
{
	/** \addtogroup thymiownetconfig */
	/*@{*/
	
	
	class ThymioWNetConfigDialog : public QWidget
	{
		Q_OBJECT
		
	private:
		std::string target;
		QVBoxLayout* mainLayout;
		QLabel* versionLabel;
		QRadioButton* channel0;
		QRadioButton* channel1;
		QRadioButton* channel2;
		QSpinBox* networkId;
		QSpinBox* nodeId;
		QPushButton* pairButton;
		QPushButton* flashButton;
		QPushButton* quitButton;
		
		Dashel::Hub hub;
		Dashel::Stream* stream;
		
		bool pairing;

	public:
		ThymioWNetConfigDialog(const std::string& target);
		~ThymioWNetConfigDialog();
		
	private slots:
		void updateSettings();
		void flashSettings();
		void togglePairingMode();
		void quit();
	
	};
	
	/*@}*/
} // namespace Aseba

#endif // THYMIO_WNET_CONFIG_H

