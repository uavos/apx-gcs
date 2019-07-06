#ifndef _XPCListener_h_
#define _XPCListener_h_

#include <algorithm>
#include <vector>

class	XPCBroadcaster;


class	XPCListener {
public:

						XPCListener();
	virtual				~XPCListener();
	
	virtual	void		ListenToMessage(
							int				inMessage,
							void *			inParam)=0;
							
private:

	typedef	std::vector<XPCBroadcaster *>	BroadcastVector;
	
	BroadcastVector	mBroadcasters;

	friend	class	XPCBroadcaster;
	
			void		BroadcasterAdded(
							XPCBroadcaster *	inBroadcaster);

			void		BroadcasterRemoved(
							XPCBroadcaster *	inBroadcaster);
			
};			

#endif