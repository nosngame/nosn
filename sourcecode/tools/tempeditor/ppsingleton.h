#ifndef _PP_SINGLETON_H_
#define _PP_SINGLETON_H_

template <class T>
class PPSingleton
{
public:
	static T* GetSingleton( )
	{
		static T obj;
		return &obj;
	}
};

#endif