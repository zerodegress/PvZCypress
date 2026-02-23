#pragma once

namespace fb
{
	template<typename T> class WeakToken
	{
	public:
		T* pRealPtr; //0x0000
	};
	//Size=0x0008

	template<typename T> class ConstWeakPtr
	{
	public:
		WeakToken<T>* pToken; //0x0000

		ConstWeakPtr()
		{

		}

		ConstWeakPtr(T* initialData) : pToken(new WeakToken<T>())
		{
			SetData(initialData);
		}

		T* GetData()
		{
			if (pToken && pToken->pRealPtr)
			{
				DWORD_PTR realPtr = (DWORD_PTR)pToken->pRealPtr;
				return (T*)(realPtr - sizeof(DWORD_PTR));
			}
			return NULL;
		}

		void SetData(T* newData)
		{
			if (pToken)
			{
				DWORD_PTR newRealPtr = reinterpret_cast<DWORD_PTR>(newData) + sizeof(DWORD_PTR);
				pToken->pRealPtr = reinterpret_cast<T*>(newRealPtr);
			}
		}
	};
	//Size=0x0008

	template<typename T> class WeakPtr : public ConstWeakPtr<T>
	{
	public:
		WeakPtr()
		{

		}
		WeakPtr(T* initialData) : ConstWeakPtr<T>(initialData)
		{
		}
	};
}