
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>


#include "main.hh"

// func class::function_name (args)
ChatDialog::ChatDialog()
{
	mySocket = new NetSocket();
	if (!mySocket->bind())
	{
		exit(1);
	}
	else
	{
		myPort = mySocket->port;
	}
	
	timer = new QTimer(this);
	mySeqNo = 0;
	setWindowTitle("P2Papp");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new QLineEdit(this);

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);

	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));

	connect(mySocket, SIGNAL(readyRead()),
		this,SLOT(readPendingDatagrams()));
}



void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...
	qDebug() << "FIX: send message to other peers: " << textline->text();

	textview->append(QString(myPort));
	textview->append(textline->text());

	//Insert the info in Dict_Message
	
	QString message = textline->text();
	Message_Pair.insert(mySeqNo, message);
	Dict_Message.insert(QString(myPort), Message_Pair);
	//qDebug() << "Dict_Message :"<< Dict_Message[QString(myPort)];
	
	//Create a rumor message
	create_rumorMessage(QString(myPort), quint16(mySeqNo));
	mySeqNo += 1;
	// Clear the textline to get ready for the next input message.
	textline->clear();
}



void ChatDialog::serializeMessage(QVariantMap& myMap)
{
	//Create a byte array variable to store the byte string
	QByteArray outData;
	//Convert a QVariantMap to QByteArray
	QDataStream outStream(&outData, QIODevice::ReadWrite); //Constructs a data stream that operates on a byte array
	outStream << myMap;
	
	if (myPort == mySocket->myPortMin){
		//qDebug() << mySocket->udpSocket;
		mySocket->writeDatagram(outData.data(), outData.size(), QHostAddress::LocalHost, myPort+1);
	}
	else if (myPort == mySocket->myPortMax){
		mySocket->writeDatagram(outData.data(), outData.size(), QHostAddress::LocalHost, myPort-1);
	}
	else{
		if (rand() % 10 + 1 < 5)
		{
			mySocket->writeDatagram(outData.data(), outData.size(), QHostAddress::LocalHost, myPort+1);
		}
		else
		{
			mySocket->writeDatagram(outData.data(), outData.size(), QHostAddress::LocalHost, myPort-1);
		}
	}
	active_timeout();
}



void ChatDialog::active_timeout(){
	timer->start(3000);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	
}



void ChatDialog::update(){
//Create Rumor Message
	create_rumorMessage(QString(myPort), quint16(mySeqNo));
}


NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four P2Papp instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;
	
}



bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
			port = p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}



//void ChatDialog::initSocket(){
//qDebug() << "FUNC: initsocket " ;
	//ChatDialog myChat;
	//TODO: what is a SerialPort
	//TODO: how many time did you init a udpsocket?
	//readPendingDatagrams();
//}


//Function is triggered if the socket is binded, and has pending datagrams.
void ChatDialog::readPendingDatagrams()
{
  while (mySocket->hasPendingDatagrams()) {
      QByteArray datagram;
      datagram.resize(mySocket->pendingDatagramSize());
      QHostAddress sender;
      quint16 senderPort;
     
	if(mySocket->readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort) != -1)
	{
				deserializeMessage(datagram);
	}
	else
	{
		return;
	}
  }
}



void ChatDialog::deserializeMessage(QByteArray datagram)
{
	QVariantMap inMap;
	QDataStream inStream(&datagram, QIODevice::ReadOnly);
	inStream >> inMap;
	//qDebug() << "inMap" << inMap;

	receiveText(inMap);
}



void ChatDialog::receiveText(QVariantMap &inMap)
{
	if (inMap.contains("Want"))
	{
		receiveStatusMessage(inMap);
	}
	else
	{
		receiveRumorMessage(inMap);
	}
}



