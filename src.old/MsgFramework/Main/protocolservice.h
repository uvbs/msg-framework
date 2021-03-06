#ifndef PROTOCOLSERVICE_H__
#define PROTOCOLSERVICE_H__

class Dispatcher;
class Actor;

class ProtocolService
{
	class Impl;
public:
	ProtocolService(void);
	~ProtocolService(void);

	void Exit();

	Dispatcher* GetDispatcher();
	Actor* GetActor();

protected:


private:
	boost::scoped_ptr<Impl> m_pImpl;
};

#endif // PROTOCOLSERVICE_H__
