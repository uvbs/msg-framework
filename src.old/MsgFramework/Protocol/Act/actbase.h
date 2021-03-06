#ifndef ACTBASE_H__
#define ACTBASE_H__

#include <boost\function.hpp>
#include <vector>

class Actor;

class ActBase
{
public:
	ActBase(Actor* pActor)
	: m_pActor(pActor) {};
	
	virtual ~ActBase(){};

	virtual int GetType() = 0;
	virtual void ReceiveData(unsigned int uHostId, std::vector<char>* ptData) = 0;

protected:
	Actor* m_pActor;
};


#endif // ACTBASE_H__
