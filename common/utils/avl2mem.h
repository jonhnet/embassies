/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
// The AVLElement class would normally be declared in the avl.cpp file, except that because it's
// a template, it needs to be in the header file.  It can only be accessed (including creation and
// destruction) by the AVLTree friend class.

//#include <limits.h>

// #pragma warning(disable: 4311)
// #pragma warning(disable: 4312)

template<class elementClass, class keyClass> class AVLElement {
public:
				 AVLElement(void);
				~AVLElement(void);

#if		AVL2_SAVE_SPACE
public:
	inline AVLElement<elementClass,keyClass>*getParent() {return (AVLElement<elementClass,keyClass> *)(parentStripped << 2);}
	inline AVLBalance						 getBalance() {return (AVLBalance)balanceStripped;}

	inline void	 setParent(
					AVLElement<elementClass,keyClass> *parent)
					{
						avl_assert((((int)parent) & 0x3) == 0);		// ie., it's aligned
						parentStripped = (((unsigned int)parent) >> 2);
					}

	inline void setBalance(
					AVLBalance						 balance)
					{
						avl_assert(balance < 4);
						balanceStripped = balance;
					}
private:

#pragma warning( disable: 4201)		// this is the nonstandard extension warning.  I know I'm using one; leave me alone
	struct {
		unsigned int			parentStripped:30;
		unsigned int			balanceStripped:2;
	};
#pragma warning( default: 4201)

#pragma warning(default: 4311)
#pragma warning(default: 4312)

#else	// AVL2_SAVE_SPACE
public:
	inline void setParent(AVLElement<elementClass,keyClass>*parent) {parentX = parent;}
	inline void setBalance(AVLBalance balance) {balanceX = balance;}

	inline AVLElement<elementClass,keyClass>*getParent() {return parentX;}
	inline AVLBalance						 getBalance() {return balanceX;}

private:
    AVLBalance	 							 balanceX;
    AVLElement<elementClass,keyClass>		*parentX;
#endif	// AVL2_SAVE_SPACE

public:
	inline AVLElement<elementClass,keyClass> *getLeft(void) { return left; }
	inline void setLeft(AVLElement<elementClass,keyClass> *val) { left = val; }
	inline AVLElement<elementClass,keyClass> *getRight(void) { return right; }
	inline void setRight(AVLElement<elementClass,keyClass> *val) { right = val; }
	inline elementClass *getElement(void) { return element; }
	inline void setElement(elementClass *val) { element = val; }
	inline keyClass getKey(void) { return getElement()->getKey(); }

private:
    AVLElement<elementClass,keyClass>		*left;
    AVLElement<elementClass,keyClass>		*right;
    elementClass							*element;
};


    template<class elementClass, class keyClass> 
AVLElement<elementClass,keyClass>::AVLElement(void)
{
    setBalance(AVLNew);
    left = right = NULL;
	setParent(NULL);
}

    template<class elementClass, class keyClass> 
AVLElement<elementClass,keyClass>::~AVLElement(void)
{
    avl_assert(getBalance() == AVLNew);
    avl_assert(left == NULL && right == NULL && getParent() == NULL);
}

//
//	The standard in-memory allocator:
//
//	Uses a pool if supplied,
//	allocates one if requested,
//	falls back to 'new' if a NULL pool is supplied.
//

template<class nodeClass>
class OptPoolMemAllocator
{
public:
	OptPoolMemAllocator();
	~OptPoolMemAllocator();

	nodeClass *Allocate();
	void Free(nodeClass *avlElement);
	void DumpPoolStats(nodeClass *avlElement);
private:
};

template<class nodeClass>
OptPoolMemAllocator<nodeClass>::OptPoolMemAllocator()
{
}

template<class nodeClass>
OptPoolMemAllocator<nodeClass>::~OptPoolMemAllocator()
{
}


template<class nodeClass>
nodeClass *OptPoolMemAllocator<nodeClass>::Allocate()
{
	nodeClass *avlElement;
	avlElement = new nodeClass;
	CERTIFY_ALLOCATION(avlElement);
	assertOK(avlElement);
	return avlElement;
}

template<class nodeClass>
void OptPoolMemAllocator<nodeClass>::Free(nodeClass *avlElement)
{
	delete avlElement;
}

template<class nodeClass>
void OptPoolMemAllocator<nodeClass>::DumpPoolStats(nodeClass *avlElement)
{
}

//
//
// Gather definitions together to describe the usual in-memory implementation
//
//

template<class elementClass, class keyClass>
class MemImpl
{
public:
	typedef AVLElement<elementClass,keyClass> *nodeClass;
	typedef OptPoolMemAllocator<AVLElement<elementClass,keyClass> > allocatorClass;
};

