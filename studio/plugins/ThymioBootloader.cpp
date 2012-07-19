#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>

#include "../../utils/HexFile.h"

#include "ThymioBootloader.h"

#include "ThymioBootloader.moc"

using namespace std;

namespace Aseba
{
	ThymioBootloaderDialog::ThymioBootloaderDialog(NodeTab* nodeTab):
		InvasivePlugin(nodeTab),
		nodeId(getNodeId()),
		target(getTarget()),
		stream(getDashelStream())
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Firmware Updater"));
		resize(544,182);

		verticalLayout = new QVBoxLayout(this);
		fileLayout = new QHBoxLayout();
		lineEdit = new QLineEdit(this);
		fileLayout->addWidget(lineEdit);
		
		fileButton = new QPushButton(tr("File..."), this);
		fileLayout->addWidget(fileButton);

		verticalLayout->addLayout(fileLayout);

		progressBar = new QProgressBar(this);
		progressBar->setValue(0);

		verticalLayout->addWidget(progressBar);

		flashLayout = new QHBoxLayout();

		spacer = new QSpacerItem(40,20,QSizePolicy::Expanding,
				QSizePolicy::Minimum);

		flashLayout->addItem(spacer);

		flashButton = new QPushButton(tr("Update"), this);
		flashLayout->addWidget(flashButton);

		quitButton = new QPushButton(tr("Quit"), this);
		flashLayout->addWidget(quitButton);

		verticalLayout->addItem(flashLayout);

		connect(fileButton, SIGNAL(clicked()),this,SLOT(openFile()));
		connect(flashButton, SIGNAL(clicked()), this, SLOT(doFlash()));
		connect(quitButton, SIGNAL(clicked()), this, SLOT(doClose()));