void ChatDialog::receiveStatusMessage(QVariantMap inMap)
{
	//if(inMap.value("Want").canConvert<QMap <QString, quint16> >()){
	//QMap <QString, quint16> req_info = inMap.value("Want").value<QMap <QString, quint16> >();
	QMap <QString, QVariant> req_info = inMap.value("Want").value<QMap <QString, QVariant> >();
	QList <QString> Origins = req_info.keys();
	//QList <quint16> SeqNos = req_info.values().value<quint16> ();
	for (int i =0; i < Origins.length(); i++){
		QString Origin = Origins[i];
		quint16 SeqNo = req_info[Origin].value<quint16> ();
		if(Dict_Message.count(Origin))
		{
			quint16 Origin_SeqNo = Dict_Message.value(Origin).end().key();
			if (SeqNo > Origin_SeqNo)
			{
				create_statusMessage(Origin, quint16(Origin_SeqNo+1));
			}
			else if (SeqNo < Origin_SeqNo)
			{
				create_rumorMessage(Origin, quint16(SeqNo+1));
			}
			else
			{
				if (rand() % 10 + 1 < 5)
				{
						//TODO: ?????? how do I find a new peer?
						create_rumorMessage(Origin, quint16(SeqNo));
				}
				else
				{
					//ceases the rumormongering process.
					return;
				}
			}
		}
		else
		{
			create_statusMessage(Origin, quint16(0));
		}
	}
}



void ChatDialog::receiveRumorMessage(QVariantMap inMap)
{
	QString ChatText = inMap.value("ChatText").value <QString> ();
	qDebug() << "ChatText: " << ChatText;
	QString Origin = inMap["Origin"].value <QString> ();
	qDebug() << "Origin: " << Origin;
	quint16 SeqNo = inMap["SeqNo"].value <quint16> ();
	qDebug() << "SeqNo: " << SeqNo;

	QMap<quint16, QString> Message_Pair;

	if (Dict_Message.contains(Origin) && ChatText != "")
	{
	textview->append(Origin + ": ");
	textview->append(ChatText);
		quint16 Origin_SeqNo = Dict_Message[Origin].end().key();
		if (SeqNo == Origin_SeqNo+1)
		{

			Message_Pair.insert(SeqNo, ChatText);
			Dict_Message.insert(Origin, Message_Pair);

      // CONNECT TO THE SENDER PORT AND ASK FOR MORE;
			create_statusMessage(Origin, quint16(SeqNo+1));
		}
		else if (SeqNo > Origin_SeqNo+1){
			qDebug() << "case 1";
			create_statusMessage(Origin, Origin_SeqNo+1);
		}
		else{
			qDebug() << "case 2";
			create_rumorMessage(Origin, Origin_SeqNo);
			//create_statusMessage(Origin, Origin_SeqNo);
		}
	}
	else
	{
		if (SeqNo == 0)
		{
			textview->append(Origin + ": ");
			textview->append(ChatText);

			Message_Pair.insert(quint16(0), ChatText);
			Dict_Message.insert(Origin, Message_Pair);

			create_statusMessage(Origin, quint16(1));
		}
		else
		{
			create_statusMessage(Origin, quint16(0));
		}
	}
}



void ChatDialog::create_rumorMessage(QString one_has_info, quint16 seqNo_to_send)
{
	QVariantMap rumorMap;
	if (Dict_Message[one_has_info].value(seqNo_to_send)!= "")
	{	
		rumorMap.insert(QString("ChatText"), Dict_Message[one_has_info].value(seqNo_to_send));
		rumorMap.insert(QString("Origin"), QString(one_has_info));
		rumorMap.insert(QString("SeqNo"), quint16(seqNo_to_send));
	
		serializeMessage(rumorMap);
	}
}



//David Gu
void ChatDialog::create_statusMessage(QString one_has_info, quint16 seqNo_to_ask)
{
	QVariantMap statusMap;
	QVariantMap info_wants = qvariant_cast<QVariantMap> (statusMap["Want"]);

	info_wants.insert(one_has_info, seqNo_to_ask);
	//QVariant info = qvariant_cast<QVariant>(info_wants);
	
	//statusMap = qvariant_cast<QVariantMap>(info_wants);
	serializeMessage(statusMap);
}



int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}
