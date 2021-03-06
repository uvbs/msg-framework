#include "stdafx.h"
#include "tcpmonitor.h"
#include "tcpsession.h"
#include "tcppacket.h"
#include "msgsignals.h"
#include "../Common/vcpool.h"
#include "../Tools/msgobjectpool.hpp"

#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/signals2.hpp>
#include <map>
#include "../Common/hostmanager.h"
#include "../Common/hostinfo.h"
#include "skconvert.h"



using namespace boost::asio;
using namespace boost::signals2;
using boost::system::error_code;
using std::tr1::shared_ptr;

typedef std::map<unsigned int , shared_ptr<TcpSession> > HsSession;

class TcpMonitor::Impl
{
public:
	Impl(io_service& io)
		: ios(io)
	{}

	io_service& ios;
	shared_ptr<ip::tcp::acceptor> ptAcceptor;
	ip::tcp::endpoint             epReceive;
	HsSession hsSession;
	MsgObjectPool<SendTcpPacket> mopPackPool;
	MsgSignals tcpSig;
};

TcpMonitor::TcpMonitor(io_service& io)
: m_pImpl(new Impl(io))
{
}

TcpMonitor::~TcpMonitor()
{

}

bool TcpMonitor::Listen( unsigned short sPort )
{
	m_pImpl->ptAcceptor = shared_ptr<ip::tcp::acceptor>(new ip::tcp::acceptor(
		m_pImpl->ios, ip::tcp::endpoint(ip::tcp::v4(), sPort)));
	m_pImpl->ptAcceptor->set_option(ip::tcp::acceptor::reuse_address(true)); 
	ReadyAccept();
	return true;
}

void TcpMonitor::ReadyAccept()
{
	shared_ptr<ip::tcp::socket> ptSock(new ip::tcp::socket(m_pImpl->ios));

	m_pImpl->ptAcceptor->async_accept(*ptSock, m_pImpl->epReceive,
		boost::bind(&TcpMonitor::AcceptHandler, this, placeholders::error, ptSock));
}

void TcpMonitor::AcceptHandler( const boost::system::error_code& ec, 
	std::tr1::shared_ptr<ip::tcp::socket> ptSocket )
{
	if(ec)
	{
		return;
	}
	
	HostInfo* pHostInfo = HostManager::Instance().TakeHost(m_pImpl->epReceive.address().to_string(),
		m_pImpl->epReceive.port(), HostManager::TT_TCP);
	AssociateSession(ptSocket, pHostInfo->uHostId);
	ReadyAccept();
}

void TcpMonitor::Connect(unsigned int uHostId)
{
	shared_ptr<ip::tcp::socket> ptSock(new ip::tcp::socket(m_pImpl->ios));
	HostInfo* pHostInfo = HostManager::Instance().FindHost(uHostId);
	shared_ptr<ip::tcp::endpoint> ptEndPoint = SkConvert::TcpEndpoint(pHostInfo);
	ptSock->async_connect(*ptEndPoint,
		boost::bind(&TcpMonitor::ConnectHandler, this, placeholders::error, ptSock, uHostId));
}

void TcpMonitor::ConnectHandler( const boost::system::error_code& ec, 
	std::tr1::shared_ptr<boost::asio::ip::tcp::socket> ptSocket,
	unsigned int uHostId)
{
	if(ec)
	{
		m_pImpl->tcpSig.EmitConResult(uHostId, false);
		return;
	}

	AssociateSession(ptSocket, uHostId);
}

void TcpMonitor::AssociateSession( 
	const std::tr1::shared_ptr<boost::asio::ip::tcp::socket>& ptSocket,
	unsigned int uHostId)
{
	shared_ptr<TcpSession> ptSession(new TcpSession(m_pImpl->ios, m_pImpl->mopPackPool));
	ptSession->SetReceiveFunc(boost::bind(&TcpMonitor::ReceiveData, this, _1, _2));
	ptSession->SetResultFunc(boost::bind(&TcpMonitor::SendResult, this, _1, _2));
	ptSession->SetBreakOffFunc(boost::bind(&TcpMonitor::BreakOff, this, _1));
	ptSession->Connected(ptSocket, uHostId);

	m_pImpl->hsSession[uHostId] = ptSession;
	m_pImpl->tcpSig.EmitConResult(uHostId, true);

}

void TcpMonitor::SendTo( unsigned int uOrder, std::vector<char>* ptData, unsigned int uHostId)
{

	HsSession::iterator iFind = m_pImpl->hsSession.find(uHostId);
	if(iFind != m_pImpl->hsSession.end())
	{
		const char* szData = ptData->data();
		std::size_t uSize = ptData->size();
		iFind->second->SendData(uOrder, szData, uSize);
	}
	VcPool::Instance().Recycle(ptData);
}

void TcpMonitor::ReceiveData(std::vector<char>* ptData, unsigned int uHostId)
{
	m_pImpl->tcpSig.EmitReceive(uHostId, ptData);
}

MsgSignals* TcpMonitor::GetSignals() const
{
	return &m_pImpl->tcpSig;
}

boost::asio::io_service& TcpMonitor::GetIOs()
{
	return m_pImpl->ios;
}

void TcpMonitor::SendResult(unsigned int uOrder, int nResultFlag)
{
	m_pImpl->tcpSig.EmitSendResult(uOrder, nResultFlag);
}

void TcpMonitor::BreakOff(unsigned int uHostId)
{
	m_pImpl->hsSession.erase(m_pImpl->hsSession.find(uHostId));
	m_pImpl->tcpSig.EmitBreakOff(uHostId);
}