		delete_myself = 0;
	}
	
	ThymioBootloaderDialog::~ThymioBootloaderDialog()
	{
	}
	
	QWidget* ThymioBootloaderDialog::createMenuEntry()
	{
		QPushButton *flashButton = new QPushButton(tr("Update firmware"));
		connect(flashButton, SIGNAL(clicked()), SLOT(showFlashDialog()));
		return flashButton;
	}
	
	void ThymioBootloaderDialog::closeAsSoonAsPossible()
	{
		// FIXME: is it correct, what to do if someone requsted a close
		// of the main window while we are flashing?
		close();
	}
	
	bool ThymioBootloaderDialog::surviveTabDestruction() const 
	{
		return delete_myself;
	}
	
	void ThymioBootloaderDialog::showFlashDialog()
	{
		progressBar->setValue(0);
		fileButton->setEnabled(true);
		flashButton->setEnabled(true);
		lineEdit->setEnabled(true);

		target->blockWrite();
		connect(target, SIGNAL(bootloaderAck(uint,uint)), this, SLOT(ackReceived(uint,uint)));
		connect(target, SIGNAL(nodeDisconnected(uint)), this, SLOT(vmDisconnected(unsigned)));

		exec();
		// Note that now nodeTab is not valid anymore

		disconnect(target, SIGNAL(nodeDisconnected(uint)),this, SLOT(vmDisconnected(unsigned)));
		disconnect(target, SIGNAL(bootloaderAck(uint,uint)), this, SLOT(ackReceived(uint,uint)));
		target->unblockWrite();
		
		// we must delete ourself, because the tab is not there any more to do bookkeeping for us
		if(delete_myself)
			deleteLater();
	}
	
	void ThymioBootloaderDialog::openFile(void)
	{
		QString name = QFileDialog::getOpenFileName(this, tr("Select hex file"), QString(), tr("Hex files (*.hex)"));
		lineEdit->setText(name);
	}

	void ThymioBootloaderDialog::doFlash(void) 
	{
		HexFile	hex;
		try {
			hex.read(lineEdit->text().toStdString());
		}
		catch(HexFile::Error& e) {
			QMessageBox::critical(this, tr("Update Error"), tr("Unable to read Hex file"));
			return;
		}
		quitButton->setEnabled(false);
		flashButton->setEnabled(false);
		fileButton->setEnabled(false);
		lineEdit->setEnabled(false);

		// Now we have a valid hex file ...
		pageMap.clear();

		for (HexFile::ChunkMap::iterator it = hex.data.begin(); it != hex.data.end(); it ++)
		{
			// get page number
			unsigned chunkAddress = it->first;
			// index inside data chunk
			unsigned chunkDataIndex = 0;
			// size of chunk in bytes
			unsigned chunkSize = it->second.size();
			// copy data from chunk to page
			do
			{
				// get page number
				unsigned pageIndex = (chunkAddress + chunkDataIndex) / 2048;
				// get address inside page
				unsigned byteIndex = (chunkAddress + chunkDataIndex) % 2048;
				// if page does not exists, create it
				if (pageMap.find(pageIndex) == pageMap.end())
					pageMap[pageIndex] = vector<uint8>(2048, (uint8)0);
				// copy data
				unsigned amountToCopy = min(2048 - byteIndex, chunkSize - chunkDataIndex);
				copy(it->second.begin() + chunkDataIndex, it->second.begin() + chunkDataIndex + amountToCopy, pageMap[pageIndex].begin() + byteIndex);
				// increment chunk data pointer
				chunkDataIndex += amountToCopy;
			}
			while(chunkDataIndex < chunkSize);
		}

		progressBar->setMaximum(pageMap.size());
		progressBar->setValue(0);
		
		// Ask the pic to switch into bootloader
		Reboot msg(nodeId);
		try
		{
			msg.serialize(stream);
			stream->flush();
			// now, wait until the disconnect event is sent.
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void ThymioBootloaderDialog::doClose(void) 
	{
		done(0);
	}

	void ThymioBootloaderDialog::ackReceived(unsigned error_code, unsigned address) 
	{
		qDebug() << "Got ack: " << error_code;
		if(error_code != 0)
		{
			// Fixme should not ignore errors...
		}
		progressBar->setValue(progressBar->value()+1);
		if(currentPage->first == 0) {
			// End of flash operation
			flashDone();
			return;
		}
		currentPage++;
		if(currentPage == pageMap.end()) {
			currentPage = pageMap.find(0);
			if(currentPage == pageMap.end()) {
				qDebug() << "BUG: No page 0 !";
				 // ARG FIXME HACK no 0 page, write null byte instead, bootloader will understand ...
				pageMap[0] = vector<uint8>(2048,(uint8)0);
				currentPage = pageMap.find(0);
			}
		}
		writePage(currentPage->first, &currentPage->second[0]);
	}
	
	void ThymioBootloaderDialog::vmDisconnected(unsigned node)
	{
		delete_myself = 1;

		qDebug() << "Bootloader entered " << node;
		if(node != nodeId)
			abort();

		currentPage = pageMap.begin();
		if(currentPage == pageMap.end()) {
			flashDone();
			return;
		}
		while(currentPage->first == 0 && currentPage != pageMap.end())
			currentPage++;
		if(currentPage == pageMap.end())
			currentPage = pageMap.find(0);
		writePage(currentPage->first, &currentPage->second[0]);
	}
	
	void ThymioBootloaderDialog::writePage(unsigned page, unsigned char * data)
	{
		qDebug() << "Write page: " << page;
		BootloaderWritePage writePage;
		writePage.dest = nodeId;
		writePage.pageNumber = page;

		try
		{
			writePage.serialize(stream);
			stream->write(data,2048);
			stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}

	void ThymioBootloaderDialog::flashDone(void)
	{
		pageMap.clear();
		// Send exit bootloader, send presence
		
		qDebug() << "Flash done, switch back to VM";

		BootloaderReset msg(nodeId);
		try
		{
			msg.serialize(stream);
			stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
		
		// send GetDescription 100 ms later
		startTimer(100);
	}
	
	void ThymioBootloaderDialog::timerEvent(QTimerEvent *event)
	{
		killTimer(event->timerId());
		
		GetDescription message;
		try
		{
			message.serialize(stream);
			stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
		
		// Don't allow to immediatly reflash, we want studio to redisplay a new tab ! 
		quitButton->setEnabled(true);
	}

	void ThymioBootloaderDialog::closeEvent(QCloseEvent *event)
	{ 
		if(quitButton->isEnabled())
			event->accept();
		else
			event->ignore();
	}

	void ThymioBootloaderDialog::handleDashelException(Dashel::DashelException e)
	{
		switch(e.source)
		{
		default:
			// oops... we are doomed
			// non-modal message box
			QMessageBox* message = new QMessageBox(QMessageBox::Critical, tr("Dashel Unexpected Error"), tr("A communication error happened during the flashing process:") + " (" + QString::number(e.source) + ") " + e.what(), QMessageBox::NoButton, this);
			message->setWindowModality(Qt::NonModal);
			message->show();
		}
	}
};
