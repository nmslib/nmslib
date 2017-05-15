/**************************************************************************}
{                                                                          }
{    cArray.h                                            		      	       }
{                                                                          }
{                                                                          }
{                 Copyright (c) 1999, 2003					Vaclav Snasel          }
{                                                                          }
{    VERSION: 2.0														        DATE 20/2/1999         }
{                                                                          }
{             following functionality:                                     }
{                                                                          }
{                                                                          }
{    UPDATE HISTORY:                                                       }
{                                                                          }
{                                                                          }
{**************************************************************************/

#ifndef __CARRAY_H__
#define __CARRAY_H__

#include <cstdlib> 
#include <cstring> 

#include "logging.h"

using namespace std;


template<class T>
class cArray
{
public:
	cArray();
	~cArray();
	const cArray<T>& operator = (const cArray& other);
	inline T& operator[](const int Index);
	inline T& operator[](const unsigned int Index);
	inline const T operator[](const int Index) const;
	inline const T operator[](const unsigned int Index) const;
	inline operator T* (void);
	inline unsigned int Size(void) const;
	inline unsigned int Count(void) const;
	inline void Clear(void);
	inline void ClearCount(void);
	inline void SetCount(unsigned int k); // tos
	inline void Resize(const unsigned int Size, const bool Move = false);
	inline void Resize(const unsigned int Size, const unsigned int Count);
	void Move(const T* Array, const unsigned int Count = 1);
	void Add(const T* Array, const unsigned int Count = 1);
	inline void Left(const unsigned int Count);
	inline void Right(const unsigned int Count);
	inline void Mid(const unsigned int Left, const unsigned int Right);
	inline void Append(const T& Value);
	inline T* GetArray(const unsigned int Index = 0);
	inline T* GetArray() const;  // mk
	void Fill(const char ch, const unsigned int Count);

private:
	unsigned int m_Size;
	unsigned int m_Count;
	T* m_Array;
	void m_Resize(const unsigned int Size, const bool Move);
};


template<class T>
cArray<T>::cArray():  m_Size(0), m_Count(0), m_Array(NULL)
{
}

template<class T>
cArray<T>::~cArray()
{
	m_Resize(0, false);
}


template<class T>
inline const cArray<T>& cArray<T>::operator = (const cArray& other)
{
	if (this != &other)
	{
		Move((T*)(*(const_cast<cArray<T>*>(&other))), other.Count());
	}
	return *this;
}


template<class T>
inline T& cArray<T>::operator[](const int Index)
{
	CHECK(Index < (int)m_Count);
	return m_Array[Index];
}


template<class T>
inline T& cArray<T>::operator[](const unsigned int Index)
{
	CHECK(Index < (int)m_Count);
	return m_Array[Index];
}


template<class T>
inline const T cArray<T>::operator[](const int Index) const
{
	CHECK(Index < (int)m_Count);
	return m_Array[Index];
}


template<class T>
inline const T cArray<T>::operator[](const unsigned int Index) const
{
	CHECK(Index < (int)m_Count);
	return m_Array[Index];
}


template<class T>
inline cArray<T>::operator T* (void)
{
	return m_Array;
}


template<class T>
inline unsigned int cArray<T>::Size(void) const
{
	return m_Size;
}


template<class T>
inline unsigned int cArray<T>::Count(void) const
{
	return m_Count;
}


template<class T>
inline void cArray<T>::Clear(void)
{
	m_Resize(0, false);
}



template<class T>
inline void cArray<T>::ClearCount(void)
{
	m_Count = 0;
}

template<class T>
inline void cArray<T>::SetCount(unsigned int k)
{
	m_Count = k;
}


template<class T>
inline void cArray<T>::Resize(const unsigned int Size, const bool Move)
{
	m_Resize(Size, Move);
}


template<class T>
inline void cArray<T>::Resize(const unsigned int Size, const unsigned int Count)
{
	m_Resize(Size, false);
	m_Count = Count < m_Size? Count: m_Size;
}


template<class T>
void cArray<T>::Move(const T* Array, const unsigned int Count)
{
	if (m_Size <= Count)
	{
		m_Resize(Count, false);
	}
	
	if (Count == 1)
	{
		m_Array[0] = Array[0];
	}
	else
	{
		memcpy(m_Array, Array, sizeof(T) * Count);
	}
	m_Count = Count;
}

template<class T>
void cArray<T>::Add(const T* Array, const unsigned int Count)
{
	if (m_Count + Count >= m_Size)
	{
		m_Resize(m_Count + Count, true);
	}
	if (Count == 1)
	{
		m_Array[m_Count++] = Array[0];
	}
	else
	{
		memcpy((char *)&m_Array[m_Count], Array, sizeof(T) * Count);
		m_Count += Count;
	}
}


template<class T>
inline void cArray<T>::Left(const unsigned int Count)
{
	m_Count = Count < m_Count ? Count : m_Count;
}


template<class T>
inline void cArray<T>::Right(const unsigned int Count)
{
	int tmp = (m_Count < Count? 0: m_Count - Count);
	m_Count = (m_Count < Count? m_Count: Count); 
	memmove(m_Array, (char* )&m_Array[tmp], sizeof(T) * m_Count);
}


template<class T>
inline void cArray<T>::Mid(const unsigned int Left, const unsigned int Right)
{
	if (Right >= Left && m_Count >= Right - Left)
	{
		m_Count = Right - Left;
		memmove(m_Array, (char* )&m_Array[Left], sizeof(T) * m_Count);
	}
}



template<class T>
inline void cArray<T>::Append(const T& Value)
{
	if (!(m_Count < m_Size))
	{
		m_Resize(m_Size + 1, true);
	}
	m_Array[m_Count] = Value;
}


template<class T>
inline T* cArray<T>::GetArray(const unsigned int Index)
{
	return &m_Array[Index];
}

template<class T>
inline T* cArray<T>::GetArray() const // mk
{
	return m_Array;
}

template<class T>
void cArray<T>::Fill(const char ch, const unsigned int Count)
{
	m_Resize(Count, false);
	memset(m_Array, ch, Count);
	m_Count = Count;
}


template<class T>
void cArray<T>::m_Resize(const unsigned int Size, const bool Move)
{
	if (Size > m_Size)
	{
		m_Size = (Size & 7) ? Size + (8 - (Size & 7)): Size;
		T *auxPtr = new T[m_Size];
		if (m_Array != NULL)
		{
			if (Move)
			{
				memcpy(auxPtr, m_Array, sizeof(T) * m_Count);
			}
			delete [] m_Array;
		}
		m_Array = auxPtr;
		if (m_Count > m_Size) //pm
		{
			m_Count = m_Size;
		}
	} 
	else if (Size == 0)
	{
		if (m_Array != NULL)
		{
			delete [] m_Array;
			m_Array = NULL;
		}
		m_Size = m_Count = 0;
	}
}


#endif            //    __CARRAY_H__
