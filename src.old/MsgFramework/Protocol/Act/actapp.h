#ifndef ACTAPP_H__
#define ACTAPP_H__

#include "actbase.h"

class ActApp : public ActBase
{
public:
	ActApp(Actor* pActor);
	virtual int GetType();
	virtual void ReceiveData(unsigned int uHostId, std::vector<char>* ptData);

	void SendData(unsigned int uHostId);

};


#endif // ACTAPP_H__
