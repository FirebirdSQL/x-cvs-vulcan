inline LinkedList::LinkedList	*getHead ()
	{
	return (LinkedList*) next;
	}

inline bool	LinkedList::more (LinkedList *node)
	{
	return (node != NULL);
	}

inline void	*LinkedList::getNext(LinkedList **node)
	{
	void *object = ((LinkedNode*)(*node))->object;
	*node = (*node)->next;

	return object;
	}

