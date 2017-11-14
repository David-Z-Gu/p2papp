#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QMap>
#include <QDataStream>
#include <QTimer>
#include <QByteArray>

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();
	
	
	quint16 port;
	int myPortMin, myPortMax;
	QUdpSocket* udpSocket;


public slots:
	// Bind this socket to a P2Papp-specific default port.
	bool bind();

private:
	
	
};


class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();
	NetSocket* mySocket;
	void serializeMessage(QVariantMap &myMap);
	void active_timeout();
	//void initSocket();
	void deserializeMessage(QByteArray datagram);
	void receiveText(QVariantMap &inMap);
	void receiveStatusMessage(QVariantMap inMap);	
	void receiveRumorMessage(QVariantMap inMap);
	void create_rumorMessage(QString one_has_info, quint16 seqNo_to_send);
	void create_statusMessage(QString one_has_info, quint16 seqNo_to_ask);


	


public slots:
	void gotReturnPressed();
	void update();
	void readPendingDatagrams();
	

private:
	QTextEdit *textview;
	QLineEdit *textline;
	QTimer *timer;
	quint16 mySeqNo;
	quint16 myPort;
	//Dict_Message takes the smae format as status message
	//It stores Orign as a key, <SeqNo, ChatText> as a value
	QMap <quint16, QString> Message_Pair;
	QMap<QString, QMap<quint16, QString> > Dict_Message;

	quint16 communicatingPort;
};


#endif // P2PAPP_MAIN_HH
